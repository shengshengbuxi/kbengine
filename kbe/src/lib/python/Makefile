# @GeneratedBy Trae AI Assistant v1.0
# KBEngine Python 3.12.11 Makefile for Linux/CentOS compilation
# This Makefile handles the compilation of Python 3.12.11 for KBEngine

ifndef KBE_ROOT
export KBE_ROOT := $(subst /kbe/src/lib/python,,$(CURDIR))
endif

ifndef KBE_CONFIG
	KBE_CONFIG=Hybrid
	ifeq ($(shell uname -m),x86_64)
		 KBE_CONFIG=Hybrid64
	endif
endif

# Python version configuration
PYTHON_VERSION=3.12
PYTHON_FULL_VERSION=3.12.11
PYTHON_LIB_NAME=libpython$(PYTHON_VERSION).a

# Build directories
BUILD_DIR=$(CURDIR)/build
INSTALL_DIR=$(CURDIR)/install
LIB_OUTPUT_DIR=$(KBE_ROOT)/kbe/src/libs

# Configure flags for Python compilation
CONFIGURE_FLAGS = --prefix=$(INSTALL_DIR) \
                  --enable-shared=no \
                  --enable-static=yes \
                  --with-ensurepip=no \
                  --disable-test-modules \
                  --enable-optimizations \
                  --with-lto

# Compiler flags
CFLAGS += -fPIC -O3
CPPFLAGS += -I$(CURDIR)/Include
LDFLAGS += -L$(LIB_OUTPUT_DIR)

# Export environment variables
export CFLAGS CPPFLAGS LDFLAGS

.PHONY: all clean install full

all: $(PYTHON_LIB_NAME)

$(PYTHON_LIB_NAME): configure
	@echo "Building Python $(PYTHON_FULL_VERSION) for KBEngine..."
	$(MAKE) -j$(shell nproc)
	@echo "Installing Python libraries..."
	$(MAKE) install
	@echo "Copying Python library to KBEngine libs directory..."
	cp -f "$(INSTALL_DIR)/lib/$(PYTHON_LIB_NAME)" "$(LIB_OUTPUT_DIR)/libpython.a"
	@echo "Setting up Python runtime environment..."
	mkdir -p "$(KBE_ROOT)/kbe/res/scripts/common/lib-dynload/"
	if [ -d "$(INSTALL_DIR)/lib/python$(PYTHON_VERSION)/lib-dynload" ]; then \
		cp -rf "$(INSTALL_DIR)/lib/python$(PYTHON_VERSION)/lib-dynload/"* "$(KBE_ROOT)/kbe/res/scripts/common/lib-dynload/"; \
	fi
	rm -rf "$(KBE_ROOT)/kbe/res/scripts/common/Lib/"*
	cp -rf "$(CURDIR)/Lib" "$(KBE_ROOT)/kbe/res/scripts/common/Lib"

configure: Makefile.pre.in
	@echo "Configuring Python $(PYTHON_FULL_VERSION)..."
	./configure $(CONFIGURE_FLAGS)

Makefile.pre.in:
	@echo "Error: Python source not properly configured. Please ensure Python $(PYTHON_FULL_VERSION) source is available."
	@false

install: all

clean:
	@echo "Cleaning Python build..."
	if [ -f Makefile ]; then $(MAKE) clean; fi
	rm -rf $(BUILD_DIR) $(INSTALL_DIR)
	rm -f Makefile pyconfig.h

full: clean all

# Help target
help:
	@echo "KBEngine Python $(PYTHON_FULL_VERSION) Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all     - Build Python library (default)"
	@echo "  clean   - Clean build artifacts"
	@echo "  install - Build and install Python library"
	@echo "  full    - Clean and rebuild everything"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  KBE_CONFIG: $(KBE_CONFIG)"
	@echo "  Python Version: $(PYTHON_FULL_VERSION)"
	@echo "  Build Directory: $(BUILD_DIR)"
	@echo "  Install Directory: $(INSTALL_DIR)"