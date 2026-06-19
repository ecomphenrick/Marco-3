#define _POSIX_C_SOURCE 200809L
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "driver.h"
#include "vga_driver.h" 

// =============================================
// DEFINIÇÕES DE LAYOUT
// =============================================
#define BOX_WIDTH    224
#define BOX_HEIGHT   224
#define BOX_X_START  48
#define BOX_Y_START  8
#define BOX_X_END    (BOX_X_START + BOX_WIDTH)
#define BOX_Y_END    (BOX_Y_START + BOX_HEIGHT)

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

static uint8_t buf_bias[BIAS_SIZE];
static uint8_t buf_beta[BETA_SIZE];
static uint8_t buf_pesos[PESOS_SIZE];
static uint8_t buf_imagem[IMAGEM_SIZE];

static char caminho_beta[256]  = "bins/beta_q.bin";
static char caminho_bias[256]  = "bins/b_q.bin";
static char caminho_pesos[256] = "bins/W_in_q.bin";

// =============================================
// HANDLER PARA CTRL+C
// =============================================
void handler(int sig) {
    if (endereco_global) munmap(endereco_global, 0x1000);
    exit(0);
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

int png_para_buffer(const char* caminho, uint8_t* buffer) {
    int largura, altura, canais;
    uint8_t* dados = stbi_load(caminho, &largura, &altura, &canais, 1);
    if (!dados) {
        printf("Erro ao abrir PNG: %s\n", caminho);
        return -1;
    }
    for (int i = 0; i < 784; i++) buffer[i] = dados[i];
    stbi_image_free(dados);
    return 0;
}

void carregar_pesos(void* endereco) {
    reset_coprocessador(endereco);
    if (ler_arquivo(caminho_beta, buf_beta,  BETA_SIZE) < 0) return;
    if (ler_arquivo(caminho_bias, buf_bias,  BIAS_SIZE) < 0) return;
    if (ler_arquivo(caminho_pesos, buf_pesos, PESOS_SIZE) < 0) return;
    
    store_beta(endereco,  buf_beta);
    store_bias(endereco,  buf_bias);
    store_pesos(endereco, buf_pesos, PESOS_COUNT);
}

// =============================================
// IMPRIME E SALVA AS MÉTRICAS GERAIS
// =============================================
void processar_e_salvar_metricas(int* acertos_classe, int* total_classe, int total, int acertos, double media_ms, double desvio_ms, double throughput) {
    double acuracia_geral = (total > 0) ? ((double)acertos / total) * 100.0 : 0.0;

    // 1. Exibe o Relatório de Desempenho no terminal (Mantido)
    printf("\n========================================\n");
    printf("          RELATÓRIO DE DESEMPENHO       \n");
    printf("========================================\n");
    printf("Total de imagens : %d\n", total);
    printf("Acertos          : %d\n", acertos);
    printf("Acurácia         : %.2f%%\n", acuracia_geral);
    printf("Latência média   : %.3f ms\n", media_ms);
    if (desvio_ms >= 0) {
        printf("Desvio padrão    : %.3f ms\n", desvio_ms);
    }
    printf("Throughput       : %.2f imagens/s\n", throughput);
    printf("========================================\n");

    // 2. Salva tanto o relatório geral quanto por classe no CSV
    FILE* m = fopen("metricas_gerais.csv", "w");
    if (!m) {
        printf("--------------------------------\n");
        printf("Erro ao criar arquivo de métricas gerais.\n");
        printf("--------------------------------\n");
        return;
    }

    fprintf(m, "=== METRICAS POR CLASSE ===\n");
    fprintf(m, "digito,total_amostras,acertos,acuracia_classe_%%\n");
    for (int i = 0; i < 10; i++) {
        double acc_classe = (total_classe[i] > 0) ? ((double)acertos_classe[i] / total_classe[i]) * 100.0 : 0.0;
        fprintf(m, "%d,%d,%d,%.2f\n", i, total_classe[i], acertos_classe[i], acc_classe);
    }

    fprintf(m, "\n=== RELATORIO DE DESEMPENHO GERAL ===\n");
    fprintf(m, "metrica,valor\n");
    fprintf(m, "Total de Imagens,%d\n", total);
    fprintf(m, "Acertos Totais,%d\n", acertos);
    fprintf(m, "Acuracia Geral %%,%.2f\n", acuracia_geral);
    fprintf(m, "Latencia Media (ms),%.3f\n", media_ms);
    if (desvio_ms >= 0) {
        fprintf(m, "Desvio Padrao (ms),%.3f\n", desvio_ms);
    } else {
        fprintf(m, "Desvio Padrao (ms),N/A\n");
    }
    fprintf(m, "Throughput (img/s),%.2f\n", throughput);
    

    fclose(m);
    printf("\n--------------------------------\n");
    printf("Relatório e métricas gerais salvos em 'metricas_gerais.csv' e 'resultado.csv'\n");
    printf("--------------------------------\n");
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
                    vga_draw_pixel(endereco, BOX_X_START + px * 8 + bx, BOX_Y_START + py * 8 + by, cor, cor, cor);
                }
            }
        }
    }
}

