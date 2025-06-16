#ifndef TIPOS_H
#define TIPOS_H

// Definições de tamanhos para entidades
#define HELI_W 60
#define HELI_H 30

#define REC_W 50
#define REC_H 60

#define BAT_W 40
#define BAT_H 50

// Enumeração para níveis de dificuldade
typedef enum {
    FACIL,
    MEDIO,
    DIFICIL
} NivelDificuldade;

// Estrutura para posição (compartilhada entre módulos)
typedef struct {
    int x;
    int y;
} Posicao;

#endif // TIPOS_H 