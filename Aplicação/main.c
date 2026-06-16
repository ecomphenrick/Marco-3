#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "api_driver_c.h"

// =============================================
// DEFINIÇÕES VGA
// =============================================
#define PIO_VGA_IN_OFFSET  0x00  // pio_vga_in  → done
#define PIO_VGA_OUT_OFFSET 0x10  // pio_vga_out → enable/reset/dados

#define BOX_WIDTH    224
#define BOX_HEIGHT   224
#define BOX_X_START  48
#define BOX_Y_START  8
#define BOX_X_END    (BOX_X_START + BOX_WIDTH)
#define BOX_Y_END    (BOX_Y_START + BOX_HEIGHT)

// Tamanhos dos buffers
#define BIAS_SIZE     256
#define BETA_SIZE     2560
#define PESOS_SIZE    200704
#define IMAGEM_SIZE   784
#define PESOS_COUNT   100352
#define TOTAL_IMAGENS 1000

typedef struct {
    uint8_t r, g, b;
} Pixel;

static Pixel canvas[VGA_WIDTH][VGA_HEIGHT];
void* endereco_global = NULL;

// Buffers globais
static uint8_t buf_bias[BIAS_SIZE];
static uint8_t buf_beta[BETA_SIZE];
static uint8_t buf_pesos[PESOS_SIZE];
static uint8_t buf_imagem[IMAGEM_SIZE];

// =============================================
// HANDLER PARA CTRL+C
// =============================================
void handler(int sig) {
    if (endereco_global) munmap(endereco_global, 0x1000);
    exit(0);
}

// =============================================
// FUNÇÕES VGA
// =============================================
void vga_reset(void* base_virtual) {
    volatile uint32_t* vga_out = (volatile uint32_t*)((uint8_t*)base_virtual + PIO_VGA_OUT_OFFSET);
    *vga_out = (1 << 27);
    usleep(1000);
    *vga_out = 0;
}

void vga_draw_pixel(void* base_virtual, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    volatile uint32_t* vga_in  = (volatile uint32_t*)((uint8_t*)base_virtual + PIO_VGA_IN_OFFSET);
    volatile uint32_t* vga_out = (volatile uint32_t*)((uint8_t*)base_virtual + PIO_VGA_OUT_OFFSET);

    uint32_t command = 0;
    command |= (x & 0x1FF);
    command |= ((y & 0xFF) << 9);
    command |= ((r & 0x07) << 17);
    command |= ((g & 0x07) << 20);
    command |= ((b & 0x07) << 23);

    *vga_out = command | (1 << 26);
    for (volatile int i = 0; i < 5; i++);
    while ((*vga_in & 0x01) == 0);
    *vga_out = command;
}

void vga_clear_screen(void* base_virtual, uint8_t r, uint8_t g, uint8_t b) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            vga_draw_pixel(base_virtual, x, y, r, g, b);
        }
    }
}

// =============================================
// BLUR
// =============================================
void aplicar_blur(uint8_t* imagem) {
    uint8_t temp[784] = {0};
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 28; x++) {
            int soma = 0, count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy, nx = x + dx;
                    if (ny >= 0 && ny < 28 && nx >= 0 && nx < 28) {
                        soma += imagem[ny * 28 + nx];
                        count++;
                    }
                }
            }
            temp[y * 28 + x] = soma / count;
        }
    }
    for (int i = 0; i < 784; i++) imagem[i] = temp[i];
}

// =============================================
// FUNÇÕES DE LEITURA DE ARQUIVO
// =============================================
int ler_arquivo(const char* caminho, uint8_t* buffer, int tamanho) {
    FILE* f = fopen(caminho, "rb");
    if (!f) {
        printf("Erro ao abrir %s\n", caminho);
        return -1;
    }
    int lidos = fread(buffer, 1, tamanho, f);
    fclose(f);
    return lidos;
}

void carregar_pesos(void* endereco) {
    reset_coprocessador(endereco);
    ler_arquivo("bins/beta_q.bin", buf_beta,  BETA_SIZE);
    ler_arquivo("bins/b_q.bin",    buf_bias,  BIAS_SIZE);
    ler_arquivo("bins/W_in_q.bin", buf_pesos, PESOS_SIZE);
    store_beta(endereco,  buf_beta);
    store_bias(endereco,  buf_bias);
    store_pesos(endereco, buf_pesos, PESOS_COUNT);
}

// =============================================
// EXIBE IMAGEM NO VGA
// =============================================
void exibe_imagem_no_vga(void* endereco, uint8_t* imagem) {
    for (int py = 0; py < 28; py++) {
        for (int px = 0; px < 28; px++) {
            uint8_t gray = imagem[py * 28 + px];
            uint8_t cor  = gray / 32;
            for (int by = 0; by < 8; by++) {
                for (int bx = 0; bx < 8; bx++) {
                    vga_draw_pixel(endereco,
                        BOX_X_START + px * 8 + bx,
                        BOX_Y_START + py * 8 + by,
                        cor, cor, cor);
                }
            }
        }
    }
}

