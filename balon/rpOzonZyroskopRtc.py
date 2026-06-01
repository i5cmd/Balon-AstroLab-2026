"""
Balon Stratosferyczny — Raspberry Pi Pico 2
Zapis danych z zyroskopu i sensora ozonu na karte SD z RTC.
"""

import machine
import utime
import os
import math
import sdcard

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

# zyroskop kalibracja
ADC_OFFSET_X = 14186
ADC_OFFSET_Y = 11484
ADC_OFFSET_Z = 11474
SENSITIVITY  = 0.015  

# stale
RTC_ADDR   = 0x68
OZONE_ADDR = 0x73
OZONE_REG  = 0x09
SAMPLE_MS  = 200       
FLUSH_MS   = 10_000    

# rtc
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

# zyroskop

def read_gyro():
    sx, sy, sz = 0, 0, 0
    N = 32
    for _ in range(N):
        sx += adc_x.read_u16()
        sy += adc_y.read_u16()
        sz += adc_z.read_u16()
        utime.sleep_us(50)
    rx = sx // N
    ry = sy // N
    rz = sz // N

    xg = (rx - ADC_OFFSET_X) * SENSITIVITY
    yg = (ry - ADC_OFFSET_Y) * SENSITIVITY
    zg = (rz - ADC_OFFSET_Z) * SENSITIVITY

    try:
        pitch = math.degrees(math.atan2(xg, math.sqrt(yg**2 + zg**2)))
        roll  = math.degrees(math.atan2(yg, math.sqrt(xg**2 + zg**2)))
    except:
        pitch, roll = 0.0, 0.0

    return pitch, roll, rx, ry, rz

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
    print("[I2C] RTC:", "OK" if RTC_ADDR in devices else "BRAK")
    print("[I2C] Ozon:", "OK" if OZONE_ADDR in devices else "BRAK")
except:
    print("[I2C] Blad szyny")

# sd
sd_ok = init_sd()
filename = rtc_filename()

# zapis sd
file_ptr = None
if sd_ok:
    try:
        file_ptr = open(filename, "a")
        if os.stat(filename)[6] == 0:
            file_ptr.write("Czas,Sekundy_Misji,Pitch_deg,Roll_deg,Ozon_ppb,Raw_X,Raw_Y,Raw_Z\n")
            file_ptr.flush()
        print("[SD] Plik:", filename)
    except Exception as e:
        print("[SD] Blad pliku:", e)
        sd_ok = False

start_tick     = utime.ticks_ms()
last_sample    = utime.ticks_ms()
last_flush     = utime.ticks_ms()



# glowne
while True:
    try:
        now = utime.ticks_ms()

        if utime.ticks_diff(now, last_sample) >= SAMPLE_MS:
            last_sample = now

            uptime_s         = utime.ticks_diff(now, start_tick) / 1000.0
            timestamp        = rtc_timestamp()
            pitch, roll, rx, ry, rz = read_gyro()
            ozone            = read_ozone()

            line = "{},{:.1f},{:.2f},{:.2f},{},{},{},{}\n".format(
                timestamp, uptime_s, pitch, roll, ozone, rx, ry, rz)

            if sd_ok and file_ptr:
                file_ptr.write(line)

                if utime.ticks_diff(now, last_flush) >= FLUSH_MS:
                    file_ptr.flush()
                    os.sync()
                    last_flush = now

            print("[{}] P:{:>6.2f}  R:{:>6.2f}  O3:{} ppb".format(
                timestamp, pitch, roll, ozone))

    except Exception as e:
        print("[BLAD]", e)
        utime.sleep_ms(1000)
