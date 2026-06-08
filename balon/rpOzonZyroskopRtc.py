"""
Balon Stratosferyczny — Raspberry Pi Pico 2
Zapis danych z zyroskopu i sensora ozonu na karte SD z RTC DS1307.
"""

import machine
import utime
import os
import math
import sdcard
import UART, Pin
import time
# piny
spi = machine.SPI(0,
                  baudrate=1_000_000,
                  polarity=0,
                  phase=0,
                  bits=8,
                  firstbit=machine.SPI.MSB,
                  sck=machine.Pin(18),
                  mosi=machine.Pin(19),
                  miso=machine.Pin(16))
cs = machine.Pin(17, machine.Pin.OUT)
cs.value(1)

i2c = machine.I2C(0, sda=machine.Pin(4), scl=machine.Pin(5), freq=10_000)

adc_x = machine.ADC(26)
adc_y = machine.ADC(27)
adc_z = machine.ADC(28)

uart = UART(0, 9600, tx=Pin(0))

OFFSET = 32768

# stale
RTC_ADDR   = 0x68
OZONE_ADDR = 0x73
OZONE_REG  = 0x09
SAMPLE_MS  = 200
FLUSH_MS   = 10_000

# rtc DS1307
def _bcd(b):
    return (b >> 4) * 10 + (b & 0x0F)

def rtc_read():
    d = i2c.readfrom_mem(RTC_ADDR, 0x00, 7)
    return (
        _bcd(d[6]) + 2000,
        _bcd(d[5] & 0x1F),
        _bcd(d[4]),
        _bcd(d[2] & 0x3F),
        _bcd(d[1]),
        _bcd(d[0] & 0x7F),
    )

def rtc_timestamp():
    try:
        y, mo, d, h, m, s = rtc_read()
        return "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}".format(y, mo, d, h, m, s)
    except:
        return "RTC_BLAD"

def rtc_filename():
    try:
        y, mo, d, h, m, s = rtc_read()
        return "/sd/lot_{:02d}{:02d}_{:02d}{:02d}.csv".format(mo, d, h, m)
    except:
        return "/sd/lot_dane.csv"

# zyroskop — oryginalny kod
def read_angles():
    xv = adc_x.read_u16()
    yv = adc_y.read_u16()
    zv = adc_z.read_u16()

    xg = (xv - OFFSET) / OFFSET
    yg = (yv - OFFSET) / OFFSET
    zg = (zv - OFFSET) / OFFSET

    pitch = math.degrees(math.atan2(xg, math.sqrt(yg**2 + zg**2)))
    roll  = math.degrees(math.atan2(yg, math.sqrt(xg**2 + zg**2)))
    return pitch, roll

# ozon
def read_ozone():
    try:
        data = i2c.readfrom_mem(OZONE_ADDR, OZONE_REG, 2)
        return (data[0] << 8) | data[1]
    except:
        return -1

# SD
def init_sd():
    try:
        sd = sdcard.SDCard(spi, cs)
        os.mount(os.VfsFat(sd), "/sd")
        print("[SD] Karta OK.")
        return True
    except Exception as e:
        print("[SD] Blad:", e)
        return False

# check
try:
    devices = i2c.scan()
    print("[I2C] Urzadzenia:", [hex(d) for d in devices])
    print("[I2C] RTC:",  "OK" if RTC_ADDR   in devices else "BRAK")
    print("[I2C] Ozon:", "OK" if OZONE_ADDR in devices else "BRAK")
except:
    print("[I2C] Blad szyny")

# sd
sd_ok    = init_sd()
filename = rtc_filename()

# plik csv
file_ptr = None
if sd_ok:
    try:
        file_ptr = open(filename, "a")
        if os.stat(filename)[6] == 0:
            file_ptr.write("Czas,Sekundy_Misji,Pitch,Roll,Ozon_ppb\n")
            file_ptr.flush()
        print("[SD] Plik:", filename)
    except Exception as e:
        print("[SD] Blad pliku:", e)
        sd_ok = False

start_tick  = utime.ticks_ms()
last_sample = utime.ticks_ms()
last_flush  = utime.ticks_ms()

print("Logowanie rozpoczete!")

# glowne
while True:
     
     uart.write("siema")
     print("wyslano")
     
    try:
        
        now = utime.ticks_ms()

        if utime.ticks_diff(now, last_sample) >= SAMPLE_MS:
            last_sample = now

            uptime_s  = utime.ticks_diff(now, start_tick) / 1000.0
            timestamp = rtc_timestamp()
            p, r      = read_angles()
            ozone     = read_ozone()

            line = "{},{:.2f},{:.2f},{:.2f},{}\n".format(
                timestamp, uptime_s, p, r, ozone)

            if sd_ok and file_ptr:
                file_ptr.write(line)
                if utime.ticks_diff(now, last_flush) >= FLUSH_MS:
                    file_ptr.flush()
                    os.sync()
                    last_flush = now

            print("[{}] P:{:>6.2f}  R:{:>6.2f}  Ozon:{} ppb".format(
                timestamp, p, r, ozone))

    except Exception as e:
        print("[BLAD]", e)
        utime.sleep_ms(1000)