// =============================================
// MODO 1: Inferência a partir de um arquivo
// =============================================
void modo_arquivo(void* endereco) {
    char caminho[256];

    printf("Digite o caminho do arquivo .bin: ");
    if (scanf("%255s", caminho) != 1) {
        printf("Caminho inválido.\n");
        return;
    }
    getchar();

    if (ler_arquivo(caminho, buf_imagem, IMAGEM_SIZE) < 0) return;

    vga_clear_screen(endereco, 0, 0, 0);
    exibe_imagem_no_vga(endereco, buf_imagem);

    carregar_pesos(endereco);
    store_imagem(endereco, buf_imagem);

    unsigned int resultado = comeca_infer(endereco) & 0x0F;
    printf("Dígito inferido: %d\n", resultado);
}

// =============================================
// MODO 2: Desenho com mouse
// =============================================
void modo_desenho(void* endereco) {
    printf("Inicializando interface gráfica...\n");
    vga_reset(endereco);
    usleep(100000);
    vga_clear_screen(endereco, 0, 0, 0);

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            canvas[x][y] = (Pixel){0, 0, 0};
        }
    }

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            if (x >= BOX_X_START && x < BOX_X_END && y >= BOX_Y_START && y < BOX_Y_END) {
                canvas[x][y] = (Pixel){0, 0, 0};
            } else if (x == BOX_X_START-1 || x == BOX_X_END || y == BOX_Y_START-1 || y == BOX_Y_END) {
                canvas[x][y] = (Pixel){7, 7, 0};
            } else {
                canvas[x][y] = (Pixel){1, 1, 1};
            }
            vga_draw_pixel(endereco, x, y, canvas[x][y].r, canvas[x][y].g, canvas[x][y].b);
        }
    }

    int fd = open("/dev/input/mice", O_RDONLY);
    if (fd < 0) { printf("Erro ao abrir o mouse.\n"); return; }

    int x = BOX_X_START + (BOX_WIDTH / 2);
    int y = BOX_Y_START + (BOX_HEIGHT / 2);
    int old_x = x, old_y = y;

    unsigned char mouse_data[3];
    printf("Desenhe com o botão esquerdo. Botão direito para confirmar.\n");

    while (read(fd, mouse_data, 3) == 3 && (mouse_data[0] & 0x07));

    int prev_buttons = 0;

    while(1) {
        if (read(fd, mouse_data, 3) > 0) {
            int buttons     = mouse_data[0] & 0x07;
            int left_click  = buttons & 0x01;
            int right_click = buttons & 0x02;

            for (int cy = old_y-3; cy <= old_y+3; cy++) {
                for (int cx = old_x-3; cx <= old_x+3; cx++) {
                    if (cx >= 0 && cx < VGA_WIDTH && cy >= 0 && cy < VGA_HEIGHT)
                        vga_draw_pixel(endereco, cx, cy, canvas[cx][cy].r, canvas[cx][cy].g, canvas[cx][cy].b);
                }
            }

            int dx = (signed char)mouse_data[1];
            int dy = (signed char)mouse_data[2];
            x += dx; y -= dy;

            if (x < 0) x = 0;
            if (x >= VGA_WIDTH)  x = VGA_WIDTH - 1;
            if (y < 0) y = 0;
            if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;

            if (left_click) {
                for (int by = y-4; by <= y+3; by++) {
                    for (int bx = x-4; bx <= x+3; bx++) {
                        if (bx >= BOX_X_START && bx < BOX_X_END && by >= BOX_Y_START && by < BOX_Y_END) {
                            canvas[bx][by] = (Pixel){7, 7, 7};
                            vga_draw_pixel(endereco, bx, by, 7, 7, 7);
                        }
                    }
                }
            }

            if (!(prev_buttons & 0x02) && right_click) break;

            for (int cy = y-3; cy <= y+3; cy++) {
                for (int cx = x-3; cx <= x+3; cx++) {
                    if (cx >= 0 && cx < VGA_WIDTH && cy >= 0 && cy < VGA_HEIGHT)
                        vga_draw_pixel(endereco, cx, cy, 7, 0, 0);
                }
            }

            old_x = x; old_y = y;
            prev_buttons = buttons;
        }
    }

    close(fd);

    uint8_t imagem[784];
    for (int cell_y = 0; cell_y < 28; cell_y++) {
        for (int cell_x = 0; cell_x < 28; cell_x++) {
            int pintado = 0;
            for (int by = 0; by < 8 && !pintado; by++) {
                for (int bx = 0; bx < 8 && !pintado; bx++) {
                    int px = BOX_X_START + cell_x * 8 + bx;
                    int py = BOX_Y_START + cell_y * 8 + by;
                    if (canvas[px][py].r == 7) pintado = 1;
                }
            }
            imagem[cell_y * 28 + cell_x] = pintado ? 255 : 0;
        }
    }

    aplicar_blur(imagem);

    printf("\nImagem capturada (28x28):\n");
    for (int row = 0; row < 28; row++) {
        for (int col = 0; col < 28; col++) {
            printf(imagem[row * 28 + col] > 127 ? "# " : "  ");
        }
        printf("\n");
    }

    carregar_pesos(endereco);
    store_imagem(endereco, imagem);

    unsigned int resultado = comeca_infer(endereco) & 0x0F;
    printf("Dígito reconhecido: %d\n", resultado);
}

