#!/usr/bin/env python

# MAX6675.py
# 2016-05-02
# Public Domain

import time
import sys
import pigpio  # http://abyz.co.uk/rpi/pigpio/python.html

"""
This script reads the temperature of a type K thermocouple
connected to a MAX6675 SPI chip.

Type K thermocouples are made of chromel (+ve) and alumel (-ve)
and are the commonest general purpose thermocouple with a
sensitivity of approximately 41 uV/C.

The MAX6675 returns a 12-bit reading in the range 0 - 4095 with
the units as 0.25 degrees centigrade.  So the reported
temperature range is 0 - 1023.75 C.

Accuracy is about +/- 2 C between 0 - 700 C and +/- 5 C
between 700 - 1000 C.

The MAX6675 returns 16 bits as follows

F   E   D   C   B   A   9   8   7   6   5   4   3   2   1   0
0  B11 B10  B9  B8  B7  B6  B5  B4  B3  B2  B1  B0  0   0   X

The reading is in B11 (most significant bit) to B0.

The conversion time is 0.22 seconds.  If you try to read more
often the sensor will always return the last read value.

PI SPI pins (HW)
main
CE0  -> 24
CE1  -> 26
SCLK -> 23
MOSI -> 19
MISO -> 21

aux
CE0  -> 12
CE1  -> 11
CE2  -> 36
SCLK -> 40
MOSI -> 38
MISO -> 35
"""

deg = u'\xb0'


def CalcAverage(array):
    avg = float(sum(array)) / len(array)
    return int(round(avg))


def PrintCelsius(value):
    return "{}{}C".format(value, deg.encode('utf8'))


def PrintAverage(resultsArray):
    return PrintCelsius(CalcAverage(resultsArray))


pi = pigpio.pi()

if not pi.connected:
    exit(0)


# 0. pi.spi_open(0, 1000000, 0)   # CE0, 1Mbps, main SPI
# 1. pi.spi_open(1, 1000000, 0)   # CE1, 1Mbps, main SPI
# 2. pi.spi_open(0, 1000000, 256) # CE0, 1Mbps, auxiliary SPI
# 3. pi.spi_open(1, 1000000, 256) # CE1, 1Mbps, auxiliary SPI
# 4. pi.spi_open(2, 1000000, 256) # CE2, 1Mbps, auxiliary SPI

options = [[0, 0],
           [1, 0],
           [0, 256],
           [1, 256],
           [2, 256],
           ]

if len(sys.argv) > 1:
   spi = options[int(sys.argv[1])]
   sensor = pi.spi_open(spi[0], 1000000, spi[1])
else:
    #fall back to main CE0
    sensor = pi.spi_open(0, 1000000, 0)  # CE0 on main SPI (CS=24, SCLK=23,MISO(DO)=21)


led_GPIO = 22
pi.set_mode(led_GPIO, pigpio.OUTPUT)


pi.write(led_GPIO, 1)
stop = time.time() + 600
results = []
samples = 4

while time.time() < stop and samples >= 0:
    c, d = pi.spi_read(sensor, 2)
    if c == 2:
        word = (d[0] << 8) | d[1]
        if (word & 0x8006) == 0:  # Bits 15, 2, and 1 should be zero.
            t = (word >> 3) / 4.0
            #print("{:.2f}".format(t))
            results.append(t)
            samples -= 1
        else:
            print("bad reading {:b}".format(word))

    time.sleep(0.25)  # Don't try to read more often than 4 times a second.

pi.spi_close(sensor)
pi.write(led_GPIO, 0)
pi.stop()
print(CalcAverage(results))
