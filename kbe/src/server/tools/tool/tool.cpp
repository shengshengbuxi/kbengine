// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com


#include "tool.h"
#include "space.h"
#include "entity.h"
#include "tool_interface.h"
#include "archiver.h"
#include "backuper.h"
#include "initprogress_handler.h"
#include "sync_entitystreamtemplate_handler.h"
#include "common/timestamp.h"
#include "common/kbeversion.h"
#include "common/sha1.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/fixed_messages.h"
#include "network/encryption_filter.h"
#include "server/components.h"
#include "server/telnet_server.h"
#include "server/py_file_descriptor.h"
#include "server/sendmail_threadtasks.h"
#include "math/math.h"
#include "pyscript/py_memorystream.h"
//#include "client_lib/client_interface.h"

//#include "../../server/baseappmgr/baseappmgr_interface.h"
//#include "../../server/cellappmgr/cellappmgr_interface.h"
#include "../../server/tools/tool/tool_interface.h"
//#include "../../server/cellapp/cellapp_interface.h"
#include "../../server/dbmgr/dbmgr_interface.h"
//#include "../../server/loginapp/loginapp_interface.h"

namespace KBEngine{

KBEngine::ScriptTimers KBEngine::Tool::scriptTimers_;

	
/**
内部定时器处理类
*/
class ScriptTimerHandler : public TimerHandler
{
public:
	ScriptTimerHandler(ScriptTimers* scriptTimers, PyObject * callback) :
		pyCallback_(callback),
		scriptTimers_(scriptTimers)
	{
		Py_INCREF(pyCallback_);
	}

	~ScriptTimerHandler()
	{
		Py_DECREF(pyCallback_);
	}

private:
	virtual void handleTimeout(TimerHandle handle, void * pUser)
	{
		int id = ScriptTimersUtil::getIDForHandle(scriptTimers_, handle);

		PyObject *pyRet = PyObject_CallFunction(pyCallback_, "i", id);
		if (pyRet == NULL)
		{
			SCRIPT_ERROR_CHECK();
			return;
		}
		return;
	}

	virtual void onRelease(TimerHandle handle, void * /*pUser*/)
	{
		scriptTimers_->releaseTimer(handle);
		delete this;
	}

	PyObject* pyCallback_;
	ScriptTimers* scriptTimers_;
};

ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(Tool);

// 创建一个用于生成实体的字典，包含了实体所有的持久化属性和数据
PyObject* createDictDataFromPersistentStream(MemoryStream& s, const char* entityType, bool instantiateComponent = true)
{
	PyObject* pyDict = PyDict_New();
	ScriptDefModule* pScriptModule = EntityDef::findScriptModule(entityType);

	if (!pScriptModule)
	{
		ERROR_MSG(fmt::format("Tool::createDictDataFromPersistentStream: not found script[{}]!\n",
			entityType));

		return pyDict;
	}

	// 先将celldata中的存储属性取出
	ScriptDefModule::PROPERTYDESCRIPTION_MAP& propertyDescrs = pScriptModule->getPersistentPropertyDescriptions();
	ScriptDefModule::PROPERTYDESCRIPTION_MAP::const_iterator iter = propertyDescrs.begin();

	try
	{
		for (; iter != propertyDescrs.end(); ++iter)
		{
			PropertyDescription* propertyDescription = iter->second;
			PyObject* pyVal = NULL;
			const char* attrname = propertyDescription->getName();

			if (propertyDescription->getDataType()->type() == DATA_TYPE_ENTITY_COMPONENT)
			{
				// 如果某个实体没有cell部分， 而组件属性没有base部分则忽略
				if (!pScriptModule->hasCell())
				{
					if (!propertyDescription->hasBase())
						continue;
				}

				EntityComponentType* pEntityComponentType = ((EntityComponentType*)propertyDescription->getDataType());

				// 和dbmgr判断保持一致，如果没有持久化属性dbmgr将不会传输数据过来
				if (pEntityComponentType->pScriptDefModule()->getPersistentPropertyDescriptions().size() == 0)
					continue;

				bool hasComponentData = false;
				s >> hasComponentData;

				if (hasComponentData)
				{
					if (!propertyDescription->hasBase())
					{
						pyVal = pEntityComponentType->createCellDataFromPersistentStream(&s);
					}
					else
					{
						if (instantiateComponent)  
						{
							pyVal = ((EntityComponentDescription*)propertyDescription)->createFromPersistentStream(pScriptModule, &s);

							if (!propertyDescription->isSameType(pyVal))
							{
								if (pyVal)
								{
									Py_DECREF(pyVal);
								}

								ERROR_MSG(fmt::format("Tool::createDictDataFromPersistentStream: {}.{} error, set to default!\n",
									entityType, attrname));

								pyVal = propertyDescription->parseDefaultStr("");
							}
						}
						else
							pyVal = pEntityComponentType->createDictDataFromPersistentStream(&s);

					}
				}
				else
				{
					if (!propertyDescription->hasBase())
					{
						pyVal = ((EntityComponentType*)propertyDescription->getDataType())->createCellDataFromPersistentStream(NULL);
					}
					else
					{

						ERROR_MSG(fmt::format("Tool::createDictDataFromPersistentStream: {}.{} error, set to default!\n",
							entityType, attrname));

						pyVal = propertyDescription->parseDefaultStr("");
					}
				}
			}
			else
			{
				pyVal = propertyDescription->createFromPersistentStream(&s);

				if (!propertyDescription->isSameType(pyVal))
				{
					if (pyVal)
					{
						Py_DECREF(pyVal);
					}

					ERROR_MSG(fmt::format("Tool::createDictDataFromPersistentStream: {}.{} error, set to default!\n",
						entityType, attrname));

					pyVal = propertyDescription->parseDefaultStr("");
				}
			}

			PyDict_SetItemString(pyDict, attrname, pyVal);
			Py_DECREF(pyVal);
		}

		if (pScriptModule->hasCell())
		{
#ifdef CLIENT_NO_FLOAT
			int32 v1, v2, v3;
			int32 vv1, vv2, vv3;
#else
			float v1, v2, v3;
			float vv1, vv2, vv3;
#endif

			s >> v1 >> v2 >> v3;
			s >> vv1 >> vv2 >> vv3;

			PyObject* position = PyTuple_New(3);
			PyTuple_SET_ITEM(position, 0, PyFloat_FromDouble((float)v1));
			PyTuple_SET_ITEM(position, 1, PyFloat_FromDouble((float)v2));
			PyTuple_SET_ITEM(position, 2, PyFloat_FromDouble((float)v3));

			PyObject* direction = PyTuple_New(3);
			PyTuple_SET_ITEM(direction, 0, PyFloat_FromDouble((float)vv1));
			PyTuple_SET_ITEM(direction, 1, PyFloat_FromDouble((float)vv2));
			PyTuple_SET_ITEM(direction, 2, PyFloat_FromDouble((float)vv3));

			PyDict_SetItemString(pyDict, "position", position);
			PyDict_SetItemString(pyDict, "direction", direction);

			Py_DECREF(position);
			Py_DECREF(direction);
		}
	}
	catch (MemoryStreamException & e)
	{
		e.PrintPosError();

		for (; iter != propertyDescrs.end(); ++iter)
		{
			PropertyDescription* propertyDescription = iter->second;

			const char* attrname = propertyDescription->getName();

			ERROR_MSG(fmt::format("Tool::createDictDataFromPersistentStream: set({}.{}) to default!\n",
				entityType, attrname));

			PyObject* pyVal = propertyDescription->parseDefaultStr("");

			PyDict_SetItemString(pyDict, attrname, pyVal);
			Py_DECREF(pyVal);
		}

		PyObject* position = PyTuple_New(3);
		PyTuple_SET_ITEM(position, 0, PyFloat_FromDouble(0.f));
		PyTuple_SET_ITEM(position, 1, PyFloat_FromDouble(0.f));
		PyTuple_SET_ITEM(position, 2, PyFloat_FromDouble(0.f));

		PyObject* direction = PyTuple_New(3);
		PyTuple_SET_ITEM(direction, 0, PyFloat_FromDouble(0.f));
		PyTuple_SET_ITEM(direction, 1, PyFloat_FromDouble(0.f));
		PyTuple_SET_ITEM(direction, 2, PyFloat_FromDouble(0.f));

		PyDict_SetItemString(pyDict, "position", position);
		PyDict_SetItemString(pyDict, "direction", direction);

		Py_DECREF(position);
		Py_DECREF(direction);
	}

	return pyDict;
}

//-------------------------------------------------------------------------------------
Tool::Tool(Network::EventDispatcher& dispatcher, 
			 Network::NetworkInterface& ninterface, 
			 COMPONENT_TYPE componentType,
			 COMPONENT_ID componentID):
	EntityApp<Entity>(dispatcher, ninterface, componentType, componentID),
	loopCheckTimerHandle_(),
	pBackuper_(),
	numProxices_(0),
	pTelnetServer_(NULL),
	pResmgrTimerHandle_(),
	pInitProgressHandler_(NULL),
	flags_(APP_FLAGS_NONE)
{
	KBEngine::Network::MessageHandlers::pMainMessageHandlers = &ToolInterface::messageHandlers;

}

//-------------------------------------------------------------------------------------
Tool::~Tool()
{
	// 不需要主动释放
	pInitProgressHandler_ = NULL;

	EntityCallAbstract::resetCallHooks();
}

//-------------------------------------------------------------------------------------	
ShutdownHandler::CAN_SHUTDOWN_STATE Tool::canShutdown()
{
	Components::COMPONENTS& cellapp_components = Components::getSingleton().getComponents(CELLAPP_TYPE);
	if (cellapp_components.size() > 0)
	{
		std::string s;
		for (size_t i = 0; i < cellapp_components.size(); ++i)
		{
			s += fmt::format("{}, ", cellapp_components[i].cid);
		}

		INFO_MSG(fmt::format("Tool::canShutdown(): Waiting for cellapp[{}] destruction!\n",
			s));

		return ShutdownHandler::CAN_SHUTDOWN_STATE_FALSE;
	}

	if (getEntryScript().get() && PyObject_HasAttrString(getEntryScript().get(), "onReadyForShutDown") > 0)
	{
		// 所有脚本都加载完毕
		PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
			const_cast<char*>("onReadyForShutDown"),
			const_cast<char*>(""));

		if (pyResult != NULL)
		{
			bool isReady = (pyResult == Py_True);
			Py_DECREF(pyResult);

			if (!isReady)
				return ShutdownHandler::CAN_SHUTDOWN_STATE_USER_FALSE;
		}
		else
		{
			SCRIPT_ERROR_CHECK();
			return ShutdownHandler::CAN_SHUTDOWN_STATE_USER_FALSE;
		}
	}

