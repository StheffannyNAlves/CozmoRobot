#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <thread>

constexpr int LARGURA = 128;
constexpr int ALTURA  = 64;   
constexpr int ENDERECO = 0x3C; // verificar se esse endereço é o certo
constexpr bool BRANCO = true;
constexpr bool PRETO  = false;


// Display SSD1306 
class Oled {
public:
    Oled() {
        fd_ = open("/dev/i2c-1", O_RDWR);
        if (fd_ < 0 || ioctl(fd_, I2C_SLAVE, ENDERECO) < 0) {
            perror("i2c");
            std::exit(1);
        }
        const uint8_t init[] = {
            0xAE, 0xD5, 0x80, 0xA8, (uint8_t)(ALTURA - 1), 0xD3, 0x00, 0x40,
            0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
            0xDA, (uint8_t)(ALTURA == 64 ? 0x12 : 0x02),
            0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
        };
        for (uint8_t c : init) comando(c);
        limpar();
        atualizar();
    }

    ~Oled() {
        limpar();
        atualizar();
        comando(0xAE);
        close(fd_);
    }

    void limpar() { std::memset(buffer_, 0, sizeof(buffer_)); }

    void pixel(int x, int y, bool cor) {
        if (x < 0 || x >= LARGURA || y < 0 || y >= ALTURA) return;
        uint8_t mascara = 1 << (y % 8);
        uint8_t& b = buffer_[x + (y / 8) * LARGURA];
        if (cor) b |= mascara; else b &= ~mascara;
    }

    void linha_horizontal(int x, int y, int largura, bool cor) {
        for (int i = 0; i < largura; i++) pixel(x + i, y, cor);
    }

    void retangulo_preenchido(int x, int y, int largura, int altura, bool cor) {
        for (int j = 0; j < altura; j++) linha_horizontal(x, y + j, largura, cor);
    }

    // Retângulo com cantos arredondados, feito linha a linha
    void retangulo_arredondado_preenchido(int x, int y, int largura, int altura, int raio, bool cor) {
        raio = std::min({raio, largura / 2, altura / 2});
        for (int j = 0; j < altura; j++) {
            int dy = 0;
            if (j < raio)              dy = raio - 1 - j;
            else if (j >= altura - raio) dy = j - (altura - raio);
            // recuo horizontal do canto (círculo de raio "raio")
            int recuo = 0;
            if (dy > 0 || j < raio || j >= altura - raio) {
                float fy = dy + 0.5f;
                recuo = raio - (int)(std::sqrt((float)(raio * raio) - fy * fy) + 0.5f);
            }
            linha_horizontal(x + recuo, y + j, largura - 2 * recuo, cor);
        }
    }

    // Triângulo preenchido: testa cada pixel da caixa que envolve a forma
    void triangulo_preenchido(int x0, int y0, int x1, int y1, int x2, int y2, bool cor) {
        auto funcao_aresta = [](int ax, int ay, int bx, int by, int px, int py) {
            return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
        };
        int min_x = std::min({x0, x1, x2}), max_x = std::max({x0, x1, x2});
        int min_y = std::min({y0, y1, y2}), max_y = std::max({y0, y1, y2});
        for (int py = min_y; py <= max_y; py++) {
            for (int px = min_x; px <= max_x; px++) {
                int e0 = funcao_aresta(x0, y0, x1, y1, px, py);
                int e1 = funcao_aresta(x1, y1, x2, y2, px, py);
                int e2 = funcao_aresta(x2, y2, x0, y0, px, py);
                bool algum_negativo = e0 < 0 || e1 < 0 || e2 < 0;
                bool algum_positivo = e0 > 0 || e1 > 0 || e2 > 0;
                if (!(algum_negativo && algum_positivo)) pixel(px, py, cor);
            }
        }
    }

    // Equivale ao display.display() da Adafruit: envia o buffer pro display físico
    void atualizar() {
        comando(0x21); comando(0); comando(LARGURA - 1);
        comando(0x22); comando(0); comando(ALTURA / 8 - 1);

        uint8_t dados[LARGURA + 1];
        dados[0] = 0x40;
        for (int pagina = 0; pagina < ALTURA / 8; pagina++) {
            std::memcpy(&dados[1], &buffer_[pagina * LARGURA], LARGURA);
            if (write(fd_, dados, sizeof(dados)) < 0) perror("write");
        }
    }

private:
    void comando(uint8_t c) {
        uint8_t d[2] = {0x00, c};
        if (write(fd_, d, 2) < 0) perror("cmd");
    }

