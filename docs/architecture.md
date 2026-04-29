# Camera Streaming System Architecture (OV5642 + SPI/I2C)

## 1. Overview

This project implements a **userspace MJPEG streaming pipeline** using the OV5642 camera sensor interfaced via:

* **I2C** → Sensor configuration
* **SPI** → Image data transfer (via Arducam FIFO)
* **TCP Socket** → MJPEG streaming to browser

The same application runs on multiple platforms (i.MX, Raspberry Pi), with **hardware differences handled via Device Tree (DTS)**.

---

## 2. High-Level Architecture

```
          +----------------------+
          |      OV5642 Sensor   |
          |  (Image Capture ISP) |
          +----------+-----------+
                     |
                     | I2C (Control)
                     v
          +----------------------+
          |   SoC (i.MX / RPi)   |
          |                      |
          |  Userspace App       |
          |  ------------------  |
          |  - Sensor Init       |
          |  - Capture Trigger   |
          |  - Frame Read (SPI)  |
          |  - MJPEG Streaming   |
          +----------+-----------+
                     |
                     | SPI (Image Data via FIFO)
                     v
          +----------------------+
          |   Arducam FIFO/CPLD  |
          +----------------------+
                     |
                     v
          +----------------------+
          |   TCP Socket Server  |
          |  (Port 8080)         |
          +----------------------+
                     |
                     v
          +----------------------+
          |   Web Browser Client |
          |  (MJPEG Stream View) |
          +----------------------+
```

---

## 3. Data Flow

### Step 1: Sensor Initialization

* OV5642 configured via I2C register writes
* Resolution, format (JPEG), and ISP settings applied

### Step 2: Capture Trigger

* SPI register write triggers frame capture
* Arducam FIFO stores compressed JPEG

### Step 3: Frame Length Read

* Frame size read via SPI registers
* Validity check performed

### Step 4: Burst Read (SPI)

* Data read in chunks from FIFO
* High-speed SPI (~8 MHz) used

### Step 5: MJPEG Streaming

* HTTP multipart response generated
* Each frame sent as:

  ```
  --frame boundary
  Content-Type: image/jpeg
  Content-Length: <size>
  ```
* Continuous streaming over TCP socket

---

## 4. Key Components

### 4.1 I2C Interface

* Used for:

  * Sensor configuration
  * Register read/write
* Communicates directly with OV5642

---

### 4.2 SPI Interface

* Used for:

  * Reading image data from FIFO
  * Triggering capture
* Operates at high speed for real-time streaming

---

### 4.3 Arducam FIFO Buffer

* Acts as:

  * Temporary image storage
  * Interface between sensor and SPI
* Prevents data loss during transfer

---

### 4.4 Userspace Application

Main responsibilities:

* Initialize hardware
* Trigger captures
* Read and parse JPEG frames
* Stream data over network

---

### 4.5 Networking Layer

* TCP server on port `8080`
* Sends MJPEG stream to clients
* Compatible with standard web browsers

---

## 5. Platform Adaptation

### i.MX Platform

* Requires **Device Tree (DTS) modifications**

  * SPI pinmux configuration
  * I2C enablement
* Demonstrates low-level hardware control

### Raspberry Pi

* Uses pre-enabled drivers
* Minimal configuration required

---

## 6. Design Decisions

### Why Userspace Instead of Kernel Driver?

* Faster development
* Easier debugging
* Sufficient for prototype and streaming use case

---

### Why MJPEG?

* Simple streaming format
* Browser-compatible
* No complex encoding pipeline required

---

### Why SPI + FIFO?

* Avoids parallel camera interface complexity
* Works across multiple platforms
* Simplifies hardware integration

---

## 7. Limitations

* No hardware acceleration (CPU-based streaming)
* Limited resolution due to FIFO size
* No synchronization or timestamping

---

## 8. Future Improvements

* V4L2 driver integration
* DMA-based SPI transfer
* H.264 encoding pipeline
* Multi-client streaming support
* Frame rate optimization

---

## 9. Summary

This project demonstrates:

* Embedded Linux hardware interfacing (SPI + I2C)
* Device Tree based board bring-up
* Real-time data streaming over network
* Cross-platform portability of userspace applications

It reflects a **complete embedded pipeline from sensor to browser**.