	int count = 0;
	Entities<Entity>::ENTITYS_MAP& entities = this->pEntities()->getEntities();
	Entities<Entity>::ENTITYS_MAP::iterator iter = entities.begin();
	for (; iter != entities.end(); ++iter)
	{
		//if(static_cast<Entity*>(iter->second.get())->hasDB())
		{
			count++;
		}
	}

	if (count > 0)
	{
		lastShutdownFailReason_ = "destroyHasDBBases";
		INFO_MSG(fmt::format("Tool::canShutdown(): Wait for the entity's into the database! The remaining {}.\n",
			count));

		return ShutdownHandler::CAN_SHUTDOWN_STATE_FALSE;
	}

	return ShutdownHandler::CAN_SHUTDOWN_STATE_TRUE;
}

//-------------------------------------------------------------------------------------	
void Tool::onShutdownBegin()
{
	EntityApp<Entity>::onShutdownBegin();

	// 通知脚本
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);
	SCRIPT_OBJECT_CALL_ARGS1(getEntryScript().get(), const_cast<char*>("onToolAppShutDown"), 
		const_cast<char*>("i"), 0, false);

	//pRestoreEntityHandlers_.clear();
}

//-------------------------------------------------------------------------------------	
void Tool::onShutdown(bool first)
{
	EntityApp<Entity>::onShutdown(first);

	if(first)
	{
		// 通知脚本
		SCOPED_PROFILE(SCRIPTCALL_PROFILE);
		SCRIPT_OBJECT_CALL_ARGS1(getEntryScript().get(), const_cast<char*>("onToolAppShutDown"), 
			const_cast<char*>("i"), 1, false);
	}

	Components::COMPONENTS& cellapp_components = Components::getSingleton().getComponents(CELLAPP_TYPE);
	if(cellapp_components.size() == 0)
	{
		int count = g_serverConfig.getTool().perSecsDestroyEntitySize;
		Entities<Entity>::ENTITYS_MAP& entities =  this->pEntities()->getEntities();

		while(count > 0 && entities.size() > 0)
		{
			std::vector<ENTITY_ID> vecs;
			
			Entities<Entity>::ENTITYS_MAP::iterator iter = entities.begin();
			for(; iter != entities.end(); ++iter)
			{
				//if(static_cast<Entity*>(iter->second.get())->hasDB() && 
				//	static_cast<Entity*>(iter->second.get())->cellEntityCall() == NULL)
				{
					vecs.push_back(static_cast<Entity*>(iter->second.get())->id());

					if(--count == 0)
						break;
				}
			}

			std::vector<ENTITY_ID>::iterator iter1 = vecs.begin();
			for(; iter1 != vecs.end(); ++iter1)
			{
				Entity* e = this->findEntity((*iter1));
				if(!e)
					continue;
				
				this->destroyEntity((*iter1), true);
			}
		}
	}
}

//-------------------------------------------------------------------------------------	
void Tool::onShutdownEnd()
{
	EntityApp<Entity>::onShutdownEnd();

	// 通知脚本
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);
	SCRIPT_OBJECT_CALL_ARGS1(getEntryScript().get(), const_cast<char*>("onToolAppShutDown"), 
		const_cast<char*>("i"), 2, false);
}

//-------------------------------------------------------------------------------------		
bool Tool::initializeWatcher()
{
	ProfileVal::setWarningPeriod(stampsPerSecond() / g_kbeSrvConfig.gameUpdateHertz());

	WATCH_OBJECT("numProxices", this, &Tool::numProxices);
	WATCH_OBJECT("numClients", this, &Tool::numClients);
	WATCH_OBJECT("load", this, &Tool::_getLoad);
	WATCH_OBJECT("stats/runningTime", &runningTime);
	return EntityApp<Entity>::initializeWatcher();
}