// =============================================
// MODO 1: Inferência a partir de arquivo
// =============================================
void modo_arquivo(void* endereco) {
    char caminho[256];
    printf("\n--------------------------------\n");
    printf("Digite o caminho do arquivo (.bin ou .png): ");
    printf("\n--------------------------------\n");
    if (scanf("%255s", caminho) != 1) return;
    getchar();

    char* ext = strrchr(caminho, '.');
    if (ext && strcmp(ext, ".png") == 0) {
        if (png_para_buffer(caminho, buf_imagem) < 0) return;
    } else if (ext && strcmp(ext, ".bin") == 0) {
        if (ler_arquivo(caminho, buf_imagem, IMAGEM_SIZE) < 0) return;
    } else {
        printf("\n--------------------------------\n");
        printf("Formato não suportado.\n");
        printf("--------------------------------\n");
        return;
    }

    vga_clear_screen(endereco, 0, 0, 0);
    exibe_imagem_no_vga(endereco, buf_imagem);
    carregar_pesos(endereco);
    store_imagem(endereco, buf_imagem);

    unsigned int resultado = comeca_infer(endereco) & 0x0F;
    printf("\n--------------------------------\n");
    printf("Dígito inferido: %d\n", resultado);
    printf("--------------------------------\n");
}

// =============================================
// MODO 2: Desenho com mouse
// =============================================
void modo_desenho(void* endereco) {
    vga_reset(endereco);
    usleep(100000);
    vga_clear_screen(endereco, 0, 0, 0);
    printf("\nIniciando o Modo Desenho\n");
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            if (x >= BOX_X_START && x < BOX_X_END && y >= BOX_Y_START && y < BOX_Y_END) canvas[x][y] = (Pixel){0, 0, 0};
            else if (x == BOX_X_START-1 || x == BOX_X_END || y == BOX_Y_START-1 || y == BOX_Y_END) canvas[x][y] = (Pixel){7, 7, 0};
            else canvas[x][y] = (Pixel){1, 1, 1};
            vga_draw_pixel(endereco, x, y, canvas[x][y].r, canvas[x][y].g, canvas[x][y].b);
        }
    }

    int fd = open("/dev/input/mice", O_RDONLY);
    if (fd < 0) return;

    int x = BOX_X_START + (BOX_WIDTH / 2), y = BOX_Y_START + (BOX_HEIGHT / 2);
    int old_x = x, old_y = y, prev_buttons = 0;
    unsigned char mouse_data[3];

    while (read(fd, mouse_data, 3) == 3 && (mouse_data[0] & 0x07));

    while(1) {
        if (read(fd, mouse_data, 3) > 0) {
            int buttons = mouse_data[0] & 0x07;
            int left_click = buttons & 0x01, right_click = buttons & 0x02;

            for (int cy = old_y-3; cy <= old_y+3; cy++) {
                for (int cx = old_x-3; cx <= old_x+3; cx++) {
                    if (cx >= 0 && cx < VGA_WIDTH && cy >= 0 && cy < VGA_HEIGHT)
                        vga_draw_pixel(endereco, cx, cy, canvas[cx][cy].r, canvas[cx][cy].g, canvas[cx][cy].b);
                }
            }

            x += (signed char)mouse_data[1]; y -= (signed char)mouse_data[2];
            if (x < 0) x = 0; if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
            if (y < 0) y = 0; if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;

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
                    if (cx >= 0 && cx < VGA_WIDTH && cy >= 0 && cy < VGA_HEIGHT) vga_draw_pixel(endereco, cx, cy, 7, 0, 0);
                }
            }
            old_x = x; old_y = y; prev_buttons = buttons;
        }
    }
    close(fd);

    uint8_t imagem[784];
    for (int cell_y = 0; cell_y < 28; cell_y++) {
        for (int cell_x = 0; cell_x < 28; cell_x++) {
            int pintado = 0;
            for (int by = 0; by < 8 && !pintado; by++) {
                for (int bx = 0; bx < 8 && !pintado; bx++) {
                    if (canvas[BOX_X_START + cell_x * 8 + bx][BOX_Y_START + cell_y * 8 + by].r == 7) pintado = 1;
                }
            }
            imagem[cell_y * 28 + cell_x] = pintado ? 255 : 0;
        }
    }

    aplicar_blur(imagem);
    carregar_pesos(endereco);
    store_imagem(endereco, imagem);
    printf("\n--------------------------------\n");
    printf("\nImagem capturada (28x28):\n");
    for (int row = 0; row < 28; row++) {
        for (int col = 0; col < 28; col++) {
            printf(imagem[row * 28 + col] > 127 ? "# " : "  ");
        }
        printf("\n");
    }
    printf("--------------------------------\n");
    printf("\n--------------------------------\n");
    printf("Dígito reconhecido: %d\n", comeca_infer(endereco) & 0x0F);
    printf("--------------------------------\n");
}