// =============================================
// MODO 3: Benchmark (1000 imagens)
// =============================================
void modo_avaliar(void* base_virtual) {
    FILE* out = fopen("resultado.csv", "w");
    if (!out) { printf("Erro ao criar arquivo de resultado.\n"); return; }

    fprintf(out, "imagem,digito_correto,digito_inferido,acerto,latencia_ms\n");

    int total = 0, acertos = 0;
    double latencia_total = 0.0;
    double latencias[TOTAL_IMAGENS];
    char nome_arquivo[64];

    printf("Iniciando benchmark com %d imagens...\n", TOTAL_IMAGENS);

    for (int i = 0; i < TOTAL_IMAGENS; i++) {
        snprintf(nome_arquivo, sizeof(nome_arquivo), "bins/%d.bin", i);
        int digito_correto = i / 100;

        if (ler_arquivo(nome_arquivo, buf_imagem, IMAGEM_SIZE) < 0) continue;

        vga_clear_screen(base_virtual, 0, 0, 0);
        exibe_imagem_no_vga(base_virtual, buf_imagem);

        carregar_pesos(base_virtual);
        store_imagem(base_virtual, buf_imagem);

        struct timespec inicio, fim;
        clock_gettime(CLOCK_MONOTONIC, &inicio);

        unsigned int resultado = comeca_infer(base_virtual) & 0x0F;

        clock_gettime(CLOCK_MONOTONIC, &fim);

        double latencia = (fim.tv_sec - inicio.tv_sec) +
                          (fim.tv_nsec - inicio.tv_nsec) / 1e9;
        double latencia_ms = latencia * 1000.0;

        latencias[total] = latencia;
        latencia_total += latencia;
        total++;

        int acerto = (resultado == (unsigned int)digito_correto) ? 1 : 0;
        if (acerto) acertos++;

        fprintf(out, "%d,%d,%u,%d,%.3f\n", i, digito_correto, resultado, acerto, latencia_ms);
        printf("Imagem %4d | Correto: %d | Inferido: %d | %s | %.3f ms\n",
            i, digito_correto, resultado, acerto ? "OK" : "ERRO", latencia_ms);
    }

    double media = latencia_total / total;
    double variancia = 0.0;
    for (int i = 0; i < total; i++) {
        double diff = latencias[i] - media;
        variancia += diff * diff;
    }
    variancia /= total;

    double desvio_pad = variancia;
    for (int i = 0; i < 20; i++) {
        if (desvio_pad == 0) break;
        desvio_pad = (desvio_pad + variancia / desvio_pad) / 2.0;
    }

    fclose(out);

    printf("\n========================================\n");
    printf("          RELATÓRIO DE DESEMPENHO       \n");
    printf("========================================\n");
    printf("Total de imagens : %d\n", total);
    printf("Acertos          : %d\n", acertos);
    printf("Acurácia         : %.2f%%\n", ((double)acertos / total) * 100.0);
    printf("Latência média   : %.3f ms\n", media * 1000.0);
    printf("Desvio padrão    : %.3f ms\n", desvio_pad * 1000.0);
    printf("Throughput       : %.2f imagens/s\n", total / latencia_total);
    printf("Resultado salvo em resultado.csv\n");
    printf("========================================\n");
}

// =============================================
// MAIN
// =============================================
int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGINT, handler);

    printf("Mapeando memória do hardware...\n");
    void* endereco = mapeia_memoria();
    if (!endereco || endereco == (void*)-1) {
        printf("Erro no mapeamento.\n");
        return -1;
    }
    endereco_global = endereco;

    vga_reset(endereco);
    usleep(100000);
    vga_clear_screen(endereco, 0, 0, 0);

    int opcao;
    while (1) {
        printf("\n=== CLASSIFICADOR DE DÍGITOS ===\n");
        printf("1. Inferência a partir de arquivo\n");
        printf("2. Desenhar e classificar (VGA + mouse)\n");
        printf("3. Benchmark (1000 imagens)\n");
        printf("0. Sair\n");
        printf("Escolha: ");

        if (scanf("%d", &opcao) != 1) {
            while(getchar() != '\n');
            continue;
        }
        getchar();

        if (opcao == 1) {
            modo_arquivo(endereco);
        } else if (opcao == 2) {
            modo_desenho(endereco);
            vga_clear_screen(endereco, 0, 0, 0);
        } else if (opcao == 3) {
            modo_avaliar(endereco);
        } else if (opcao == 0) {
            printf("Saindo...\n");
            munmap(endereco, 0x1000);
            break;
        } else {
            printf("Opção inválida!\n");
        }
    }

    return 0;
}