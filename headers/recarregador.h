#ifndef RECARREGADOR_H
#define RECARREGADOR_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "headers/tipos.h"
#include "headers/bateria.h"

//------------------------------------------
// DEFINIÇÕES E ESTRUTURA DO RECARREGADOR
//------------------------------------------
// Estrutura do recarregador
typedef struct {
    Posicao pos;
    bool ativo;
    bool ocupado;  // Se está recarregando uma bateria
    int tempo_recarga;  // Tempo em milissegundos
    int tempo_atual;    // Contador atual
    void* bateria_conectada;  // Ponteiro para a bateria que está recarregando
    SDL_Texture* texture;
    Uint32 tempo_ultimo_tick; // Para controle de tempo real
} Recarregador;

//------------------------------------------
// FUNÇÕES DO RECARREGADOR (DECLARAÇÕES)
//------------------------------------------
// Funções do recarregador
bool carregar_recarregador(SDL_Renderer* renderer, Recarregador* rec, const char* caminho_img);
void liberar_recarregador(Recarregador* rec);
void desenhar_recarregador(SDL_Renderer* renderer, Recarregador* rec);
void inicializar_recarregador(Recarregador* rec, NivelDificuldade nivel);
void atualizar_recarregador(Recarregador* rec);
void conectar_bateria(Recarregador* rec, void* bateria);
void desconectar_bateria(Recarregador* rec);

#endif // RECARREGADOR_H 