import subprocess as sp
import sys

# pylint: disable=import-error
from thermal_model.logger import LOGGER

# pylint: enable=import-error

from .estimate_parameters import main as _main
from .plot_results import plot_results


_MLCALL = "matlab -nodisplay -batch \"matlab.project.loadProject('battery_temperature_modelling.prj'); {}\""


def test(a):
    LOGGER.debug(a)


def _call_matlab_script(cmd):
    with sp.Popen(
        cmd,
        stdout=sys.stdout,
        stderr=sp.STDOUT,
        universal_newlines=True,
        shell=True,
        bufsize=1,
    ) as p:
        pass

    if p.returncode != 0:
        raise sp.CalledProcessError(p.returncode, p.args)


def main(gen=True, est=True, air=True, noair=True, plots=False):
    if gen:
        LOGGER.info("Generating simulated data")
        _call_matlab_script([_MLCALL.format("gen_data")])
    else:
        LOGGER.warning("Skipping generation of simulated data")

    if est:
        LOGGER.info("Running parameter estimation on simulated data")
        _main()
    else:
        LOGGER.warning("Skipping parameter estimation")

    if air:
        LOGGER.info("Evaluating air estimation")
        _call_matlab_script([_MLCALL.format("eval_air_estimation")])
    else:
        LOGGER.warning("Skipping air estimation")

    if noair:
        LOGGER.info("Evaluating no-air estimation")
        _call_matlab_script([_MLCALL.format("eval_noair_estimation")])
    else:
        LOGGER.warning("Skipping noair estimation")

    if plots:
        LOGGER.info("Generating plots")
        plot_results()