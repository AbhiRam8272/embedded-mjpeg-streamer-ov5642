## 🔍 DTS Changes (Project-Specific)

This project modifies the original `imx6ul-14x14-evk.dtsi` to enable SPI and I2C interfaces required for the OV5642 camera.

Only the relevant changes are shown below.

---

### ✅ SPI (ECSPI1) Enablement

```dts
&ecspi1 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_ecspi1>;
    status = "okay";

    spidev@0 {
        compatible = "spidev";
        reg = <0>;
        spi-max-frequency = <8000000>;
    };
};
```

✔ Enables SPI controller
✔ Exposes `/dev/spidevX.X` for userspace access

---

### ✅ I2C Enablement

```dts
&i2c1 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_i2c1>;
    status = "okay";
};
```

✔ Enables I2C bus
✔ Used for OV5642 register communication

---

### ✅ Pin Multiplexing (IOMUX)

```dts
pinctrl_ecspi1: ecspi1grp {
    fsl,pins = <
        MX6UL_PAD_CSI_DATA04__ECSPI1_SCLK 0x10b0
        MX6UL_PAD_CSI_DATA05__ECSPI1_MOSI 0x10b0
        MX6UL_PAD_CSI_DATA06__ECSPI1_MISO 0x10b0
        MX6UL_PAD_CSI_DATA07__GPIO4_IO18  0x10b0
    >;
};

pinctrl_i2c1: i2c1grp {
    fsl,pins = <
        MX6UL_PAD_UART4_TX_DATA__I2C1_SCL 0x4001b8b1
        MX6UL_PAD_UART4_RX_DATA__I2C1_SDA 0x4001b8b1
    >;
};
```

✔ Routes physical pins to SPI and I2C peripherals
✔ Essential for hardware communication

---

## 🔌 i.MX6UL Pin Connections (SPI + I2C)

This section maps the **Device Tree pinmux configuration** to the **actual physical signals** used in the project.

---

## 📍 SPI (ECSPI1) Pin Mapping

| i.MX6UL Pad  | Function (Configured) | Signal           | Connected To |
| ------------ | --------------------- | ---------------- | ------------ |
| `CSI_DATA04` | `ECSPI1_SCLK`         | SPI Clock        | Arducam SCLK |
| `CSI_DATA05` | `ECSPI1_MOSI`         | Master Out       | Arducam MOSI |
| `CSI_DATA06` | `ECSPI1_MISO`         | Master In        | Arducam MISO |
| `CSI_DATA07` | `GPIO4_IO18`          | Chip Select (CS) | Arducam CS   |

---

### Corresponding DTS Configuration

```dts id="spi6ul"
pinctrl_ecspi1: ecspi1grp {
    fsl,pins = <
        MX6UL_PAD_CSI_DATA04__ECSPI1_SCLK 0x10b0
        MX6UL_PAD_CSI_DATA05__ECSPI1_MOSI 0x10b0
        MX6UL_PAD_CSI_DATA06__ECSPI1_MISO 0x10b0
        MX6UL_PAD_CSI_DATA07__GPIO4_IO18  0x10b0
    >;
};
```

---

## 📍 I2C (I2C1) Pin Mapping

| i.MX6UL Pad     | Function (Configured) | Signal | Connected To |
| --------------- | --------------------- | ------ | ------------ |
| `UART4_TX_DATA` | `I2C1_SCL`            | Clock  | OV5642 SCL   |
| `UART4_RX_DATA` | `I2C1_SDA`            | Data   | OV5642 SDA   |

---

### Corresponding DTS Configuration

```dts id="i2c6ul"
pinctrl_i2c1: i2c1grp {
    fsl,pins = <
        MX6UL_PAD_UART4_TX_DATA__I2C1_SCL 0x4001b8b1
        MX6UL_PAD_UART4_RX_DATA__I2C1_SDA 0x4001b8b1
    >;
};
```

---

## ⚡ Power Connections

| Signal | Connection    |
| ------ | ------------- |
| VCC    | 3.3V supply   |
| GND    | Common ground |

---

## 🧠 Important Notes

* i.MX6UL pins are **multiplexed**, meaning each pin can serve multiple functions
* The Device Tree selects the required function using **IOMUX configuration**
* Incorrect pinmux settings will result in:

  * No SPI communication
  * I2C device not detected

## 🧠 Summary of Changes

| Feature | Original DTS           | Modified DTS         |
| ------- | ---------------------- | -------------------- |
| SPI     | Disabled / not exposed | Enabled with spidev  |
| I2C     | May be disabled        | Enabled              |
| Pinmux  | Default                | Custom for SPI + I2C |

---

## 💡 Key Insight

The userspace application remains unchanged across platforms.
Only the Device Tree is modified to adapt the hardware.

This demonstrates:

* Board bring-up capability
* Hardware abstraction using Device Tree

