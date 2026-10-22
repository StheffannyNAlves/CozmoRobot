#include "botao.h"

#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

#define GPIO_CHIP        "/dev/gpiochip0"
#define DEBOUNCE_NS      (30ULL * 1000000ULL)

static int fd_linha = -1;
static uint64_t ultimo_evento_ns = 0;

static uint64_t agora_ns(void)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (uint64_t)t.tv_sec * 1000000000ULL + t.tv_nsec;
}

void botao_init(unsigned linha)
{
  int chip = open(GPIO_CHIP, O_RDONLY);
  if (chip < 0) { perror("open " GPIO_CHIP); exit(1); }

  struct gpio_v2_line_request req;
  memset(&req, 0, sizeof(req));
  req.offsets[0] = linha;
  req.num_lines  = 1;
  strncpy(req.consumer, "cozmo", sizeof(req.consumer) - 1);
  req.config.flags = GPIO_V2_LINE_FLAG_INPUT
                   | GPIO_V2_LINE_FLAG_BIAS_PULL_UP
                   | GPIO_V2_LINE_FLAG_EDGE_FALLING
                   | GPIO_V2_LINE_FLAG_EDGE_RISING;

  if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &req) < 0)
  { perror("GPIO_V2_GET_LINE_IOCTL"); exit(1); }

  close(chip);
  fd_linha = req.fd;
}

bool botao_esperar(unsigned ms)
{
  uint64_t fim = agora_ns() + (uint64_t)ms * 1000000ULL;

  for (;;)
  {
    uint64_t agora = agora_ns();
    if (agora >= fim)
      return false;

    struct pollfd pfd = { .fd = fd_linha, .events = POLLIN };
    int restante_ms = (int)((fim - agora) / 1000000ULL) + 1;

    if (poll(&pfd, 1, restante_ms) <= 0)
      return false;                       // timeout (ou interrompido por sinal)

    struct gpio_v2_line_event ev;
    if (read(fd_linha, &ev, sizeof(ev)) != sizeof(ev))
      continue;

    // Debounce: ignora qualquer evento a menos de 30 ms do anterior.
    // Assim o quique da pressão e o da soltura não geram cliques falsos.
    bool estavel = (ev.timestamp_ns - ultimo_evento_ns) > DEBOUNCE_NS;
    ultimo_evento_ns = ev.timestamp_ns;

    if (estavel && ev.id == GPIO_V2_LINE_EVENT_FALLING_EDGE)
      return true;                        // clique = borda de descida estável
  }
}