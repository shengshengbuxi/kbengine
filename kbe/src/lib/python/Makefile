
all:: config.status

CONFIG_ARGS =

CFLAGS += -I/usr/include/openssl
LDFLAGS += -L/usr/local/openssl/lib64 -lssl -lcrypto -ldl

ifneq ("$(wildcard /usr/local/openssl/lib64/libssl.a)", "")
	CONFIG_SSL = 
else
	CONFIG_SSL = --with-openssl=$(KBE_ROOT)/kbe/src/lib/dependencies/openssl
endif

ifeq ($(pythonbuild), release)
	CONFIG_ARGS += --with-openssl=/usr/local/openssl --disable-shared --enable-optimizations
else
ifneq (,$(findstring 64,$(KBE_CONFIG)))
	CONFIG_ARGS += --enable-optimizations --with-openssl=/usr/local/openssl --disable-shared
endif
endif

config.status:
	@chmod -R 755 .
	export KBE_ROOT=$(KBE_ROOT)
	$(SHELL) configure $(CONFIG_ARGS) $(CONFIG_SSL)
	@make

.PHONY: clean

clean::
	echo "clean"

# KBE modifys:
# 1: PCbuild vcxprojs to KBE-vcxproj

# 2: configure
# OPENSSL_LDFLAGS="-L${KBE_ROOT}/kbe/src/libs"

# 3:Modules/Setup.dist

# 4: setup.py
#       system_lib_dirs = ['/lib64', '/usr/lib64', '/lib', '/usr/lib', '$(KBE_ROOT)/kbe/src/libs']
#       system_include_dirs = ['/usr/include', '$(KBE_ROOT)/kbe/src/lib/dependencies/zlib']
