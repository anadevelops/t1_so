#ifndef TIPOS_H
#define TIPOS_H

// Definições de tamanhos para entidades
#define HELI_W 60
#define HELI_H 30

#define REC_W 50
#define REC_H 60

#define BAT_W 40
#define BAT_H 50

#define FOGUETE_W 8
#define FOGUETE_H 16
#define FOGUETE_VELOCIDADE 3

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

// Estrutura do foguete
typedef struct {
    Posicao pos;
    bool ativo;
    int direcao; // 1 para cima, -1 para baixo
    int velocidade;
} Foguete;

// Estrutura para o estado das teclas de movimento do helicóptero
typedef struct {
    bool w;
    bool a;
    bool s;
    bool d;
} TeclasMovimento;

#endif // TIPOS_H 