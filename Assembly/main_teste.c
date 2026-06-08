#include <stdio.h>
#include <sys/mman.h>
#include "api_driver_c.h"

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Iniciando mapeamento...\n");
    
    // Chama a função em Assembly para abrir o /dev/mem
    void* endereco = mapeia_memoria();

    if (!endereco || endereco == (void*)-1){
        printf("Erro no mapeamento.\n");
        return -1;
    }

    printf("Resetando VGA...\n");
    vga_reset(endereco);

    printf("Limpando a tela (fundo azul escuro)...\n");
    vga_clear_screen(endereco, 0, 0, 2); 

    printf("Desenhando um quadrado vermelho no centro...\n");
    for(int y = 100; y < 140; y++) {
        for(int x = 140; x < 180; x++) {
            vga_draw_pixel(endereco, x, y, 7, 7, 7); 
        }
    }
    
    printf("VGA finalizado. Olhe para o monitor!\n");
    return 0;
}