/* Lift runtime bridge for model2_geo (not part of the stand-alone geo lib). */
#ifndef MODEL2_GEO_LIFT_H
#define MODEL2_GEO_LIFT_H

#include "model2_geo.h"
#include "model2_geo_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

void model2_geo_fifo_ops_from_lift(model2_geo_fifo_ops_t *out);
void model2_geo_hw_ops_from_lift(model2_geo_hw_ops_t *out);

/* Fill fifo+hw from model2_hw / model2_rom and call model2_geo_init. */
int model2_geo_init_from_lift(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEL2_GEO_LIFT_H */
