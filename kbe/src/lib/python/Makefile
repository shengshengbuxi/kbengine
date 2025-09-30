# @GeneratedBy Trae AI Assistant v1.0
# KBEngine Python 3.12.11 Makefile for Linux/CentOS compilation
# Based on the working Python 3.7 Makefile from kbeold

all:: config.status

CONFIG_ARGS =

# SSL configuration - check for system SSL or use KBEngine's OpenSSL
ifneq ("$(wildcard /usr/lib/x86_64-linux-gnu/libssl.a)", "")
	CONFIG_SSL =
else
	CONFIG_SSL = --with-openssl=$(KBE_ROOT)/kbe/src/lib/dependencies/openssl
endif

# Optimization configuration based on build type
ifeq ($(pythonbuild), release)
	CONFIG_ARGS += --enable-optimizations 
else
ifneq (,$(findstring 64,$(KBE_CONFIG)))
	CONFIG_ARGS += --enable-optimizations 
endif
endif

config.status:
	@chmod -R 755 .
	export KBE_ROOT=$(KBE_ROOT)
	$(SHELL) configure $(CONFIG_ARGS) $(CONFIG_SSL)
	@make

install: 
	@chmod -R 755 .
	export KBE_ROOT=$(KBE_ROOT) && \
	export CPPFLAGS="-I$(KBE_ROOT)/kbe/src/lib/dependencies/zlib -I$(KBE_ROOT)/kbe/src/lib/dependencies/openssl/include" && \
	export LDFLAGS="-L$(KBE_ROOT)/kbe/src/libs" && \
	$(SHELL) configure --prefix=$(KBE_ROOT)/kbe/src/lib/python/install \
		--enable-shared=no \
		--enable-static=yes \
		--with-ensurepip=no \
		--disable-test-modules \
		$(CONFIG_SSL) && \
	make && make install
	cp -f "$(KBE_ROOT)/kbe/src/lib/python/install/lib/libpython3.12.a" "$(KBE_ROOT)/kbe/src/libs/libpython.a"
	mkdir -p "$(KBE_ROOT)/kbe/res/scripts/common/lib-dynload/"
	if [ -d "$(KBE_ROOT)/kbe/src/lib/python/install/lib/python3.12/lib-dynload" ]; then \
		cp -rf "$(KBE_ROOT)/kbe/src/lib/python/install/lib/python3.12/lib-dynload/"* "$(KBE_ROOT)/kbe/res/scripts/common/lib-dynload/"; \
	fi
	rm -rf "$(KBE_ROOT)/kbe/res/scripts/common/Lib/"*
	cp -rf "$(KBE_ROOT)/kbe/src/lib/python/Lib" "$(KBE_ROOT)/kbe/res/scripts/common/Lib"

.PHONY: clean

clean::
	echo "clean"

# KBE modifys:
# 1: PCbuild vcxprojs to KBE-vcxproj

# 2: configure
# OPENSSL_LDFLAGS="-L${KBE_ROOT}/kbe/src/libs"

# 3: Modules/Setup.dist -> Modules/Setup (Python 3.12 change)

# 4: setup.py
#    system_lib_dirs = ['/lib64', '/usr/lib64', '/lib', '/usr/lib', '$(KBE_ROOT)/kbe/src/libs']
#    system_include_dirs = ['/usr/include', '$(KBE_ROOT)/kbe/src/lib/dependencies/zlib']

# 5: Modules/Setup SSL and zlib configurations:
#    SSL=$(KBE_ROOT)/kbe/src/lib/dependencies/vsopenssl
#    _ssl _ssl.c -I$(SSL)/include -L$(SSL)/lib -lssl -lcrypto
#    zlib zlibmodule.c -I$(KBE_ROOT)/kbe/src/lib/dependencies/zlib -L$(KBE_ROOT)/kbe/src/libs -lz