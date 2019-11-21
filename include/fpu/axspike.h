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


#define FF_INIT_1(a, e, m) \
  flexfloat_t ff_a, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init(&ff_a, env); \
  ff_init(&ff_res, env); \
  flexfloat_set_bits(&ff_a, a);

#define FF_INIT_1_double(a, e, m) \
  uint64_t aa = a; \
  flexfloat_t ff_a, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init_double(&ff_a, *(double *)( &aa ), env); \
  ff_init_double(&ff_res, 0.0, env);

#define FF_INIT_2(a, b, e, m) \
  flexfloat_t ff_a, ff_b, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init(&ff_a, env); \
  ff_init(&ff_b, env); \
  ff_init(&ff_res, env); \
  flexfloat_set_bits(&ff_a, a); \
  flexfloat_set_bits(&ff_b, b);

#define FF_INIT_2_double(a, b, e, m) \
  uint64_t aa = a; uint64_t bb = b; \
  flexfloat_t ff_a, ff_b, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init_double(&ff_a, *(double *)( &aa ), env); \
  ff_init_double(&ff_b, *(double *)( &bb ), env); \
  ff_init_double(&ff_res, 0.0, env);

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

#define FF_INIT_3_double(a, b, c, e, m) \
  uint64_t aa =a, bb = b, cc = c; \
  flexfloat_t ff_a, ff_b, ff_c, ff_res; \
  flexfloat_desc_t env = (flexfloat_desc_t) {e,m}; \
  ff_init_double(&ff_a, *(double *)( &aa ), env); \
  ff_init_double(&ff_b, *(double *)( &bb ), env); \
  ff_init_double(&ff_c, *(double *)( &cc ), env); \
  ff_init_double(&ff_res, 0.0, env);

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

#define FF_EXEC_2_double(s, name, a, b, e, m) \
  FF_INIT_2_double(a, b, e, m) \
  feclearexcept(FE_ALL_EXCEPT); \
  name(&ff_res, &ff_a, &ff_b); \
  update_fflags_fenv(s); \
  double res_double = ff_get_double(&ff_res); \
  return (*(uint64_t *)( &res_double ));

#define FF_EXEC_3(s, name, a, b, c, e, m) \
  FF_INIT_3(a, b, c, e, m) \
  feclearexcept(FE_ALL_EXCEPT); \
  name(&ff_res, &ff_a, &ff_b, &ff_c); \
  update_fflags_fenv(s); \
  return flexfloat_get_bits(&ff_res);

#define FF_EXEC_3_double(s, name, a, b, c, e, m) \
  FF_INIT_3_double(a, b, c, e, m) \
  feclearexcept(FE_ALL_EXCEPT); \
  name(&ff_res, &ff_a, &ff_b, &ff_c); \
  update_fflags_fenv(s); \
  double res_double = ff_get_double(&ff_res); \
  return (*(uint64_t *)( &res_double ));  
// FlexFloat decs

uint64_t QEMU_FLATTEN 
lib_flexfloat_madd_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m);
uint64_t QEMU_FLATTEN 
lib_flexfloat_msub_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m);
uint64_t QEMU_FLATTEN 
lib_flexfloat_nmsub_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m);
uint64_t QEMU_FLATTEN 
lib_flexfloat_nmadd_round(uint64_t a, uint64_t b, uint64_t c, CPURISCVState *cpuenv, uint8_t e, uint8_t m);

uint64_t QEMU_FLATTEN 
lib_flexfloat_add_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m);
uint64_t QEMU_FLATTEN 
lib_flexfloat_sub_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m);

uint64_t QEMU_FLATTEN 
lib_flexfloat_mul_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m);
uint64_t QEMU_FLATTEN 
lib_flexfloat_div_round(uint64_t a, uint64_t b, CPURISCVState *cpuenv, uint8_t e, uint8_t m);


uint64_t QEMU_FLATTEN 
lib_flexfloat_sqrt_round(uint64_t a, CPURISCVState *cpuenv, uint8_t e, uint8_t m);


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