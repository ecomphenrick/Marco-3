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

#endif