//-------------------------------------------------------------------------------------
bool Tool::installPyModules()
{
	Entity::installScript(getScript().getModule());
	//Proxy::installScript(getScript().getModule());
	Space::installScript(getScript().getModule());
	EntityComponent::installScript(getScript().getModule());
	GlobalDataClient::installScript(getScript().getModule());

	registerScript(Entity::getScriptType());
	//registerScript(Proxy::getScriptType());
	registerScript(EntityComponent::getScriptType());

	// 将app标记注册到脚本
	std::map<uint32, std::string> flagsmaps = createAppFlagsMaps();
	std::map<uint32, std::string>::iterator fiter = flagsmaps.begin();
	for (; fiter != flagsmaps.end(); ++fiter)
	{
		if (PyModule_AddIntConstant(getScript().getModule(), fiter->second.c_str(), fiter->first))
		{
			ERROR_MSG(fmt::format("Tool::onInstallPyModules: Unable to set KBEngine.{}.\n", fiter->second));
		}
	}

	// 注册创建entity的方法到py 
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		time,							__py_gametime,												METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		createEntity,					__py_createEntity,											METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		createEntityLocally,			__py_createEntityLocally,											METH_VARARGS,			0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		createEntityAnywhere,			__py_createEntityAnywhere,									METH_VARARGS,			0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		createEntityRemotely,			__py_createEntityRemotely,									METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		createEntityFromDBID,			__py_createEntityFromDBID,									METH_VARARGS,			0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		createEntityAnywhereFromDBID,	__py_createEntityAnywhereFromDBID,							METH_VARARGS,			0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		createEntityRemotelyFromDBID,	__py_createEntityRemotelyFromDBID,							METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		executeRawDatabaseCommand,		__py_executeRawDatabaseCommand,								METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		quantumPassedPercent,			__py_quantumPassedPercent,									METH_VARARGS,			0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		charge,							__py_charge,												METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		registerReadFileDescriptor,		PyFileDescriptor::__py_registerReadFileDescriptor,			METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		registerWriteFileDescriptor,	PyFileDescriptor::__py_registerWriteFileDescriptor,			METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		deregisterReadFileDescriptor,	PyFileDescriptor::__py_deregisterReadFileDescriptor,		METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		deregisterWriteFileDescriptor,	PyFileDescriptor::__py_deregisterWriteFileDescriptor,		METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		reloadScript,					__py_reloadScript,											METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		isShuttingDown,					__py_isShuttingDown,										METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		address,						__py_address,												METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		deleteEntityByDBID,				__py_deleteEntityByDBID,									METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		lookUpEntityByDBID,				__py_lookUpEntityByDBID,									METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		setAppFlags,					__py_setFlags,												METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		getAppFlags,					__py_getFlags,												METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		addTimer,						__py_addTimer,											METH_VARARGS,	0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		delTimer,						__py_delTimer,											METH_VARARGS,	0);
		
	return EntityApp<Entity>::installPyModules();
}

//-------------------------------------------------------------------------------------
void Tool::onInstallPyModules()
{
	// 添加globalData, globalBases支持
	//pBaseAppData_ = new GlobalDataClient(DBMGR_TYPE, GlobalDataServer::TOOL_DATA);
	//registerPyObjectToScript("baseAppData", pBaseAppData_);

}

