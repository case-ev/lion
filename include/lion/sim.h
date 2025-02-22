/// @file
/// @brief Simulation creation, configuration and running.
#pragma once

#include "params.h"
#include "status.h"
#include "vector.h"

#include <gsl/gsl_min.h>
#include <gsl/gsl_odeiv2.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define _LION_LOGFILE_MAX 64

/// @defgroup types Types
/// @defgroup functions Functions

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations

typedef struct lion_sim lion_sim_t;

// Debug declarations

#ifndef NDEBUG
typedef struct _idebug_heap_info {
  void                     *addr;
  size_t                    size;
  char                      file[FILENAME_MAX];
  int                       line;
  struct _idebug_heap_info *next;
} _idebug_heap_info_t;

_idebug_heap_info_t *heapinfo_new(lion_sim_t *sim);
void                 heapinfo_free_node(_idebug_heap_info_t *node);
void                 heapinfo_clean(lion_sim_t *sim);
void                 heapinfo_push(lion_sim_t *sim, void *addr, size_t size, const char *file, int line);
size_t               heapinfo_popaddr(lion_sim_t *sim, void *addr);
size_t               heapinfo_count(lion_sim_t *sim);
#endif

/// @addtogroup types
/// @{

/// @brief Regime in which the simulation operates.
///
/// This enum indicates which domains the temperature model considers. Currently
/// only surface simulation is allowed, but air considerations are planned.
typedef enum lion_regime {
  LION_ONLYSF,  ///< Surface temperature.
  LION_ONLYAIR, ///< Air temperature.
  LION_BOTH,    ///< Surface and air temperature.
} lion_regime_t;

/// @brief Stepper algorithm for the ode solver.
///
/// The types of steppers allowed are those allowed by GSL, and considers
/// both explicit and implicit solvers.
typedef enum lion_stepper {
  LION_STEPPER_RK2,     ///< Explicit Runge-Kutta (2, 3).
  LION_STEPPER_RK4,     ///< Explicit Runge-Kutta 4.
  LION_STEPPER_RKF45,   ///< Explicit Runge-Kutta-Fehlberg (4, 5).
  LION_STEPPER_RKCK,    ///< Explicit Runge-Kutta Cash-Karp (4, 5).
  LION_STEPPER_RK8PD,   ///< Explicit Runge-Kutta Prince-Dormand (8, 9).
  LION_STEPPER_RK1IMP,  ///< Implicit Euler.
  LION_STEPPER_RK2IMP,  ///< Implicit Runge-Kutta 2.
  LION_STEPPER_RK4IMP,  ///< Implicit Runge-Kutta 4.
  LION_STEPPER_BSIMP,   ///< Implicit Bulirsch-Stoer.
  LION_STEPPER_MSADAMS, ///< Multistep Adams.
  LION_STEPPER_MSBDF,   ///< Multistep backwards differentiation.
} lion_stepper_t;

/// @brief Minimizer algorithm for the optimization problem.
///
/// The types of minimizers allowed are those allowed by GSL.
typedef enum lion_minimizer {
  LION_MINIMIZER_GOLDENSECTION, ///< Golden section.
  LION_MINIMIZER_BRENT,         ///< Brent.
  LION_MINIMIZER_QUADGOLDEN,    ///< Brent with safeguarded step-length.
} lion_minimizer_t;

/// @brief Jacobian calculation method.
///
/// The following methods for jacobian calculation are currently supported:
/// - LION_JACOBIAN_ANALYTICAL : uses the analytical equations to calculate the jacobian.
/// - LION_JACOBIAN_2POINT     : uses central differences to numerically calculate the jacobian.
typedef enum lion_jacobian_method {
  LION_JACOBIAN_ANALYTICAL, ///< Analytical method.
  LION_JACOBIAN_2POINT,     ///< Central differences method.
} lion_jacobian_method_t;

