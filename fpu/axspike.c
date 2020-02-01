#include "qemu/osdep.h"

#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>

#include "fpu/flexfloat.h"
#include "fpu/axspike.h"
#include "fpu/softfloat.h"

#include <math.h> // SQRT uses the qrt function.

uint8_t exp_bits_d = 11;
uint8_t frac_bits_d = 52;

uint8_t exp_bits_f = 8;
uint8_t frac_bits_f = 23;

// uint8_t shift_bits = 0;
// uint64_t input_mask  = 0xFFFFFFFFFFFFFFFF;
// uint64_t output_mask = 0xFFFFFFFFFFFFFFFF;

#define USE_CONV_COMP_CONV_METHOD 1
// #define USE_TRUNCATION_METHOD     1

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

static inline void restoreFFRoundingMode(int mode)
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

// Elementaries
static inline uint64_t lib_flexfloat_madd(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint64_t c, uint8_t e, uint8_t m, uint8_t original_length) {

#if defined( USE_CONV_COMP_CONV_METHOD )
  FF_EXEC_3_double(cpuenv, ff_fma, a, b, c, e, m, original_length)
#elif defined( USE_TRUNCATION_METHOD )
  FF_EXEC_3_shift(cpuenv, ff_fma, a, b, c, e, m, original_length)
#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}

static inline uint64_t lib_flexfloat_msub(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint64_t c, uint8_t e, uint8_t m, uint8_t original_length) {

#if defined( USE_CONV_COMP_CONV_METHOD )
  FF_INIT_3_double(a, b, c, e, m, original_length)
  ff_inverse(&ff_c, &ff_c);
  feclearexcept(FE_ALL_EXCEPT);
  ff_fma(&ff_res, &ff_a, &ff_b, &ff_c);
  update_fflags_fenv(cpuenv);
  double res_double = ff_get_double(&ff_res);
  return (*(uint64_t *)( &res_double )); 

#elif defined( USE_TRUNCATION_METHOD )
  FF_INIT_3_shift(a, b, c, e, m, original_length)
  ff_inverse(&ff_c, &ff_c);
  feclearexcept(FE_ALL_EXCEPT);
  ff_fma(&ff_res, &ff_a, &ff_b, &ff_c);
  update_fflags_fenv(cpuenv);
  return (flexfloat_get_bits(&ff_res) << shift_bits); /* [1|   e  |  m  |  ... zeroes ...] */

#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);

#endif

}

static inline uint64_t lib_flexfloat_nmsub(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint64_t c, uint8_t e, uint8_t m, uint8_t original_length) {

#if defined( USE_CONV_COMP_CONV_METHOD )
  FF_INIT_3_double(a, b, c, e, m, original_length)
  ff_inverse(&ff_a, &ff_a);
  feclearexcept(FE_ALL_EXCEPT);
  ff_fma(&ff_res, &ff_a, &ff_b, &ff_c);
  update_fflags_fenv(cpuenv);

  /* // original one using get_bits
  return flexfloat_get_bits(&ff_res); */
  double res_double = ff_get_double(&ff_res);
  return (*(uint64_t *)( &res_double )); 

#elif defined( USE_TRUNCATION_METHOD )
  FF_INIT_3_shift(a, b, c, e, m, original_length)
  ff_inverse(&ff_a, &ff_a);
  feclearexcept(FE_ALL_EXCEPT);
  ff_fma(&ff_res, &ff_a, &ff_b, &ff_c);
  update_fflags_fenv(cpuenv);
  return (flexfloat_get_bits(&ff_res) << shift_bits);

#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}

static inline uint64_t lib_flexfloat_nmadd(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint64_t c, uint8_t e, uint8_t m, uint8_t original_length) {

#if defined( USE_CONV_COMP_CONV_METHOD )
  FF_INIT_3_double(a, b, c, e, m, original_length)
  feclearexcept(FE_ALL_EXCEPT);
  ff_fma(&ff_res, &ff_a, &ff_b, &ff_c);
  update_fflags_fenv(cpuenv);
  ff_inverse(&ff_res, &ff_res);

  /* // original one using get_bits
  return flexfloat_get_bits(&ff_res); */
  double res_double = ff_get_double(&ff_res);
  return (*(uint64_t *)( &res_double ));

#elif defined( USE_TRUNCATION_METHOD )
  FF_INIT_3_shift(a, b, c, e, m, original_length)
  feclearexcept(FE_ALL_EXCEPT);
  ff_fma(&ff_res, &ff_a, &ff_b, &ff_c);
  update_fflags_fenv(cpuenv);
  ff_inverse(&ff_res, &ff_res);
  return (flexfloat_get_bits(&ff_res) << shift_bits);

#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}

static inline uint64_t lib_flexfloat_add(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint8_t e, uint8_t m, uint8_t original_length) {

#if defined( USE_CONV_COMP_CONV_METHOD )
  if( likely(original_length == 64) )
  {
    FF_EXEC_2_double(cpuenv, ff_add, a, b, e, m)
  }
  else // (original_length == 32)
  {
    FF_EXEC_2_float(cpuenv, ff_add, a, b, e, m)
  }
#elif defined( USE_TRUNCATION_METHOD )
  FF_EXEC_2_shift(cpuenv, ff_add, a, b, e, m, original_length)
#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}

static inline uint64_t lib_flexfloat_sub(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint8_t e, uint8_t m, uint8_t original_length) {

#if defined( USE_CONV_COMP_CONV_METHOD )
  if( likely(original_length == 64) )
  {
    FF_EXEC_2_double(cpuenv, ff_sub, a, b, e, m)
  }
  else // (original_length == 32)
  {
    FF_EXEC_2_float(cpuenv, ff_sub, a, b, e, m)
  }
#elif defined( USE_TRUNCATION_METHOD )
  FF_EXEC_2_shift(cpuenv, ff_sub, a, b, e, m, original_length)
#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}

static inline uint64_t lib_flexfloat_mul(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint8_t e, uint8_t m, uint8_t original_length) {

#if defined( USE_CONV_COMP_CONV_METHOD )
  if( likely(original_length == 64) )
  {
    FF_EXEC_2_double(cpuenv, ff_mul, a, b, e, m)
  }
  else // (original_length == 32)
  {
    FF_EXEC_2_float(cpuenv, ff_mul, a, b, e, m)
  }
#elif defined( USE_TRUNCATION_METHOD )
  FF_EXEC_2_shift(cpuenv, ff_mul, a, b, e, m, original_length)
#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}

static inline uint64_t lib_flexfloat_div(CPURISCVState *cpuenv, uint64_t a, uint64_t b, uint8_t e, uint8_t m, uint8_t original_length) {
#if defined( USE_CONV_COMP_CONV_METHOD )
  if( likely(original_length == 64) )
  {
    FF_EXEC_2_double(cpuenv, ff_div, a, b, e, m)
  }
  else // (original_length == 32)
  {
    FF_EXEC_2_float(cpuenv, ff_div, a, b, e, m)
  }
#elif defined( USE_TRUNCATION_METHOD )
  FF_EXEC_2_shift(cpuenv, ff_div, a, b, e, m, original_length)
#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif
}

// Wrapers for fpu-helper.c

uint64_t QEMU_FLATTEN 
lib_flexfloat_madd_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_madd(cpuenv, a, b, c, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_msub_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_msub(cpuenv, a, b, c, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_nmsub_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_nmsub(cpuenv, a, b, c, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_nmadd_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_nmadd(cpuenv, a, b, c, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_add_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_add(cpuenv, a, b, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_sub_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_sub(cpuenv, a, b, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_mul_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_mul(cpuenv, a, b, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_div_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  uint64_t result = lib_flexfloat_div(cpuenv, a, b, e, m, original_length);
  restoreFFRoundingMode(old);
  return result;
}

uint64_t QEMU_FLATTEN 
lib_flexfloat_sqrt_round(uint64_t a, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  /* // Original one, using get bits to get the results in LSB 1+e+m bits
  FF_INIT_1(a, e, m, original_length) */

#if defined( USE_CONV_COMP_CONV_METHOD )
  FF_INIT_1_double(a, e, m)

  feclearexcept(FE_ALL_EXCEPT);
  ff_init_double(&ff_res, sqrt(ff_get_double(&ff_a)), env);
  update_fflags_fenv(cpuenv);
  restoreFFRoundingMode(old);

  /* // original one using get_bits
  return flexfloat_get_bits(&ff_res); */
  double res_double = ff_get_double(&ff_res);
  return (*(uint64_t *)( &res_double ));

#elif defined( USE_TRUNCATION_METHOD )

  FF_INIT_1_shift(a, e, m, original_length)

  feclearexcept(FE_ALL_EXCEPT);
  ff_init_double(&ff_res, sqrt(ff_get_double(&ff_a)), env);
  update_fflags_fenv(cpuenv);
  restoreFFRoundingMode(old);

  return (flexfloat_get_bits(&ff_res) << shift_bits);

#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}

// Same as lib_flexfloat_sqrt_round, but the precise value is computed using SQRTF(float) instead of SQRT(double)
uint64_t QEMU_FLATTEN 
lib_flexfloat_sqrtf_round(uint64_t a, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
  int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
  /* // Original one, using get bits to get the results in LSB 1+e+m bits
  FF_INIT_1(a, e, m, original_length) */

#if defined( USE_CONV_COMP_CONV_METHOD )

    FF_INIT_1_float(a, e, m)

  feclearexcept(FE_ALL_EXCEPT);
  ff_init_float(&ff_res, sqrtf((float)ff_get_float(&ff_a)), env);
  update_fflags_fenv(cpuenv);
  restoreFFRoundingMode(old);

  /* // original one using get_bits
  return flexfloat_get_bits(&ff_res); */
  float res_float = ff_get_float(&ff_res);
  return (*(uint32_t *)( &res_float ));

#elif defined( USE_TRUNCATION_METHOD )

  FF_INIT_1_shift(a, e, m, original_length)

  feclearexcept(FE_ALL_EXCEPT);
  ff_init_double(&ff_res, sqrtf((float)ff_get_double(&ff_a)), env);
  update_fflags_fenv(cpuenv);
  restoreFFRoundingMode(old);

  return (flexfloat_get_bits(&ff_res) << shift_bits);

#else
  fprintf(stderr, "Not implemented #else directive %d", __LINE__);
  exit(-1);
#endif

}









// uint64_t QEMU_FLATTEN 
// lib_flexfloat_add_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
//   int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);

//   uint64_t aa =a, bb=b;
//   printf("[ qemu:  a  ]   = 0x%lX\n", *(uint64_t *)( &aa ));
//   printf("[ qemu:  b  ]   = 0x%lX\n", *(uint64_t *)( &bb ));

//   // uint64_t result = lib_flexfloat_add(cpuenv, a, b, e, m, original_length);
//   // FF_INIT_2(a, b, e, m, original_length) 

//   flexfloat_t ff_a, ff_b, ff_res; 
//   flexfloat_desc_t env = (flexfloat_desc_t) {11,51}; 

//   ff_init_double(&ff_a, *(double *)(&a), env); 
//   ff_init_double(&ff_b, *(double *)(&b), env); 
//   ff_init_double(&ff_res, 0.0, env); 

//   // ff_init(&ff_a, env); 
//   // ff_init(&ff_b, env); 
//   // ff_init(&ff_res, env); 
//   // flexfloat_set_bits(&ff_a, a); 
//   // flexfloat_set_bits(&ff_b, b);

//     uint64_t reduced_a = flexfloat_get_bits(&ff_a);
//     uint64_t reduced_b = flexfloat_get_bits(&ff_b);

//   printf("[ qemu: reduced a  ]   = 0x%lX\n", *(uint64_t *)( &reduced_a ));
//   printf("[ qemu: reduced b  ]   = 0x%lX\n", *(uint64_t *)( &reduced_b ));

//   feclearexcept(FE_ALL_EXCEPT); 
//   ff_add(&ff_res, &ff_a, &ff_b); 
//   update_fflags_fenv(cpuenv); 
//   // return flexfloat_get_bits(&ff_res);

//   uint64_t result = flexfloat_get_bits(&ff_res);
//   double res_d = ff_get_double(&ff_res);

//   printf("[ qemu:  r  ]   = 0x%lX\n", *(uint64_t *)( &result ));
//   printf("[ qemu:  r  ]   = 0x%lX\n", *(uint64_t *)( &res_d ));

//   restoreFFRoundingMode(old);
//   return result;
// }

// uint64_t QEMU_FLATTEN 
// lib_flexfloat_add_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m, uint8_t original_length) {
//   int old = setFFRoundingMode(cpuenv, cpuenv->fp_status.float_rounding_mode);
//   double result; // = lib_flexfloat_add(cpuenv, a, b, e, m, original_length);

//   // Init
//   uint64_t aa = a; uint64_t bb = b; 
//   flexfloat_t ff_a, ff_b, ff_res; 
//   flexfloat_desc_t env = (flexfloat_desc_t) {11,52}; 
//   ff_init_double(&ff_a, *(double *)( &aa ), env); 
//   ff_init_double(&ff_b, *(double *)( &bb ), env); 
//   ff_init_double(&ff_res, 0.0, env);

//   // Compute
//   feclearexcept(FE_ALL_EXCEPT); 

//   printf("a->exp = %d \n", ff_a.desc.exp_bits);
//   printf("a->frac = %d \n", ff_a.desc.frac_bits);
//   printf("b->exp = %d \n", ff_b.desc.exp_bits);
//   printf("b->frac = %d \n", ff_b.desc.frac_bits);



//   ff_add(&ff_res, &ff_a, &ff_b); 
//   update_fflags_fenv(cpuenv); 
//   result = ff_get_double(&ff_res);

//   restoreFFRoundingMode(old);
//   return (*(uint64_t *)( &result ));
// }