#include "qemu/osdep.h"

#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>

#include "fpu/flexfloat.h"
#include "fpu/axspike.h"
#include "fpu/softfloat.h"


uint8_t exp_bits = 11;
uint8_t frac_bits = 52;


// updates the fflags from fenv exceptions
static inline void update_fflags_fenv(CPURISCVState *cpuenv)
{
  int ex = fetestexcept(FE_ALL_EXCEPT);
  int flags = (!!(ex & FE_INEXACT)) |
              (!!(ex & FE_UNDERFLOW) << 1) |
              (!!(ex & FE_OVERFLOW)  << 2) |
              (!!(ex & FE_DIVBYZERO) << 3) |
              (!!(ex & FE_INVALID)   << 4);

  // set_fflags(s, flags);
  set_float_exception_flags(flags, &cpuenv->fp_status);
}

static inline void restoreFFRoundingMode(unsigned int mode)
{
  fesetround(mode);
}

/* We set the hardware FPU of the host to use the appropriate rounding mode,
  depending on the RM field of the instruction */
/*
  when helper functions are called, the context FRM and fp_status were alredy set !! 
  so this function here, only needs to copy the actual FRM to the Hardware FPU,
  which will be used in the FlexFloat computation.
*/
static inline unsigned int setFFRoundingMode(CPURISCVState *cpuenv, unsigned int mode)
{
  int old = fegetround();
  switch (mode) {
    case 0: fesetround(FE_TONEAREST); break;
    case 1: fesetround(FE_TOWARDZERO); break;
    case 2: fesetround(FE_DOWNWARD); break;
    case 3: fesetround(FE_UPWARD); break;
    case 4: printf("Unimplemented roudning mode nearest ties to max magnitude"); exit(-1); break;
    case 7: // DYNamic rounding case
    {
      switch (cpuenv->frm) { //s->fcsr.frm) {
        case 0: fesetround(FE_TONEAREST); break;
        case 1: fesetround(FE_TOWARDZERO); break;
        case 2: fesetround(FE_DOWNWARD); break;
        case 3: fesetround(FE_UPWARD); break;
        case 4: printf("Unimplemented roudning mode nearest ties to max magnitude"); exit(-1); break;
      }
    }
  }
  return old; // Just in case.
}

static inline uint64_t lib_flexfloat_madd(CPURISCVState *cpuenv, unsigned int a, unsigned int b, unsigned int c, uint8_t e, uint8_t m) {
  FF_EXEC_3(cpuenv, ff_fma, a, b, c, e, m)
}


uint64_t QEMU_FLATTEN 
lib_flexfloat_madd_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_madd(cpuenv, a, b, c, e, m);
  restoreFFRoundingMode(old);
  return result;
}


