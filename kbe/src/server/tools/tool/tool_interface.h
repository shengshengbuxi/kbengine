// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com


#if defined(DEFINE_IN_INTERFACE)
	#undef KBE_TOOL_INTERFACE_H
#endif


#ifndef KBE_TOOL_INTERFACE_H
#define KBE_TOOL_INTERFACE_H

// common include	
#if defined(TOOL)
#include "tool.h"
#endif
#include "tool_interface_macros.h"
#include "entity_interface_macros.h"
//#include "proxy_interface_macros.h"
#include "network/interface_defs.h"
#include "server/server_errors.h"
//#define NDEBUG
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

/**
	TOOL������Ϣ�ӿ��ڴ˶���
*/
NETWORK_INTERFACE_DECLARE_BEGIN(ToolInterface)
	// ĳapp����������ߡ�
	TOOL_MESSAGE_DECLARE_ARGS0(reqClose,											NETWORK_FIXED_MESSAGE)

	// ĳapp��������look��
	TOOL_MESSAGE_DECLARE_ARGS0(lookApp,											NETWORK_FIXED_MESSAGE)

	// ĳ��app����鿴��app����״̬��
	TOOL_MESSAGE_DECLARE_ARGS0(queryLoad,										NETWORK_FIXED_MESSAGE)

	// consoleԶ��ִ��python��䡣
	TOOL_MESSAGE_DECLARE_STREAM(onExecScriptCommand,								NETWORK_VARIABLE_MESSAGE)

	// ĳappע���Լ��Ľӿڵ�ַ����app
	TOOL_MESSAGE_DECLARE_ARGS11(onRegisterNewApp,								NETWORK_VARIABLE_MESSAGE,
									int32,											uid, 
									std::string,									username,
									COMPONENT_TYPE,									componentType, 
									COMPONENT_ID,									componentID, 
									COMPONENT_ORDER,								globalorderID,
									COMPONENT_ORDER,								grouporderID,
									uint32,											intaddr, 
									uint16,											intport,
									uint32,											extaddr, 
									uint16,											extport,
									std::string,									extaddrEx)

	// dbmgr��֪�Ѿ�����������tool����cellapp�ĵ�ַ
	// ��ǰapp��Ҫ������ȥ�����ǽ�������
	TOOL_MESSAGE_DECLARE_ARGS11(onGetEntityAppFromDbmgr,							NETWORK_VARIABLE_MESSAGE,
									int32,											uid, 
									std::string,									username,
									COMPONENT_TYPE,									componentType, 
									COMPONENT_ID,									componentID, 
									COMPONENT_ORDER,								globalorderID,
									COMPONENT_ORDER,								grouporderID,
									uint32,											intaddr, 
									uint16,											intport,
									uint32,											extaddr, 
									uint16,											extport,
									std::string,									extaddrEx)



	// ĳapp�����ȡһ��entityID�εĻص�
	TOOL_MESSAGE_DECLARE_ARGS6(onDbmgrInitCompleted,								NETWORK_VARIABLE_MESSAGE,
									GAME_TIME,										gametime, 
									ENTITY_ID,										startID,
									ENTITY_ID,										endID,
									COMPONENT_ORDER,								startGlobalOrder,
									COMPONENT_ORDER,								startGroupOrder,
									std::string,									digest)

	// ĳ��app��app��֪���ڻ״̬��
	TOOL_MESSAGE_DECLARE_ARGS2(onAppActiveTick,									NETWORK_FIXED_MESSAGE,
									COMPONENT_TYPE,									componentType, 
									COMPONENT_ID,									componentID)


	// ���ݿ��в�ѯ���Զ�entity������Ϣ���� 
	TOOL_MESSAGE_DECLARE_STREAM(onEntityAutoLoadCBFromDBMgr,						NETWORK_VARIABLE_MESSAGE)


	// executeRawDatabaseCommand��dbmgr�Ļص�
	TOOL_MESSAGE_DECLARE_STREAM(onExecuteRawDatabaseCommandCB,					NETWORK_VARIABLE_MESSAGE)

	// ����رշ�����
	TOOL_MESSAGE_DECLARE_STREAM(reqCloseServer,									NETWORK_VARIABLE_MESSAGE)

	// ��������flags
	TOOL_MESSAGE_DECLARE_STREAM(reqSetFlags,										NETWORK_VARIABLE_MESSAGE)

	// дentity��db�ص���
	TOOL_MESSAGE_DECLARE_ARGS5(onWriteToDBCallback,								NETWORK_FIXED_MESSAGE,
									ENTITY_ID,										eid,
									DBID,											entityDBID,
									uint16,											dbInterfaceIndex,
									CALLBACK_ID,									callbackID,
									bool,											success)

	// createEntityFromDBID�Ļص�
	TOOL_MESSAGE_DECLARE_STREAM(onCreateEntityFromDBIDCallback,					NETWORK_FIXED_MESSAGE)

	// �����ѯwatcher����
	TOOL_MESSAGE_DECLARE_STREAM(queryWatcher,									NETWORK_VARIABLE_MESSAGE)

	//// ��ֵ�ص�
	//TOOL_MESSAGE_DECLARE_STREAM(onChargeCB,										NETWORK_VARIABLE_MESSAGE)

	// ��ʼprofile
	TOOL_MESSAGE_DECLARE_STREAM(startProfile,									NETWORK_VARIABLE_MESSAGE)

	// ��������ݿ�ɾ��ʵ��
	TOOL_MESSAGE_DECLARE_STREAM(deleteEntityByDBIDCB,							NETWORK_VARIABLE_MESSAGE)
	
	// lookUpEntityByDBID�Ļص�
	TOOL_MESSAGE_DECLARE_STREAM(lookUpEntityByDBIDCB,							NETWORK_VARIABLE_MESSAGE)


	// ����ǿ��ɱ����ǰapp
	TOOL_MESSAGE_DECLARE_STREAM(reqKillServer,									NETWORK_VARIABLE_MESSAGE)

	

NETWORK_INTERFACE_DECLARE_END()

#ifdef DEFINE_IN_INTERFACE
	#undef DEFINE_IN_INTERFACE
#endif

}
#endif
