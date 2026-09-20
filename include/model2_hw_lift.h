/* Lift → model2_hw host ops (board RAM, display, geo uploads). */
#ifndef MODEL2_HW_LIFT_H
#define MODEL2_HW_LIFT_H

#include "model2_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

void model2_hw_host_ops_from_lift(model2_hw_host_ops_t *out);

/* Bind host ops from lift and reset HW (call after ROM load). */
void model2_hw_init_from_lift(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEL2_HW_LIFT_H */
