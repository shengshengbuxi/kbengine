// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com


#ifndef KBE_TOOL_H
#define KBE_TOOL_H
	
// common include	
#include "entity.h"
//#include "proxy.h"
#include "profile.h"
#include "server/entity_app.h"
//#include "server/pendingLoginmgr.h"
//#include "server/forward_messagebuffer.h"
#include "network/endpoint.h"

//#define NDEBUG
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

namespace Network{
	class Channel;
	class Bundle;
}

//class Proxy;
class Backuper;
class Archiver;
class TelnetServer;
//class RestoreEntityHandler;
class InitProgressHandler;

class Tool :	public EntityApp<Entity>,
				public Singleton<Tool>
{
public:
	enum TimeOutType
	{
		TIMEOUT_CHECK_STATUS = TIMEOUT_ENTITYAPP_MAX + 1,
		TIMEOUT_MAX
	};
	
	Tool(Network::EventDispatcher& dispatcher, 
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~Tool();
	
	virtual bool installPyModules();
	virtual void onInstallPyModules();
	virtual bool uninstallPyModules();

	bool run();
	
	/** 
		相关处理接口 
	*/
	virtual void handleTimeout(TimerHandle handle, void * arg);
	virtual void handleGameTick();
	void handleCheckStatusTick();
	void handleBackup();
	void handleArchive();

	/** 
		初始化相关接口 
	*/
	bool initialize();
	bool initializeBegin();
	bool initializeEnd();
	void finalise();
	
	virtual ShutdownHandler::CAN_SHUTDOWN_STATE canShutdown();
	virtual void onShutdownBegin();
	virtual void onShutdown(bool first);
	virtual void onShutdownEnd();

	virtual bool initializeWatcher();

	static PyObject* __py_quantumPassedPercent(PyObject* self, PyObject* args);
	float _getLoad() const { return getLoad(); }
	virtual void onUpdateLoad();

	virtual void onChannelDeregister(Network::Channel * pChannel);

	/** 网络接口
		dbmgr告知已经启动的其他tool或者cellapp的地址
		当前app需要主动的去与他们建立连接
	*/
	virtual void onGetEntityAppFromDbmgr(Network::Channel* pChannel, 
							int32 uid, 
							std::string& username, 
							COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ORDER globalorderID, COMPONENT_ORDER grouporderID,
							uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx);
	
	/** 网络接口
		某个client向本app告知处于活动状态。
	*/
	//void onClientActiveTick(Network::Channel* pChannel);

	/** 网络接口
		数据库中查询的自动entity加载信息返回
	*/
	void onEntityAutoLoadCBFromDBMgr(Network::Channel* pChannel, MemoryStream& s);

	/** 网络接口
		请求设置flags
	*/
	void reqSetFlags(Network::Channel* pChannel, MemoryStream& s);

	/** 
		创建了一个entity回调
	*/
	virtual Entity* onCreateEntity(PyObject* pyEntity, ScriptDefModule* sm, ENTITY_ID eid);

	/** 
		创建一个entity 
	*/
	static PyObject* __py_createEntityLocally(PyObject* self, PyObject* args);
	static PyObject* __py_createEntity(PyObject* self, PyObject* args);
	static PyObject* __py_createEntityFromDBID(PyObject* self, PyObject* args);
	/** 
		从db获取信息创建一个entity
	*/
	void createEntityFromDBID(const char* entityType, DBID dbid, PyObject* pyCallback, const std::string& dbInterfaceName);

	/** 网络接口
		createEntityFromDBID的回调。
	*/
	void onCreateEntityFromDBIDCallback(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 
		向dbmgr请求执行一个数据库命令
	*/
	static PyObject* __py_executeRawDatabaseCommand(PyObject* self, PyObject* args);
	void executeRawDatabaseCommand(const char* datas, uint32 size, PyObject* pycallback, ENTITY_ID eid, const std::string& dbInterfaceName);
	void onExecuteRawDatabaseCommandCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		dbmgr发送初始信息
		startID: 初始分配ENTITY_ID 段起始位置
		endID: 初始分配ENTITY_ID 段结束位置
		startGlobalOrder: 全局启动顺序 包括各种不同组件
		startGroupOrder: 组内启动顺序， 比如在所有tool中第几个启动。
		machineGroupOrder: 在machine中真实的组顺序, 提供底层在某些时候判断是否为第一个tool时使用
	*/
	void onDbmgrInitCompleted(Network::Channel* pChannel, 
		GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID, COMPONENT_ORDER startGlobalOrder, 
		COMPONENT_ORDER startGroupOrder, const std::string& digest);

	
	/**
		获取游戏时间
	*/
	static PyObject* __py_gametime(PyObject* self, PyObject* args);

	/** 网络接口
		写entity到db回调
	*/
	void onWriteToDBCallback(Network::Channel* pChannel, ENTITY_ID eid, DBID entityDBID, 
		uint16 dbInterfaceIndex, CALLBACK_ID callbackID, bool success);

	/**
		增加proxices计数
	*/
	void incProxicesCount() { ++numProxices_; }

	/**
		减少proxices计数
	*/
	void decProxicesCount() { --numProxices_; }

	/**
		获得proxices计数
	*/
	int32 numProxices() const { return numProxices_; }

	/**
		获得numClients计数
	*/
	int32 numClients() { return this->networkInterface().numExtChannels(); }
	

	/** 网络接口
		请求在其他APP灾难恢复返回结果
	*/
	void onRequestRestoreCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);


	/** 网络接口
		某个app请求查看该app
	*/
	virtual void lookApp(Network::Channel* pChannel);


	/**
		重新导入所有的脚本
	*/
	static PyObject* __py_reloadScript(PyObject* self, PyObject* args);
	virtual void reloadScript(bool fullReload);
	virtual void onReloadScript(bool fullReload);

	/**
		获取进程是否正在关闭中
	*/
	static PyObject* __py_isShuttingDown(PyObject* self, PyObject* args);

	/**
		获取进程内部网络地址
	*/
	static PyObject* __py_address(PyObject* self, PyObject* args);

	/**
		通过dbid从数据库中删除一个实体

		从数据库删除实体， 如果实体不在线则可以直接删除回调返回true， 如果在线则回调返回的是entity的entityCall， 其他任何原因都返回false.
	*/
	static PyObject* __py_deleteEntityByDBID(PyObject* self, PyObject* args);

	/** 网络接口
		通过dbid从数据库中删除一个实体的回调
	*/
	void deleteEntityByDBIDCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/**
		通过dbid查询一个实体是否从数据库检出

		如果实体在线回调返回baseentitycall，如果实体不在线则回调返回true，其他任何原因都返回false.
	*/
	static PyObject* __py_lookUpEntityByDBID(PyObject* self, PyObject* args);

	/** 网络接口
		如果实体在线回调返回baseentitycall，如果实体不在线则回调返回true，其他任何原因都返回false.
	*/
	void lookUpEntityByDBIDCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);


	uint32 flags() const { return flags_; }
	void flags(uint32 v) { flags_ = v; }
	static PyObject* __py_setFlags(PyObject* self, PyObject* args);
	static PyObject* __py_getFlags(PyObject* self, PyObject* args);

	
	/** Timer操作
	*/
	static PyObject* __py_addTimer(PyObject* self, PyObject* args);
	static PyObject* __py_delTimer(PyObject* self, PyObject* args);

	static ScriptTimers &scriptTimers() { return scriptTimers_; }

	
protected:
	static ScriptTimers										scriptTimers_;

	TimerHandle												loopCheckTimerHandle_;


	// 备份存档相关
	KBEShared_ptr< Backuper >								pBackuper_;	
	KBEShared_ptr< Archiver >								pArchiver_;	

	int32													numProxices_;

	TelnetServer*											pTelnetServer_;


	TimerHandle												pResmgrTimerHandle_;

	InitProgressHandler*									pInitProgressHandler_;
	
	// APP的标志
	uint32													flags_;
};

}

#endif // KBE_TOOL_H
