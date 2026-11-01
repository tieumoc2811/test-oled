// Test man hinh OLED SSD1306 (RP2040): tu do tat ca cap chan I2C hop le + dia chi 0x3C/0x3D, roi ve hinh thu.
// Ket qua do cung in ra cong serial USB (CDC) - mo Serial Monitor / PuTTY / Chrome serial terminal de xem.
// Mac dinh man 128x32. Man 128x64: cmake .. -DOLED_H=64
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#ifndef OLED_H
#define OLED_H 32
#endif
#define PAGES (OLED_H / 8)

static uint8_t fb[128 * PAGES];
static i2c_inst_t *inst;
static uint8_t addr;

static bool wr(const uint8_t *d, size_t n) { return i2c_write_timeout_us(inst, addr, d, n, false, 20000) > 0; }
static void cmd(const uint8_t *c, size_t n) { uint8_t b[40]; b[0] = 0x00; memcpy(b + 1, c, n); wr(b, n + 1); }

static void oled_init(void) {
  const uint8_t ini[] = {0xAE, 0xD5, 0x80, 0xA8, OLED_H - 1, 0xD3, 0, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
                         0xDA, OLED_H == 32 ? 0x02 : 0x12, 0x81, 0xFF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF};
  cmd(ini, sizeof ini);
}
static void flush(void) {
  for (int p = 0; p < PAGES; p++) {
    const uint8_t c[] = {0x21, 0, 127, 0x22, (uint8_t)p, (uint8_t)p}; cmd(c, sizeof c);
    uint8_t d[129]; d[0] = 0x40; memcpy(d + 1, fb + p * 128, 128); wr(d, 129);
  }
}
static void px(int x, int y) { if (x >= 0 && x < 128 && y >= 0 && y < OLED_H) fb[(y >> 3) * 128 + x] |= 1 << (y & 7); }

// Quet cac cap chan I2C hop le (SDA = GP0,2,4,... SCL = SDA+1; SDA chia 4 du 0 -> I2C0, du 2 -> I2C1)
static bool find_oled(void) {
  static const uint8_t A[2] = {0x3C, 0x3D};
  for (int sda = 0; sda <= 28; sda += 2) {
    i2c_inst_t *i = (sda % 4 == 0) ? i2c0 : i2c1;
    i2c_init(i, 100000);
    gpio_set_function(sda, GPIO_FUNC_I2C); gpio_set_function(sda + 1, GPIO_FUNC_I2C);
    gpio_pull_up(sda); gpio_pull_up(sda + 1);
    for (int k = 0; k < 2; k++) {
      uint8_t b = 0x00; inst = i; addr = A[k];
      if (i2c_write_timeout_us(i, A[k], &b, 1, false, 5000) > 0) {
        printf("TIM THAY OLED: SDA=GP%d SCL=GP%d dia chi 0x%02X (I2C%d)\n", sda, sda + 1, A[k], i == i2c0 ? 0 : 1);
        i2c_set_baudrate(i, 400000);
        return true;
      }
    }
    gpio_set_function(sda, GPIO_FUNC_NULL); gpio_set_function(sda + 1, GPIO_FUNC_NULL);
    gpio_disable_pulls(sda); gpio_disable_pulls(sda + 1); i2c_deinit(i);
  }
  return false;
}

int main(void) {
  stdio_init_all();
  sleep_ms(3000);   // cho may tinh mo cong serial
  printf("Test OLED %dx%d\n", 128, OLED_H);
  while (!find_oled()) { printf("Khong thay OLED tren cap chan I2C nao (0x3C/0x3D). Kiem tra day/nguon. Thu lai...\n"); sleep_ms(3000); }
  oled_init();
  for (int n = 0;; n++) {
    memset(fb, 0, sizeof fb);
    switch (n % 4) {
      case 0: memset(fb, 0xFF, sizeof fb); break;                                   // trang toan man hinh
      case 1: break;                                                                // den toan man hinh
      case 2: for (int y = 0; y < OLED_H; y++) for (int x = 0; x < 128; x++) if (((x / 4) + (y / 4)) & 1) px(x, y); break;   // ban co
      default:                                                                      // vien + duong cheo
        for (int x = 0; x < 128; x++) { px(x, 0); px(x, OLED_H - 1); }
        for (int y = 0; y < OLED_H; y++) { px(0, y); px(127, y); }
        for (int x = 0; x < 128; x++) px(x, x * (OLED_H - 1) / 127);
    }
    flush();
    printf("khung %d\n", n % 4);
    sleep_ms(1000);
  }
}
