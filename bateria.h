#ifndef BATERIA_H
#define BATERIA_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "tipos.h"

#define BAT_W 40
#define BAT_H 30
#define MAX_FOGUETES 10

// Níveis de dificuldade
typedef enum {
    FACIL = 0,
    MEDIO = 1,
    DIFICIL = 2
} NivelDificuldade;

// Estrutura da bateria
typedef struct {
    Posicao pos;
    bool ativa;
    bool conectada;  // Se está conectada ao recarregador
    int foguetes_atual;
    int foguetes_max;
    SDL_Texture* texture;
} Bateria;

// Funções da bateria
bool carregar_bateria(SDL_Renderer* renderer, Bateria* bat, const char* caminho_img);
void liberar_bateria(Bateria* bat);
void desenhar_bateria(SDL_Renderer* renderer, Bateria* bat);
void inicializar_bateria(Bateria* bat, NivelDificuldade nivel);
bool detectar_colisao_bateria_recarregador(Bateria* bat, Posicao pos_rec, int rec_w, int rec_h);

#endif // BATERIA_H 