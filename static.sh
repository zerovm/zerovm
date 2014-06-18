#!/bin/sh

###############################################################################
# this script will create statically linked zerovm 2/2*. to do that `make`    #
# must be issued first (it will create all needed object files and resources) #
# since validator library generates warnings this script is not a part of     #
# zerovm makefile. after reworking validator main makefile will be updated    #
# and this script will be removed                                             #
#                                                                             #
# notice: statically linked zerovm will have "zvm" name                       #
###############################################################################
diet g++ -o zvm -O3 -s -static -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now obj/zerovm.o obj/manifest.o obj/setup.o obj/channel.o obj/qualify.o obj/report.o obj/zlog.o obj/signal_common.o obj/signal.o obj/to_app.o obj/switch_to_app.o obj/to_trap.o obj/syscall_hook.o obj/prefetch.o obj/preload.o obj/userspace.o obj/usermap.o obj/context.o obj/trap.o obj/etag.o obj/accounting.o obj/daemon.o obj/snapshot.o obj/ztrace.o obj/serializer.o -Lobj -L/usr/lib -L/usr/local/lib -lglib-2.0 -pthread -luuid -lrt /home/bortoq/git/validator/out/Release/libvalidator.a -L/usr/local/lib -lzvm_zpipes -lzbroker_cli -lzmtp

