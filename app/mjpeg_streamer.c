#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ov5642.h"

#define SPI_DEV "/dev/spidev0.1"
#define I2C_ADDR 0x3C
#define SPEED 8000000  // Boosted to 8MHz for video streaming!
#define CHUNK_SIZE 4000 
#define PORT 8080

// ---------------- I2C & SPI HELPERS ----------------
int i2c_write(int fd, uint16_t reg, uint8_t val) {
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg msg;
    uint8_t buf[3] = {reg >> 8, reg & 0xFF, val};
    msg.addr = I2C_ADDR; msg.flags = 0; msg.len = 3; msg.buf = buf;
    packets.msgs = &msg; packets.nmsgs = 1;
    return ioctl(fd, I2C_RDWR, &packets);
}

int i2c_read(int fd, uint16_t reg, uint8_t *val) {
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg msgs[2];
    uint8_t reg_buf[2] = {reg >> 8, reg & 0xFF};
    msgs[0].addr = I2C_ADDR; msgs[0].flags = 0; msgs[0].len = 2; msgs[0].buf = reg_buf;
    msgs[1].addr = I2C_ADDR; msgs[1].flags = I2C_M_RD; msgs[1].len = 1; msgs[1].buf = val;
    packets.msgs = msgs; packets.nmsgs = 2;
    return ioctl(fd, I2C_RDWR, &packets);
}

int spi_transfer(int fd, uint8_t *tx, uint8_t *rx, int len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx, .rx_buf = (unsigned long)rx,
        .len = len, .speed_hz = SPEED, .bits_per_word = 8,
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
    //load_table(fd, OV5642_QVGA_Preview);
    usleep(100000); 
    //load_table(fd, OV5642_JPEG_Capture_QSXGA);
    usleep(100000); 
    
    // Lock to 320x240 for high framerate live streaming
    printf("[INFO] Applying 320x240 Video Resolution...\n");
    load_table(fd, ov5642_640x480);
    usleep(100000);
    printf("[OK] OV5642 Video Mode configured\n");
}

// ---------------- MAIN ----------------
int main() {
    int spi_fd = open(SPI_DEV, O_RDWR);
    int i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (spi_fd < 0 || i2c_fd < 0) return -1;

    spi_write_reg(spi_fd, 0x02, 0x00);
    usleep(10000);
    ov5642_init(i2c_fd);
    sleep(1); 
    spi_write_reg(spi_fd, 0x03, 0x02); 

    // ---------------- SOCKET SETUP ----------------
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    
    printf("\n=======================================================\n");
    printf("[SUCCESS] MJPEG Server is LIVE!\n");
    printf("[SUCCESS] Open a web browser on your laptop and go to:\n");
    printf("          http://<YOUR_RASPBERRY_PI_IP>:%d\n", PORT);
    printf("=======================================================\n\n");

    while(1) {
        int client_fd = accept(server_fd, NULL, NULL);
        printf("[INFO] Client connected! Streaming video...\n");

        // Send MJPEG HTTP Header
        const char *http_hdr = "HTTP/1.0 200 OK\r\n"
                               "Connection: close\r\n"
                               "Cache-Control: no-cache\r\n"
                               "Cache-Control: private\r\n"
                               "Pragma: no-cache\r\n"
                               "Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\n\r\n";
        send(client_fd, http_hdr, strlen(http_hdr), MSG_NOSIGNAL);

        int client_connected = 1;
        while(client_connected) {
            // 1. Trigger Capture
            spi_write_reg(spi_fd, 0x04, 0x01); 
            spi_write_reg(spi_fd, 0x04, 0x01); 
            spi_write_reg(spi_fd, 0x04, 0x02); 
            
            while (!(spi_read_reg(spi_fd, 0x41) & 0x08)) usleep(500);

            // 2. Read Length
            spi_read_reg(spi_fd, 0x42); 
            uint32_t len1 = spi_read_reg(spi_fd, 0x42); 
            uint32_t len2 = spi_read_reg(spi_fd, 0x43);
            uint32_t len3 = spi_read_reg(spi_fd, 0x44); 
            int length = ((len3 & 0x7F) << 16) | (len2 << 8) | len1;

            if (length == 0 || length >= 524288) continue; // Skip bad frames

            // 3. Send Frame Boundary Header
            char frame_hdr[128];
            sprintf(frame_hdr, "--mjpegstream\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", length);
            if (send(client_fd, frame_hdr, strlen(frame_hdr), MSG_NOSIGNAL) < 0) {
                client_connected = 0; break;
            }

            // 4. Reset Read Pointer
            spi_write_reg(spi_fd, 0x04, 0x10); 

            // 5. Burst Read and Stream directly to Socket
            uint8_t tx_buf[CHUNK_SIZE + 1];
            uint8_t rx_buf[CHUNK_SIZE + 1];
            tx_buf[0] = 0x3C; 
            for (int i = 1; i <= CHUNK_SIZE; i++) tx_buf[i] = 0x00;

            int remaining = length;
            while (remaining > 0) {
                int current_chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
                struct spi_ioc_transfer tr = {
                    .tx_buf = (unsigned long)tx_buf, .rx_buf = (unsigned long)rx_buf,
                    .len = current_chunk + 1, .speed_hz = SPEED, .bits_per_word = 8,
                };
                ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);

                // Stream rx_buf[1] through current_chunk directly to the network
                if (send(client_fd, rx_buf + 1, current_chunk, MSG_NOSIGNAL) < 0) {
                    client_connected = 0; break;
                }
                remaining -= current_chunk;
            }
            
            // End of frame carriage return
            if (send(client_fd, "\r\n\r\n", 4, MSG_NOSIGNAL) < 0) client_connected = 0;
        }
        printf("[INFO] Client disconnected. Waiting for new viewer...\n");
        close(client_fd);
    }
    close(spi_fd); close(i2c_fd);
    return 0;
}