// =============================================
// MODO 3: Benchmark Inteligente (1000 imagens)
// =============================================
void modo_avaliar(void* base_virtual) {
    FILE* out = fopen("resultado.csv", "w");
    if (!out) {
        printf("Erro ao criar o arquivo 'resultado.csv'.\n");
        return;
    }

    fprintf(out, "imagem,digito_correto,digito_inferido,acerto,latencia_ms\n");

    int total = 0, acertos = 0;
    int acertos_classe[10] = {0}, total_classe[10] = {0};
    double latencia_total = 0.0;
    double latencias[TOTAL_IMAGENS];
    char nome_arquivo[64];
    int arquivos_nao_encontrados = 0;

    // Carga inicial dos parâmetros da rede
    if (ler_arquivo(caminho_beta, buf_beta,  BETA_SIZE) < 0) { fclose(out); return; }
    if (ler_arquivo(caminho_bias, buf_bias,  BIAS_SIZE) < 0) { fclose(out); return; }
    if (ler_arquivo(caminho_pesos, buf_pesos, PESOS_SIZE) < 0) { fclose(out); return; }
    
    store_beta(base_virtual,  buf_beta);
    store_bias(base_virtual,  buf_bias);
    
    // ATENÇÃO: Verifique se o seu hardware exige PESOS_COUNT (100352) ou PESOS_SIZE (200704)
    store_pesos(base_virtual, buf_pesos, PESOS_COUNT);

    printf("Iniciando benchmark com %d imagens...\n", TOTAL_IMAGENS);

    for (int i = 0; i < TOTAL_IMAGENS; i++) {
        reset_coprocessador(base_virtual);
        
        // Monta o nome do arquivo. Certifique-se de que existem arquivos de 0.bin a 999.bin na pasta bins/
        snprintf(nome_arquivo, sizeof(nome_arquivo), "bins/%d.bin", i);
        
        // No padrão MNIST sequencial, cada bloco de 100 imagens pertence a um dígito (0 a 9)
        int digito_correto = i / 100;

        if (ler_arquivo(nome_arquivo, buf_imagem, IMAGEM_SIZE) < 0) {
            arquivos_nao_encontrados++;
            continue; // Se o arquivo não existir, pula para a próxima iteração
        }
        
        store_imagem(base_virtual, buf_imagem);

        struct timespec inicio, fim;
        clock_gettime(CLOCK_MONOTONIC, &inicio);
        
        unsigned int resultado = comeca_infer(base_virtual) & 0x0F;
        
        clock_gettime(CLOCK_MONOTONIC, &fim);

        double latencia = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
        latencias[total] = latencia;
        latencia_total += latencia;

        total_classe[digito_correto]++;
        int acerto = (resultado == (unsigned int)digito_correto) ? 1 : 0;
        if (acerto) {
            acertos++;
            acertos_classe[digito_correto]++;
        }

        fprintf(out, "%d,%d,%u,%d,%.3f\n", i, digito_correto, resultado, acerto, latencia * 1000.0);
        // CORRIGIDO: Agora multiplicando a latência por 1000.0 para exibir corretamente em milissegundos
        printf("Imagem %4d | Correto: %d | Inferido: %d | %s | %.3f ms\n", i, digito_correto, resultado, acerto ? "OK" : "ERRO", latencia * 1000.0);
        total++;
    }
    fclose(out);

    if (arquivos_nao_encontrados > 0) {
        printf("Aviso: %d arquivos binários não foram encontrados na pasta 'bins/'.\n", arquivos_nao_encontrados);
    }

    if (total == 0) {
        printf("Erro crítico: Nenhuma imagem pôde ser carregada para o benchmark. Verifique a pasta 'bins/'.\n");
        return;
    }

    // Cálculo estatístico de desvio padrão
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

    // Exibe a acurácia por classe
    printf("\n=== ACURÁCIA INDIVIDUAL POR DÍGITO ===\n");
    for(int i = 0; i < 10; i++) {
        printf("Dígito %d -> Total: %d | Acertos: %d | Acurácia: %.2f%%\n", 
               i, total_classe[i], acertos_classe[i], 
               (total_classe[i] > 0) ? ((double)acertos_classe[i]/total_classe[i])*100.0 : 0.0);
    }

    // Grava e exibe os resultados gerais
    processar_e_salvar_metricas(acertos_classe, total_classe, total, acertos, media * 1000.0, desvio_pad * 1000.0, total / latencia_total);
}

