BIN  = tool
SRCS =						\
	archiver				\
	entity					\
	entity_component		\
	tool					\
	tool_interface		\
	backuper				\
	entity_autoloader		\
	initprogress_handler				\
	sync_entitystreamtemplate_handler	\
	space					\
	main					\
	profile

ASMS =

MY_LIBS =		\
	server		\
	entitydef	\
	pyscript	\
	network		\
	thread
	

BUILD_TIME_FILE = main
USE_PYTHON = 1
USE_G3DMATH=1
USE_OPENSSL = 1

ifndef NO_USE_LOG4CXX
	NO_USE_LOG4CXX = 0
	CPPFLAGS += -DLOG4CXX_STATIC
endif

CPPFLAGS += -DKBE_TOOL

#HAS_PCH = 1
USE_JWSMTP = 1

ifndef KBE_ROOT
export KBE_ROOT := $(subst /kbe/src/server/tools/$(BIN),,$(CURDIR))
endif

include $(KBE_ROOT)/kbe/src/build/common.mak

