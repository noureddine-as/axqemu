#include <stdio.h>
#include <stdlib.h>

#include "fpu/flexfloat.h"
#include "fpu/axspike.h"
#include "fpu/softfloat.h"

// extern uint8_t exp_bits;
// extern uint8_t frac_bits;

static uint8_t exp_bits = 11;
static uint8_t frac_bits = 52;


float64_t f64_add_d_custom(float64_t frs1, float64_t frs2, float_status *status)
{
  float64_t f64_a, f64_b, f64_out;
  f64_a = frs1;
  f64_b = frs2;

  // translate f64 to double then to flexfloat < Exp , Frac >
  double dRS1, dRS2;
  dRS1 = *(double *)&f64_a;
  dRS2 = *(double *)&f64_b;

  // do addition in flexfloat < Exp , Frac >
  flexfloat_t reduced_a, reduced_b, reduced_out;
  ff_init_double(&reduced_a, dRS1, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_b, dRS2, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_out, 0.0, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_add(&reduced_out, &reduced_a, &reduced_b);

  ///////////////////////////////////
  // @TODO test vectors collection.
  ///////////////////////////////////
  // if (this->fpu_dump_enabled)
  //   fprintf(this->fpu_dump_files["FADD_D"], "%016" PRIX64 " %016" PRIX64 " %016" PRIX64 "\r\n", flexfloat_get_bits(&reduced_a), flexfloat_get_bits(&reduced_b), flexfloat_get_bits(&reduced_out));

  // translate back to double (get double)
  double dRD = ff_get_double(&reduced_out);

  // then to f64()
  f64_out = *(float64_t *)&dRD;

  return f64_out;
}

float64_t f64_sub_d_custom(float64_t frs1, float64_t frs2, float_status *status)
{
  float64_t f64_a, f64_b, f64_out;
  f64_a = frs1;
  f64_b = frs2;

  // translate f64 to double then to flexfloat < Exp , Frac >
  double dRS1, dRS2;
  dRS1 = *(double *)&f64_a;
  dRS2 = *(double *)&f64_b;

  // do addition in flexfloat < Exp , Frac >
  flexfloat_t reduced_a, reduced_b, reduced_out;
  ff_init_double(&reduced_a, dRS1, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_b, dRS2, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_out, 0.0, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_sub(&reduced_out, &reduced_a, &reduced_b);

  // Export test vectors
  //   printf("Done FSUB_D");
  // if (this->fpu_dump_enabled)
  //   fprintf(this->fpu_dump_files["FSUB_D"], "%016" PRIX64 " %016" PRIX64 " %016" PRIX64 "\r\n", flexfloat_get_bits(&reduced_a), flexfloat_get_bits(&reduced_b), flexfloat_get_bits(&reduced_out));

  // translate back to double (get double)
  double dRD = ff_get_double(&reduced_out);

  // then to f64()
  f64_out = *(float64_t *)&dRD;

  return f64_out;
}

float64_t f64_mul_d_custom(float64_t frs1, float64_t frs2, float_status *status)
{
  float64_t f64_a, f64_b, f64_out;
  f64_a = frs1;
  f64_b = frs2;

  // translate f64 to double then to flexfloat < Exp , Frac >
  double dRS1, dRS2;
  dRS1 = *(double *)&f64_a;
  dRS2 = *(double *)&f64_b;

  // do addition in flexfloat < Exp , Frac >
  flexfloat_t reduced_a, reduced_b, reduced_out;
  ff_init_double(&reduced_a, dRS1, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_b, dRS2, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_out, 0.0, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_mul(&reduced_out, &reduced_a, &reduced_b);

  // Export test vectors
  // if (this->fpu_dump_enabled)
  //   fprintf(this->fpu_dump_files["FMUL_D"], "%016" PRIX64 " %016" PRIX64 " %016" PRIX64 "\r\n", flexfloat_get_bits(&reduced_a), flexfloat_get_bits(&reduced_b), flexfloat_get_bits(&reduced_out));

  // translate back to double (get double)
  double dRD = ff_get_double(&reduced_out);

  // then to f64()
  f64_out = *(float64_t *)&dRD;

  return f64_out;
}

float64_t f64_div_d_custom(float64_t frs1, float64_t frs2, float_status *status)
{
  float64_t f64_a, f64_b, f64_out;
  f64_a = frs1;
  f64_b = frs2;

  // translate f64 to double then to flexfloat < Exp , Frac >
  double dRS1, dRS2;
  dRS1 = *(double *)&f64_a;
  dRS2 = *(double *)&f64_b;

  // do addition in flexfloat < Exp , Frac >
  flexfloat_t reduced_a, reduced_b, reduced_out;
  ff_init_double(&reduced_a, dRS1, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_b, dRS2, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_out, 0.0, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_div(&reduced_out, &reduced_a, &reduced_b);

  // Export test vectors
  // if (this->fpu_dump_enabled)
  //   fprintf(this->fpu_dump_files["FDIV_D"], "%016" PRIX64 " %016" PRIX64 " %016" PRIX64 "\r\n", flexfloat_get_bits(&reduced_a), flexfloat_get_bits(&reduced_b), flexfloat_get_bits(&reduced_out));

  // translate back to double (get double)
  double dRD = ff_get_double(&reduced_out);

  // then to f64()
  f64_out = *(float64_t *)&dRD;

  return f64_out;
}

/* @TODO: DIVision is still being done using float64_t
  */
float64_t f64_sqrt_d_custom(float64_t frs1, float_status *status)
{
  // Copy in the input
  float64_t f64_a;  f64_a = frs1;

  // Input pre-processing: reducing input precision
  flexfloat_t reduced_in;
  double Din, Din_reduced;
  Din = *(double *)&(f64_a.v);
  // Reduced Input
  ff_init_double(&reduced_in, Din, (flexfloat_desc_t){exp_bits, frac_bits});
  Din_reduced = ff_get_double(&reduced_in);

  // printf("-- Reducing the Input precision --\n");
  // printf("Din         = %.18f\n", Din);
  // printf("Din_reduced = %.18f\n", Din_reduced);
  // printf("Din         = 0x%0lX\n", *(uint64_t *)&(Din));
  // printf("Din_reduced = 0x%0lX\n",  *(uint64_t *)&(Din_reduced));
  // printf("-----------------------------------------------------------\n");

  // Do the operation with reduced precision inputs.

  // perform the sqrt on the reduced value.
  f64_a.v = float64_sqrt(*(uint64_t *)&(Din_reduced), status);
  // perform the sqrt on the original value.
  // f64_a.v = float64_sqrt(frs1.v, status);

  // // Reducing the result USING GET_BITS =====>   NOK
  flexfloat_t reduced_out;
  double Dout, Dout_reduced;
  Dout = *(double *)&(f64_a.v);

  // Reduced Output
  ff_init_double(&reduced_out, Dout, (flexfloat_desc_t){exp_bits, frac_bits});
  Dout_reduced = ff_get_double(&reduced_out);

  // printf("-- Reducing the output precision --\n");
  // printf("Dout         = %.18f\n", Dout);
  // printf("Dout_reduced = %.18f\n", Dout_reduced);
  // printf("Dout         = 0x%0lX\n", *(uint64_t *)&(Dout));
  // printf("Dout_reduced = 0x%0lX\n",  *(uint64_t *)&(Dout_reduced));

  float64_t fRd; fRd.v = *(uint64_t *)&(Dout_reduced);  
  return fRd; //f64_a;
}
// float64_t f64_sqrt_d_custom(float64_t frs1, float_status *status)
// {
//   float64_t f64_a, f64_out;
//   flexfloat_t reduced_in;
//   double Din;
//   f64_a = frs1;
//   Din = *(double *)&f64_a;
//   ff_init_double(&reduced_in, Din, (flexfloat_desc_t){exp_bits, frac_bits});

//   f64_a.v = float64_sqrt(frs1.v, status); //f64_sqrt(frs1);

//   // translate f64 to double then to flexfloat < Exp , Frac >
//   // double dRS1;
//   // dRS1 = *(double *)&f64_a;

//   // do addition in flexfloat < Exp , Frac >
//   flexfloat_t reduced_out;
//   ff_init(&reduced_out, (flexfloat_desc_t){exp_bits, frac_bits});
//   flexfloat_set_bits(&reduced_out, f64_a.v  /* @TODO : eliminate the LSBs ? */ );
//   //ff_init_double(&reduced_out, dRS1, (flexfloat_desc_t){exp_bits, frac_bits});

//   // if (this->fpu_dump_enabled)
//   //   fprintf(this->fpu_dump_files["FSQRT_D"], "%016" PRIX64 " %016" PRIX64 "\r\n", flexfloat_get_bits(&reduced_in),
//   //                                                                               flexfloat_get_bits(&reduced_out));

//   // translate back to double (get double)
//   double dRD = ff_get_double(&reduced_out);

//   // then to f64()
//   f64_out = *(float64_t *)&dRD;

//   return f64_out;
// }

// @TODO use the flags and the status
float64_t f64_madd_d_custom(float64_t frs1, float64_t frs2, float64_t frs3, float_status *s)
{
  float64_t f64_a, f64_b, f64_c, f64_out;
  f64_a = frs1;
  f64_b = frs2;
  f64_c = frs3;

  // translate f64 to double then to flexfloat < Exp , Frac >
  double dRS1, dRS2, dRS3;
  dRS1 = *(double *)&f64_a;
  dRS2 = *(double *)&f64_b;
  dRS3 = *(double *)&f64_c;

  // do addition in flexfloat < Exp , Frac >
  flexfloat_t reduced_a, reduced_b, reduced_c, reduced_out;
  ff_init_double(&reduced_a, dRS1, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_b, dRS2, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_c, dRS3, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_init_double(&reduced_out, 0.0, (flexfloat_desc_t){exp_bits, frac_bits});
  ff_fma(&reduced_out, &reduced_a, &reduced_b, &reduced_c);

  // Export test vectors
  // printf("Done FMADD_D");
  // if (this->fpu_dump_enabled)
  //   fprintf(this->fpu_dump_files["FMADD_D"], "%016" PRIX64 " %016" PRIX64 " %016" PRIX64 " %016" PRIX64 "\r\n", flexfloat_get_bits(&reduced_a),
  //                                                                                              flexfloat_get_bits(&reduced_b),
  //                                                                                              flexfloat_get_bits(&reduced_c),
  //                                                                                              flexfloat_get_bits(&reduced_out));

  // translate back to double (get double)
  double dRD = ff_get_double(&reduced_out);

  // then to f64()
  f64_out = *(float64_t *)&dRD;

  return f64_out;
}

// typedef enum ax_mode
// {
//   AX_MODE_PRECISE = 0,
//   AX_MODE_ACCEPTABLE_1 = 1,
//   AX_MODE_ACCEPTABLE_2 = 2,
//   AX_MODE_CUSTOM = 3,
//   AX_MODE_DEGRADED = 4
// } ax_mode_t;

// typedef struct FPUClass {
//     /*< private >*/

//     /*< public >*/

//     /* Keep non-pointer data at the end to minimize holes.  */
//     ax_mode_t ax_mode;
//     uint8_t exp_bits;
//     uint8_t frac_bits;

//     // Addresses
//     // uint32_t _approx_start;
//     // uint32_t _approx_end;
//     // uint32_t _eval_start;
//     // uint32_t _eval_end;

//     // Insn stats
//     uint64_t executed_insns;
//     bool insn_stats_enabled;
//     const char *insn_stats_filename;

//     // FPU in/out dump
//     bool fpu_dump_enabled;
//     const char *fpu_dump_filename_prefix;

//     // @TODO Port these to C
//     // std::map<std::string, uint64_t> axspike_histogram;
//     // std::map<std::string, FILE*> fpu_dump_files;
//     // std::vector<std::string> fpu_insns = {"FADD_D" ,
//     //                                     "FDIV_D" ,
//     //                                     "FMUL_D" ,
//     //                                     "FMADD_D" ,
//     //                                     "FSUB_D" ,
//     //                                     "FMSUB_D" ,
//     //                                     "FSQRT_D"};

// } FPUClass;