// =============================================
// MODO 4: Benchmark com arquivo csv (Com Desvio Padrão)
// =============================================
void modo_avaliar_csv(void* base_virtual, const char* arquivo_entrada) {
    FILE* in = fopen(arquivo_entrada, "r");
    FILE* out = fopen("resultado.csv", "w");
    if (!in || !out) { if(in) fclose(in); if(out) fclose(out); return; }

    fprintf(out, "imagem,digito_correto,digito_inferido,acerto,latencia_ms\n");

    char linha[512];
    int total = 0, acertos = 0;
    int acertos_classe[10] = {0}, total_classe[10] = {0};
    double latencia_total = 0.0;
    
    double latencias[TOTAL_IMAGENS];
    int primeira_linha = 1;
    
    if (ler_arquivo(caminho_beta, buf_beta,  BETA_SIZE) < 0) { fclose(in); fclose(out); return; }
    if (ler_arquivo(caminho_bias, buf_bias,  BIAS_SIZE) < 0) { fclose(in); fclose(out); return; }
    if (ler_arquivo(caminho_pesos, buf_pesos, PESOS_SIZE) < 0) { fclose(in); fclose(out); return; }
    store_beta(base_virtual,  buf_beta);
    store_bias(base_virtual,  buf_bias);
    store_pesos(base_virtual, buf_pesos, PESOS_COUNT);

    printf("Iniciando inferência em lote (Benchmark CSV)...\n");

    while (fgets(linha, sizeof(linha), in)) {
        if (primeira_linha && strstr(linha, "digito") != NULL) { primeira_linha = 0; continue; }
        primeira_linha = 0;
        linha[strcspn(linha, "\r\n")] = 0;

        char* token_digito = strtok(linha, ",");
        char* token_caminho = strtok(NULL, ",");

        if (token_digito && token_caminho) {
            int digito_predito = atoi(token_digito);
            if (digito_predito < 0 || digito_predito > 9) continue;

            char caminho_limpo[256]; int idx = 0;
            for(int i = 0; token_caminho[i] != '\0'; i++) {
                if(token_caminho[i] != '"' && token_caminho[i] != '\'') caminho_limpo[idx++] = token_caminho[i];
            }
            caminho_limpo[idx] = '\0';
            char* ext = strrchr(caminho_limpo, '.');
            
            reset_coprocessador(base_virtual);
            if (ext && strcmp(ext, ".png") == 0) {
                if (png_para_buffer(caminho_limpo, buf_imagem) < 0) continue;
            } else if (ext && strcmp(ext, ".bin") == 0) {
                if (ler_arquivo(caminho_limpo, buf_imagem, IMAGEM_SIZE) < 0) continue;
            } else continue;

            store_imagem(base_virtual, buf_imagem);
            
            struct timespec inicio, fim;
            clock_gettime(CLOCK_MONOTONIC, &inicio);
            unsigned int resultado = comeca_infer(base_virtual) & 0x0F;
            clock_gettime(CLOCK_MONOTONIC, &fim);

            double latencia = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
            
            if (total < TOTAL_IMAGENS) {
                latencias[total] = latencia;
            }
            latencia_total += latencia;

            total_classe[digito_predito]++;
            int acerto = (resultado == (unsigned int)digito_predito) ? 1 : 0;
            if (acerto) {
                acertos++;
                acertos_classe[digito_predito]++;
            }

            fprintf(out, "%s,%d,%u,%d,%.3f\n", caminho_limpo, digito_predito, resultado, acerto, latencia * 1000.0);
            // CORRIGIDO: Multiplicando a latência por 1000.0 no printf para exibir em ms no terminal
            printf("Imagem %s | Correto: %d | Inferido: %d | %s | %.3f ms\n",
                caminho_limpo, digito_predito, resultado, acerto ? "OK" : "ERRO", latencia * 1000.0);
            total++;
        }
    }
    fclose(in); fclose(out);

    // Cálculo do desvio padrão
    double media = (total > 0) ? latencia_total / total : 0.0;
    double variancia = 0.0;
    double desvio_pad = 0.0;

    if (total > 0) {
        int limite_estatistico = (total < TOTAL_IMAGENS) ? total : TOTAL_IMAGENS;
        for (int i = 0; i < limite_estatistico; i++) {
            double diff = latencias[i] - media;
            variancia += diff * diff;
        }
        variancia /= limite_estatistico;
        
        desvio_pad = variancia;
        for (int i = 0; i < 20; i++) { 
            if (desvio_pad == 0) break; 
            desvio_pad = (desvio_pad + variancia / desvio_pad) / 2.0; 
        }
    }

    printf("\n=== ACURÁCIA INDIVIDUAL POR DÍGITO ===\n");
    for(int i = 0; i < 10; i++) {
        printf("Dígito %d -> Média de Acerto: %.2f%%\n", i, (total_classe[i] > 0) ? ((double)acertos_classe[i]/total_classe[i])*100.0 : 0.0);
    }

    processar_e_salvar_metricas(
        acertos_classe, 
        total_classe, 
        total, 
        acertos, 
        media * 1000.0, 
        desvio_pad * 1000.0, 
        (latencia_total > 0) ? total / latencia_total : 0.0
    );
}

