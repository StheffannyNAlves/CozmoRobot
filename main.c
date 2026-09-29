#include "pico/stdlib.h"
#include "pico/time.h"
#include "ssd1306.h"
#include <stdlib.h>

#define OLHO_LARGURA   35
#define OLHO_ALTURA    25
#define OLHO_ESPACO    18      
#define POS_X_BASE     ((SSD1306_WIDTH - (2 * OLHO_LARGURA + OLHO_ESPACO)) / 2)
#define POS_Y_BASE     ((SSD1306_HEIGHT - OLHO_ALTURA) / 2)

#define LIMITE_MOVIMENTO_X 15
#define LIMITE_MOVIMENTO_Y 10

typedef enum { NORMAL, BRAVO, TRISTE } emocao_t;

static void desenhar_olhos(int x, int y, int altura, emocao_t emocao)
{
    int deslocamento_y = (OLHO_ALTURA - altura) / 2;  // mantém o olho centrado ao encolher
    int y_olho = y + deslocamento_y;
    int x_olho_direito = x + OLHO_LARGURA + OLHO_ESPACO;

    ssd1306_limpar();
    ssd1306_retangulo_preenchido(x,               y_olho, OLHO_LARGURA, altura, true);
    ssd1306_retangulo_preenchido(x_olho_direito,  y_olho, OLHO_LARGURA, altura, true);

    if (emocao == BRAVO)
    {
        // Corta o canto superior interno de cada olho, como sobrancelha franzida
        ssd1306_triangulo_preenchido(
            x - 2, y_olho - 2,
            x + OLHO_LARGURA, y_olho - 2,
            x - 2, y_olho + 10,
            false
        );
        ssd1306_triangulo_preenchido(
            x_olho_direito + OLHO_LARGURA + 2, y_olho - 2,
            x_olho_direito, y_olho - 2,
            x_olho_direito + OLHO_LARGURA + 2, y_olho + 10,
            false
        );
    }
    else if (emocao == TRISTE)
    {
        // Corta o canto superior externo, como sobrancelha caída
        ssd1306_triangulo_preenchido(
            x + OLHO_LARGURA + 2, y_olho - 2,
            x, y_olho - 2,
            x + OLHO_LARGURA + 2, y_olho + 10,
            false
        );
        ssd1306_triangulo_preenchido(
            x_olho_direito - 2, y_olho - 2,
            x_olho_direito + OLHO_LARGURA, y_olho - 2,
            x_olho_direito - 2, y_olho + 10,
            false
        );
    }

    ssd1306_atualizar();
}

static void piscar(int x, int y, emocao_t emocao)
{
    // Fecha o olho
    for (int altura = OLHO_ALTURA; altura >= 2; altura -= 4)
    {
        desenhar_olhos(x, y, altura, emocao);
        sleep_ms(15);
    }
    // Abre o olho
    for (int altura = 2; altura <= OLHO_ALTURA; altura += 4)
    {
        desenhar_olhos(x, y, altura, emocao);
        sleep_ms(15);
    }
}

static void mover_para(int x_inicial, int y_inicial, int x_final, int y_final, emocao_t emocao)
{
    const int passos = 12;

    for (int i = 1; i <= passos; i++)
    {
        // Interpolação linear: calcula a posição intermediária em cada passo
        int x = x_inicial + (x_final - x_inicial) * i / passos;
        int y = y_inicial + (y_final - y_inicial) * i / passos;
        desenhar_olhos(x, y, OLHO_ALTURA, emocao);
        sleep_ms(20);
    }
}

int main(void)
{
    stdio_init_all();
    ssd1306_init();

    srand(to_ms_since_boot(get_absolute_time()));

    int pos_x_atual = POS_X_BASE;
    int pos_y_atual = POS_Y_BASE;
    emocao_t emocao_atual = NORMAL;

    desenhar_olhos(pos_x_atual, pos_y_atual, OLHO_ALTURA, emocao_atual);

    while (true)
    {
        sleep_ms(1000 + (rand() % 2000));  // tempo variável parado, olhando

        piscar(pos_x_atual, pos_y_atual, emocao_atual);

        // 1 a cada 3 ciclos, muda de posição
        if (rand() % 3 == 0)
        {
            int alvo_x = POS_X_BASE + (rand() % (2 * LIMITE_MOVIMENTO_X)) - LIMITE_MOVIMENTO_X;
            int alvo_y = POS_Y_BASE + (rand() % (2 * LIMITE_MOVIMENTO_Y)) - LIMITE_MOVIMENTO_Y;

            mover_para(pos_x_atual, pos_y_atual, alvo_x, alvo_y, emocao_atual);
            pos_x_atual = alvo_x;
            pos_y_atual = alvo_y;
        }

        // 1 a cada 4 ciclos, troca a emoção
        if (rand() % 4 == 0)
        {
            int sorteio = rand() % 3;
            emocao_atual = (sorteio == 0) ? NORMAL : (sorteio == 1) ? BRAVO : TRISTE;
            desenhar_olhos(pos_x_atual, pos_y_atual, OLHO_ALTURA, emocao_atual);
        }
    }
}