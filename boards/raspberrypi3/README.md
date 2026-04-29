Raspberry Pi uses default SPI/I2C drivers, no DTS changes required

# Raspberry Pi Connections (OV5642 + Arducam SPI)

This section describes the hardware connections between the **Raspberry Pi** and the **OV5642 camera module (via Arducam SPI interface)**.

---

## 🔌 Interfaces Used

* **SPI** → Image data transfer
* **I2C** → Sensor configuration
* **Power** → 3.3V supply

---

## 📍 Pin Connections

### SPI (Camera Data - Arducam FIFO)

| Raspberry Pi Pin | GPIO   | Function | Camera Module |
| ---------------- | ------ | -------- | ------------- |
| Pin 19           | GPIO10 | MOSI     | MOSI          |
| Pin 21           | GPIO9  | MISO     | MISO          |
| Pin 23           | GPIO11 | SCLK     | SCLK          |
| Pin 24           | GPIO8  | CE0      | CS            |

---

### I2C (Sensor Control - OV5642)

| Raspberry Pi Pin | GPIO  | Function | Camera Module |
| ---------------- | ----- | -------- | ------------- |
| Pin 3            | GPIO2 | SDA      | SDA           |
| Pin 5            | GPIO3 | SCL      | SCL           |

---

### Power

| Raspberry Pi Pin | Function | Camera Module |
| ---------------- | -------- | ------------- |
| Pin 1            | 3.3V     | VCC           |
| Pin 6            | GND      | GND           |

---

## ⚙️ Enable Interfaces on Raspberry Pi

Run:

```bash
sudo raspi-config
```

Enable:

* SPI
* I2C

Then reboot:

```bash
sudo reboot
```

---

## 🔍 Verify Devices

Check SPI:

```bash
ls /dev/spidev*
```

Check I2C:

```bash
ls /dev/i2c*
```

---

## 🧠 Notes

* Raspberry Pi already provides SPI and I2C drivers — **no Device Tree changes required**
* Camera data is buffered through **Arducam FIFO (SPI)**, not directly via CSI
* Ensure stable 3.3V supply to avoid corrupted frames

---

## ⚠️ Common Issues

* ❌ `/dev/spidev0.0` not found → SPI not enabled
* ❌ `/dev/i2c-1` missing → I2C not enabled
* ❌ Corrupted images → check wiring and SPI speed

---

## 🏁 Summary

The Raspberry Pi setup acts as a **reference platform** where:

* Hardware interfaces are pre-configured
* Focus is on validating the userspace application

All hardware-specific complexity is handled in the i.MX platform via Device Tree.
.
