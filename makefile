# Build IOTrace with the Intel Pin build system.
#
#   make PIN_ROOT=/path/to/pin-kit            -> obj-intel64/iotrace.so
#   make PIN_ROOT=/path/to/pin-kit TARGET=ia32 -> obj-ia32/iotrace.so
#   make PIN_ROOT=/path/to/pin-kit clean

ifndef PIN_ROOT
$(error PIN_ROOT is not set. Run: make PIN_ROOT=/path/to/pin-kit)
endif

CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config
include makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules
