# -*- coding: utf-8 -*-
from KBEDebug import *
import KBEngine

def onToolAppReady():
    """
    KBEngine method.
    Tool已经准备好了
    """
        

                             
def onReadyForShutDown():
    """
    KBEngine method.
    进程询问脚本层：我要shutdown了，脚本是否准备好了？
    如果返回True，则进程会进入shutdown的流程，其它值会使得进程在过一段时间后再次询问。
    用户可以在收到消息时进行脚本层的数据清理工作，以让脚本层的工作成果不会因为shutdown而丢失。
    """
    INFO_MSG("onReadyForShutDown()")
    return True


def onToolShutDown(state):
    """
    KBEngine method.
    这个Patch被关闭前的回调函数
    @param state: 0 : 在断开所有客户端之前
                              1 : 在将所有entity写入数据库之前
                              2 : 所有entity被写入数据库之后
    @type state: int
    """
    INFO_MSG("onToolShutDown: state=%i" % state)


def onInit(isReload):
    """
    KBEngine method.
    当引擎启动后初始化完所有的脚本后这个接口被调用
    @param isReload: 是否是被重写加载脚本后触发的
    @type isReload: bool
    """
    INFO_MSG("onInit::isReload:%s" % isReload)

    # ToolTask.start()


def onFini():
    """
    KBEngine method.
    引擎正式关闭
    """
    INFO_MSG("onFini()")

