PREFIX ?= /usr/local
DESTDIR ?=
FLAGS0=-fPIE -Wall -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
GLIB=`pkg-config --cflags glib-2.0`
TAG_ENCRYPTION ?= G_CHECKSUM_SHA1
CCFLAGS0=-c -m64 -fPIC -D_GNU_SOURCE -DTAG_ENCRYPTION=$(TAG_ENCRYPTION) -I. $(GLIB)

CXXFLAGS0=-m64 -Wno-variadic-macros $(GLIB)
LIBS=-lglib-2.0 -lvalidator -lstdc++
TESTLIBS=-Llib/gtest -lgtest $(LIBS)

CCFLAGS1=-std=gnu89 -Wdeclaration-after-statement $(FLAGS0) $(CCFLAGS0)
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare $(CCFLAGS0)
CXXFLAGS1=-c -std=c++98 -D_GNU_SOURCE=1 -I. -Ilib $(CXXFLAGS0) $(FLAGS0)
CXXFLAGS2=-Wl,-z,noexecstack $(CXXFLAGS0) -Lobj -pie -Wl,-z,relro -Wl,-z,now

all: CCFLAGS1 += -DNDEBUG -O2 -s
all: CCFLAGS2 += -DNDEBUG -O2 -s
all: CXXFLAGS1 := -DNDEBUG -O2 -s $(CXXFLAGS1)
all: CXXFLAGS2 := -DNDEBUG -O2 -s $(CXXFLAGS2)
all: create_dirs zerovm

debug: CCFLAGS1 += -DDEBUG -g
debug: CCFLAGS2 += -DDEBUG -g
debug: CXXFLAGS1 := -DDEBUG -g $(CXXFLAGS1)
debug: CXXFLAGS2 := -DDEBUG -g $(CXXFLAGS2)
debug: create_dirs zerovm tests

OBJS=obj/manifest.o obj/setup.o obj/channel.o obj/qualify.o obj/report.o obj/zlog.o obj/signal_common.o obj/signal.o obj/to_app.o obj/switch_to_app.o obj/to_trap.o obj/syscall_hook.o obj/prefetch.o obj/preload.o obj/sel_rt.o obj/trap.o obj/etag.o obj/accounting.o obj/daemon.o obj/snapshot.o obj/ztrace.o obj/userspace.o obj/usermap.o obj/serializer.o
CC=@gcc
CXX=@g++

create_dirs:
	@mkdir obj -p

zerovm: obj/zerovm.o $(OBJS)
	$(CC) -o $@ $(CXXFLAGS2) $^ $(LIBS)

tests: test_compile
	@printf "UNIT TESTS %048o\n" 0
	@cd tests/unit;\
	./manifest_parser_test;\
	./service_runtime_tests;\
	cd ..

test_compile: tests/unit/manifest_parser_test tests/unit/service_runtime_tests

obj/manifest_parser_test.o: tests/unit/manifest_parser_test.cc
	$(CXX) $(CXXFLAGS1) -o $@ $^
tests/unit/manifest_parser_test: obj/manifest_parser_test.o $(OBJS)
	$(CXX) $(CXXFLAGS2) -o $@ $^ $(TESTLIBS)

obj/sel_ldr_test.o: tests/unit/sel_ldr_test.cc
	$(CXX) $(CXXFLAGS1) -o $@ $^
obj/unittest_main.o: tests/unit/unittest_main.cc
	$(CXX) $(CXXFLAGS1) -o $@ $^
tests/unit/service_runtime_tests: obj/sel_ldr_test.o obj/unittest_main.o $(OBJS)
	$(CXX) $(CXXFLAGS2) -o $@ $^ $(TESTLIBS)

.PHONY: clean clean_intermediate install

clean: clean_intermediate
	@rm -f zerovm
	@echo ZeroVM has been deleted

clean_intermediate:
	@rm -f tests/unit/manifest_parser_test tests/unit/service_runtime_tests obj/*
	@echo intermediate files has been deleted
	@echo unit tests has been deleted

install:
	install -D -m 0755 zerovm $(DESTDIR)$(PREFIX)/bin/zerovm
	install -D -m 0644 api/zvm.h $(DESTDIR)$(PREFIX)/x86_64-nacl/include/zvm.h

obj/channel.o: src/channels/channel.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/prefetch.o: src/channels/prefetch.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/preload.o: src/channels/preload.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/trap.o: src/syscalls/trap.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/setup.o: src/main/setup.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/manifest.o: src/main/manifest.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/to_app.o: src/syscalls/to_app.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/to_trap.o: src/syscalls/to_trap.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/zerovm.o: src/main/zerovm.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/signal_common.o: src/platform/signal_common.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/syscall_hook.o: src/syscalls/syscall_hook.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/switch_to_app.o: src/syscalls/switch_to_app.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_rt.o: src/loader/sel_rt.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/signal.o: src/platform/signal.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/qualify.o: src/platform/qualify.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/report.o: src/main/report.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/zlog.o: src/main/zlog.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/etag.o: src/main/etag.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/accounting.o: src/main/accounting.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/daemon.o: src/syscalls/daemon.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/snapshot.o: src/syscalls/snapshot.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/ztrace.o: src/syscalls/ztrace.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/userspace.o: src/loader/userspace.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/usermap.o: src/loader/usermap.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/serializer.o: src/channels/serializer.c
	$(CC) $(CCFLAGS1) -o $@ $^
