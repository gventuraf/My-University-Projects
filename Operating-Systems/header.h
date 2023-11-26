/****************************************************
*                                                   *
*			Operating Systems Project               *
*                                                   *
*			Submission Date: 08/12/2018             *
*             										*
*                                                   *
*****************************************************/

#ifndef MAIN_HEADER
#define MAIN_HEADER


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/msg.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>

#define ON 1
#define OFF 0
#define ID_SHM_ARM 1
#define ID_SHM_EST 2
#define ID_SHM_STOCK 3
#define ID_SHM_NOMES_PRODUTOS 4
#define MAX_CHARS 150
#define INT_TO_CHAR_MAX 12
#define DESOCUPADO 0
#define OCUPADO 1
#define KEYS_FILE ".keys.txt"
#define LOG_FILE "log.txt"
#define PIPE_NAME "named_pipe"
#define NUM_BASES 4
#define SEMLOG "/sem_log"
#define SEMEST "/sem_est"
#define SEMSTOCK "/sem_stock"
#define SEMTERM "/sem_term"
#define MQID 5
#define MSGMAX 100
#define ABASTECIMENTO "abastecimento"
#define DRONE "drone"
#define MSG_DRONE 14
#define SET_DRONES 1
#define ORDER 2
#define INV_SYNTAX 3
#define FALTA_STOCK 1
#define WAIT_TIME 2
#define CDRONE 1
#define CORDER 2
#define true 1
#define false 0
#define OK 1

#define DISTANCE 1

typedef struct sNomeProduto {
	char nome[MAX_CHARS];
	struct sNomeProduto *next;
} NomeProduto;

typedef struct sProduto_Qtd {
	char nome[MAX_CHARS];
	int qtd;
	struct sProduto_Qtd *next;
} Produto_Qtd;

typedef struct sArmazem {
	char nome_armazem[MAX_CHARS];
	int num_produtos;
	unsigned int x, y, id;
	Produto_Qtd *produtos;
	struct sArmazem *next;
} Armazem;

typedef struct sEstatistica {
	unsigned int total_encomendas, total_produtos_carregados;
	unsigned int total_encomendas_entregues, total_produtos_entregues;
	double tempo_medio;
} Estatistica;

typedef struct sStock {
	char nome_produto[MAX_CHARS];
	unsigned int id_armazem, qtd;
} Stock;

typedef struct sEncomenda {
	long id;
	int id_arm;
	char nome_produto[MAX_CHARS], nome_encomenda[MAX_CHARS];
	unsigned int qtd, x_final, y_final, x_arm, y_arm;
} Encomenda;

struct lista_encomendas {
	Encomenda *encomenda;
	struct lista_encomendas *next;
};

typedef struct sDrone {
	double x, y;
	int estado, estou_numa_base;
	unsigned int id;
	Encomenda *encomenda;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_t tid;
	Estatistica *shm_est;
	struct sDrone *next;
} Drone;

typedef struct sConfig {
	int x, y, num_drones, t_abast, qtd_abast, uni_tempo, num_armazens, qtd_produtos;
	NomeProduto *nomes_produtos;
	Armazem *armazens;
} Config;

typedef struct sPonteiros{
	Config *config;
	Armazem *shm_addr_arm;
	Estatistica *shm_addr_est;
	Stock *shm_addr_stock;
	NomeProduto *shm_addr_nomes_produtos;
	pid_t *armazens, central;
} Ponteiros;

struct ponteiros_central{
	Armazem *shm_arm;
	Estatistica *shm_est;
	Stock *shm_stock;
	NomeProduto *shm_np;
	Drone *drones;
	int fd_pipe;
	struct lista_encomendas *encomendas_suspensas;
};

typedef struct sMsgBuffer{
	long mtype;
	char text[MSGMAX];
} MsgBuffer;



int create_file(char *nome);
int read_config(char *nome_ficheiro, Config *config);
void *create_shm_armazens(int num_armazens);
void set_shm_arm(Armazem *addr, Armazem *armazens);
void *create_shm_estatistica();
void set_shm_est(Estatistica *);
void *create_shm_stock(int n);
void set_shm_stock(Stock *addr, Armazem *armazens);
void copia_armazem(Armazem *para, Armazem *de);
int soma_produtos_total(Armazem *p);
void partial_clear_config(Config *c);
void set_shm_nomes_produtos(NomeProduto *shm, Config *config);
void *create_shm_nomes_produtos(int n);
void sigint_handler (int ignore);
void sigint_handler_sim(int signum);
void *terminacao_controlada(void *arg);
void setup_thread(Ponteiros *ponteiros, Config *config, Armazem *a, Estatistica *e, Stock *s, NomeProduto *n, pid_t *, pid_t);
void log_ecra(char *s);
int cria_msg_queue();
MsgBuffer *cria_msg_abastecimento(int id_arm, char *prod, int qtd_abast);
char *escolhe_produto(int id_arm, int seed, Armazem *ptr);
int notin(char *nome_prod, NomeProduto *prod);

void *drone_start(void *drone_info);
int drones_id_init(pthread_t *ids, Drone *info, int n);
void drones_info_init(Drone *a, pthread_mutex_t *mutexes,
							pthread_cond_t *conds, int n);
int cond_init(pthread_cond_t *a, int n);
int mutex_init(pthread_mutex_t *a, int n);
void *attach_shm(int type);
void ler_inteiros(char *str, int *d, int *t, int*);
MsgBuffer *cria_msg_drone(int id_arm, long id_enc, int qtd_abast);
void envia_mensagem(int queue_id, MsgBuffer *msg);
void base_mais_proxima(int x_drone, int y_drone, int *xb, int *yb);
int acrescenta_drones(Drone**, int, Estatistica*);
void retira_drones(Drone**, int);
double get_double_from_time(int seconds);
void get_command(int fd, char *buffer);
void escreve_log(char *s);
int add_enc_suspensa(struct lista_encomendas **lista, Encomenda *enc);
int set_drones(char *command, Drone **drones, int *num_drones, Estatistica *shm_est);
int avalia_comando(char *command, NomeProduto *produtos);
int is_order_valid(char *command, NomeProduto *produtos);
int is_drone_set_valid(char *command);
double get_distancia_percurso(double xi, double yi, unsigned int xm, unsigned int ym, unsigned int xf, unsigned int yf);
void *sigint_handler_central(void *arg);
void setup_thread_term_central(struct ponteiros_central *p, Armazem *arm, Estatistica *est, Stock *stock, NomeProduto *np, Drone *drones, int fd, struct lista_encomendas *le);
int try_resend_enc(struct lista_encomendas *enc, Stock *stock, Armazem *armazens, Estatistica *shm_est, Drone *drones);


int take_care_of_drone(MsgBuffer*, int, int);
void take_care_of_abast(MsgBuffer*, Stock *);
int avalia_msg(MsgBuffer *);
void atualiza_shm_est(Estatistica *, int);
void terminar_armazem(Stock *shm_stock, Estatistica *shm_est, sem_t *sem_term);
void nova_encomenda(char *command, Stock *stock, Armazem *armazens, Estatistica *shm_est, Drone *drones, struct lista_encomendas **suspensas);

int get_msgQid();

void sigusr1_handler(int n);
void *rotina_thread_est(void *arg);

int get_random_int();

double distance(double x1, double y1, double x2, double y2);
int move_towards(double *drone_x, double *drone_y, double target_x, double target_y);

#endif
