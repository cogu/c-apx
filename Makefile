include config.inc
CC ?= gcc

CFLAGS += -Wall -Wextra -O0 -g -DSW_VERSION_LITERAL=$(VERSION)

LDFLAGS += -pthread

INSTALL ?= install

BUILDDIR = build

# Directories containing source code
SRCDIR = \
	adt/src \
	apx/common/src \
	apx/server/src \
	msocket/src \
	msocket/src \
	remotefile/src \
	util/src \
	bstr/src \
	dtl_type/src \

# Source code files
C_SOURCES = \
	adt/src/adt_ary.c \
	adt/src/adt_bytearray.c \
	adt/src/adt_hash.c \
	adt/src/adt_list.c \
	adt/src/adt_stack.c \
	adt/src/adt_str.c \
	apx/common/src/apx_allocator.c \
	apx/common/src/apx_dataElement.c \
	apx/common/src/apx_dataSignature.c \
	apx/common/src/apx_dataTrigger.c \
	apx/common/src/apx_datatype.c \
	apx/common/src/apx_file.c \
	apx/common/src/apx_fileManager.c \
	apx/common/src/apx_fileMap.c \
	apx/common/src/apx_node.c \
	apx/common/src/apx_nodeData.c \
	apx/common/src/apx_nodeInfo.c \
	apx/common/src/apx_nodeManager.c \
	apx/common/src/apx_parser.c \
	apx/common/src/apx_port.c \
	apx/common/src/apx_portDataBuffer.c \
	apx/common/src/apx_portDataMap.c \
	apx/common/src/apx_portref.c \
	apx/common/src/apx_router.c \
	apx/common/src/apx_routerPortMapEntry.c \
	apx/common/src/apx_error.c \
	apx/common/src/apx_portAttributes.c \
	apx/common/src/apx_attributeParser.c \
	apx/common/src/apx_stream.c \
	apx/common/src/filestream.c \
	apx/server/src/apx_server.c \
	apx/server/src/apx_serverConnection.c \
	apx/server/src/server_main.c \
	msocket/src/msocket.c \
	msocket/src/msocket_server.c \
	msocket/src/osutil.c \
	remotefile/src/rmf.c \
	util/src/headerutil.c \
	util/src/pack.c \
	util/src/ringbuf.c \
	util/src/soa.c \
	util/src/soa_chunk.c \
	util/src/soa_fsa.c \
	util/bstr/src/bstr.c \
	util/dtl_type/src/dtl_dv.c \
	util/dtl_type/src/dtl_sv.c \
	util/dtl_type/src/dtl_av.c \
	util/dtl_type/src/dtl_hv.c \

# Paths containing interface header files
INCLUDES = \
	-I util/inc \
	-I remotefile/inc \
	-I msocket/inc \
	-I adt/inc \
	-I apx/common/inc \
	-I apx/server/inc \
	-I bstr/inc \
	-I dtl_type/inc \

	

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
