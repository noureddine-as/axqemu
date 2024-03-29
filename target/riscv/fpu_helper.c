/*
 * RISC-V FPU Emulation Helpers for QEMU.
 *
 * Copyright (c) 2016-2017 Sagar Karandikar, sagark@eecs.berkeley.edu
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "qemu/host-utils.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"
#include "fpu/softfloat.h"
#include "fpu/axspike.h"

// HAND-MADE Configurations -----------------------------------------------

// Use FlexFloat or GVSoC ------------------------------
// #define USE_FLEXFLOAT 1
#define USE_GVSOC_DEF               1
// -----------------------------------------------------

// WHAT KIND OF DEBUGGING TO ENABLE --------------------
#define ENABLE_TEXTUAL_TEST_VECTOR          0
#define ENABLE_BINARY_TEST_VECTOR           1
#define ENABLE_TEXTUAL_PERF_DEBUG           0
// -----------------------------------------------------

// HOW THE INSTRUMENTATION IS Activated/Deactivated
#define INSTRUMENTATION_APPROX_BY_DEFAULT    0
#define INSTRUMENTATION_VPT_STYLE            1
// -----------------------------------------------------

// -------------------------------------------------------------------------



/* User, Non-Standard Variable Precision in Time CSRs */
/* @TODO : These should be part of the CPU, if we want to operate with several precisions
           for each core !!!
*/

#define VPT_ENABLE_BIT_MASK             0x01
#define VPT_INSTRUMENTATION_BIT_MASK    0x01

uint8_t vpt_status = 0;    /* ENABLE_BIT = 0 --> VPT disabled
                              ENABLE_BIT = 1 --> VPT enabled
                            */
uint8_t vpt_frac_bits_f = 23;
uint8_t vpt_frac_bits_d = 52;
uint8_t vpt_exec_mode = 0;

// uint8_t exp_bits_f;
// uint8_t exp_bits_d;

// Simulation parameters
extern uint8_t exp_bits_d;
extern uint8_t frac_bits_d;
extern uint8_t exp_bits_f;
extern uint8_t frac_bits_f;
extern FILE    *binary_test_vector_file;

extern uint64_t non_approx_region_start;
extern uint64_t non_approx_region_size;


uint8_t enable_binary_test_vector        = ENABLE_BINARY_TEST_VECTOR;
uint8_t enable_instrumentation_vpt_style = INSTRUMENTATION_VPT_STYLE;


// #define MARK_APPROXIMATE   ( (((uint32_t)0x00000001) << 26) )
#define MARK_APPROXIMATE   ((uint32_t)0x00000000)

#if ( ENABLE_TEXTUAL_TEST_VECTOR ) // In this case, Textual is defined, and binary is not
#define LOG_TEXTUAL_TEST_VECTOR_3(name, nanbox_values)    fprintf(stderr, "%s %X %016lX %016lX %016lX %016lX %X\n", name , \
                                                                                 (uint8_t)env->fp_status.float_rounding_mode, \
                                                                                 (nanbox_values ? frs1 | (uint64_t)0xFFFFFFFF00000000 : frs1), \
                                                                                 (nanbox_values ? frs2 | (uint64_t)0xFFFFFFFF00000000 : frs2), \
                                                                                 (nanbox_values ? frs3 | (uint64_t)0xFFFFFFFF00000000 : frs3), \
                                                                                 (nanbox_values ? final_result | (uint64_t)0xFFFFFFFF00000000 : final_result), \
                                                                                 (uint8_t)env->fp_status.float_exception_flags)

#define LOG_TEXTUAL_TEST_VECTOR_2(name, nanbox_values)    fprintf(stderr, "%s %X %016lX %016lX %s %016lX %X\n", name , \
                                                                                 (uint8_t)env->fp_status.float_rounding_mode, \
                                                                                 (nanbox_values ? frs1 | (uint64_t)0xFFFFFFFF00000000 : frs1), \
                                                                                 (nanbox_values ? frs2 | (uint64_t)0xFFFFFFFF00000000 : frs2), \
                                                                                 (nanbox_values ? "FFFFFFFF00000000" : "0000000000000000"), \
                                                                                 (nanbox_values ? final_result | (uint64_t)0xFFFFFFFF00000000 : final_result), \
                                                                                 (uint8_t)env->fp_status.float_exception_flags)

