#include "open_circuit.h"

#include <lion/lion.h>
#include <lion_utils/vendor/log.h>
#include <math.h>

double lion_voc(double soc, lion_params_t *params) {
  double term0 = params->ocv.vl;

  double term1_ct  = params->ocv.v0 - params->ocv.vl;
  double term1_exp = params->ocv.gamma * (soc - 1.0);
  double term1     = term1_ct * exp(term1_exp);

  double term2 = params->ocv.alpha * params->ocv.vl * (soc - 1.0);

  double term3_ct          = (1.0 - params->ocv.alpha) * params->ocv.vl;
  double term3_inner_left  = exp(-params->ocv.beta);
  double term3_inner_right = exp(-params->ocv.beta * sqrt(soc));
  double term3_inner       = term3_inner_left - term3_inner_right;
  double term3             = term3_ct * term3_inner;

  double ocv = term0 + term1 + term2 + term3;
  return ocv;
}

double lion_voc_grad(double soc, lion_params_t *params) {
  // This function corresponds to dVoc/dSoC
  double term1_coeff = params->ocv.gamma * (params->ocv.v0 - params->ocv.vl);
  double term1_exp   = exp(params->ocv.gamma * (soc - 1.0));
  double term1       = term1_coeff * term1_exp;

  double term2 = params->ocv.alpha * params->ocv.vl;

  double term3_num_coeff = (1.0 - params->ocv.alpha) * params->ocv.vl * params->ocv.beta;
  double term3_num_exp   = exp(-params->ocv.beta * sqrt(soc));
  double term3_num       = term3_num_coeff * term3_num_exp;
  double term3_den       = 2 * sqrt(soc);
  double term3           = term3_num / term3_den;

  return term1 + term2 + term3;
}
