//------------------------------------------
// DEFINIÇÃO DAS THREADS E ESTRATÉGIAS DE SINCRONIZAÇÃO
//------------------------------------------

Definição das Threads e Estratégias de Sincronização e Exclusão Mútua

1. Threads Necessárias

Thread
Responsabilidade
Thread principal (main)
Inicializa o jogo, cria as demais threads, trata a entrada do jogador e coordena a lógica principal do jogo.
Thread do helicóptero
Controla o movimento com base nas teclas pressionadas. Verifica colisões com obstáculos ou foguetes.
Thread da bateria 0
Controla disparos, movimentação até o depósito, espera pela recarga e retorno. Coordena-se com o recarregador e a ponte.
Thread da bateria 1
Idem para a bateria 1.
Thread do recarregador
Responsável por recarregar as baterias solicitantes, uma por vez, respeitando a ordem de chegada e o tempo aleatório de recarga (entre 0.1s e 0.5s).
Thread de foguete (1 por foguete)
Cada foguete disparado por uma bateria é representado por uma thread que se encerra ao colidir com algo ou sair da tela.
Thread de renderização(opcional)
Pode ser usada para centralizar o desenho da tela. Caso não seja usada, todas as threads que desenham devem usar um mutex para renderização segura.

2. Estratégias de Sincronização e Exclusão Mútua

Recurso/Evento
Mecanismo de Sincronização
Depósito (recarregamento)
pthread_mutex_t mutex_deposito para garantir que apenas uma bateria esteja sendo recarregada por vez.
Coordenação com recarregador
Uso de pthread_cond_t cond_recarga para suspender a thread da bateria até o recarregador estar disponível.
Ponte
pthread_mutex_t mutex_ponte para garantir que somente uma bateria use a ponte (ida ou volta) de cada vez.
Controle de foguetes
Cada foguete é uma thread isolada. O acesso ao buffer de foguetes (caso exista) pode ser protegido com mutex_foguetes.
Renderização da tela
Todas as chamadas de desenho na tela (ex: gotoxy, printf) devem ser protegidas por pthread_mutex_t mutex_render para evitar corrupção.
Teclado/Entrada
Leitura centralizada na thread principal ou protegida por mutex se acessada em paralelo.

3. Justificativa

Concorrência controlada: Permite que baterias, helicóptero e foguetes operem simultaneamente sem interferência indevida.
Evita condições de corrida e deadlocks: O uso disciplinado de mutexes e variáveis de condição assegura a integridade dos recursos.
Isolamento de responsabilidades: Cada thread tem uma função clara no sistema.
Desempenho e modularidade: Estrutura escalável para testes e manutenção.
Renderização segura: Protege a tela de artefatos visuais indesejados em um ambiente multithread.
