#include "ssd1306.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_DISPOSITIVO "/dev/i2c-1"

static int fd_i2c = -1;
static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

static void escrever(const uint8_t *dados, size_t tamanho)
{
  if (write(fd_i2c, dados, tamanho) != (ssize_t)tamanho)
    perror("i2c write");
}

static void enviar_comando(uint8_t comando)
{
  uint8_t dados[2] = { 0x00, comando };
  escrever(dados, 2);
}

static void iniciar_i2c(void)
{
  fd_i2c = open(I2C_DISPOSITIVO, O_RDWR);
  if (fd_i2c < 0) { perror("open /dev/i2c-1"); exit(1); }

  if (ioctl(fd_i2c, I2C_SLAVE, SSD1306_ADDR) < 0)
  { perror("ioctl I2C_SLAVE"); exit(1); }
}

void ssd1306_init(void)
{
  iniciar_i2c();
  usleep(100 * 1000);

  static const uint8_t init[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
    0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
  };

  for (size_t i = 0; i < sizeof(init); i++)
    enviar_comando(init[i]);

  ssd1306_limpar();
  ssd1306_atualizar();
}

void ssd1306_limpar(void)
{
  memset(buffer, 0, sizeof(buffer));
}

void ssd1306_pixel(int x, int y, bool cor)
{
  if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT)
    return;

  int indice = x + (y / 8) * SSD1306_WIDTH;
  uint8_t mascara = 1 << (y % 8);

  if (cor)
    buffer[indice] |= mascara;
  else
    buffer[indice] &= ~mascara;
}

void ssd1306_retangulo_preenchido(int x, int y, int largura, int altura, bool cor)
{
  for (int py = y; py < y + altura; py++)
    for (int px = x; px < x + largura; px++)
      ssd1306_pixel(px, py, cor);
}

static int funcao_aresta(int ax, int ay, int bx, int by, int px, int py)
{
  return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void ssd1306_triangulo_preenchido(int x0, int y0, int x1, int y1, int x2, int y2, bool cor)
{
  int min_x = x0, max_x = x0, min_y = y0, max_y = y0;

  if (x1 < min_x) { min_x = x1; }
  if (x1 > max_x) { max_x = x1; }
  if (x2 < min_x) { min_x = x2; }
  if (x2 > max_x) { max_x = x2; }
  if (y1 < min_y) { min_y = y1; }
  if (y1 > max_y) { max_y = y1; }
  if (y2 < min_y) { min_y = y2; }
  if (y2 > max_y) { max_y = y2; }
  for (int py = min_y; py <= max_y; py++)
  {
    for (int px = min_x; px <= max_x; px++)
    {
      int e0 = funcao_aresta(x0, y0, x1, y1, px, py);
      int e1 = funcao_aresta(x1, y1, x2, y2, px, py);
      int e2 = funcao_aresta(x2, y2, x0, y0, px, py);

      bool neg = (e0 < 0) || (e1 < 0) || (e2 < 0);
      bool pos = (e0 > 0) || (e1 > 0) || (e2 > 0);

      if (!(neg && pos))
        ssd1306_pixel(px, py, cor);
    }
  }
}

void ssd1306_atualizar(void)
{
  enviar_comando(0x21); enviar_comando(0); enviar_comando(SSD1306_WIDTH - 1);
  enviar_comando(0x22); enviar_comando(0); enviar_comando(7);

  uint8_t dados[SSD1306_WIDTH + 1];
  dados[0] = 0x40;

  for (int pagina = 0; pagina < 8; pagina++)
  {
    memcpy(&dados[1], &buffer[pagina * SSD1306_WIDTH], SSD1306_WIDTH);
    escrever(dados, SSD1306_WIDTH + 1);
  }
}