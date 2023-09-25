# Install python3 HID package https://pypi.org/project/hid/
import hid
import time
import os
from threading import Thread
from tkinter import *

USB_VID = 0xcafe

def send_sliders(sliders):
    try:
        print("trying to open HID device with VID = 0x%X" % USB_VID)
        for d in hid.enumerate(USB_VID):
            print(d)
            dev = hid.Device(d['vendor_id'], d['product_id'])
            if not dev:
                    return
            while True:
                # Get input from console and encode to UTF8 for array of chars.
                # hid generic inout is single report therefore by HIDAPI requirement
                # it must be preceeded with 0x00 as dummy reportID
                print(b"\x00" + bytes([s.get() for s in sliders]))
                #dev.write(b"\x00" + bytes([s.get() for s in sliders]))
                dev.write(b"\x00" + bytes([0xff for _ in range(400)]))
                str_in = dev.read(401)
                print("Received from HID Device: ", str_in, '\n')
        print("could not find such device")
        os._exit(1)
    except BaseException as e:
        print(e)
        exit(1)


master = Tk()
names = ["red", "green", "blue", "white", "amber", "UV", "dimming", "strobe"]
sliders = [Scale(master, from_=0, to=255, tickinterval=8) for i in range(512)]
for i, s in enumerate(sliders):
    s.set(0)
    s.grid(row=0, column = i)
    Label(master, text=(names[i] if i < len(names) else f"channel {i}")).grid(row=1, column=i)

t = Thread(target=send_sliders, args=[sliders])
t.start()
mainloop()
os._exit(1)