#define LOG_TEXTUAL_TEST_VECTOR_1(name, nanbox_values)    fprintf(stderr, "%s %X %016lX %s %s %016lX %X\n", name , \
                                                                                 (uint8_t)env->fp_status.float_rounding_mode, \
                                                                                 (nanbox_values ? frs1 | (uint64_t)0xFFFFFFFF00000000 : frs1), \
                                                                                 (nanbox_values ? "FFFFFFFF00000000" : "0000000000000000"), \
                                                                                 (nanbox_values ? "FFFFFFFF00000000" : "0000000000000000"), \
                                                                                 (nanbox_values ? final_result | (uint64_t)0xFFFFFFFF00000000 : final_result), \
                                                                                 (uint8_t)env->fp_status.float_exception_flags)

#define LOG_BINARY_TEST_VECTOR_3(opcode, nanbox_values)    {} 
#define LOG_BINARY_TEST_VECTOR_2(opcode, nanbox_values)    {}
#define LOG_BINARY_TEST_VECTOR_1(opcode, nanbox_values)    {}

#elif ( ENABLE_TEXTUAL_PERF_DEBUG ) // Help debugging the executable itself

#define LOG_TEXTUAL_TEST_VECTOR_3(name, nanbox_values)    fprintf(stderr, "%016lX\n", env->pc)
#define LOG_TEXTUAL_TEST_VECTOR_2(name, nanbox_values)    fprintf(stderr, "%016lX\n", env->pc)
#define LOG_TEXTUAL_TEST_VECTOR_1(name, nanbox_values)    fprintf(stderr, "%016lX\n", env->pc)

#define LOG_BINARY_TEST_VECTOR_3(opcode, nanbox_values)    {} 
#define LOG_BINARY_TEST_VECTOR_2(opcode, nanbox_values)    {}
#define LOG_BINARY_TEST_VECTOR_1(opcode, nanbox_values)    {}

#elif ( ENABLE_BINARY_TEST_VECTOR ) // In this case, Binary is defined, and textual is not

typedef struct __attribute__((__packed__, scalar_storage_order("big-endian"))) {
    uint32_t opp;
    uint64_t rs1;
    uint64_t rs2;
    uint64_t rs3;
    uint64_t rd;
    uint8_t  status;
} binary_test_vector_t ;

#define TV_STRUCT_SIZE      (sizeof(binary_test_vector_t))

// We don't need the RND_mode, it's incoded in the opcode

#define LOG_BINARY_TEST_VECTOR_3(opcode, nanbox_values)    binary_test_vector_t tv_instance = {0}; \
                                            tv_instance.opp = opcode; \
                                            tv_instance.rs1 = (nanbox_values ? frs1 | (uint64_t)0xFFFFFFFF00000000 : frs1); \
                                            tv_instance.rs2 = (nanbox_values ? frs2 | (uint64_t)0xFFFFFFFF00000000 : frs2); \
                                            tv_instance.rs3 = (nanbox_values ? frs3 | (uint64_t)0xFFFFFFFF00000000 : frs3); \
                                            tv_instance.rd  = (nanbox_values ? final_result | (uint64_t)0xFFFFFFFF00000000 : final_result); \
                                            tv_instance.status = (uint8_t)env->fp_status.float_exception_flags; \
                                            if ( !(   ( nanbox_values   && (tv_instance.rs1 == (uint64_t)0xFFFFFFFF00000000) && (tv_instance.rs2 == (uint64_t)0xFFFFFFFF00000000) && (tv_instance.rs3 == (uint64_t)0xFFFFFFFF00000000)) \
                                                   || ((!nanbox_values) && ((tv_instance.rs1 == (uint64_t)0x0000000000000000) && (tv_instance.rs2 == (uint64_t)0x0000000000000000) && (tv_instance.rs3 == (uint64_t)0x0000000000000000))) \
                                                  ) \
                                               ){                                                                \
                                                fwrite(&tv_instance, TV_STRUCT_SIZE, 1, binary_test_vector_file); \
                                                fflush(binary_test_vector_file);                                    \
                                            }

