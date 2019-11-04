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


typedef struct
{
  uint16_t v;
} float16_t;
typedef struct
{
  uint32_t v;
} float32_t;
typedef struct
{
  uint64_t v;
} float64_t;
typedef struct
{
  uint64_t v[2];
} float128_t;



float64_t f64_add_d_custom(float64_t frs1, float64_t frs2, float_status *status);
float64_t f64_sub_d_custom(float64_t frs1, float64_t frs2, float_status *status);
float64_t f64_mul_d_custom(float64_t frs1, float64_t frs2, float_status *status);

float64_t f64_madd_d_custom(float64_t frs1, float64_t frs2, float64_t frs3, float_status *status);
float64_t f64_sqrt_d_custom(float64_t frs1, float_status *status);
float64_t f64_div_d_custom(float64_t frs1, float64_t frs2, float_status *status);


#endif // AXSPIKE_H