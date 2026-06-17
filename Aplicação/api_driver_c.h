#ifndef COPROCESSADOR_API_H
#define COPROCESSADOR_API_H
#include <stdint.h>

// =================================================================
//                    API DO COPROCESSADOR
// =================================================================
// Controle de mapeamento e hardware
void* mapeia_memoria(void);
void reset_coprocessador(void* base_virtual);

// Transferência de dados (agora recebem ponteiro para buffer)
void store_bias(void* base_virtual, void* buffer);
void store_beta(void* base_virtual, void* buffer);
void store_imagem(void* base_virtual, void* buffer);
void store_pesos(void* base_virtual, void* buffer, int count);

// Execução
unsigned int comeca_infer(void* base_virtual);

// =================================================================
//                       API DO MÓDULO VGA
// =================================================================
#define VGA_WIDTH  320
#define VGA_HEIGHT 240

void vga_reset(void* base_virtual);
void vga_draw_pixel(void* base_virtual, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
void vga_clear_screen(void* base_virtual, uint8_t r, uint8_t g, uint8_t b);

#endif