#define LOG_BINARY_TEST_VECTOR_2(opcode, nanbox_values)    binary_test_vector_t tv_instance = {0}; \
                                            tv_instance.opp = opcode; \
                                            tv_instance.rs1 = (nanbox_values ? frs1 | (uint64_t)0xFFFFFFFF00000000 : frs1); \
                                            tv_instance.rs2 = (nanbox_values ? frs2 | (uint64_t)0xFFFFFFFF00000000 : frs2); \
                                            tv_instance.rs3 = (nanbox_values ? (uint64_t)0xFFFFFFFF00000000 : (uint64_t)0x0000000000000000); \
                                            tv_instance.rd  = (nanbox_values ? final_result | (uint64_t)0xFFFFFFFF00000000 : final_result); \
                                            tv_instance.status = (uint8_t)env->fp_status.float_exception_flags; \
                                            if ( !(   ( nanbox_values   && (tv_instance.rs1 == (uint64_t)0xFFFFFFFF00000000) && (tv_instance.rs2 == (uint64_t)0xFFFFFFFF00000000) && (tv_instance.rs3 == (uint64_t)0xFFFFFFFF00000000)) \
                                                   || ((!nanbox_values) && ((tv_instance.rs1 == (uint64_t)0x0000000000000000) && (tv_instance.rs2 == (uint64_t)0x0000000000000000) && (tv_instance.rs3 == (uint64_t)0x0000000000000000))) \
                                                  ) \
                                               ){                                                                \
                                                fwrite(&tv_instance, TV_STRUCT_SIZE, 1, binary_test_vector_file); \
                                                fflush(binary_test_vector_file);                                    \
                                            }

#define LOG_BINARY_TEST_VECTOR_1(opcode, nanbox_values)    binary_test_vector_t tv_instance = {0}; \
                                            tv_instance.opp = opcode; \
                                            tv_instance.rs1 = (nanbox_values ? frs1 | (uint64_t)0xFFFFFFFF00000000 : frs1); \
                                            tv_instance.rs2 = (nanbox_values ? (uint64_t)0xFFFFFFFF00000000 : (uint64_t)0x0000000000000000); \
                                            tv_instance.rs3 = (nanbox_values ? (uint64_t)0xFFFFFFFF00000000 : (uint64_t)0x0000000000000000); \
                                            tv_instance.rd  = (nanbox_values ? final_result | (uint64_t)0xFFFFFFFF00000000 : final_result); \
                                            tv_instance.status = (uint8_t)env->fp_status.float_exception_flags; \
                                            if ( !(   ( nanbox_values   && (tv_instance.rs1 == (uint64_t)0xFFFFFFFF00000000) && (tv_instance.rs2 == (uint64_t)0xFFFFFFFF00000000) && (tv_instance.rs3 == (uint64_t)0xFFFFFFFF00000000)) \
                                                   || ((!nanbox_values) && ((tv_instance.rs1 == (uint64_t)0x0000000000000000) && (tv_instance.rs2 == (uint64_t)0x0000000000000000) && (tv_instance.rs3 == (uint64_t)0x0000000000000000))) \
                                                  ) \
                                               ){                                                                \
                                                fwrite(&tv_instance, TV_STRUCT_SIZE, 1, binary_test_vector_file); \
                                                fflush(binary_test_vector_file);                                    \
                                            }

#define LOG_TEXTUAL_TEST_VECTOR_3(name, nanbox_values)    {} 
#define LOG_TEXTUAL_TEST_VECTOR_2(name, nanbox_values)    {}
#define LOG_TEXTUAL_TEST_VECTOR_1(name, nanbox_values)    {}

