#ifndef VGA_DRIVER_H
#define VGA_DRIVER_H

#include <stdint.h>

// Offsets baseados no soc_system.qsys
#define PIO_VGA_IN_OFFSET  0x00
#define PIO_VGA_OUT_OFFSET 0x10

// Dimensões do monitor
#define VGA_WIDTH  320
#define VGA_HEIGHT 240

// Assinatura das funções modulares de vídeo
void vga_reset(void* base_virtual);
void vga_draw_pixel(void* base_virtual, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
void vga_clear_screen(void* base_virtual, uint8_t r, uint8_t g, uint8_t b);

#endif // VGA_DRIVER_H