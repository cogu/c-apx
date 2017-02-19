
CC ?= gcc

CFLAGS += -Wall -Wextra -O0 -g

LDFLAGS += -pthread

INSTALL ?= install

ARCH ?= $(shell arch)

BUILDDIR = build/$(ARCH)

# Directories containing source code
SRCDIR = \
	../../adt/src \
	../../apx/common/src \
	../../apx/server/src \
	../../msocket/src \
	../../msocket/src \
	../../remotefile/src \
	../../util/src \

# Source code files
C_SOURCES = \
	../../adt/src/adt_ary.c \
	../../adt/src/adt_bytearray.c \
	../../adt/src/adt_hash.c \
	../../adt/src/adt_list.c \
	../../adt/src/adt_stack.c \
	../../adt/src/adt_str.c \
	../../apx/common/src/apx_allocator.c \
	../../apx/common/src/apx_dataElement.c \
	../../apx/common/src/apx_dataSignature.c \
	../../apx/common/src/apx_dataTrigger.c \
	../../apx/common/src/apx_datatype.c \
	../../apx/common/src/apx_file.c \
	../../apx/common/src/apx_fileManager.c \
	../../apx/common/src/apx_fileMap.c \
	../../apx/common/src/apx_node.c \
	../../apx/common/src/apx_nodeData.c \
	../../apx/common/src/apx_nodeInfo.c \
	../../apx/common/src/apx_nodeManager.c \
	../../apx/common/src/apx_parser.c \
	../../apx/common/src/apx_port.c \
	../../apx/common/src/apx_portDataBuffer.c \
	../../apx/common/src/apx_portDataMap.c \
	../../apx/common/src/apx_portref.c \
	../../apx/common/src/apx_router.c \
	../../apx/common/src/apx_routerPortMapEntry.c \
	../../apx/common/src/apx_stream.c \
	../../apx/common/src/filestream.c \
	../../apx/server/src/apx_server.c \
	../../apx/server/src/apx_serverConnection.c \
	../../apx/server/src/server_main.c \
	../../msocket/src/msocket.c \
	../../msocket/src/msocket_server.c \
	../../msocket/src/osutil.c \
	../../remotefile/src/rmf.c \
	../../util/src/headerutil.c \
	../../util/src/pack.c \
	../../util/src/pstr.c \
	../../util/src/ringbuf.c \
	../../util/src/scan.c \
	../../util/src/soa.c \
	../../util/src/soa_chunk.c \
	../../util/src/soa_fsa.c \
	
	
# Paths containing interface header files
INCLUDES = \
	-I ../../util/inc \
	-I ../../remotefile/inc \
	-I ../../msocket/inc \
	-I ../../adt/inc \
	-I ../common/inc \
	-I ../server/inc \
	-I inc
	

EXECUTABLE = $(BUILDDIR)/apx_server

OBJECTS = \
	$(addprefix $(BUILDDIR)/, $(notdir $(C_SOURCES:.c=.o)))

DEPS = $(patsubst %.o,%.d,$(OBJECTS))

vpath %.c $(SRCDIR)

all: $(BUILDDIR) $(EXECUTABLE)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

$(BUILDDIR)/%.o : %.c
	$(CC) -MD -MT $@ -MF $(patsubst %.o,%.d,$@) -c $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean install

.NOTPARALLEL:

-include $(DEPS)
