# Install python3 HID package https://pypi.org/project/hid/
import hid
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

def get_pixel(i):
    return colorsys.hsv_to_rgb((cos(now() / 10) + 1) / 2, 1, 1)

def get_pixels():
    return [get_pixel(i) for i in range(NUM_PIXELS)]

def to_bytes(pixels):
    flat = [round(color * 255) for colors in pixels for color in colors]
    print(flat)
    assert(len(flat) <= DMX_CHANNELS_NUM)
    padding = [0 for _ in range(DMX_CHANNELS_NUM - len(flat))]
    return (b"" + bytes(flat) + bytes(padding))


print("trying to open HID device with VID = 0x%X" % USB_VID)
for d in hid.enumerate(USB_VID):
    print(d)
    dev = hid.Device(d['vendor_id'], d['product_id'])
    if not dev:
        print("device not found")
        exit(1)
    dev.write(b"\x01" + bytes([0 for _ in range(32)])) # reset the slice counter
    str_in = dev.read(DMX_CHANNELS_NUM + 1)
    print("Received from HID Device: ", str_in, '\n')
    while True:
        data = to_bytes(get_pixels())
        for report in range(REPORTS_PER_UNIVERSE):
            slc = data[report * CHANNELS_PER_REPORT:(report + 1) * CHANNELS_PER_REPORT]
            # Encode to UTF8 for array of chars.
            # hid generic inout is single report therefore by HIDAPI requirement
            # it must be preceeded with 0x00 as dummy reportID
            dev.write(b"\x00" + slc)
            str_in = dev.read(DMX_CHANNELS_NUM + 1)
            print("Received from HID Device: ", str_in, '\n')
        time.sleep(0.1)
print("could not find such device")
exit(1)
