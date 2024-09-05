#pragma once

#include <gsl/gsl_min.h>

#include <lion/params.h>

#define LION_CURRENT_OPTMIN -1e3
#define LION_CURRENT_OPTMAX 1e3

double lion_current(double power, double open_circuit_voltage,
                    double internal_resistance, lion_params_t *params);
double lion_current_grad_voc(double power, double open_circuit_voltage,
                             double internal_resistance, lion_params_t *params);
double lion_current_optimize_targetfn(double current, void *params);
double lion_current_optimize(gsl_min_fminimizer *s, double power, double soc,
                             double open_circuit_voltage, double initial_guess,
                             double epsabs, double epsrel, int max_iter,
                             lion_params_t *params);
