#include "vga_driver.h"
#include <unistd.h>

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