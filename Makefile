
CXX=${CHERIOT_LLVM_ROOT}/clang++
LD=${CHERIOT_LLVM_ROOT}/ld.lld
OBJCOPY=${CHERIOT_LLVM_ROOT}/llvm-objcopy

# Create
cpu0_irom.vhx: boot.elf Makefile
	${OBJCOPY} -O binary boot.elf - | hexdump -v -e '"%08X" "\n"' > cpu0_irom.vhx

boot.elf: boot.S.o boot.cc.o irom.ldscript Makefile
	$(LD) --script=irom.ldscript --relax -o boot.elf boot.cc.o boot.S.o

boot.cc.o: boot.cc Makefile
	$(CXX) -c -std=c++20 -target riscv32-unknown-unknown -mcpu=cheriot -mabi=cheriot -mxcheri-rvc -mrelax -fshort-wchar -nostdinc -Os -g -fomit-frame-pointer -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables -fno-c++-static-destructors -fno-rtti -Werror -fvisibility=hidden -fvisibility-inlines-hidden -I${CHERIOT_RTOS_SDK}/include/c++-config -I${CHERIOT_RTOS_SDK}/include/libc++ -I${CHERIOT_RTOS_SDK}/include -I${CHERIOT_RTOS_SDK}/include/platform/arty-a7 -I${CHERIOT_RTOS_SDK}/include/platform/synopsis -I${CHERIOT_RTOS_SDK}/include/platform/ibex -I${CHERIOT_RTOS_SDK}/include/platform/generic-riscv -DIBEX -DIBEX_SAFE -DCPU_TIMER_HZ=20000000 -o boot.cc.o boot.cc -O1

boot.S.o: boot.S
	$(CXX) -c -target riscv32-unknown-unknown -mcpu=cheriot -mabi=cheriot -mxcheri-rvc -mrelax -fshort-wchar -nostdinc  -I${CHERIOT_RTOS_SDK}/include/c++-config -I${CHERIOT_RTOS_SDK}/include/libc++ -I${CHERIOT_RTOS_SDK}/include -I${CHERIOT_RTOS_SDK}/include/platform/arty-a7 -I${CHERIOT_RTOS_SDK}/include/platform/synopsis -I${CHERIOT_RTOS_SDK}/include/platform/ibex -I${CHERIOT_RTOS_SDK}/include/platform/generic-riscv -DIBEX -DIBEX_SAFE -DCPU_TIMER_HZ=20000000 -o boot.S.o boot.S


clean:
	rm -f boot.elf boot.S.o boot.cc.o cpu0_irom.vhx