//-------------------------------------------------------------------------------------
bool Tool::uninstallPyModules()
{	
	if(g_kbeSrvConfig.getTool().profiles.open_pyprofile)
	{
		script::PyProfile::stop("kbengine");

		char buf[MAX_BUF];
		kbe_snprintf(buf, MAX_BUF, "tool%u.pyprofile", startGroupOrder_);
		script::PyProfile::dump("kbengine", buf);
		script::PyProfile::remove("kbengine");
	}

	

	Entity::uninstallScript();
	EntityComponent::uninstallScript();

	return EntityApp<Entity>::uninstallPyModules();
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_gametime(PyObject* self, PyObject* args)
{
	return PyLong_FromUnsignedLong(Tool::getSingleton().time());
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_quantumPassedPercent(PyObject* self, PyObject* args)
{
	return PyLong_FromLong(Tool::getSingleton().tickPassedPercent());
}

//-------------------------------------------------------------------------------------
void Tool::onUpdateLoad()
{

}

//-------------------------------------------------------------------------------------
bool Tool::run()
{
	return EntityApp<Entity>::run();
}

//-------------------------------------------------------------------------------------
void Tool::handleTimeout(TimerHandle handle, void * arg)
{
	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_CHECK_STATUS:
			this->handleCheckStatusTick();
			return;
		default:
			break;
	}

	EntityApp<Entity>::handleTimeout(handle, arg);
}

//-------------------------------------------------------------------------------------
void Tool::handleCheckStatusTick()
{
}

//-------------------------------------------------------------------------------------
void Tool::handleGameTick()
{
	AUTO_SCOPED_PROFILE("gameTick");

	// 一定要在最前面
	updateLoad();

	EntityApp<Entity>::handleGameTick();

	handleBackup();
	handleArchive();
}

//-------------------------------------------------------------------------------------
void Tool::handleBackup()
{
	AUTO_SCOPED_PROFILE("backup");
	pBackuper_->tick();
}

//-------------------------------------------------------------------------------------
void Tool::handleArchive()
{
	AUTO_SCOPED_PROFILE("archive");
	pArchiver_->tick();
}

//-------------------------------------------------------------------------------------
bool Tool::initialize()
{
	if (!EntityApp<Entity>::initialize())
		return false;

	

	return true;
}

//-------------------------------------------------------------------------------------
bool Tool::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool Tool::initializeEnd()
{
	// 添加一个timer， 每秒检查一些状态
	loopCheckTimerHandle_ = this->dispatcher().addTimer(1000000, this,
							reinterpret_cast<void *>(TIMEOUT_CHECK_STATUS));

	if(Resmgr::respool_checktick > 0)
	{
		pResmgrTimerHandle_ = this->dispatcher().addTimer(int(Resmgr::respool_checktick * 1000000),
			Resmgr::getSingletonPtr(), NULL);

		INFO_MSG(fmt::format("Tool::initializeEnd: started resmgr tick({}s)!\n", 
			Resmgr::respool_checktick));
	}

	pBackuper_.reset(new Backuper());
	pArchiver_.reset(new Archiver());

	new SyncEntityStreamTemplateHandler(this->networkInterface());

	// 如果需要pyprofile则在此处安装
	// 结束时卸载并输出结果
	if(g_kbeSrvConfig.getTool().profiles.open_pyprofile)
	{
		script::PyProfile::start("kbengine");
	}

	pTelnetServer_ = new TelnetServer(&this->dispatcher(), &this->networkInterface());
	pTelnetServer_->pScript(&this->getScript());

	bool ret = pTelnetServer_->start(g_kbeSrvConfig.getTool().telnet_passwd, 
		g_kbeSrvConfig.getTool().telnet_deflayer, 
		g_kbeSrvConfig.getTool().telnet_port);

	Components::getSingleton().extraData4(pTelnetServer_->port());
	return ret;
}

//-------------------------------------------------------------------------------------
void Tool::finalise()
{
	if(pTelnetServer_)
	{
		pTelnetServer_->stop();
		SAFE_RELEASE(pTelnetServer_);
	}

	//pRestoreEntityHandlers_.clear();
	loopCheckTimerHandle_.cancel();
	pResmgrTimerHandle_.cancel();
	scriptTimers_.cancelAll();


	EntityApp<Entity>::finalise();
}

//-------------------------------------------------------------------------------------
void Tool::onChannelDeregister(Network::Channel * pChannel)
{
	//ENTITY_ID pid = pChannel->proxyID();

	// 如果是cellapp死亡了
	if(pChannel->isInternal())
	{
		Components::ComponentInfos* cinfo = Components::getSingleton().findComponent(pChannel);
		if(cinfo)
		{
			/*if(cinfo->componentType == CELLAPP_TYPE)
			{
				onCellAppDeath(pChannel);
			}*/
		}
	}

	EntityApp<Entity>::onChannelDeregister(pChannel);
	
	// 有关联entity的客户端退出则需要设置entity的client
	//if(pid > 0)
	//{
	//	Proxy* proxy = static_cast<Proxy*>(this->findEntity(pid));
	//	if(proxy)
	//	{
	//		proxy->onClientDeath();
	//	}
	//}
}

//-------------------------------------------------------------------------------------
void Tool::onGetEntityAppFromDbmgr(Network::Channel* pChannel, int32 uid, std::string& username, 
						COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ORDER globalorderID, COMPONENT_ORDER grouporderID,
						uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx)
{
	if(pChannel->isExternal())
		return;

	// 
	return;

}

//-------------------------------------------------------------------------------------
Entity* Tool::onCreateEntity(PyObject* pyEntity, ScriptDefModule* sm, ENTITY_ID eid)
{

	if (PyType_IsSubtype(sm->getScriptType(), Space::getScriptType()))
	{
		return new(pyEntity) Space(eid, sm);
	}

	return EntityApp<Entity>::onCreateEntity(pyEntity, sm, eid);
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_createEntity(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	PyObject* params = NULL;
	char* entityType = NULL;
	int ret = 0;
	DBID dbid = 0;
	uint16 dbInterfaceIndex = 0;

	if (argCount == 4) 
		ret = PyArg_ParseTuple(args, "s|O|L|U", &entityType, &params, &dbid, &dbInterfaceIndex);
	else if (argCount == 3) 
		ret = PyArg_ParseTuple(args, "s|O|L", &entityType, &params, &dbid);
	else if (argCount == 2)
		ret = PyArg_ParseTuple(args, "s|O", &entityType, &params);
	else
		ret = PyArg_ParseTuple(args, "s", &entityType);

	if (entityType == NULL || !ret)
	{
		PyErr_Format(PyExc_AssertionError, "Tool::createEntity: args error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	if (dbid != 0) 
	{
		if (strlen(g_kbeSrvConfig.dbInterface(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->db_autoIncrementInit) != 0)
		{
			PyErr_Format(PyExc_AssertionError, "Tool::createEntity: database is auto increment, dbid not assigned!");
			PyErr_PrintEx(0);
			return NULL;
		}
	}

	PyObject* e = Tool::getSingleton().createEntity(entityType, params, false);
	if (e != NULL)
	{
		if (strlen(g_kbeSrvConfig.dbInterface(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->db_autoIncrementInit) == 0) 
		{
			if (dbid == 0) 
				dbid = genUUID64();

			static_cast<Entity*>(e)->dbid(dbInterfaceIndex, dbid);
		}

		static_cast<Entity*>(e)->initializeEntity(params);

		//if (strlen(g_kbeSrvConfig.dbInterface(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(static_cast<Entity*>(e)->dbInterfaceIndex()))->db_autoIncrementInit) == 0) 
		//{
		//	static_cast<Entity*>(e)->writeToDB(NULL, NULL, NULL);
		//}
			
		Py_INCREF(e);
	}

	return e;
}


//-------------------------------------------------------------------------------------
PyObject* Tool::__py_createEntityLocally(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	PyObject* params = NULL;
	char* entityType = NULL;
	int ret = 0;

	if (argCount == 2)
		ret = PyArg_ParseTuple(args, "s|O", &entityType, &params);
	else
		ret = PyArg_ParseTuple(args, "s", &entityType);

	if (entityType == NULL || !ret)
	{
		PyErr_Format(PyExc_AssertionError, "Tool::createEntity: args error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	PyObject* e = Tool::getSingleton().createEntity(entityType, params);
	if (e != NULL)
		Py_INCREF(e);

	return e;
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_createEntityFromDBID(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	PyObject* pyCallback = NULL;
	const char* entityType = NULL;
	int ret = 0;
	DBID dbid = 0;
	PyObject* pyEntityType = NULL;
	PyObject* pyDBInterfaceName = NULL;
	std::string dbInterfaceName = "default";

	switch(argCount)
	{
	case 4:
		{
			ret = PyArg_ParseTuple(args, "O|K|O|O", &pyEntityType, &dbid, &pyCallback, &pyDBInterfaceName);
			break;
		}
	case 3:
		{
			ret = PyArg_ParseTuple(args, "O|K|O", &pyEntityType, &dbid, &pyCallback);
			break;
		}
	case 2:
		{
			ret = PyArg_ParseTuple(args, "O|K", &pyEntityType, &dbid);
			break;
		}
	default:
		{
			PyErr_Format(PyExc_AssertionError, "%s: args require 2 or 3 args, gived %d!\n",
				__FUNCTION__, argCount);	
			PyErr_PrintEx(0);
			return NULL;
		}
	};

	if (!ret)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::createEntityFromDBID: args error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	if (pyDBInterfaceName)
	{
		dbInterfaceName = PyUnicode_AsUTF8AndSize(pyDBInterfaceName, NULL);

		DBInterfaceInfo* pDBInterfaceInfo = g_kbeSrvConfig.dbInterface(dbInterfaceName);
		if (pDBInterfaceInfo->isPure)
		{
			ERROR_MSG(fmt::format("KBEngine::createEntityFromDBID: dbInterface({}) is a pure database does not support Entity! "
				"kbengine[_defs].xml->dbmgr->databaseInterfaces->*->pure\n",
				dbInterfaceName));

			return NULL;
		}

		int dbInterfaceIndex = pDBInterfaceInfo->index;
		if (dbInterfaceIndex < 0)
		{
			PyErr_Format(PyExc_TypeError, "Tool::createEntityFromDBID: not found dbInterface(%s)!",
				dbInterfaceName.c_str());

			PyErr_PrintEx(0);
			return NULL;
		}
	}

	if(pyEntityType)
	{
		entityType = PyUnicode_AsUTF8AndSize(pyEntityType, NULL);
		if (!entityType)
		{
			SCRIPT_ERROR_CHECK();
		}
	}

	if(entityType == NULL || strlen(entityType) <= 0 || ret == -1)
	{
		PyErr_Format(PyExc_AssertionError, "Tool::createEntityFromDBID: args error, entityType=%s!", 
			(entityType ? entityType : "NULL"));

		PyErr_PrintEx(0);

		return NULL;
	}

	if (EntityDef::findScriptModule(entityType) == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "Tool::createEntityFromDBID: entityType(%s) error!", entityType);
		PyErr_PrintEx(0);
		return NULL;
	}

	if(dbid <= 0)
	{
		PyErr_Format(PyExc_AssertionError, "Tool::createEntityFromDBID: dbid error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	if(pyCallback && !PyCallable_Check(pyCallback))
	{
		pyCallback = NULL;

		PyErr_Format(PyExc_AssertionError, "Tool::createEntityFromDBID: callback error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	Tool::getSingleton().createEntityFromDBID(entityType, dbid, pyCallback, dbInterfaceName);
	S_Return;
}

//-------------------------------------------------------------------------------------
void Tool::createEntityFromDBID(const char* entityType, DBID dbid, PyObject* pyCallback, const std::string& dbInterfaceName)
{
	Components::ComponentInfos* dbmgrinfos = Components::getSingleton().getDbmgr();
	if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		PyErr_Format(PyExc_AssertionError, "Tool::createEntityFromDBID: not found dbmgr!\n");
		PyErr_PrintEx(0);
		return;
	}

	DBInterfaceInfo* pDBInterfaceInfo = g_kbeSrvConfig.dbInterface(dbInterfaceName);
	if (pDBInterfaceInfo->isPure)
	{
		ERROR_MSG(fmt::format("Tool::createEntityFromDBID: dbInterface({}) is a pure database does not support Entity! "
			"kbengine[_defs].xml->dbmgr->databaseInterfaces->*->pure\n",
			dbInterfaceName));

		return;
	}

	int dbInterfaceIndex = pDBInterfaceInfo->index;
	if (dbInterfaceIndex < 0)
	{
		PyErr_Format(PyExc_TypeError, "Tool::createEntityFromDBID: not found dbInterface(%s)!", 
			dbInterfaceName.c_str());

		PyErr_PrintEx(0);
		return;
	}

	CALLBACK_ID callbackID = 0;
	if(pyCallback != NULL)
	{
		callbackID = callbackMgr().save(pyCallback);
	}

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	pBundle->newMessage(DbmgrInterface::queryEntity);

	ENTITY_ID entityID = idClient_.alloc();
	KBE_ASSERT(entityID > 0);

	DbmgrInterface::queryEntityArgs7::staticAddToBundle((*pBundle), 
		dbInterfaceIndex, g_componentID, 0, dbid, entityType, callbackID, entityID);
	
	dbmgrinfos->pChannel->send(pBundle);
}

//-------------------------------------------------------------------------------------
void Tool::onCreateEntityFromDBIDCallback(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	std::string entityType;
	DBID dbid;
	CALLBACK_ID callbackID;
	bool success = false;
	bool wasActive = false;
	ENTITY_ID entityID;
	COMPONENT_ID wasActiveCID = 0;
	ENTITY_ID wasActiveEntityID = 0;
	uint16 dbInterfaceIndex = 0;
	COMPONENT_ID createToComponentID = 0;

	s >> createToComponentID;
	s >> dbInterfaceIndex;
	s >> entityType;
	s >> dbid;
	s >> callbackID;
	s >> success;
	s >> entityID;
	s >> wasActive;

	if(wasActive)
	{
		s >> wasActiveCID;
		s >> wasActiveEntityID;
	}

	if (createToComponentID != g_componentID)
	{
		ERROR_MSG(fmt::format("Tool::onCreateEntityFromDBID: createToComponentID({}) != currComponentID({}), "
			"dbInterfaceIndex={}, entityType={}, dbid={}, callbackID={}, success={}, entityID={}, wasActive={}, wasActiveCID={}, wasActiveEntityID={}!\n",
			createToComponentID, g_componentID, dbInterfaceIndex, entityType, dbid, callbackID, success, entityID, wasActive, wasActiveCID, wasActiveEntityID));

		KBE_ASSERT(false);
	}

	if(!success)
	{
		if(callbackID > 0)
		{
			PyObject* baseEntityRef = NULL;

			if(wasActive && wasActiveCID > 0 && wasActiveEntityID > 0)
			{
				Entity* pEntity = this->findEntity(wasActiveEntityID);
				if(pEntity)
				{
					baseEntityRef = static_cast<PyObject*>(pEntity);
					Py_INCREF(baseEntityRef);
				}
				else
				{
					// 如果createEntityFromDBID类接口返回实体已经检出且在当前进程上，但是当前进程上无法找到实体时应该给出错误
					// 这种情况通常是异步的环境中从db查询到已经检出，但等回调时可能实体已经销毁了而造成的
					if(wasActiveCID != g_componentID)
					{
						baseEntityRef = static_cast<PyObject*>(new EntityCall(EntityDef::findScriptModule(entityType.c_str()), 
							NULL, wasActiveCID, wasActiveEntityID, ENTITYCALL_TYPE_BASE));
					}
					else
					{
						ERROR_MSG(fmt::format("Tool::onCreateEntityFromDBID: create {}({}) is failed! A local reference, But it has been destroyed!\n",
							entityType.c_str(), dbid));

						baseEntityRef = Py_None;
						Py_INCREF(baseEntityRef);
						wasActive = false;
					}
				}
			}
			else
			{
				baseEntityRef = Py_None;
				Py_INCREF(baseEntityRef);
				wasActive = false;

				ERROR_MSG(fmt::format("Tool::onCreateEntityFromDBID: create {}({}) is failed!\n",
					entityType.c_str(), dbid));
			}

			// baseEntityRef, dbid, wasActive
			PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
			if(pyfunc != NULL)
			{
				SCOPED_PROFILE(SCRIPTCALL_PROFILE);
				PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
													const_cast<char*>("OKi"), 
													baseEntityRef, dbid, wasActive);

				if(pyResult != NULL)
					Py_DECREF(pyResult);
				else
					SCRIPT_ERROR_CHECK();
			}
			else
			{
				ERROR_MSG(fmt::format("Tool::onCreateEntityFromDBID: not found callback:{}.\n",
					callbackID));
			}

			Py_DECREF(baseEntityRef);
		}
		
		s.done();
		return;
	}

	KBE_ASSERT(entityID > 0);
	EntityDef::context().currEntityID = entityID;
	EntityDef::context().currComponentType = TOOL_TYPE;

	PyObject* pyDict = createDictDataFromPersistentStream(s, entityType.c_str());
	PyObject* e = Tool::getSingleton().createEntity(entityType.c_str(), pyDict, false, entityID);
	if(e)
	{
		static_cast<Entity*>(e)->dbid(dbInterfaceIndex, dbid);
		static_cast<Entity*>(e)->initializeEntity(pyDict);
		Py_DECREF(pyDict);

		KBE_SHA1 sha;
		uint32 digest[5];
		sha.Input(s.data(), s.length());
		sha.Result(digest);
		static_cast<Entity*>(e)->setDirty((uint32*)&digest[0]);
	}
	else
	{
		ERROR_MSG(fmt::format("Tool::onCreateEntityFromDBID: create {}({}) is failed, e == NULL!\n", 
			entityType.c_str(), dbid));

		if(callbackID > 0)
		{
			PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
			if(pyfunc)
			{
				// 不需要通知脚本
			}
		}

		return;
	}

	if(callbackID > 0)
	{
		//if(e != NULL)
		//	Py_INCREF(e);

		// baseEntityRef, dbid, wasActive
		PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
		if(pyfunc != NULL)
		{
			SCOPED_PROFILE(SCRIPTCALL_PROFILE);
			PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
												const_cast<char*>("OKi"), 
												e, dbid, wasActive);

			if(pyResult != NULL)
				Py_DECREF(pyResult);
			else
				SCRIPT_ERROR_CHECK();
		}
		else
		{
			ERROR_MSG(fmt::format("Tool::onCreateEntityFromDBID: not found callback:{}.\n",
				callbackID));
		}
	}
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_executeRawDatabaseCommand(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	PyObject* pycallback = NULL;
	PyObject* pyDBInterfaceName = NULL;
	int ret = 0;
	ENTITY_ID eid = -1;

	char* data = NULL;
	Py_ssize_t size;
	
	if (argCount == 4)
		ret = PyArg_ParseTuple(args, "s#|O|i|O", &data, &size, &pycallback, &eid, &pyDBInterfaceName);
	else if(argCount == 3)
		ret = PyArg_ParseTuple(args, "s#|O|i", &data, &size, &pycallback, &eid);
	else if(argCount == 2)
		ret = PyArg_ParseTuple(args, "s#|O", &data, &size, &pycallback);
	else if(argCount == 1)
		ret = PyArg_ParseTuple(args, "s#", &data, &size);

	if(!ret)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::executeRawDatabaseCommand: args error!");
		PyErr_PrintEx(0);
		S_Return;
	}
	
	std::string dbInterfaceName = "default";
	if (pyDBInterfaceName)
	{
		dbInterfaceName = PyUnicode_AsUTF8AndSize(pyDBInterfaceName, NULL);
		
		if (!g_kbeSrvConfig.dbInterface(dbInterfaceName))
		{
			PyErr_Format(PyExc_TypeError, "KBEngine::executeRawDatabaseCommand: args4, incorrect dbInterfaceName(%s)!", 
				dbInterfaceName.c_str());
			
			PyErr_PrintEx(0);
			S_Return;
		}
	}

	Tool::getSingleton().executeRawDatabaseCommand(data, (uint32)size, pycallback, eid, dbInterfaceName);
	S_Return;
}

//-------------------------------------------------------------------------------------
void Tool::executeRawDatabaseCommand(const char* datas, uint32 size, PyObject* pycallback, ENTITY_ID eid, const std::string& dbInterfaceName)
{
	if(datas == NULL)
	{
		ERROR_MSG("KBEngine::executeRawDatabaseCommand: execute error!\n");
		return;
	}

	Components::ComponentInfos* dbmgrinfos = Components::getSingleton().getDbmgr();
	if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		ERROR_MSG("KBEngine::executeRawDatabaseCommand: not found dbmgr!\n");
		return;
	}

	int dbInterfaceIndex = g_kbeSrvConfig.dbInterfaceName2dbInterfaceIndex(dbInterfaceName);
	if (dbInterfaceIndex < 0)
	{
		ERROR_MSG(fmt::format("KBEngine::executeRawDatabaseCommand: not found dbInterface({})!\n",
			dbInterfaceName));

		return;
	}

	//INFO_MSG(fmt::format("KBEngine::executeRawDatabaseCommand{}:{}.\n", 
	//	(eid > 0 ? (fmt::format("(entityID={})", eid)) : ""), datas));

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	(*pBundle).newMessage(DbmgrInterface::executeRawDatabaseCommand);
	(*pBundle) << eid;
	(*pBundle) << (uint16)dbInterfaceIndex;
	(*pBundle) << componentID_ << componentType_;

	CALLBACK_ID callbackID = 0;

	if(pycallback && PyCallable_Check(pycallback))
		callbackID = callbackMgr().save(pycallback);

	(*pBundle) << callbackID;
	(*pBundle) << size;
	(*pBundle).append(datas, size);
	dbmgrinfos->pChannel->send(pBundle);
}

//-------------------------------------------------------------------------------------
void Tool::onExecuteRawDatabaseCommandCB(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	std::string err;
	CALLBACK_ID callbackID = 0;
	uint32 nrows = 0;
	uint32 nfields = 0;
	uint64 affectedRows = 0;
	uint64 lastInsertID = 0;

	PyObject* pResultSet = NULL;
	PyObject* pAffectedRows = NULL;
	PyObject* pLastInsertID = NULL;
	PyObject* pErrorMsg = NULL;

	s >> callbackID;
	s >> err;

	if(err.size() <= 0)
	{
		s >> nfields;

		pErrorMsg = Py_None;
		Py_INCREF(pErrorMsg);

		if(nfields > 0)
		{
			pAffectedRows = Py_None;
			Py_INCREF(pAffectedRows);

			pLastInsertID = Py_None;
			Py_INCREF(pLastInsertID);

			s >> nrows;

			pResultSet = PyList_New(nrows);
			for (uint32 i = 0; i < nrows; ++i)
			{
				PyObject* pRow = PyList_New(nfields);
				for(uint32 j = 0; j < nfields; ++j)
				{
					std::string cell;
					s.readBlob(cell);

					PyObject* pCell = NULL;
						
					if(cell == "KBE_QUERY_DB_NULL")
					{
						Py_INCREF(Py_None);
						pCell = Py_None;
					}
					else
					{
						pCell = PyBytes_FromStringAndSize(cell.data(), cell.length());
					}

					PyList_SET_ITEM(pRow, j, pCell);
				}

				PyList_SET_ITEM(pResultSet, i, pRow);
			}
		}
		else
		{
			pResultSet = Py_None;
			Py_INCREF(pResultSet);

			pErrorMsg = Py_None;
			Py_INCREF(pErrorMsg);

			s >> affectedRows;

			pAffectedRows = PyLong_FromUnsignedLongLong(affectedRows);

			s >> lastInsertID;
			pLastInsertID = PyLong_FromUnsignedLongLong(lastInsertID);
		}
	}
	else
	{
			pResultSet = Py_None;
			Py_INCREF(pResultSet);

			pErrorMsg = PyUnicode_FromString(err.c_str());

			pAffectedRows = Py_None;
			Py_INCREF(pAffectedRows);

			pLastInsertID = Py_None;
			Py_INCREF(pLastInsertID);
	}

	s.done();

	if(callbackID > 0)
	{
		SCOPED_PROFILE(SCRIPTCALL_PROFILE);

		PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
		if(pyfunc != NULL)
		{
			PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
												const_cast<char*>("OOOO"), 
												pResultSet, pAffectedRows, pLastInsertID, pErrorMsg);

			if(pyResult != NULL)
				Py_DECREF(pyResult);
			else
				SCRIPT_ERROR_CHECK();
		}
		else
		{
			ERROR_MSG(fmt::format("Tool::onExecuteRawDatabaseCommandCB: not found callback:{}.\n",
				callbackID));
		}
	}

	Py_XDECREF(pResultSet);
	Py_XDECREF(pAffectedRows);
	Py_XDECREF(pLastInsertID);
	Py_XDECREF(pErrorMsg);
}

//-------------------------------------------------------------------------------------
void Tool::onDbmgrInitCompleted(Network::Channel* pChannel, 
		GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID, 
		COMPONENT_ORDER startGlobalOrder, COMPONENT_ORDER startGroupOrder, 
		const std::string& digest)
{
	if(pChannel->isExternal())
		return;

	EntityApp<Entity>::onDbmgrInitCompleted(pChannel, gametime, startID, endID,
		startGlobalOrder, startGroupOrder, digest);

	// 再次同步自己的新信息(startGlobalOrder, startGroupOrder等)到machine
	Components::getSingleton().broadcastSelf();

	// 这里需要更新一下python的环境变量
	this->getScript().setenv("KBE_BOOTIDX_GLOBAL", getenv("KBE_BOOTIDX_GLOBAL"));
	this->getScript().setenv("KBE_BOOTIDX_GROUP", getenv("KBE_BOOTIDX_GROUP"));

	SCOPED_PROFILE(SCRIPTCALL_PROFILE);

	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(), 
										const_cast<char*>("onInit"), 
										const_cast<char*>("i"), 
										0);

	if(pyResult != NULL)
		Py_DECREF(pyResult);
	else
		SCRIPT_ERROR_CHECK();

	if (!pInitProgressHandler_)
		pInitProgressHandler_ = new InitProgressHandler(this->networkInterface());

	pInitProgressHandler_->start();
}

//-------------------------------------------------------------------------------------
void Tool::onWriteToDBCallback(Network::Channel* pChannel, ENTITY_ID eid, 
	DBID entityDBID, uint16 dbInterfaceIndex, CALLBACK_ID callbackID, bool success)
{
	if(pChannel->isExternal())
		return;

	Entity* pEntity = pEntities_->find(eid);
	if(pEntity == NULL)
	{
		return;
	}

	pEntity->onWriteToDBCallback(eid, entityDBID, dbInterfaceIndex, callbackID, -1, success);
}


//-------------------------------------------------------------------------------------
void Tool::onEntityAutoLoadCBFromDBMgr(Network::Channel* pChannel, MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	pInitProgressHandler_->onEntityAutoLoadCBFromDBMgr(pChannel, s);
}

//-------------------------------------------------------------------------------------
void Tool::reqSetFlags(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;

	uint32 flags = 0;
	s >> flags;

	Tool::getSingleton().flags(flags);

	flags = Tool::getSingleton().flags();

	DEBUG_MSG(fmt::format("Tool::reqSetFlags: {}\n", flags));

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	bool success = true;
	(*pBundle) << success;
	(*pBundle) << flags;
	pChannel->send(pBundle);
}

//-------------------------------------------------------------------------------------
void Tool::lookApp(Network::Channel* pChannel)
{
	if(pChannel->isExternal())
		return;


	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	
	(*pBundle) << g_componentType;
	(*pBundle) << componentID_;

	ShutdownHandler::SHUTDOWN_STATE state = shuttingdown();
	int8 istate = int8(state);
	(*pBundle) << istate;
	(*pBundle) << this->entitiesSize();
	//(*pBundle) << numClients();
	//(*pBundle) << numProxices();

	uint32 port = 0;
	if(pTelnetServer_)
		port = pTelnetServer_->port();

	(*pBundle) << port;

	pChannel->send(pBundle);
}


//-------------------------------------------------------------------------------------
PyObject* Tool::__py_reloadScript(PyObject* self, PyObject* args)
{
	bool fullReload = true;
	int argCount = (int)PyTuple_Size(args);
	if(argCount == 1)
	{
		if(!PyArg_ParseTuple(args, "b", &fullReload))
		{
			PyErr_Format(PyExc_TypeError, "KBEngine::reloadScript(fullReload): args error!");
			PyErr_PrintEx(0);
			return 0;
		}
	}

	Tool::getSingleton().reloadScript(fullReload);
	S_Return;
}

//-------------------------------------------------------------------------------------
void Tool::reloadScript(bool fullReload)
{
	//if (pBundleImportEntityDefDatas_)
	//{
	//	Network::Bundle::reclaimPoolObject(pBundleImportEntityDefDatas_);
	//	pBundleImportEntityDefDatas_ = NULL;
	//}

	EntityApp<Entity>::reloadScript(fullReload);
}

//-------------------------------------------------------------------------------------
void Tool::onReloadScript(bool fullReload)
{
	Entities<Entity>::ENTITYS_MAP& entities = pEntities_->getEntities();
	Entities<Entity>::ENTITYS_MAP::iterator eiter = entities.begin();
	for(; eiter != entities.end(); ++eiter)
	{
		static_cast<Entity*>(eiter->second.get())->reload(fullReload);
	}

	EntityApp<Entity>::onReloadScript(fullReload);
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_isShuttingDown(PyObject* self, PyObject* args)
{
	return PyBool_FromLong(Tool::getSingleton().isShuttingdown() ? 1 : 0);
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_address(PyObject* self, PyObject* args)
{
	PyObject* pyobj = PyTuple_New(2);
	const Network::Address& addr = Tool::getSingleton().networkInterface().intEndpoint().addr();
	PyTuple_SetItem(pyobj, 0,  PyLong_FromUnsignedLong(addr.ip));
	PyTuple_SetItem(pyobj, 1,  PyLong_FromUnsignedLong(addr.port));
	return pyobj;
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_deleteEntityByDBID(PyObject* self, PyObject* args)
{
	uint16 currargsSize = (uint16)PyTuple_Size(args);
	if (currargsSize < 3 || currargsSize > 4)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::deleteEntityByDBID: args != (entityType, dbID, pycallback, dbInterfaceName)!");
		PyErr_PrintEx(0);
		return NULL;
	}
	
	char* entityType = NULL;
	PyObject* pycallback = NULL;
	PyObject* pyDBInterfaceName = NULL;
	DBID dbid = 0;
	std::string dbInterfaceName = "default";

	if (currargsSize == 3)
	{
		if (!PyArg_ParseTuple(args, "s|K|O", &entityType, &dbid, &pycallback))
		{
			PyErr_Format(PyExc_TypeError, "KBEngine::deleteEntityByDBID: args error!");
			PyErr_PrintEx(0);
			return NULL;
		}
	}
	else if (currargsSize == 4)
	{
		if (!PyArg_ParseTuple(args, "s|K|O|O", &entityType, &dbid, &pycallback, &pyDBInterfaceName))
		{
			PyErr_Format(PyExc_TypeError, "KBEngine::deleteEntityByDBID: args error!");
			PyErr_PrintEx(0);
			return NULL;
		}

		dbInterfaceName = PyUnicode_AsUTF8AndSize(pyDBInterfaceName, NULL);
	}

	ScriptDefModule* sm = EntityDef::findScriptModule(entityType);
	if(sm == NULL)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::deleteEntityByDBID: entityType(%s) not found!", entityType);
		PyErr_PrintEx(0);
		return NULL;
	}

	if(dbid == 0)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::deleteEntityByDBID: dbid is 0!");
		PyErr_PrintEx(0);
		return NULL;
	}
	
	if(!PyCallable_Check(pycallback))
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::deleteEntityByDBID: invalid pycallback!");
		PyErr_PrintEx(0);
		return NULL;
	}

	Components::ComponentInfos* dbmgrinfos = Components::getSingleton().getDbmgr();
	if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		ERROR_MSG("KBEngine::deleteEntityByDBID({}): not found dbmgr!\n");
		return NULL;
	}

	DBInterfaceInfo* pDBInterfaceInfo = g_kbeSrvConfig.dbInterface(dbInterfaceName);
	if (pDBInterfaceInfo->isPure)
	{
		ERROR_MSG(fmt::format("Tool::deleteEntityByDBID: dbInterface({}) is a pure database does not support Entity! "
			"kbengine[_defs].xml->dbmgr->databaseInterfaces->*->pure\n",
			dbInterfaceName));

		return NULL;
	}

	int dbInterfaceIndex = pDBInterfaceInfo->index;
	if (dbInterfaceIndex < 0)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::deleteEntityByDBID: not found dbInterface(%s)!", dbInterfaceName.c_str());
		PyErr_PrintEx(0);
		return NULL;
	}

	CALLBACK_ID callbackID = Tool::getSingleton().callbackMgr().save(pycallback);

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	(*pBundle).newMessage(DbmgrInterface::deleteEntityByDBID);
	(*pBundle) << (uint16)dbInterfaceIndex;
	(*pBundle) << g_componentID;
	(*pBundle) << dbid;
	(*pBundle) << callbackID;
	(*pBundle) << sm->getUType();
	dbmgrinfos->pChannel->send(pBundle);

	S_Return;
}

//-------------------------------------------------------------------------------------
void Tool::deleteEntityByDBIDCB(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	ENTITY_ID entityID = 0;
	COMPONENT_ID entityInAppID = 0;
	bool success = false;
	CALLBACK_ID callbackID;
	DBID entityDBID;
	ENTITY_SCRIPT_UID sid;

	s >> success >> entityID >> entityInAppID >> callbackID >> sid >> entityDBID;

	ScriptDefModule* sm = EntityDef::findScriptModule(sid);
	if(sm == NULL)
	{
		ERROR_MSG(fmt::format("Tool::deleteEntityByDBIDCB: entityUType({}) not found!\n", sid));
		return;
	}

	if(callbackID > 0)
	{
		// true or false or entityCall
		PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
		if(pyfunc != NULL)
		{
			PyObject* pyval = NULL;
			if(success)
			{
				pyval = Py_True;
				Py_INCREF(pyval);
			}
			else if(entityID > 0 && entityInAppID > 0)
			{
				Entity* e = static_cast<Entity*>(this->findEntity(entityID));
				if(e != NULL)
				{
					pyval = e;
					Py_INCREF(pyval);
				}
				else
				{
					pyval = static_cast<EntityCall*>(new EntityCall(sm, NULL, entityInAppID, entityID, ENTITYCALL_TYPE_BASE));
				}
			}
			else
			{
				pyval = Py_False;
				Py_INCREF(pyval);
			}

			SCOPED_PROFILE(SCRIPTCALL_PROFILE);
			PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
												const_cast<char*>("O"), 
												pyval);

			if(pyResult != NULL)
				Py_DECREF(pyResult);
			else
				SCRIPT_ERROR_CHECK();

			Py_DECREF(pyval);
		}
		else
		{
			ERROR_MSG(fmt::format("Tool::deleteEntityByDBIDCB: not found callback:{}.\n",
				callbackID));
		}
	}
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_lookUpEntityByDBID(PyObject* self, PyObject* args)
{
	uint16 currargsSize = (uint16)PyTuple_Size(args);
	if (currargsSize < 3 || currargsSize > 5)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: args != (entityType, dbID, pycallback, dbInterfaceName)!");
		PyErr_PrintEx(0);
		return NULL;
	}
	
	char* entityType = NULL;
	PyObject* pycallback = NULL;
	DBID dbid = 0;
	bool getRawData = false;
	std::string dbInterfaceName = "default";

	if (currargsSize == 3)
	{
		if (!PyArg_ParseTuple(args, "s|K|O", &entityType, &dbid, &pycallback))
		{
			PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: args error!");
			PyErr_PrintEx(0);
			return NULL;
		}
	}
	else if (currargsSize == 4)
	{
		if (!PyArg_ParseTuple(args, "s|K|O|b", &entityType, &dbid, &pycallback, &getRawData))
		{
			PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: args error!");
			PyErr_PrintEx(0);
			return NULL;
		}

	}
	else if (currargsSize == 5)
	{
		PyObject* pyDBInterfaceName = NULL;

		if (!PyArg_ParseTuple(args, "s|K|O|b|O", &entityType, &dbid, &pycallback, &getRawData, &pyDBInterfaceName))
		{
			PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: args error!");
			PyErr_PrintEx(0);
			return NULL;
		}

		dbInterfaceName = PyUnicode_AsUTF8AndSize(pyDBInterfaceName, NULL);
	}
	else
	{
		KBE_ASSERT(false);
	}

	ScriptDefModule* sm = EntityDef::findScriptModule(entityType);
	if(sm == NULL)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: entityType(%s) not found!", entityType);
		PyErr_PrintEx(0);
		return NULL;
	}

	if(dbid == 0)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: dbid is 0!");
		PyErr_PrintEx(0);
		return NULL;
	}
	
	if(!PyCallable_Check(pycallback))
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: invalid pycallback!");
		PyErr_PrintEx(0);
		return NULL;
	}
	
	DBInterfaceInfo* pDBInterfaceInfo = g_kbeSrvConfig.dbInterface(dbInterfaceName);
	if (pDBInterfaceInfo->isPure)
	{
		ERROR_MSG(fmt::format("Tool::lookUpEntityByDBID: dbInterface({}) is a pure database does not support Entity! "
			"kbengine[_defs].xml->dbmgr->databaseInterfaces->*->pure\n",
			dbInterfaceName));

		return NULL;
	}

	int dbInterfaceIndex = pDBInterfaceInfo->index;
	if (dbInterfaceIndex < 0)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::lookUpEntityByDBID: not found dbInterface(%s)!", dbInterfaceName.c_str());
		PyErr_PrintEx(0);
		return NULL;
	}

	Components::ComponentInfos* dbmgrinfos = Components::getSingleton().getDbmgr();
	if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		ERROR_MSG("KBEngine::lookUpEntityByDBID({}): not found dbmgr!\n");
		return NULL;
	}

	CALLBACK_ID callbackID = Tool::getSingleton().callbackMgr().save(pycallback);

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	(*pBundle).newMessage(DbmgrInterface::lookUpEntityByDBID);
	(*pBundle) << (uint16)dbInterfaceIndex;
	(*pBundle) << g_componentID;
	(*pBundle) << g_componentType;
	(*pBundle) << dbid;
	(*pBundle) << callbackID;
	(*pBundle) << sm->getUType();
	(*pBundle) << getRawData;
	dbmgrinfos->pChannel->send(pBundle);

	S_Return;
}

