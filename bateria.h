#ifndef BATERIA_H
#define BATERIA_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "tipos.h"  // Já inclui BAT_W, BAT_H e NivelDificuldade

// Estrutura da Bateria
typedef struct {
    int id;
    Posicao pos;
    bool ativa;
    int foguetes_atual;
    int foguetes_max;
    NivelDificuldade nivel;
    bool conectada;  // Se está conectada ao recarregador
    bool na_ponte;   // Se está atravessando a ponte
    SDL_Texture* texture;
} Bateria;

// Funções da Bateria
bool carregar_bateria(SDL_Renderer* renderer, Bateria* bat, const char* caminho_img);
void liberar_bateria(Bateria* bat);
void inicializar_bateria(Bateria* bat, NivelDificuldade nivel);
void desenhar_bateria(SDL_Renderer* renderer, Bateria* bat);
void* thread_bateria(void* arg);

bool detectar_colisao_bateria_recarregador(Bateria* bat, Posicao rec_pos, int rec_w, int rec_h);

#endif // BATERIA_H 