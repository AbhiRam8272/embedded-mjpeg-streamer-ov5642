# Embedded MJPEG Streamer (OV5642)

A cross-platform **embedded Linux camera streaming system** using the OV5642 image sensor with SPI + I2C interfaces.
The project demonstrates **Device Tree–based hardware bring-up**, real-time image capture, and MJPEG streaming over a TCP socket.

---

## 🚀 Features

* 📷 OV5642 camera integration
* 🔌 SPI (image data) + I2C (sensor control)
* 🌐 MJPEG streaming over HTTP (browser compatible)
* ⚡ Chunk-based high-speed SPI transfer (~8 MHz)
* 🧠 Cross-platform userspace application (i.MX + Raspberry Pi)

---

## 🧩 Project Structure

```bash
embedded-mjpeg-streamer-ov5642/
├── app/                  # Userspace application
│   ├── capture_still.c
│   └── mjpeg_streamer.c
├── boards/
│   ├── imx/
│   │   └── dts/          # Device Tree configuration
│   └── raspberrypi/
│       └── config/       # Minimal setup (no DTS changes)
├── docs/
│   └── architecture.md   # System design
└── README.md
```

---

## 🏗️ Architecture

* **I2C** → Configures OV5642 registers
* **SPI** → Reads JPEG data via Arducam FIFO
* **Userspace App** → Controls capture + streaming
* **TCP Socket** → Streams MJPEG to browser

👉 See [`docs/architecture.md`](docs/architecture.md) for full design

---

## ⚙️ Platform Details

### i.MX (Embedded Linux)

* Requires **Device Tree (DTS) modification**

  * SPI pin configuration
  * I2C enablement
* Demonstrates low-level hardware bring-up

---

### Raspberry Pi

* Uses built-in SPI/I2C drivers
* No DTS modification required

---

## ▶️ Build & Run

### 1. Compile

```bash
gcc app/mjpeg_streamer.c -o streamer
gcc app/capture_still.c -o capture
```

---

### 2. Run MJPEG Stream

```bash
sudo ./streamer
```

Open browser:

```
http://<board-ip>:8080
```

---

### 3. Capture Still Image

```bash
sudo ./capture
```

---

## 🧠 Key Concepts Demonstrated

* Embedded Linux userspace development
* SPI & I2C communication
* Device Tree (DTS) configuration
* Camera pipeline design
* Socket programming (MJPEG streaming)

---

## ⚠️ Limitations

* No hardware acceleration
* Single-client streaming
* Limited by FIFO buffer size

---

## 🔮 Future Work

* V4L2 driver integration
* DMA-based SPI transfer
* H.264 encoding
* Multi-client streaming

---

## 👨‍💻 Author

Developed as part of embedded systems practice focusing on **camera bring-up and streaming pipeline design**.