//-------------------------------------------------------------------------------------
void Tool::lookUpEntityByDBIDCB(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	ENTITY_ID entityID = 0;
	COMPONENT_ID entityInAppID = 0;
	bool success = false;
	CALLBACK_ID callbackID;
	DBID entityDBID;
	ENTITY_SCRIPT_UID sid;
	bool rawData = false;

	s >> success >> entityID >> entityInAppID >> callbackID >> sid >> entityDBID >> rawData;

	ScriptDefModule* sm = EntityDef::findScriptModule(sid);
	if(sm == NULL)
	{
		ERROR_MSG(fmt::format("Tool::lookUpEntityByDBIDCB: entityUType({}) not found!\n", sid));
		return;
	}


	if(callbackID > 0)
	{
		// true or false or entityCall
		PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
		if(pyfunc != NULL)
		{
			PyObject* pyRawData = NULL;

			if (success && rawData && s.length() > 0)
			{
				EntityDef::context().currEntityID = entityID;
				EntityDef::context().currComponentType = TOOL_TYPE;
				pyRawData = createDictDataFromPersistentStream(s,  sm->getName(), false);
			}
			else {
				pyRawData = Py_None;
				Py_INCREF(pyRawData);
			}

			PyObject* pyEntity = NULL;

			if(entityID > 0 && entityInAppID > 0)
			{
				Entity* e = static_cast<Entity*>(this->findEntity(entityID));
				if(e != NULL)
				{
					pyEntity = e;
					Py_INCREF(pyEntity);
				}
				else
				{
					pyEntity = static_cast<EntityCall*>(new EntityCall(sm, NULL, entityInAppID, entityID, ENTITYCALL_TYPE_BASE));
				}
			} 
			else
			{
				pyEntity = Py_None;
				Py_INCREF(pyEntity);
			}

			SCOPED_PROFILE(SCRIPTCALL_PROFILE);
			PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
												const_cast<char*>("OO"), 
												pyEntity, pyRawData);

			if(pyResult != NULL)
				Py_DECREF(pyResult);
			else
				SCRIPT_ERROR_CHECK();

			Py_DECREF(pyEntity);
			Py_DECREF(pyRawData);
		}
		else
		{
			ERROR_MSG(fmt::format("Tool::lookUpEntityByDBIDCB: not found callback:{}.\n",
				callbackID));
		}
	}
}


