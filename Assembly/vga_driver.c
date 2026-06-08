#include <stdint.h>
#include "api_driver_c.h"

// Offsets baseados no seu soc_system.qsys atualizado
#define PIO_VGA_IN_OFFSET  0x00
#define PIO_VGA_OUT_OFFSET 0x10

void vga_reset(void* base_virtual) {
    volatile uint32_t* vga_out = (volatile uint32_t*)((uint8_t*)base_virtual + PIO_VGA_OUT_OFFSET);
    
    // Envia 1 para o bit de reset (27) e depois 0 para liberar
    *vga_out = (1 << 27);
    *vga_out = 0;
}

void vga_draw_pixel(void* base_virtual, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    volatile uint32_t* vga_in  = (volatile uint32_t*)((uint8_t*)base_virtual + PIO_VGA_IN_OFFSET);
    volatile uint32_t* vga_out = (volatile uint32_t*)((uint8_t*)base_virtual + PIO_VGA_OUT_OFFSET);

    uint32_t command = 0;
    
    // Empacotamento dos dados
    command |= (x & 0x1FF);               // x: bits [8:0]
    command |= ((y & 0xFF) << 9);         // y: bits [16:9]
    command |= ((r & 0x07) << 17);        // R: bits [19:17]
    command |= ((g & 0x07) << 20);        // G: bits [22:20]
    command |= ((b & 0x07) << 23);        // B: bits [25:23]

    // Envia instrução com Enable em nível ALTO (bit 26)
    *vga_out = command | (1 << 26);

    // Polling do sinal done (bit 0)
    while ((*vga_in & 0x01) == 0) {
        // Aguarda
    }

    // Remove Enable
    *vga_out = command;
}

void vga_clear_screen(void* base_virtual, uint8_t r, uint8_t g, uint8_t b) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            vga_draw_pixel(base_virtual, x, y, r, g, b);
        }
    }
}