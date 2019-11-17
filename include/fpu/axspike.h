#ifndef AXSPIKE_H
#define AXSPIKE_H

#include <stdint.h>

#include "fpu/flexfloat.h"
#include "fpu/softfloat.h"


#define DEBUG_OUT_CHANNEL stderr
#define AXSPIKE_INFO(str) fprintf(DEBUG_OUT_CHANNEL, "----------------------------------------------------------------------------\n" \
                                                     "AxSpike - INFO | " #str "\n"                                                    \
                                                     "----------------------------------------------------------------------------\n")

#define AXSPIKE_TODO(str) fprintf(DEBUG_OUT_CHANNEL, "----------------------------------------------------------------------------\n" \
                                                     "AxSpike - @TODO | " #str "\n"                                                   \
                                                     "----------------------------------------------------------------------------\n")


// typedef struct
// {
//   uint16_t v;
// } float16_t;
// typedef struct
// {
//   uint32_t v;
// } float32_t;
// typedef struct
// {
//   uint64_t v;
// } float64_t;
// typedef struct
// {
//   uint64_t v[2];
// } float128_t;



// float64_t f64_add_d_custom(float64_t frs1, float64_t frs2, float_status *status);
// float64_t f64_sub_d_custom(float64_t frs1, float64_t frs2, float_status *status);
// float64_t f64_mul_d_custom(float64_t frs1, float64_t frs2, float_status *status);

// float64_t f64_madd_d_custom(float64_t frs1, float64_t frs2, float64_t frs3, float_status *status);
// float64_t f64_sqrt_d_custom(float64_t frs1, float_status *status);
// float64_t f64_div_d_custom(float64_t frs1, float64_t frs2, float_status *status);


#define FF_INIT_1(a, e, m) \
  flexfloat_t ff_a, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init(&ff_a, env); \
  ff_init(&ff_res, env); \
  flexfloat_set_bits(&ff_a, a);

#define FF_INIT_2(a, b, e, m) \
  flexfloat_t ff_a, ff_b, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init(&ff_a, env); \
  ff_init(&ff_b, env); \
  ff_init(&ff_res, env); \
  flexfloat_set_bits(&ff_a, a); \
  flexfloat_set_bits(&ff_b, b);

#define FF_INIT_3(a, b, c, e, m) \
  flexfloat_t ff_a, ff_b, ff_c, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init(&ff_a, env); \
  ff_init(&ff_b, env); \
  ff_init(&ff_c, env); \
  ff_init(&ff_res, env); \
  flexfloat_set_bits(&ff_a, a); \
  flexfloat_set_bits(&ff_b, b); \
  flexfloat_set_bits(&ff_c, c);

#define FF_EXEC_1(s, name, a, e, m) \
  FF_INIT_(a, e, m) \
  feclearexcept(FE_ALL_EXCEPT); \
  name(&ff_res, &ff_a); \
  update_fflags_fenv(s); \
  return flexfloat_get_bits(&ff_res);

#define FF_EXEC_2(s, name, a, b, e, m) \
  FF_INIT_2(a, b, e, m) \
  feclearexcept(FE_ALL_EXCEPT); \
  name(&ff_res, &ff_a, &ff_b); \
  update_fflags_fenv(s); \
  return flexfloat_get_bits(&ff_res);

#define FF_EXEC_3(s, name, a, b, c, e, m) \
  FF_INIT_3(a, b, c, e, m) \
  feclearexcept(FE_ALL_EXCEPT); \
  name(&ff_res, &ff_a, &ff_b, &ff_c); \
  update_fflags_fenv(s); \
  return flexfloat_get_bits(&ff_res);

// FlexFloat decs

uint64_t QEMU_FLATTEN 
lib_flexfloat_madd_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m);


// Helper stuff

// #define LIB_CALL1(name, s0) name(&iss->cpu.state, s0)
// #define LIB_CALL2(name, s0, s1) name(&iss->cpu.state, s0, s1)
// #define LIB_CALL3(name, s0, s1, s2) name(&iss->cpu.state, s0, s1, s2)
// #define LIB_CALL4(name, s0, s1, s2, s3) name(&iss->cpu.state, s0, s1, s2, s3)
// #define LIB_CALL5(name, s0, s1, s2, s3, s4) name(&iss->cpu.state, s0, s1, s2, s3, s4)
// #define LIB_CALL6(name, s0, s1, s2, s3, s4, s5) name(&iss->cpu.state, s0, s1, s2, s3, s4, s5)

// #define LIB_FF_CALL1(name, s0, s1, s2) LIB_CALL3(name, s0, s1, s2)
// #define LIB_FF_CALL2(name, s0, s1, s2, s3) LIB_CALL4(name, s0, s1, s2, s3)
// #define LIB_FF_CALL3(name, s0, s1, s2, s3, s4) LIB_CALL5(name, s0, s1, s2, s3, s4)
// #define LIB_FF_CALL4(name, s0, s1, s2, s3, s4, s5) LIB_CALL6(name, s0, s1, s2, s3, s4, s5)


#endif // AXSPIKE_H