#else // Do nothing in this case
    #define LOG_BINARY_TEST_VECTOR_3(opcode, nanbox_values)    {} 
    #define LOG_BINARY_TEST_VECTOR_2(opcode, nanbox_values)    {}
    #define LOG_BINARY_TEST_VECTOR_1(opcode, nanbox_values)    {}

    #define LOG_TEXTUAL_TEST_VECTOR_3(name, nanbox_values)    {} 
    #define LOG_TEXTUAL_TEST_VECTOR_2(name, nanbox_values)    {}
    #define LOG_TEXTUAL_TEST_VECTOR_1(name, nanbox_values)    {}
#endif



target_ulong riscv_cpu_get_fflags(CPURISCVState *env)
{
    int soft = get_float_exception_flags(&env->fp_status);
    target_ulong hard = 0;

    hard |= (soft & float_flag_inexact) ? FPEXC_NX : 0;
    hard |= (soft & float_flag_underflow) ? FPEXC_UF : 0;
    hard |= (soft & float_flag_overflow) ? FPEXC_OF : 0;
    hard |= (soft & float_flag_divbyzero) ? FPEXC_DZ : 0;
    hard |= (soft & float_flag_invalid) ? FPEXC_NV : 0;

    return hard;
}

void riscv_cpu_set_fflags(CPURISCVState *env, target_ulong hard)
{
    int soft = 0;

    soft |= (hard & FPEXC_NX) ? float_flag_inexact : 0;
    soft |= (hard & FPEXC_UF) ? float_flag_underflow : 0;
    soft |= (hard & FPEXC_OF) ? float_flag_overflow : 0;
    soft |= (hard & FPEXC_DZ) ? float_flag_divbyzero : 0;
    soft |= (hard & FPEXC_NV) ? float_flag_invalid : 0;

    set_float_exception_flags(soft, &env->fp_status);
}

void helper_set_rounding_mode(CPURISCVState *env, uint32_t rm)
{
    int softrm;

    if (rm == 7) {
        rm = env->frm;
    }
    switch (rm) {
    case 0:
        softrm = float_round_nearest_even;
        break;
    case 1:
        softrm = float_round_to_zero;
        break;
    case 2:
        softrm = float_round_down;
        break;
    case 3:
        softrm = float_round_up;
        break;
    case 4:
        softrm = float_round_ties_away;
        break;
    default:
        riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
    }

    set_float_rounding_mode(softrm, &env->fp_status);
}