// =============================================
// MAIN
// =============================================
int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGINT, handler);

    printf("Mapeando memória do hardware...\n");
    void* endereco = mapeia_memoria();
    if (!endereco || endereco == (void*)-1) return -1;
    endereco_global = endereco;

    vga_reset(endereco);
    usleep(100000);
    vga_clear_screen(endereco, 0, 0, 0);
    char csv_entrada[128];
    
    int opcao;
    while (1) {
        printf("\n--------------------------------\n");
        printf("Modos de Classificação\n");
        printf("--------------------------------\n");
        printf("1. Inferência a partir de arquivo\n");
        printf("2. Desenhar e classificar (VGA + mouse)\n");
        printf("3. Benchmark (1000 imagens)\n");
        printf("4. Benchmark com arquivo csv\n");
        printf("--------------------------------\n");
        printf("Modos de Alteração dos Pesos\n");
        printf("--------------------------------\n");
        printf("5. Configurar caminho do arquivo BETA (Atual: %s)\n", caminho_beta);
        printf("6. Configurar caminho do arquivo BIAS (Atual: %s)\n", caminho_bias);
        printf("7. Configurar caminho do arquivo PESOS (Atual: %s)\n", caminho_pesos);
        printf("8. Restaurar arquivos de rede padrões (bins/)\n");
        printf("0. Sair\n");
        printf("--------------------------------\n");
        printf("Escolha: ");

        if (scanf("%d", &opcao) != 1) {
            while(getchar() != '\n');
            continue;
        }
        getchar();

        if (opcao == 1) modo_arquivo(endereco);
        else if (opcao == 2) { modo_desenho(endereco); vga_clear_screen(endereco, 0, 0, 0); }
        else if (opcao == 3) modo_avaliar(endereco);
        else if (opcao == 4) {
            printf("\nDigite o caminho do CSV de entrada (ex: caminhos.csv): ");
            if (fgets(csv_entrada, sizeof(csv_entrada), stdin) != NULL) csv_entrada[strcspn(csv_entrada, "\r\n")] = 0;
            modo_avaliar_csv(endereco, csv_entrada);
        } else if (opcao == 5) {
            printf("Novo caminho BETA (.bin): "); if (scanf("%255s", caminho_beta) == 1) getchar();
        } else if (opcao == 6) {
            printf("Novo caminho BIAS (.bin): "); if (scanf("%255s", caminho_bias) == 1) getchar();
        } else if (opcao == 7) {
            printf("Novo caminho PESOS (.bin): "); if (scanf("%255s", caminho_pesos) == 1) getchar();
        } else if (opcao == 8) {
            strcpy(caminho_beta, "bins/beta_q.bin"); strcpy(caminho_bias, "bins/b_q.bin"); strcpy(caminho_pesos, "bins/W_in_q.bin");
            printf("Padrões restaurados!\n");
        } else if (opcao == 0) {
            printf("Saindo....\n");
            munmap(endereco, 0x1000);
            break;
        }
    }
    return 0;
}
