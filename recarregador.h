#ifndef RECARREGADOR_H
#define RECARREGADOR_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "tipos.h"
#include "bateria.h"

#define REC_W 80
#define REC_H 50

// Estrutura do recarregador
typedef struct {
    Posicao pos;
    bool ativo;
    bool ocupado;  // Se está recarregando uma bateria
    NivelDificuldade nivel;
    int tempo_recarga;  // Tempo em milissegundos
    int tempo_atual;    // Contador atual
    Bateria* bateria_conectada;  // Bateria sendo recarregada
    SDL_Texture* texture;
} Recarregador;

// Funções do recarregador
bool carregar_recarregador(SDL_Renderer* renderer, Recarregador* rec, const char* caminho_img);
void liberar_recarregador(Recarregador* rec);
void desenhar_recarregador(SDL_Renderer* renderer, Recarregador* rec);
void inicializar_recarregador(Recarregador* rec, NivelDificuldade nivel);
void atualizar_recarregador(Recarregador* rec);
bool conectar_bateria(Recarregador* rec, Bateria* bat);
void desconectar_bateria(Recarregador* rec);

#endif // RECARREGADOR_H 