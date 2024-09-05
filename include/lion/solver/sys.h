#pragma once

#include <lion/params.h>

#define LION_SLV_DIMENSION 2

int lion_slv_system(double t, const double state[], double out[], void *inputs);
int lion_slv_jac(double t, const double state[], double *dfdy, double dfdt[],
                 void *inputs);