# Install python3 HID package https://pypi.org/project/hid/
# pip3 install pyusb
import usb.core
import usb.util

import time
import os
import colorsys
from math import cos

USB_VID = 0xcafe
DMX_CHANNELS_NUM = 512
NUM_PIXELS = 50
CHANNELS_PER_REPORT = 32
REPORTS_PER_UNIVERSE = int(DMX_CHANNELS_NUM / CHANNELS_PER_REPORT)
assert(REPORTS_PER_UNIVERSE * CHANNELS_PER_REPORT == DMX_CHANNELS_NUM)

def now():
    return (time.monotonic_ns() / 1e9)

def get_pixel_kick(i):
    h = (cos(now() / 5) + 1) / 2
    kick = abs((((-now() * 2) % 1)))
    #v = (((NUM_PIXELS - i) / NUM_PIXELS) ** (1/8)) * bang
    pos = (i) / NUM_PIXELS
    v = max(0, kick - pos)
    return colorsys.hsv_to_rgb(h, 1, v)

def get_pixel_sine(i):
    pos = i / NUM_PIXELS
    c = (cos(now()) + 1) /2
    d = pos -c if pos > c else c - pos
    v = (1 - d) ** (9)

    return colorsys.hsv_to_rgb(1, 1, v)

def get_pixel(i):
    return get_pixel_kick(i)

def get_pixels():
    return [get_pixel(i) for i in range(NUM_PIXELS)]

def to_bytes(pixels):
    flat = [round(color * 255) for colors in pixels for color in colors]
    assert(len(flat) <= DMX_CHANNELS_NUM)
    padding = [0 for _ in range(DMX_CHANNELS_NUM - len(flat))]
    return (b"" + bytes(flat) + bytes(padding))

print("trying to open device with VID = 0x0000 & PID = 0x0001")
dev = usb.core.find(idVendor=0x0000, idProduct=0x0001)
if dev is None:
    raise ValueError('Device not found')

cfg = dev.get_active_configuration()
intf = cfg[(0, 0)]

outep = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match= \
        lambda e: \
            usb.util.endpoint_direction(e.bEndpointAddress) == \
            usb.util.ENDPOINT_OUT)

assert outep is not None

prev_ts = 0
while True:
    data = to_bytes(get_pixels())
    print(outep.write(data))
    print("fps: " + str(1 / (time.monotonic() - prev_ts)))
    prev_ts = time.monotonic()
