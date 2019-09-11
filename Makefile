include config.inc
CC ?= gcc

CFLAGS += -Wall -O0 -g -DSW_VERSION_LITERAL=$(VERSION) -DADT_RBFH_ENABLE=1 -DPLATFORM_BYTE_ORDER=0

LDFLAGS += -pthread

INSTALL ?= install

BUILDDIR = build

# Directories containing source code
SRCDIR = \
	adt/src \
	apx/common/src \
	apx/server/src \
	apx/server_extension/socket/src \
	msocket/src \
	msocket/src \
	remotefile/src \
	cutil/src \
	bstr/src \
	dtl_type/src \
	dtl_json/src \

# Source code files
SHARED_SOURCES = \
	adt/src/adt_ary.c \
	adt/src/adt_bytearray.c \
	adt/src/adt_bytes.c \
	adt/src/adt_hash.c \
	adt/src/adt_list.c \
	adt/src/adt_stack.c \
	adt/src/adt_str.c \
	adt/src/adt_set.c \
	adt/src/adt_ringbuf.c \
	apx/common/src/apx_allocator.c \
	apx/common/src/apx_attributeParser.c \
	apx/common/src/apx_bytePortMap.c \
	apx/common/src/apx_compiler.c \
	apx/common/src/apx_connectionBase.c \
	apx/common/src/apx_dataElement.c \
	apx/common/src/apx_dataSignature.c \
	apx/common/src/apx_datatype.c \
	apx/common/src/apx_event.c \
	apx/common/src/apx_eventListener.c \
	apx/common/src/apx_eventLoop.c \
	apx/common/src/apx_file2.c \
	apx/common/src/apx_fileManager.c \
	apx/common/src/apx_fileManagerLocal.c \
	apx/common/src/apx_fileManagerRemote.c \
	apx/common/src/apx_fileManagerShared.c \
	apx/common/src/apx_fileMap.c \
	apx/common/src/apx_logEvent.c \
	apx/common/src/apx_node.c \
	apx/common/src/apx_nodeData.c \
	apx/common/src/apx_nodeDataManager.c \
	apx/common/src/apx_parser.c \
	apx/common/src/apx_port.c \
	apx/common/src/apx_portAttributes.c \
	apx/common/src/apx_portConnectionEntry.c \
	apx/common/src/apx_portConnectionTable.c \
	apx/common/src/apx_portDataMap.c \
	apx/common/src/apx_portDataProps.c \
	apx/common/src/apx_portDataRef.c \
	apx/common/src/apx_portTriggerList.c \
	apx/common/src/apx_routingTable.c \
	apx/common/src/apx_routingTableEntry.c \
	apx/common/src/apx_stream.c \
	apx/common/src/apx_typeAttribute.c \
	apx/common/src/apx_vm.c \
	apx/common/src/apx_vmSerializer.c \
	bstr/src/bstr.c \
	cutil/src/pack.c \
	cutil/src/filestream.c \
	cutil/src/soa.c \
	cutil/src/soa_chunk.c \
	cutil/src/soa_fsa.c \
	dtl_json/src/dtl_json_reader.c \
	dtl_json/src/dtl_json_writer.c \
	dtl_type/src/dtl_dv.c \
	dtl_type/src/dtl_sv.c \
	dtl_type/src/dtl_av.c \
	dtl_type/src/dtl_hv.c \
	msocket/src/msocket.c \
	msocket/src/msocket_server.c \
	msocket/src/osutil.c \
	remotefile/src/rmf.c \
	remotefile/src/numheader.c \

SERVER_SOURCES = apx/server/src/apx_connectionManager.c \
    apx/server/src/apx_server.c \
	apx/server/src/apx_serverConnectionBase.c \
	apx/server/src/apx_serverExtension.c \

SERVER_EXTENSION_SOURCES ?= apx/server_extension/socket/apx_serverSocketConnection.c \
	apx/server_extension/socket/apx_serverSocketExtension.c \
	apx/server_extension/socket/apx_socketServer.c \

SERVER_EXTENSION_INCLUDES ?= -I apx/server_extension/socket/inc

SERVER_MAIN_SOURCE ?= apx/server/src/server_main.c

LIB_SOURCES = $(SHARED_SOURCES)

# Paths containing interface header files
INCLUDES = \
	-I adt/inc \
	-I apx/common/inc \
	-I apx/server/inc \
	-I bstr/inc \
	-I cutil/inc \
	-I dtl_json/inc \
	-I dtl_type/inc \
	-I msocket/inc \
	-I remotefile/inc \
	$(SERVER_EXTENSION_INCLUDES)

EXECUTABLE = $(BUILDDIR)/apx_server
CLIENTLIB = $(BUILDDIR)/libapxclient.a

SHARED_OBJECTS = \
	$(addprefix $(BUILDDIR)/, $(notdir $(SHARED_SOURCES:.c=.o)))

SERVER_OBJECTS = \
	$(addprefix $(BUILDDIR)/, $(notdir $(SERVER_SOURCES:.c=.o)))

SERVER_EXT_OBJECTS = \
	$(addprefix $(BUILDDIR)/, $(notdir $(SERVER_EXTENSION_SOURCES:.c=.o)))

SERVER_MAIN_OBJECT = \
	$(addprefix $(BUILDDIR)/, $(notdir $(SERVER_MAIN_SOURCE:.c=.o)))

DEPS = $(patsubst %.o,%.d,$(OBJECTS))

vpath %.c $(SRCDIR)

server: $(BUILDDIR) $(EXECUTABLE)

lib: $(BUILDDIR) $(CLIENTLIB)

all: server lib

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(EXECUTABLE): $(SHARED_OBJECTS) $(SERVER_OBJECTS) $(SERVER_EXT_OBJECTS) $(SERVER_MAIN_OBJECT)
	$(CC) $(SHARED_OBJECTS) $(SERVER_OBJECTS) $(SERVER_EXT_OBJECTS) $(SERVER_MAIN_OBJECT) $(LDFLAGS) -o $(EXECUTABLE)

$(CLIENTLIB): $(SHARED_OBJECTS)
	$(AR) rcs $(CLIENTLIB) $(SHARED_OBJECTS)

$(BUILDDIR)/%.o : %.c
	$(CC) -MD -MT $@ -MF $(patsubst %.o,%.d,$@) -c $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean install

.NOTPARALLEL:

-include $(DEPS)
