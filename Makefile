include config.inc
CC ?= gcc

BUILD ?= release
BUILD_ROOT = build

COMMON_CFLAGS = -Wall -Wextra -DSW_VERSION_LITERAL=$(VERSION)

ifeq ($(BUILD),debug)
  CFLAGS += $(COMMON_CFLAGS) -O0 -g
  BUILDDIR = $(BUILD_ROOT)/debug
else ifeq ($(BUILD),release)
  CFLAGS += $(COMMON_CFLAGS) -O2 -DNDEBUG
  BUILDDIR = $(BUILD_ROOT)/release
else ifeq ($(BUILD),test)
  CFLAGS += $(COMMON_CFLAGS) -O0 -g
  BUILDDIR = $(BUILD_ROOT)/debug
  .DEFAULT_GOAL := test
else
  $(error Unknown BUILD=$(BUILD). Use 'debug', 'release', or 'test')
endif

TEST_CFLAGS = $(COMMON_CFLAGS) -O0 -g -DUNIT_TEST
TEST_BUILDDIR = $(BUILD_ROOT)/test

LDFLAGS += -pthread

INSTALL ?= install

# Directories containing source code
SRCDIR = \
	adt/src \
	apx/common/src \
	apx/server/src \
	msocket/src \
	remotefile/src \
	util/src \
	bstr/src \
	dtl_type/src

TEST_SRCDIR = \
	apx/common/test \
	apx/server/test \
	apx/client/src \
	apx/client/test \
	remotefile/test \
	util/test \
	cutest

# Source code files
SHARED_SOURCES = \
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
	util/dtl_type/src/dtl_hv.c

SERVER_SOURCES = apx/server/src/apx_server.c \
	apx/server/src/apx_serverConnection.c \
	apx/server/src/server_main.c \

LIB_SOURCES = $(SHARED_SOURCES)

TEST_SOURCES = \
	$(filter-out msocket/src/msocket.c msocket/src/msocket_server.c msocket/src/osutil.c, $(SHARED_SOURCES)) \
	apx/server/src/apx_serverConnection.c \
	apx/server/src/apx_testServer.c \
	apx/client/src/apx_clientSession.c \
	apx/client/src/apx_sessionCmd.c \
	msocket/src/testsocket.c \
	cutest/CuTest.c \
	apx/common/test/test_main.c \
	apx/common/test/testsuite_apx_allocator.c \
	apx/common/test/testsuite_apx_attributeParser.c \
	apx/common/test/testsuite_apx_dataElement.c \
	apx/common/test/testsuite_apx_dataSignature.c \
	apx/common/test/testsuite_apx_file.c \
	apx/common/test/testsuite_apx_fileMap.c \
	apx/common/test/testsuite_apx_node.c \
	apx/common/test/testsuite_apx_nodeData.c \
	apx/common/test/testsuite_apx_nodeInfo.c \
	apx/common/test/testsuite_apx_parser.c \
	apx/common/test/testsuite_apx_port.c \
	apx/common/test/testsuite_apx_portDataMap.c \
	apx/common/test/testsuite_apx_portMapEntry.c \
	apx/common/test/testsuite_apx_router.c \
	apx/common/test/testsuite_dataTrigger.c \
	apx/server/test/testsuite_apx_testServer.c \
	apx/client/test/testsuite_apx_clientSession.c \
	apx/client/test/testsuite_apx_sessionCmd.c \
	remotefile/test/testsuite_remotefile.c \
	util/test/testsuite_soa_fsa.c

# Paths containing interface header files
INCLUDES = \
	-I util/inc \
	-I remotefile/inc \
	-I msocket/inc \
	-I adt/inc \
	-I apx/common/inc \
	-I apx/server/inc \
	-I bstr/inc \
	-I dtl_type/inc

TEST_INCLUDES = $(INCLUDES) \
	-I apx/client/inc \
	-I cutest

EXECUTABLE = $(BUILDDIR)/apx_server
CLIENTLIB = $(BUILDDIR)/libapxclient.a
TEST_EXECUTABLE = $(TEST_BUILDDIR)/apx_unit_test

SHARED_OBJECTS = \
	$(addprefix $(BUILDDIR)/, $(notdir $(SHARED_SOURCES:.c=.o)))

SERVER_OBJECTS = \
	$(addprefix $(BUILDDIR)/, $(notdir $(SERVER_SOURCES:.c=.o)))

TEST_OBJECTS = \
	$(addprefix $(TEST_BUILDDIR)/, $(notdir $(TEST_SOURCES:.c=.o)))

DEPS = $(patsubst %.o,%.d,$(SHARED_OBJECTS) $(SERVER_OBJECTS) $(TEST_OBJECTS))

vpath %.c $(SRCDIR) $(TEST_SRCDIR)

server: $(BUILDDIR) $(EXECUTABLE)

lib: $(BUILDDIR) $(CLIENTLIB)

test: $(TEST_BUILDDIR) $(TEST_EXECUTABLE)
	mkdir -p $(TEST_BUILDDIR)/run
	cd $(TEST_BUILDDIR)/run && ../apx_unit_test

all: server lib

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TEST_BUILDDIR):
	mkdir -p $(TEST_BUILDDIR)

$(EXECUTABLE): $(SHARED_OBJECTS) $(SERVER_OBJECTS)
	$(CC) $(SHARED_OBJECTS) $(SERVER_OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

$(CLIENTLIB): $(SHARED_OBJECTS)
	$(AR) rcs $(CLIENTLIB) $(SHARED_OBJECTS)

$(TEST_EXECUTABLE): $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) $(LDFLAGS) -o $(TEST_EXECUTABLE)

$(BUILDDIR)/%.o : %.c
	$(CC) -MD -MT $@ -MF $(patsubst %.o,%.d,$@) -c $(CFLAGS) $(INCLUDES) $< -o $@

$(TEST_BUILDDIR)/%.o : %.c
	$(CC) -MD -MT $@ -MF $(patsubst %.o,%.d,$@) -c $(TEST_CFLAGS) $(TEST_INCLUDES) $< -o $@

clean:
	rm -rf $(BUILD_ROOT)

.PHONY: all server lib test clean install

.NOTPARALLEL:

-include $(DEPS)
