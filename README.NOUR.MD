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

- Proxy-kernel
Programs compiled for the proxy kernel using the NewLib (libgloss) library can still work using the Linux mode. i.e. the linux mode can also play the role of the proxy kernel.

## Logging Test Vectors from AXQEMU

To activate instruction testVector dumps (to be used in HW testbenches), activate the `ENABLE_TEST_VECTOR` macro in `fpu_helper.c` then rebuild.
The dump is output at the STDERR, so you will have to reorient STDERR to an external file of course.

## Variable FPU QEMU development conventions

**@AXSPIKE** or **@AXQEMU** : means that the note is related to AXSPIKE development process
**@TODO**    : something should be revisited/corrected/enhanced.
**@AXSPIKE_TEST** : lines inserted to test AXSPIKE-related functionalities.

We defined **USE_AXSPIKE** flag in `softfloat.c` that will choose if we should use **softfloat** or **flexfloat**.

# DELETED LEGACY STUFF

## (OLD + Non functional) Logging Test Vectors from AXQEMU

Test vector logging is done through the STDERR.

### Logging the OPCODE

To do that, you should activate it at compile time by going to the `target/riscv/translate.c` file and activate the following macro or define it as being empty (a comment for example)
```C
// Activate dumping Test Vectors in the STDERR
#define LOG_TEST_VECTOR                 fprintf(stderr, "%X", (uint32_t)(ctx->opcode))
// DeActivate dumping Test Vectors in the STDERR
#define LOG_TEST_VECTOR                 /* fprintf(stderr, "%X", (uint32_t)(ctx->opcode)) */
```

Then add `LOG_TEST_VECTOR` wherever you want inside the `trans_*` functions. Example:

```C
static bool trans_fmadd_s(DisasContext *ctx, arg_fmadd_s *a)
{
    REQUIRE_FPU;
    REQUIRE_EXT(ctx, RVF);
    gen_set_rm(ctx, a->rm);
    gen_helper_fmadd_s(cpu_fpr[a->rd], cpu_env, cpu_fpr[a->rs1],
                       cpu_fpr[a->rs2], cpu_fpr[a->rs3]);
    // @AXQEMU added for test vector logging
    LOG_TEST_VECTOR;
    mark_fs_dirty(ctx);
    return true;
}
```

### Logging the inputs + expected output [+ expected flag ?]

This part is coded in the `fpu_helper.c` file

```C
#define LOG_TEST_VECTOR_3    fprintf(stderr, " %lX %lX %lX %lX %X\n", frs1, frs2, frs3, final_result, env->fp_status.float_exception_flags)
#define LOG_TEST_VECTOR_2    fprintf(stderr, " %lX %lX 0 %lX %X\n", frs1, frs2, final_result, env->fp_status.float_exception_flags)
#define LOG_TEST_VECTOR_1    fprintf(stderr, " %lX 0 0 %lX %X\n", frs1, final_result, env->fp_status.float_exception_flags)
```

### Putting it all together

```bash
aitsaidn@marseille:~/PhD/playground/ctest/test_vector_generator$ ./compile_linux_qemu.sh 1> /dev/null 
# AXQEMU[ linux user mode ]: Exp_Bits_d = 11         Frac_Bits_d = 52
#                           Exp_Bits_f = 8         Frac_Bits_f = 23
02E7F7D3 4010000000000000 405EDD3C07EE0B0B 0 405FDD3C07EE0B0B 0
0AE7F7D3 4010000000000000 405EDD3C07EE0B0B 0 C05DDD3C07EE0B0B 0
12E7F7D3 4010000000000000 405EDD3C07EE0B0B 0 407EDD3C07EE0B0B 0
72D7F7C3 4010000000000000 405EDD3C07EE0B0B 402E3EF9DB22D0E5 407FCF33D6C72192 1
58057453 42F6E9E0 0 0 4131C71C 1
1AE7F7D3 402E3EF9DB22D0E5 405EDD3C07EE0B0B 0 3FBF5BEAE72B2E49 1
72D7F7CF 4010000000000000 405EDD3C07EE0B0B 402E3EF9DB22D0E5 C07FCF33D6C72192 1
```


