PNACL_TOOL=/usr/lib/nacl/default-sdk/toolchain/linux_x86_glibc

CC=${PNACL_TOOL}/bin/x86_64-nacl-gcc

#DEBUG=-g
CFLAGS = -I/usr/include/zerovm -I/usr/include/zerovm/networking -I.
CFLAGS += -Wall -std=c99 -Wno-long-long -O2 -msse4.1 -m64 $(DEBUG)
LDFLAGS = -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl \
	-L/usr/lib/zerovm -lzrt -lnetworking -lfs -lstdc++

GENNODE=generator.uint32_t
MANNODE=nodeman
SRCNODE=nodesrc
DSTNODE=nodedst

NEXES = $(GENNODE).nexe $(MANNODE).nexe $(SRCNODE).nexe $(DSTNODE).nexe
GENNODE_OBJECTS = generator.uint32_t.o
MANNODE_OBJECTS = comm.o bitonic_sort.o manager/main_man.o manager/histanlz.o manager/comm_man.o dsort.o sort.o
SRCNODE_OBJECTS = comm.o source/comm_src.o source/main_src.o cpuid.o bitonic_sort.o dsort.o sort.o
DSTNODE_OBJECTS = dest/main_dst.o dest/comm_dst.o comm.o dsort.o sort.o

all: cleanlog prepare $(NEXES) 

$(GENNODE).nexe: $(GENNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(MANNODE).nexe: $(MANNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCNODE).nexe: $(SRCNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(DSTNODE).nexe: $(DSTNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

cleanlog:
	@rm -f log/*.log
	
prepare: createdirs
	@./genmanifest.sh
 	
createdirs:
	@mkdir -p log data

clean: cleanlog 
	@rm -f $(NEXES)
	@rm -f $(GENNODE_OBJECTS) $(MANNODE_OBJECTS) $(SRCNODE_OBJECTS) $(DSTNODE_OBJECTS)
	@rm -f manifest/*.manifest
	@rm -f data/*.data


