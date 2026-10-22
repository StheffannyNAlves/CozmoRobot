#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "eyes.h"
#include "botao.h"

#define LIMITE_MOVIMENTO_X 15
#define LIMITE_MOVIMENTO_Y 10
#define PIN_BOTAO          17   /* GPIO BCM 17 = pino físico 11 */

void olhos_atraso_ms(unsigned ms)
{
    struct timespec req = { .tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000L };
    struct timespec rem;

    while (nanosleep(&req, &rem) == -1 && errno == EINTR)
    {
        req = rem;
    }
}

static volatile sig_atomic_t rodando = 1;
static void ao_sair(int s) { (void)s; rodando = 0; }

static void instalar_sinais(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = ao_sair;
    sigemptyset(&sa.sa_mask);
    /* sem SA_RESTART: o poll() do botão é interrompido e o laço vê rodando == 0 */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(void)
{
    instalar_sinais();

    ssd1306_init();
    botao_init(PIN_BOTAO);
    srand((unsigned)time(NULL));

    int pos_x = POS_X_BASE;
    int pos_y = POS_Y_BASE;
    emocao_t emocao = NORMAL;

    desenhar_olhos(pos_x, pos_y, OLHO_ALTURA, emocao);

    while (rodando)
    {
        if (botao_esperar(1000 + (rand() % 2000)))
        {
            emocao = proxima_emocao(emocao);
        }

        piscar(pos_x, pos_y, emocao);

        if (rand() % 3 == 0)
        {
            int alvo_x = POS_X_BASE + (rand() % (2 * LIMITE_MOVIMENTO_X)) - LIMITE_MOVIMENTO_X;
            int alvo_y = POS_Y_BASE + (rand() % (2 * LIMITE_MOVIMENTO_Y)) - LIMITE_MOVIMENTO_Y;
            mover_para(pos_x, pos_y, alvo_x, alvo_y, emocao);
            pos_x = alvo_x;
            pos_y = alvo_y;
        }
    }

    ssd1306_limpar();
    ssd1306_atualizar();
    return 0;
}