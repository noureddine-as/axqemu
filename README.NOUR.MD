# QEMU with variable-precision FPU capability

## Install 

Print available targets
```
../configure --help
```

For our case
```bash
cd build
../configure --target-list=riscv64-softmmu,riscv64-linux-user
make
```

In order to make debugging QEMU itself using GDB (on x86) possible:
```
../configure --target-list=riscv64-linux-user --enable-debug
```


## Usage

### To be sourced/defined
- First source the following
```
BUILDROOT_OUTPUT=/home/aitsaidn/PhD/playground/axspike/buildroot-2019.08/output

QEMU_SOFTMMU=/home/aitsaidn/PhD/playground/axspike/qemu/build/riscv64-softmmu/qemu-system-riscv64
QEMU_LINUX_USER=/home/aitsaidn/PhD/playground/axspike/qemu/build/riscv64-linux-user/qemu-riscv64

GDB_LINUX=/home/aitsaidn/PhD/playground/axspike/gdb-8.3.1-riscv64-linux-gnu

export RISCV="$BUILDROOT_OUTPUT/host"
export PATH=$PATH:$RISCV/bin
export PATH=$PATH:$GDB_LINUX/bin
export MAKEFLAGS="$MAKEFLAGS -j`nproc`"
```

### Running a RISC-V Linux Program
- Compiling and running
Compile
```bash
riscv64-linux-gcc -ggdb -o3 -g fpu.c -o fpu
riscv64-linux-objdump -s -S --endian=big fpu > fpu-dump.asm
```

run
```bash 
$QEMU_LINUX_USER -L $BUILDROOT_OUTPUT/images/rootfs -D logfile.log -d in_asm,cpu,fpu fpu
```

- Launch with GDB
```bash
$QEMU_LINUX_USER -g 1234 -L $BUILDROOT_OUTPUT/images/rootfs -D logfile.log -d in_asm,cpu,fpu fpu
```     

### Running a RISC-V Bare-Metal Program
- Baremetal, for example the hello-world example from ZephyrOS
```bash
./qemu-system-riscv64 -nographic -machine sifive_e -net none -pidfile qemu.pid -serial mon:stdio -kernel /home/aitsaidn/PhD/playground/axspike/axspike-next/zephyr.elf
```
with dumps:
```bash
./qemu-system-riscv64 -nographic -machine sifive_e -net none -pidfile qemu.pid -serial mon:stdio -kernel /home/aitsaidn/PhD/playground/axspike/axspike-next/zephyr.elf -D logfile.log -d in_asm,cpu,fpu
```

- More about linker, qemu, virt machine ...
https://twilco.github.io/riscv-from-scratch/2019/04/27/riscv-from-scratch-2.html

- Proxy-kernel ?


## Variable FPU QEMU development conventions

**@AXSPIKE** : means that the note is related to AXSPIKE development process
**@TODO**    : something should be revisited/corrected/enhanced.
**@AXSPIKE_TEST** : lines inserted to test AXSPIKE-related functionalities.

We defined **USE_AXSPIKE** flag in `softfloat.c` that will choose if we should use **softfloat** or **flexfloat**.

 