//-------------------------------------------------------------------------------------
PyObject* Tool::__py_getFlags(PyObject* self, PyObject* args)
{
	return PyLong_FromUnsignedLong(Tool::getSingleton().flags());
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_setFlags(PyObject* self, PyObject* args)
{
	if(PyTuple_Size(args) != 1)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::setFlags: argsSize != 1!");
		PyErr_PrintEx(0);
		return NULL;
	}

	uint32 flags;

	if(!PyArg_ParseTuple(args, "I", &flags))
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::setFlags: args error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	Tool::getSingleton().flags(flags);
	S_Return;
}

//-------------------------------------------------------------------------------------

PyObject* Tool::__py_addTimer(PyObject* self, PyObject* args)
{
	float interval, repeat;
	PyObject *pyCallback;

	if (!PyArg_ParseTuple(args, "ffO", &interval, &repeat, &pyCallback))
		S_Return;

	if (!PyCallable_Check(pyCallback))
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::addTimer: '%.200s' object is not callable", 
			(pyCallback ? pyCallback->ob_type->tp_name : "NULL"));

		PyErr_PrintEx(0);
		S_Return;
	}

	ScriptTimers * pTimers = &scriptTimers();
	ScriptTimerHandler *handler = new ScriptTimerHandler(pTimers, pyCallback);

	ScriptID id = ScriptTimersUtil::addTimer(&pTimers, interval, repeat, 0, handler);

	if (id == 0)
	{
		delete handler;
		PyErr_SetString(PyExc_ValueError, "Unable to add timer");
		PyErr_PrintEx(0);
		S_Return;
	}

	return PyLong_FromLong(id);
}

//-------------------------------------------------------------------------------------
PyObject* Tool::__py_delTimer(PyObject* self, PyObject* args)
{
	ScriptID timerID;

	if (!PyArg_ParseTuple(args, "i", &timerID))
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::delTimer: args error!");
		PyErr_PrintEx(0);
		S_Return;
	}

	if (!ScriptTimersUtil::delTimer(&scriptTimers(), timerID))
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::delTimer: error!");
		PyErr_PrintEx(0);
		return PyLong_FromLong(-1);
	}

	return PyLong_FromLong(timerID);
}

//-------------------------------------------------------------------------------------

}