/// @brief Simulation metaparameters and hyperparameters.
///
/// These parameters are not associated to the runtime of the sim itself, but rather
/// with its configurations, choice of algorithms, parameters of those algorithms, etc.
typedef struct lion_sim_config {
  /* Sim metadata */

  const char *sim_name; ///< Name of the simulation.

  /* Simulation metadata */

  lion_regime_t          sim_regime;       ///< Regime to simulate.
  lion_stepper_t         sim_stepper;      ///< Stepper algorithm.
  lion_minimizer_t       sim_minimizer;    ///< Minimizer algorithm.
  lion_jacobian_method_t sim_jacobian;     ///< Jacobian method.
  double                 sim_time_seconds; ///< Total simulation time in seconds.
  double                 sim_step_seconds; ///< Time of each simulation step in seconds.
  double                 sim_epsabs;       ///< Absolute epsilon for update.
  double                 sim_epsrel;       ///< Relative epsilon for update.
  uint64_t               sim_min_maxiter;  ///< Maximum iterations of each minimization problem.

  /* Logging configuration */

  const char *log_dir;     ///< Directory for the logs.
  int         log_stdlvl;  ///< Level of the stderr logger.
  int         log_filelvl; ///< Level of the file logger.
} lion_sim_config_t;

/// @brief Simulation state variables.
///
/// This includes all relevant variables of the simulation, including electrical and thermal variables,
/// degradation variables, etc.
typedef struct lion_sim_state {
  double   time; ///< Simulation time.
  uint64_t step; ///< Simulation step index (starts at 1).

  // System inputs
  double power;               ///< Power being drawn from the cell.
  double ambient_temperature; ///< Ambient temperature around the cell.

  // Electrical state
  double voltage;                  ///< Voltage in the terminals of the cell.
  double current;                  ///< Current drawn from the cell.
  double ref_open_circuit_voltage; ///< Reference open circuit voltage of the cell.
  double open_circuit_voltage;     ///< Temperature aware open circuit voltage of the cell.
  double internal_resistance;      ///< Internal resistance of the cell.

  // Degradation state
  uint64_t cycle;          ///< Number of cycles the battery has been through
  double   soh;            ///< State of health of the cell
  uint64_t _cycle_step;    ///< Step within the cycle
  double   _soc_mean;      ///< Average state of charge of the cycle
  double   _soc_max;       ///< Maximum state of charge of the cycle
  double   _soc_min;       ///< Minimum state of charge of the cycle
  double   _acc_discharge; ///< Accumulated discharge

  // Thermal state
  double ehc;                  ///< Entropic heat coefficient according to an empirical model.
  double generated_heat;       ///< Heat generated by the cell due to ohmic and entropic heating.
  double internal_temperature; ///< Internal temperature of the cell.
  double surface_temperature;  ///< Surface temperature of the cell.

  // Charge state
  double kappa;            ///< Dimensionless variable which quantifies the changes in electrolite conductivity.
  double soc_nominal;      ///< Nominal state of charge.
  double capacity_nominal; ///< Nominal capacity.
  double soc_use;          ///< Usable state of charge considering temperature.
  double capacity_use;     ///< Usable capacity considering temperature.

  // Next state placeholders
  double _next_soc_nominal;          ///< Placeholder for the next nominal state of charge.
  double _next_internal_temperature; ///< Placeholder for the next internal temperature.
} lion_sim_state_t;

/// @brief Inputs for the solver.
///
/// Both the current state and the parameters of the system are passed at each iteration of the solver,
/// to be used for the update function as well as the Jacobian calculation.
typedef struct lion_slv_inputs {
  lion_sim_state_t *sys_inputs; ///< System state.
  lion_params_t    *sys_params; ///< System parameters.
} lion_slv_inputs_t;

