#ifndef BATERIA_H
#define BATERIA_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "tipos.h"  // Já inclui BAT_W, BAT_H e NivelDificuldade

#define MAX_FOGUETES 20  // Máximo de foguetes por bateria

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
    bool recarregando;  // Se está sendo recarregada
    int tempo_recarga_atual;  // Tempo atual de recarga
    int tempo_recarga_total;  // Tempo total necessário para recarga
    bool voltando_para_area_original;  // Se está voltando para sua área original
    SDL_Texture* texture;
    int velocidade;  // Velocidade de movimento
    int direcao;     // 1 para direita, -1 para esquerda
    int tempo_disparo_personalizado;  // Tempo de disparo personalizado para cada bateria
    Foguete foguetes[MAX_FOGUETES];  // Array de foguetes
    int tempo_ultimo_disparo;  // Controle de frequência de disparo
} Bateria;

// Funções da Bateria
bool carregar_bateria(SDL_Renderer* renderer, Bateria* bat, const char* caminho_img);
void liberar_bateria(Bateria* bat);
void inicializar_bateria(Bateria* bat, NivelDificuldade nivel);
void desenhar_bateria(SDL_Renderer* renderer, Bateria* bat);
void mover_bateria(Bateria* bat, int largura_tela);
void disparar_foguete(Bateria* bat);
void mover_foguetes(Bateria* bat, int altura_tela);
void desenhar_foguetes(SDL_Renderer* renderer, Bateria* bat);
bool verificar_colisao_foguete_helicoptero(Foguete* foguete, Posicao heli_pos, int heli_w, int heli_h);
void* thread_bateria(void* arg);

bool detectar_colisao_bateria_recarregador(Bateria* bat, Posicao rec_pos, int rec_w, int rec_h);

#endif // BATERIA_H 