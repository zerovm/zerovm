PNACL_TOOL=/usr/lib/nacl/default-sdk/toolchain/linux_x86_glibc
NAME=hello
INCLUDE_FLAGS=-I/usr/include/zerovm

all: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L/usr/lib/zerovm -lzrt -lfs -lstdc++
	sed s@{ABS_PATH}@${CURDIR}/@g hello.manifest.template > hello.manifest
	zerovm -M$(NAME).manifest -v2

$(NAME).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.data *.back