    int fd_;
    uint8_t buffer_[LARGURA * ALTURA / 8] = {};
};


Oled oled;
std::mt19937 gerador_aleatorio{std::random_device{}()};
volatile std::sig_atomic_t rodando = 1;

int sortear(int n) { return std::uniform_int_distribution<int>(0, n - 1)(gerador_aleatorio); }
void esperar_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

const int olho_esq_x = 42, olho_esq_y = 20;
const int olho_dir_x = 86, olho_dir_y = 20;
const int largura_olho = 32, altura_olho = 20, raio_olho = 6;

void normal() {
    oled.limpar();
    int k1 = sortear(5), k2 = sortear(5);
    oled.retangulo_arredondado_preenchido(olho_esq_x - largura_olho / 2, olho_esq_y - altura_olho / 2 + k1, largura_olho, altura_olho - k1, raio_olho, BRANCO);
    oled.retangulo_arredondado_preenchido(olho_dir_x - largura_olho / 2, olho_dir_y - altura_olho / 2 + k2, largura_olho, altura_olho - k2, raio_olho, BRANCO);
    oled.atualizar();
    esperar_ms(1000 * (sortear(5) + 1));
    oled.limpar();
}

void piscar() {
    oled.linha_horizontal(0, 20, LARGURA, BRANCO);
    oled.atualizar();
    esperar_ms(3);
    oled.limpar();
}

void feliz() {
    oled.retangulo_arredondado_preenchido(olho_esq_x - largura_olho / 2, olho_esq_y - altura_olho / 2, largura_olho, altura_olho, raio_olho, BRANCO);
    oled.retangulo_arredondado_preenchido(olho_dir_x - largura_olho / 2, olho_dir_y - altura_olho / 2, largura_olho, altura_olho, raio_olho, BRANCO);
    oled.retangulo_arredondado_preenchido(olho_esq_x - largura_olho / 2, olho_esq_y - 5, largura_olho, altura_olho / 2 + 5, raio_olho, PRETO);
    oled.retangulo_arredondado_preenchido(olho_dir_x - largura_olho / 2, olho_dir_y - 5, largura_olho, altura_olho / 2 + 5, raio_olho, PRETO);
    oled.atualizar();
    esperar_ms(3000);
    oled.limpar();
}

void bravo() {
    for (int i = 0; i < 6; i += 2) {
        oled.retangulo_arredondado_preenchido(olho_esq_x - largura_olho / 2, olho_esq_y - altura_olho / 2, largura_olho, altura_olho, raio_olho, BRANCO);
        oled.retangulo_arredondado_preenchido(olho_dir_x - largura_olho / 2, olho_dir_y - altura_olho / 2, largura_olho, altura_olho, raio_olho, BRANCO);
        oled.triangulo_preenchido(10 + i, 10, 118 - i, 10, 64, 20 + i, PRETO);
        oled.atualizar();
    }
    esperar_ms(1000 * (sortear(5) + 1));
    oled.limpar();
}

void triste() {
    for (int i = 0; i < 6; i += 2) {
        oled.retangulo_arredondado_preenchido(olho_esq_x - largura_olho / 2, olho_esq_y - altura_olho / 2, largura_olho, altura_olho, raio_olho, BRANCO);
        oled.retangulo_arredondado_preenchido(olho_dir_x - largura_olho / 2, olho_dir_y - altura_olho / 2, largura_olho, altura_olho, raio_olho, BRANCO);
        oled.triangulo_preenchido(10, 20 + i, 64, 10, 10, 10, PRETO);
        oled.triangulo_preenchido(120, 20 + i, 64, 10, 120, 10, PRETO);
        oled.atualizar();
    }
    esperar_ms(1000 * (sortear(5) + 1));
    oled.limpar();
}

int main() {
    std::signal(SIGINT, [](int) { rodando = 0; });

    while (rodando) {
        normal();
        piscar();
        feliz();
    }
    return 0;   // limpa e desliga o display
}