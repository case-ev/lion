import csv
import datetime

import serial

from monitor.logger import LOGGER


DEFAULT_MEASUREMENTS = [
    "sf1",
    "sf2",
    "air1",
    "air2",
]


def parse_line(line):
    decoded = line.decode()
    if decoded.endswith("\n"):
        decoded = decoded[:-1]
    if decoded.endswith("\r"):
        decoded = decoded[:-1]
    return decoded.split(",")


def start(out_dir, port, baud_rate=9600, timeout=5, measurement_names=None, *, skip_header=False):
    LOGGER.info("Starting communication to port '%s', br%i", port, baud_rate)
    ser = serial.Serial(port, baud_rate, timeout=timeout)

    names = []
    if measurement_names is None:
        measurements = DEFAULT_MEASUREMENTS
    else:
        measurements = measurement_names
    for name in measurements:
        names.extend([f"res_{name}", f"temp_{name}"])

    LOGGER.info("Opening csv file '%s'", out_dir)
    with open(out_dir, "w", newline="") as file:
        writer = csv.writer(file)
        if not skip_header:
            writer.writerow([
                "timestamp_utc",
                "unix_time_utc",
                "cell",
                *names,
            ])

        LOGGER.info("Entering communication loop")
        while ser.is_open:
            try:
                line = ser.readline()

                if not line:
                    LOGGER.info("Ending communication to port '%s'", port)
                    break
                else:
                    LOGGER.debug(line)
                    timestamp = datetime.datetime.now(datetime.UTC)
                    unix = int((timestamp - datetime.datetime(1970,
                               1, 1, tzinfo=datetime.UTC)).total_seconds())

                    try:
                        parsed = parse_line(line)
                    except ValueError:
                        LOGGER.warning("Found invalid line: '%s'", line)
                        continue
                    writer.writerow([timestamp, unix, *parsed])
            except Exception as e:
                LOGGER.warning("Found exception '%s'", e)
                ser.close()