uint64_t helper_fmadd_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_muladd(frs1, frs2, frs3, 0, &env->fp_status);
    } else {
        final_result = lib_flexfloat_madd_round(frs1, frs2, frs3, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_3("FMADD_S", 1);
    }
#else
    final_result = float32_muladd(frs1, frs2, frs3, 0, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fmadd_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1;  
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs3_in; frs3_in.v = frs3;
    float64_t frs_out;
    frs_out = f64_madd_d_custom(frs1_in, frs2_in, frs3_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        // fprintf(stderr, "MADD_D Executed PRECISELY !!!\n");
        final_result = float64_muladd(frs1, frs2, frs3, 0, &env->fp_status);
    } else {
        // fprintf(stderr, "MADD_D Executed APPROXIMATELY  --- @ADDR = %08lX     - prec_d = %d\n", env->pc, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d);
        final_result = lib_flexfloat_madd_round(frs1, frs2, frs3, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_3("FMADD_D", 0);
    }
#else
    final_result = float64_muladd(frs1, frs2, frs3, 0, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fmsub_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_muladd(frs1, frs2, frs3, float_muladd_negate_c, &env->fp_status);
    } else {
        final_result = lib_flexfloat_msub_round(frs1, frs2, frs3, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_3("FMSUB_S", 1);
    }
#else
    final_result = float32_muladd(frs1, frs2, frs3, float_muladd_negate_c, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fmsub_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1;  
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs3_in; frs3_in.v = frs3; frs3_in.v = frs3_in.v ^ ((uint64_t)1 << 63);
    float64_t frs_out;
    frs_out = f64_madd_d_custom(frs1_in, frs2_in, frs3_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_muladd(frs1, frs2, frs3, float_muladd_negate_c, &env->fp_status);
    } else {
        final_result = lib_flexfloat_msub_round(frs1, frs2, frs3, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_3("FMSUB_D", 0);
    }
#else
    final_result = float64_muladd(frs1, frs2, frs3, float_muladd_negate_c, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fnmsub_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_muladd(frs1, frs2, frs3, float_muladd_negate_product, &env->fp_status);
    } else {
        final_result = lib_flexfloat_nmsub_round(frs1, frs2, frs3, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_3("FNMSUB_S", 1);
    }
#else
    final_result = float32_muladd(frs1, frs2, frs3, float_muladd_negate_product, &env->fp_status);
#endif
    return final_result;
}

#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)

uint64_t helper_fnmsub_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = (uint64_t)(frs1 ^ F64_SIGN);
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs3_in; frs3_in.v = frs3;
    float64_t frs_out;
    frs_out = f64_madd_d_custom(frs1_in, frs2_in, frs3_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_muladd(frs1, frs2, frs3, float_muladd_negate_product, &env->fp_status);
    } else {
        final_result = lib_flexfloat_nmsub_round(frs1, frs2, frs3, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_3("FNMSUB_D", 0);
    }
#else
    final_result = float64_muladd(frs1, frs2, frs3, float_muladd_negate_product, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fnmadd_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_muladd(frs1, frs2, frs3, float_muladd_negate_c | float_muladd_negate_product, &env->fp_status);
    } else {
        final_result = lib_flexfloat_nmadd_round(frs1, frs2, frs3, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_3("FNMADD_S", 1);
    }
#else
    final_result = float32_muladd(frs1, frs2, frs3, float_muladd_negate_c | float_muladd_negate_product, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fnmadd_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1; frs1_in.v = frs1_in.v ^ ((uint64_t)1 << 63);
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs3_in; frs3_in.v = frs3; frs3_in.v = frs3_in.v ^ ((uint64_t)1 << 63);
    float64_t frs_out;
    frs_out = f64_madd_d_custom(frs1_in, frs2_in, frs3_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_muladd(frs1, frs2, frs3, float_muladd_negate_c | float_muladd_negate_product, &env->fp_status);
    } else {
        final_result = lib_flexfloat_nmadd_round(frs1, frs2, frs3, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_3(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_3("FNMADD_D", 0);
    }
#else
    final_result = float64_muladd(frs1, frs2, frs3, float_muladd_negate_c | float_muladd_negate_product, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fadd_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_add(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_add_round(frs1, frs2, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_2("FADD_S", 1);
    }
#else
    final_result = float32_add(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fsub_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_sub(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_sub_round(frs1, frs2, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_2("FSUB_S", 1);
    }
#else
    final_result = float32_sub(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fmul_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_mul(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_mul_round(frs1, frs2, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_2("FMUL_S", 1);
    }
#else
    final_result = float32_mul(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fdiv_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_div(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_div_round(frs1, frs2, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_2("FDIV_S", 1);
    }
#else
    final_result = float32_div(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fmin_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float32_minnum(frs1, frs2, &env->fp_status);
}

uint64_t helper_fmax_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float32_maxnum(frs1, frs2, &env->fp_status);
}

uint64_t helper_fsqrt_s(CPURISCVState *env, uint64_t frs1, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float32_sqrt(frs1, &env->fp_status);
    } else {
        final_result = lib_flexfloat_sqrtf_round(frs1, env, exp_bits_f, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_f : frac_bits_f, (uint8_t)32);
        LOG_BINARY_TEST_VECTOR_1(opcode | MARK_APPROXIMATE, 1);
        LOG_TEXTUAL_TEST_VECTOR_1("FSQRT_S", 1);
    }
#else
    final_result = float32_sqrt(frs1, &env->fp_status);
#endif
    return final_result;
}

target_ulong helper_fle_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float32_le(frs1, frs2, &env->fp_status);
}

target_ulong helper_flt_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float32_lt(frs1, frs2, &env->fp_status);
}

target_ulong helper_feq_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float32_eq_quiet(frs1, frs2, &env->fp_status);
}

target_ulong helper_fcvt_w_s(CPURISCVState *env, uint64_t frs1)
{
    return float32_to_int32(frs1, &env->fp_status);
}

target_ulong helper_fcvt_wu_s(CPURISCVState *env, uint64_t frs1)
{
    return (int32_t)float32_to_uint32(frs1, &env->fp_status);
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_l_s(CPURISCVState *env, uint64_t frs1)
{
    return float32_to_int64(frs1, &env->fp_status);
}

uint64_t helper_fcvt_lu_s(CPURISCVState *env, uint64_t frs1)
{
    return float32_to_uint64(frs1, &env->fp_status);
}
#endif

uint64_t helper_fcvt_s_w(CPURISCVState *env, target_ulong rs1)
{
    return int32_to_float32((int32_t)rs1, &env->fp_status);
}

uint64_t helper_fcvt_s_wu(CPURISCVState *env, target_ulong rs1)
{
    return uint32_to_float32((uint32_t)rs1, &env->fp_status);
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_s_l(CPURISCVState *env, uint64_t rs1)
{
    return int64_to_float32(rs1, &env->fp_status);
}

uint64_t helper_fcvt_s_lu(CPURISCVState *env, uint64_t rs1)
{
    return uint64_to_float32(rs1, &env->fp_status);
}
#endif

target_ulong helper_fclass_s(uint64_t frs1)
{
    float32 f = frs1;
    bool sign = float32_is_neg(f);

    if (float32_is_infinity(f)) {
        return sign ? 1 << 0 : 1 << 7;
    } else if (float32_is_zero(f)) {
        return sign ? 1 << 3 : 1 << 4;
    } else if (float32_is_zero_or_denormal(f)) {
        return sign ? 1 << 2 : 1 << 5;
    } else if (float32_is_any_nan(f)) {
        float_status s = { }; /* for snan_bit_is_one */
        return float32_is_quiet_nan(f, &s) ? 1 << 9 : 1 << 8;
    } else {
        return sign ? 1 << 1 : 1 << 6;
    }
}

uint64_t helper_fadd_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1;  
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs_out;
    frs_out = f64_add_d_custom(frs1_in, frs2_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_add(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_add_round(frs1, frs2, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_2("FADD_D", 0);
    }
#else
    final_result = float64_add(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fsub_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1;  
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs_out;
    frs_out = f64_sub_d_custom(frs1_in, frs2_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_sub(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_sub_round(frs1, frs2, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_2("FSUB_D", 0);
    }
#else
    final_result = float64_sub(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fmul_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1;  
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs_out;
    frs_out = f64_mul_d_custom(frs1_in, frs2_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_mul(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_mul_round(frs1, frs2, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_2("FMUL_D", 0);
    }
#else
    final_result = float64_mul(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fdiv_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1;  
    float64_t frs2_in; frs2_in.v = frs2;
    float64_t frs_out;
    frs_out = f64_div_d_custom(frs1_in, frs2_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_div(frs1, frs2, &env->fp_status);
    } else {
        final_result = lib_flexfloat_div_round(frs1, frs2, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_2(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_2("FDIV_D", 0);
    }
#else
    final_result = float64_div(frs1, frs2, &env->fp_status);
#endif
    return final_result;
}

uint64_t helper_fmin_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float64_minnum(frs1, frs2, &env->fp_status);
}

uint64_t helper_fmax_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float64_maxnum(frs1, frs2, &env->fp_status);
}

uint64_t helper_fcvt_s_d(CPURISCVState *env, uint64_t rs1)
{
    return float64_to_float32(rs1, &env->fp_status);
}

uint64_t helper_fcvt_d_s(CPURISCVState *env, uint64_t rs1)
{
    return float32_to_float64(rs1, &env->fp_status);
}

uint64_t helper_fsqrt_d(CPURISCVState *env, uint64_t frs1, uint32_t opcode)
{
    uint64_t final_result;
#if defined( USE_FLEXFLOAT )
    float64_t frs1_in; frs1_in.v = frs1;  
    float64_t frs_out;
    frs_out = f64_sqrt_d_custom(frs1_in, &env->fp_status);
    final_result = frs_out.v;
#elif defined( USE_GVSOC_DEF )
  #if (INSTRUMENTATION_APPROX_BY_DEFAULT)
    if(unlikely(  (non_approx_region_start != 0) && (non_approx_region_size != 0) &&
                    ((env->pc >= non_approx_region_start) && (env->pc < non_approx_region_start + non_approx_region_size))))
    {
  #elif ( INSTRUMENTATION_VPT_STYLE )
    if( (vpt_exec_mode & VPT_INSTRUMENTATION_BIT_MASK) == 0x00 ){
  #endif
        final_result = float64_sqrt(frs1, &env->fp_status);
    } else {
        final_result = lib_flexfloat_sqrt_round(frs1, env, exp_bits_d, (vpt_status & VPT_ENABLE_BIT_MASK)? vpt_frac_bits_d : frac_bits_d, (uint8_t)64);
        LOG_BINARY_TEST_VECTOR_1(opcode | MARK_APPROXIMATE, 0);
        LOG_TEXTUAL_TEST_VECTOR_1("FSQRT_D", 0);
    }
#else
    final_result = float64_sqrt(frs1, &env->fp_status);
#endif
    return final_result;
}

target_ulong helper_fle_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float64_le(frs1, frs2, &env->fp_status);
}

target_ulong helper_flt_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float64_lt(frs1, frs2, &env->fp_status);
}

target_ulong helper_feq_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    return float64_eq_quiet(frs1, frs2, &env->fp_status);
}

target_ulong helper_fcvt_w_d(CPURISCVState *env, uint64_t frs1)
{
    return float64_to_int32(frs1, &env->fp_status);
}

target_ulong helper_fcvt_wu_d(CPURISCVState *env, uint64_t frs1)
{
    return (int32_t)float64_to_uint32(frs1, &env->fp_status);
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_l_d(CPURISCVState *env, uint64_t frs1)
{
    return float64_to_int64(frs1, &env->fp_status);
}

uint64_t helper_fcvt_lu_d(CPURISCVState *env, uint64_t frs1)
{
    return float64_to_uint64(frs1, &env->fp_status);
}
#endif

uint64_t helper_fcvt_d_w(CPURISCVState *env, target_ulong rs1)
{
    return int32_to_float64((int32_t)rs1, &env->fp_status);
}

uint64_t helper_fcvt_d_wu(CPURISCVState *env, target_ulong rs1)
{
    return uint32_to_float64((uint32_t)rs1, &env->fp_status);
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_d_l(CPURISCVState *env, uint64_t rs1)
{
    return int64_to_float64(rs1, &env->fp_status);
}

uint64_t helper_fcvt_d_lu(CPURISCVState *env, uint64_t rs1)
{
    return uint64_to_float64(rs1, &env->fp_status);
}
#endif

target_ulong helper_fclass_d(uint64_t frs1)
{
    float64 f = frs1;
    bool sign = float64_is_neg(f);

    if (float64_is_infinity(f)) {
        return sign ? 1 << 0 : 1 << 7;
    } else if (float64_is_zero(f)) {
        return sign ? 1 << 3 : 1 << 4;
    } else if (float64_is_zero_or_denormal(f)) {
        return sign ? 1 << 2 : 1 << 5;
    } else if (float64_is_any_nan(f)) {
        float_status s = { }; /* for snan_bit_is_one */
        return float64_is_quiet_nan(f, &s) ? 1 << 9 : 1 << 8;
    } else {
        return sign ? 1 << 1 : 1 << 6;
    }
}
