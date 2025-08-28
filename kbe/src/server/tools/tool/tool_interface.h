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
	TOOL所有消息接口在此定义
*/
NETWORK_INTERFACE_DECLARE_BEGIN(ToolInterface)
	// 某app主动请求断线。
	TOOL_MESSAGE_DECLARE_ARGS0(reqClose,											NETWORK_FIXED_MESSAGE)

	// 某app主动请求look。
	TOOL_MESSAGE_DECLARE_ARGS0(lookApp,											NETWORK_FIXED_MESSAGE)

	// 某个app请求查看该app负载状态。
	TOOL_MESSAGE_DECLARE_ARGS0(queryLoad,										NETWORK_FIXED_MESSAGE)

	// console远程执行python语句。
	TOOL_MESSAGE_DECLARE_STREAM(onExecScriptCommand,								NETWORK_VARIABLE_MESSAGE)

	// 某app注册自己的接口地址到本app
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

	// dbmgr告知已经启动的其他tool或者cellapp的地址
	// 当前app需要主动的去与他们建立连接
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



	// 某app请求获取一个entityID段的回调
	TOOL_MESSAGE_DECLARE_ARGS6(onDbmgrInitCompleted,								NETWORK_VARIABLE_MESSAGE,
									GAME_TIME,										gametime, 
									ENTITY_ID,										startID,
									ENTITY_ID,										endID,
									COMPONENT_ORDER,								startGlobalOrder,
									COMPONENT_ORDER,								startGroupOrder,
									std::string,									digest)

	// 某个app向本app告知处于活动状态。
	TOOL_MESSAGE_DECLARE_ARGS2(onAppActiveTick,									NETWORK_FIXED_MESSAGE,
									COMPONENT_TYPE,									componentType, 
									COMPONENT_ID,									componentID)


	// 数据库中查询的自动entity加载信息返回 
	TOOL_MESSAGE_DECLARE_STREAM(onEntityAutoLoadCBFromDBMgr,						NETWORK_VARIABLE_MESSAGE)


	// executeRawDatabaseCommand从dbmgr的回调
	TOOL_MESSAGE_DECLARE_STREAM(onExecuteRawDatabaseCommandCB,					NETWORK_VARIABLE_MESSAGE)

	// 请求关闭服务器
	TOOL_MESSAGE_DECLARE_STREAM(reqCloseServer,									NETWORK_VARIABLE_MESSAGE)

	// 请求设置flags
	TOOL_MESSAGE_DECLARE_STREAM(reqSetFlags,										NETWORK_VARIABLE_MESSAGE)

	// 写entity到db回调。
	TOOL_MESSAGE_DECLARE_ARGS5(onWriteToDBCallback,								NETWORK_FIXED_MESSAGE,
									ENTITY_ID,										eid,
									DBID,											entityDBID,
									uint16,											dbInterfaceIndex,
									CALLBACK_ID,									callbackID,
									bool,											success)

	// createEntityFromDBID的回调
	TOOL_MESSAGE_DECLARE_STREAM(onCreateEntityFromDBIDCallback,					NETWORK_FIXED_MESSAGE)

	// 请求查询watcher数据
	TOOL_MESSAGE_DECLARE_STREAM(queryWatcher,									NETWORK_VARIABLE_MESSAGE)

	//// 充值回调
	//TOOL_MESSAGE_DECLARE_STREAM(onChargeCB,										NETWORK_VARIABLE_MESSAGE)

	// 开始profile
	TOOL_MESSAGE_DECLARE_STREAM(startProfile,									NETWORK_VARIABLE_MESSAGE)

	// 请求从数据库删除实体
	TOOL_MESSAGE_DECLARE_STREAM(deleteEntityByDBIDCB,							NETWORK_VARIABLE_MESSAGE)
	
	// lookUpEntityByDBID的回调
	TOOL_MESSAGE_DECLARE_STREAM(lookUpEntityByDBIDCB,							NETWORK_VARIABLE_MESSAGE)


	// 请求强制杀死当前app
	TOOL_MESSAGE_DECLARE_STREAM(reqKillServer,									NETWORK_VARIABLE_MESSAGE)

	

NETWORK_INTERFACE_DECLARE_END()

#ifdef DEFINE_IN_INTERFACE
	#undef DEFINE_IN_INTERFACE
#endif

}
#endif
