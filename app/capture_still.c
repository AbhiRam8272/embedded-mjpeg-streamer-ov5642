#include <time.h> // Add this to the top of your file with the other includes
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include "ov5642.h"

#define SPI_DEV "/dev/spidev0.1"
#define I2C_ADDR 0x3C
#define SPEED 8000000
#define CHUNK_SIZE 65000 

// ---------------- I2C ----------------
int i2c_write(int fd, uint16_t reg, uint8_t val) {
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg msg;
    uint8_t buf[3] = {reg >> 8, reg & 0xFF, val};

    msg.addr = I2C_ADDR;
    msg.flags = 0;
    msg.len = 3;
    msg.buf = buf;

    packets.msgs = &msg;
    packets.nmsgs = 1;
    return ioctl(fd, I2C_RDWR, &packets);
}

int i2c_read(int fd, uint16_t reg, uint8_t *val) {
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg msgs[2];
    uint8_t reg_buf[2] = {reg >> 8, reg & 0xFF};

    msgs[0].addr = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = reg_buf;

    msgs[1].addr = I2C_ADDR;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 1;
    msgs[1].buf = val;

    packets.msgs = msgs;
    packets.nmsgs = 2;
    return ioctl(fd, I2C_RDWR, &packets);
}

// ---------------- SPI ----------------
int spi_transfer(int fd, uint8_t *tx, uint8_t *rx, int len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .speed_hz = SPEED,
        .bits_per_word = 8,
    };
    return ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
}

uint8_t spi_read_reg(int fd, uint8_t addr) {
    uint8_t tx[2] = {addr & 0x7F, 0x00};
    uint8_t rx[2];
    spi_transfer(fd, tx, rx, 2);
    return rx[1];
}

void spi_write_reg(int fd, uint8_t addr, uint8_t val) {
    // 0x80 sets the MSB high to indicate a Write operation to the Arducam CPLD
    uint8_t tx[2] = {addr | 0x80, val};
    uint8_t rx[2];
    spi_transfer(fd, tx, rx, 2);
}

// ---------------- SENSOR INIT ----------------
void load_table(int fd, const struct sensor_reg *table) {
    for (int i = 0; table[i].reg != 0xffff; i++) {
        i2c_write(fd, table[i].reg, table[i].val);
        usleep(500);
    }
}

void ov5642_init(int fd) {
    printf("[INFO] Initializing OV5642...\n");

    printf("[INFO] Booting ISP...\n");
    load_table(fd, OV5642_QVGA_Preview);
    usleep(100000);

    // Load the native 5MP config
    printf("[INFO] Loading QSXGA JPEG config...\n");
    load_table(fd, OV5642_JPEG_Capture_QSXGA);
    usleep(100000); 

    // REMOVE the 1024x768 table entirely to fix the vertical banding artifact!

    // Increase JPEG compression slightly so the massive 5MP image doesn't 
    // overflow the 512KB SRAM chip. (Default is usually 0x08. 0x10 compresses more).
    i2c_write(fd, 0x4407, 0x10); 

    printf("[OK] OV5642 configured\n");
}

// ---------------- MAIN ----------------
int main() {
    int spi_fd = open(SPI_DEV, O_RDWR);
    if (spi_fd < 0) {
        perror("[ERROR] Failed to open SPI device");
        return -1;
    }

    int i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        perror("[ERROR] Failed to open I2C device");
        return -1;
    }

    printf("[OK] SPI + I2C ready\n");

    spi_write_reg(spi_fd, 0x02, 0x00);
    usleep(10000);

    // Sensor check
    uint8_t h, l;
    i2c_read(i2c_fd, 0x300A, &h);
    i2c_read(i2c_fd, 0x300B, &l);
    printf("Sensor ID: %02X %02X\n", h, l);

    // 1. BOOT ONCE
    ov5642_init(i2c_fd);
    
    printf("[INFO] Letting AEC/AWB stabilize (1 second)...\n");
    sleep(1); 
    spi_write_reg(spi_fd, 0x03, 0x02); 

    printf("\n==========================================\n");
    printf(" CAMERA READY. Press [ENTER] to snap a photo.\n");
    printf(" Type 'q' and [ENTER] to quit.\n");
    printf("==========================================\n");

    // 2. STAY ALIVE AND WAIT FOR TRIGGER
    while (1) {
        char input = getchar();
        if (input == 'q') {
            printf("Exiting...\n");
            break;
        }
        if (input != '\n') continue; // Ignore extra keystrokes

        // Generate a unique filename using the current time
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        char filepath[128];
        sprintf(filepath, "images/image_%04d%02d%02d_%02d%02d%02d.jpg",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec);

        // ---------------- START CORRECT CAPTURE SEQUENCE ----------------
        spi_write_reg(spi_fd, 0x04, 0x01); 
        spi_write_reg(spi_fd, 0x04, 0x01); 
        
        printf("[INFO] Triggering capture...\n");
        spi_write_reg(spi_fd, 0x04, 0x02); 
        
        while (!(spi_read_reg(spi_fd, 0x41) & 0x08)) {
            usleep(1000);
        }

        // ---------------- READ LENGTH ----------------
        spi_read_reg(spi_fd, 0x42); 

        uint32_t len1 = spi_read_reg(spi_fd, 0x42); 
        uint32_t len2 = spi_read_reg(spi_fd, 0x43);
        uint32_t len3 = spi_read_reg(spi_fd, 0x44); 
        
        int length = ((len3 & 0x7F) << 16) | (len2 << 8) | len1;

        if (length == 0 || length >= 524288) {
            printf("[ERROR] Invalid length (%d). Skipping.\n", length);
            continue;
        }

        // ---------------- FAST CHUNKED BURST READ ----------------
        spi_write_reg(spi_fd, 0x04, 0x10);

        uint8_t tx_buf[CHUNK_SIZE + 1];
        uint8_t rx_buf[CHUNK_SIZE + 1];

        tx_buf[0] = 0x3C; 
        for (int i = 1; i <= CHUNK_SIZE; i++) tx_buf[i] = 0x00;

        FILE *fp = fopen(filepath, "wb");
        if (!fp) {
            perror("[ERROR] Failed to open file");
            continue;
        }

        int started = 0;
        int count = 0;
        uint8_t prev = 0;
        int remaining = length;

        while (remaining > 0) {
            int current_chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

            struct spi_ioc_transfer tr = {
                .tx_buf = (unsigned long)tx_buf,
                .rx_buf = (unsigned long)rx_buf,
                .len = current_chunk + 1,
                .speed_hz = SPEED,
                .bits_per_word = 8,
            };

            if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
                perror("[ERROR] SPI transfer failed");
                break;
            }

            for (int i = 1; i <= current_chunk; i++) {
                uint8_t cur = rx_buf[i];

                if (!started && prev == 0xFF && cur == 0xD8) {
                    started = 1;
                    fwrite(&prev, 1, 1, fp);
                    fwrite(&cur, 1, 1, fp);
                    count += 2;
                } else if (started) {
                    fwrite(&cur, 1, 1, fp);
                    count++;
                }

                if (started && prev == 0xFF && cur == 0xD9) {
                    remaining = 0; 
                    break;
                }
                prev = cur;
            }
            remaining -= current_chunk;
        }

        fclose(fp);
        printf("[SUCCESS] Saved %d bytes to %s\n", count, filepath);
        printf("\nPress [ENTER] to snap another photo, or 'q' to quit.\n");
    }

    close(spi_fd);
    close(i2c_fd);

    return 0;
}
