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
		��ش���ӿ� 
	*/
	virtual void handleTimeout(TimerHandle handle, void * arg);
	virtual void handleGameTick();
	void handleCheckStatusTick();
	void handleBackup();
	void handleArchive();

	/** 
		��ʼ����ؽӿ� 
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

	/** ����ӿ�
		dbmgr��֪�Ѿ�����������tool����cellapp�ĵ�ַ
		��ǰapp��Ҫ������ȥ�����ǽ�������
	*/
	virtual void onGetEntityAppFromDbmgr(Network::Channel* pChannel, 
							int32 uid, 
							std::string& username, 
							COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ORDER globalorderID, COMPONENT_ORDER grouporderID,
							uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx);
	
	/** ����ӿ�
		ĳ��client��app��֪���ڻ״̬��
	*/
	//void onClientActiveTick(Network::Channel* pChannel);

	/** ����ӿ�
		���ݿ��в�ѯ���Զ�entity������Ϣ����
	*/
	void onEntityAutoLoadCBFromDBMgr(Network::Channel* pChannel, MemoryStream& s);

	/** ����ӿ�
		��������flags
	*/
	void reqSetFlags(Network::Channel* pChannel, MemoryStream& s);

	/** 
		������һ��entity�ص�
	*/
	virtual Entity* onCreateEntity(PyObject* pyEntity, ScriptDefModule* sm, ENTITY_ID eid);

	/** 
		����һ��entity 
	*/
	static PyObject* __py_createEntityLocally(PyObject* self, PyObject* args);
	static PyObject* __py_createEntity(PyObject* self, PyObject* args);
	static PyObject* __py_createEntityFromDBID(PyObject* self, PyObject* args);
	/** 
		��db��ȡ��Ϣ����һ��entity
	*/
	void createEntityFromDBID(const char* entityType, DBID dbid, PyObject* pyCallback, const std::string& dbInterfaceName);

	/** ����ӿ�
		createEntityFromDBID�Ļص���
	*/
	void onCreateEntityFromDBIDCallback(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 
		��dbmgr����ִ��һ�����ݿ�����
	*/
	static PyObject* __py_executeRawDatabaseCommand(PyObject* self, PyObject* args);
	void executeRawDatabaseCommand(const char* datas, uint32 size, PyObject* pycallback, ENTITY_ID eid, const std::string& dbInterfaceName);
	void onExecuteRawDatabaseCommandCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** ����ӿ�
		dbmgr���ͳ�ʼ��Ϣ
		startID: ��ʼ����ENTITY_ID ����ʼλ��
		endID: ��ʼ����ENTITY_ID �ν���λ��
		startGlobalOrder: ȫ������˳�� �������ֲ�ͬ���
		startGroupOrder: ��������˳�� ����������tool�еڼ���������
		machineGroupOrder: ��machine����ʵ����˳��, �ṩ�ײ���ĳЩʱ���ж��Ƿ�Ϊ��һ��toolʱʹ��
	*/
	void onDbmgrInitCompleted(Network::Channel* pChannel, 
		GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID, COMPONENT_ORDER startGlobalOrder, 
		COMPONENT_ORDER startGroupOrder, const std::string& digest);

	
	/**
		��ȡ��Ϸʱ��
	*/
	static PyObject* __py_gametime(PyObject* self, PyObject* args);

	/** ����ӿ�
		дentity��db�ص�
	*/
	void onWriteToDBCallback(Network::Channel* pChannel, ENTITY_ID eid, DBID entityDBID, 
		uint16 dbInterfaceIndex, CALLBACK_ID callbackID, bool success);

	/**
		����proxices����
	*/
	void incProxicesCount() { ++numProxices_; }

	/**
		����proxices����
	*/
	void decProxicesCount() { --numProxices_; }

	/**
		���proxices����
	*/
	int32 numProxices() const { return numProxices_; }

	/**
		���numClients����
	*/
	int32 numClients() { return this->networkInterface().numExtChannels(); }
	

	/** ����ӿ�
		����������APP���ѻָ����ؽ��
	*/
	void onRequestRestoreCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);


	/** ����ӿ�
		ĳ��app����鿴��app
	*/
	virtual void lookApp(Network::Channel* pChannel);


	/**
		���µ������еĽű�
	*/
	static PyObject* __py_reloadScript(PyObject* self, PyObject* args);
	virtual void reloadScript(bool fullReload);
	virtual void onReloadScript(bool fullReload);

	/**
		��ȡ�����Ƿ����ڹر���
	*/
	static PyObject* __py_isShuttingDown(PyObject* self, PyObject* args);

	/**
		��ȡ�����ڲ������ַ
	*/
	static PyObject* __py_address(PyObject* self, PyObject* args);

	/**
		ͨ��dbid�����ݿ���ɾ��һ��ʵ��

		�����ݿ�ɾ��ʵ�壬 ���ʵ�岻���������ֱ��ɾ���ص�����true�� ���������ص����ص���entity��entityCall�� �����κ�ԭ�򶼷���false.
	*/
	static PyObject* __py_deleteEntityByDBID(PyObject* self, PyObject* args);

	/** ����ӿ�
		ͨ��dbid�����ݿ���ɾ��һ��ʵ��Ļص�
	*/
	void deleteEntityByDBIDCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/**
		ͨ��dbid��ѯһ��ʵ���Ƿ�����ݿ���

		���ʵ�����߻ص�����baseentitycall�����ʵ�岻������ص�����true�������κ�ԭ�򶼷���false.
	*/
	static PyObject* __py_lookUpEntityByDBID(PyObject* self, PyObject* args);

	/** ����ӿ�
		���ʵ�����߻ص�����baseentitycall�����ʵ�岻������ص�����true�������κ�ԭ�򶼷���false.
	*/
	void lookUpEntityByDBIDCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);


	uint32 flags() const { return flags_; }
	void flags(uint32 v) { flags_ = v; }
	static PyObject* __py_setFlags(PyObject* self, PyObject* args);
	static PyObject* __py_getFlags(PyObject* self, PyObject* args);

	
	/** Timer����
	*/
	static PyObject* __py_addTimer(PyObject* self, PyObject* args);
	static PyObject* __py_delTimer(PyObject* self, PyObject* args);

	static ScriptTimers &scriptTimers() { return scriptTimers_; }

	
protected:
	static ScriptTimers										scriptTimers_;

	TimerHandle												loopCheckTimerHandle_;


	// ���ݴ浵���
	KBEShared_ptr< Backuper >								pBackuper_;	
	KBEShared_ptr< Archiver >								pArchiver_;	

	int32													numProxices_;

	TelnetServer*											pTelnetServer_;


	TimerHandle												pResmgrTimerHandle_;

	InitProgressHandler*									pInitProgressHandler_;
	
	// APP�ı�־
	uint32													flags_;
};

}

#endif // KBE_TOOL_H
