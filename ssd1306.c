#include "ssd1306.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <string.h>

#define I2C_PORTA i2c0
#define I2C_SDA   4
#define I2C_SCL   5

static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];



static void enviar_comando(uint8_t comando)
{
  uint8_t dados[2];

  dados[0] = 0x00;       // Co = 0, D/C# = 0 -> comando
  dados[1] = comando;

  i2c_write_blocking(I2C_PORTA, SSD1306_ADDR, dados, 2, false);
}



static void iniciar_i2c(void)
{
  i2c_init(I2C_PORTA, 400000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
}



void ssd1306_init(void)
{
  iniciar_i2c();

  sleep_ms(100);

  enviar_comando(0xAE); // Desliga o display

  enviar_comando(0xD5); // Define o clock do display
  enviar_comando(0x80);

  enviar_comando(0xA8); // Taxa de multiplexação
  enviar_comando(0x3F);

  enviar_comando(0xD3); // Deslocamento do display
  enviar_comando(0x00);

  enviar_comando(0x40); // Linha inicial = 0

  enviar_comando(0x8D); // Bomba de carga
  enviar_comando(0x14);

  enviar_comando(0x20); // Modo de endereçamento de memória
  enviar_comando(0x00); // Modo horizontal

  enviar_comando(0xA1); // Inverte a ordem dos segmentos

  enviar_comando(0xC8); // Direção de varredura das linhas (COM)

  enviar_comando(0xDA); // Pinos COM
  enviar_comando(0x12);

  enviar_comando(0x81); // Contraste
  enviar_comando(0x7F);

  enviar_comando(0xD9); // Pré-carga
  enviar_comando(0xF1);

  enviar_comando(0xDB); // VCOMH
  enviar_comando(0x40);

  enviar_comando(0xA4); // Display segue a RAM

  enviar_comando(0xA6); // Modo normal (não invertido)

  enviar_comando(0xAF); // Liga o display

  ssd1306_limpar();
  ssd1306_atualizar();
}


// ---------------------------------------------------------
// Limpa o framebuffer (a "tela" na memória)
// ---------------------------------------------------------

void ssd1306_limpar(void)
{
  memset(buffer, 0, sizeof(buffer));
}


// ---------------------------------------------------------
// Acende ou apaga um pixel
// ---------------------------------------------------------

void ssd1306_pixel(int x, int y, bool cor)
{
  if (x < 0 || x >= SSD1306_WIDTH)
    return;

  if (y < 0 || y >= SSD1306_HEIGHT)
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
  {
    for (int px = x; px < x + largura; px++)
    {
      ssd1306_pixel(px, py, cor);
    }
  }
}



static int funcao_aresta(int ax, int ay, int bx, int by, int px, int py)
{
  return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}


void ssd1306_triangulo_preenchido(int x0, int y0, int x1, int y1, int x2, int y2, bool cor)
{
  // Calcula a caixa que envolve o triângulo, pra não testar a tela inteira
  int min_x = x0, max_x = x0, min_y = y0, max_y = y0;

  if (x1 < min_x) min_x = x1; if (x1 > max_x) max_x = x1;
  if (x2 < min_x) min_x = x2; if (x2 > max_x) max_x = x2;
  if (y1 < min_y) min_y = y1; if (y1 > max_y) max_y = y1;
  if (y2 < min_y) min_y = y2; if (y2 > max_y) max_y = y2;

  for (int py = min_y; py <= max_y; py++)
  {
    for (int px = min_x; px <= max_x; px++)
    {
      int e0 = funcao_aresta(x0, y0, x1, y1, px, py);
      int e1 = funcao_aresta(x1, y1, x2, y2, px, py);
      int e2 = funcao_aresta(x2, y2, x0, y0, px, py);

      bool algum_negativo = (e0 < 0) || (e1 < 0) || (e2 < 0);
      bool algum_positivo = (e0 > 0) || (e1 > 0) || (e2 > 0);

      // Se os três sinais forem iguais, o pixel está dentro do triângulo
      if (!(algum_negativo && algum_positivo))
        ssd1306_pixel(px, py, cor);
    }
  }
}


// ---------------------------------------------------------
// Envia o framebuffer inteiro para o OLED
// ---------------------------------------------------------

void ssd1306_atualizar(void)
{
  enviar_comando(0x21); // Endereço de coluna
  enviar_comando(0);
  enviar_comando(SSD1306_WIDTH - 1);

  enviar_comando(0x22); // Endereço de página
  enviar_comando(0);
  enviar_comando(7);

  uint8_t dados[SSD1306_WIDTH + 1];
  dados[0] = 0x40; // Co = 0, D/C# = 1 → dados

  for (int pagina = 0; pagina < 8; pagina++)
  {
    memcpy(&dados[1], &buffer[pagina * SSD1306_WIDTH], SSD1306_WIDTH);

    i2c_write_blocking(I2C_PORTA, SSD1306_ADDR, dados, SSD1306_WIDTH + 1, false);
  }
}