/// @brief Simulation runtime, used for setup and simulation.
///
/// This contains all the variables which will be used by the simulation, both during the setup
/// and during the runtime on a step-by-step basis.
typedef struct lion_sim {
  lion_sim_config_t *conf;                         ///< Hyperparameters and sim metadata.
  lion_params_t     *params;                       ///< System parameters.
  lion_sim_state_t   state;                        ///< System state.
  lion_slv_inputs_t  inputs;                       ///< Inputs to the solver.
  lion_status_t (*init_hook)(lion_sim_t *sim);     ///< Hook called upon initialization.
  lion_status_t (*update_hook)(lion_sim_t *sim);   ///< Hook called on each update of the simulation.
  lion_status_t (*finished_hook)(lion_sim_t *sim); ///< Hook called when the simulation is finished.

  /* Data handles */

  gsl_odeiv2_system              sys;                   ///< Handle to the ode system.
  gsl_odeiv2_driver             *driver;                ///< Driver for the ode system.
  gsl_min_fminimizer            *sys_min;               ///< Handle to the minimizer.
  const gsl_odeiv2_step_type    *step_type;             ///< Stepper used by the ode system.
  const gsl_min_fminimizer_type *minimizer;             ///< Minimizer used by the optimizer.

  char  log_filename[FILENAME_MAX + _LION_LOGFILE_MAX]; ///< Name of the log file.
  FILE *log_file;                                       ///< Handle to the log file.

#ifndef NDEBUG
  /* Internal debug information */

  int64_t              _idebug_malloced_total;
  size_t               _idebug_malloced_size;
  _idebug_heap_info_t *_idebug_heap_head;
#endif
} lion_sim_t;

/// Version of the simulator.
typedef struct lion_version {
  const char *major; ///< Major version.
  const char *minor; ///< Minor version.
  const char *patch; ///< Patch number.
} lion_version_t;

/// @}

/// @addtogroup functions
/// @{

/// @brief Create a new configuration.
///
/// @param[out] out Variable to store the new configuration.
lion_status_t lion_sim_config_new(lion_sim_config_t *out);

/// Create a default configuration.
lion_sim_config_t lion_sim_config_default(void);

/// @brief Create a new simulation.
///
/// Sets up the simulation with a set of configuration and parameters.
/// @param[in]  conf    Pointer to the simulation configuration.
/// @param[in]  params  Pointer to the simulation parameters.
/// @param[out] out     Pointer to where the sim will be created.
lion_status_t lion_sim_new(lion_sim_config_t *conf, lion_params_t *params, lion_sim_t *out);

/// Initialize the simulation.
lion_status_t lion_sim_init(lion_sim_t *sim);

/// Reset the simulation.
lion_status_t lion_sim_reset(lion_sim_t *sim);

/// @brief Step the simulation in time.
///
/// Steps the simulation forward considering some power and ambient temperature values.
/// @param[in]  sim                  Simulation to step forward.
/// @param[in]  power                Power extracted from the cell.
/// @param[in]  ambient_temperature  Ambient temperature around the cell.
lion_status_t lion_sim_step(lion_sim_t *sim, double power, double ambient_temperature);

/// @brief Runs the simulation.
///
/// Runs the simulation considering a vector of values.
/// @param[in]  sim                  Simulation to run.
/// @param[in]  power                Power extracted from the cell at each time step.
/// @param[in]  ambient_temperature  Ambient temperature around the cell at each time step.
lion_status_t lion_sim_run(lion_sim_t *sim, lion_vector_t *power, lion_vector_t *ambient_temperature);

/// Get the version of the simulator.
lion_version_t lion_sim_get_version(lion_sim_t *sim);

/// Check whether the simulation should close.
int lion_sim_should_close(lion_sim_t *sim);

/// Get the max number of iterations.
uint64_t lion_sim_max_iters(lion_sim_t *sim);

/// Clean up the simulation.
lion_status_t lion_sim_cleanup(lion_sim_t *sim);

/// @}

#ifdef __cplusplus
}
#endif
