/****************************************************
*                                                   *
*			Operating Systems Project               *
*                                                   *
*			Submission Date: 08/12/2018             *
*             										*
*                                                   *
*****************************************************/

#include "header.h"

pthread_mutex_t mutex, mutex_su1;
pthread_cond_t cond, cond_su1;
pthread_t tid, tid_est;
int flag, show_est;
sem_t *sem_log, *sem_est, *sem_term, *sem_stock;

int main()
{
	Config config;
	pid_t pid, *pid_armazens, pid_central;
	Armazem *shm_addr_arm;
	Estatistica *shm_addr_est;
	Stock *shm_addr_stock;
	NomeProduto *shm_addr_nomes_produtos;
	char buffer[MAX_CHARS];
	int i;
	sigset_t set;
	Ponteiros ponteiros;
	int msgQ_id, sleep_time, armazem_a_abastecer, seed;
	MsgBuffer *tmp_msg;


	/*Signal Handling - Cria a máscara*/
	sigfillset (&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	/*	Initialize mutex and cond	for the thread that terminates the prgram and the other (statistics)	*/
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	pthread_mutex_init(&mutex_su1, NULL);
	pthread_cond_init(&cond_su1, NULL);

	/*Initialize POSIX semaphores*/
	sem_log = sem_open(SEMLOG, O_CREAT, 0600, 1);
	sem_est = sem_open(SEMEST, O_CREAT, 0600, 1);
	sem_term = sem_open(SEMTERM, O_CREAT, 0600, 1);
	sem_stock = sem_open(SEMSTOCK, O_CREAT, 0600, 1);

	sem_close(sem_term);

	flag = show_est = OFF;

	/*Cria ficheiro keys.txt e log.txt	*/
	if (create_file(KEYS_FILE))
	{
		perror("Erro");
		return -1;
	}
	if (create_file(LOG_FILE))
	{
		perror("Erro");
		return -1;
	}

	/*	escreve para o log e ecrã o início do programa	*/
	log_ecra("Inicio do programa");

	/*Lê config.txt*/
	if (read_config("config.txt", &config) == -1)
		return -1;

	/*		Alocação da SHM para os armazens*/
	shm_addr_arm = (Armazem *)create_shm_armazens(config.num_armazens);
	if (shm_addr_arm == (Armazem *)((void *)(-1)))
	{
		printf("Erro a criar memória partilhada\n");
		return -1;
	}
	/*	Alocação da SHM para as estatísticas, estrutura Estatistica	*/
	shm_addr_est = (Estatistica *)create_shm_estatistica();
	if (shm_addr_est == (Estatistica *)((void *)(-1)))
	{
		printf("Erro a criar memória partilhada\n");
		return -1;
	}
	/*	Alocação da shm para o stock de todos os armazens*/
	shm_addr_stock = (Stock *)create_shm_stock(soma_produtos_total(config.armazens));
	if (shm_addr_stock == (Stock *)((void *)(-1)))
	{
		printf("Erro a criar memória partilhada\n");
		return -1;
	}
	/*	Alocação da shm para os nomes dos produtos	*/
	shm_addr_nomes_produtos = (NomeProduto *)create_shm_nomes_produtos(config.qtd_produtos);
	if (shm_addr_nomes_produtos == (NomeProduto *)((void *)(-1)))
	{
		printf("Erro a criar memória partilhada\n");
		return -1;
	}
	/*	Inicializar a shm das estatísticas	*/
	set_shm_est(shm_addr_est);

	/*	Inicializar a shm dos armazéns	*/
	set_shm_arm(shm_addr_arm, config.armazens);

	/*	Inicializar shm dos stocks */
	set_shm_stock(shm_addr_stock, config.armazens);

	/*	Inicializar shm dos nomes dos produtos	*/
	set_shm_nomes_produtos(shm_addr_nomes_produtos, &config);


	/*	SETUP DA SYSTEM V MESSAGE QUEUE	*/
	if ( (msgQ_id = cria_msg_queue()) == -1)
	{
		printf("Error: msgQueue\n");
		return -1;
	}


	/*	Criar processos armazens	*/
	pid_armazens = (pid_t *)malloc(config.num_armazens * sizeof(pid_t));
	for (i=1; i<=config.num_armazens; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			sprintf(buffer, "%d, %d", i, config.uni_tempo);
			execl("armazem.out", "armazem.out", buffer, (char *)NULL);
		}
		if (pid < 0)
		{
			printf("Erro em fork do armazem #%d\n", i);
			return -1;
		}

		pid_armazens[i] = pid;
		sprintf(buffer, "Criacao do armazem #%d, PID [%d]", i, pid);
		log_ecra(buffer);
	}

	/*	Cria processo Central	*/
	pid = fork();
	if (pid == 0)
	{
		sprintf(buffer, "%d %d %d %d %d", config.num_drones, config.uni_tempo, config.x, config.y, config.t_abast);
		execl("central.out", "central.out", buffer, (char *)NULL);
	}
	else if (pid < 0)
	{
		printf("Erro em fork da central\n");
		return -1;
	}
	else{
		pid_central = pid;
		//sprintf(buffer, "Criacao da central PID [%d]", pid);
		//log_ecra(buffer);	NÃO É PEDIDO
	}

	/*	Setup para a thread responsável pelo cleanup após SIGINT	*/
	setup_thread(&ponteiros, &config, shm_addr_arm, shm_addr_est, shm_addr_stock, shm_addr_nomes_produtos, pid_armazens, pid_central);

	if (pthread_create(&tid, NULL, terminacao_controlada, (void *)&ponteiros))
	{
		perror("Erro");
		return -1;
	}

	/*	Setup para a thread responsável pelas estatisticas após SIGUSR1	*/
	if (pthread_create(&tid_est, NULL, rotina_thread_est, (void *)shm_addr_est))
	{
		perror("Erro");
		return -1;
	}

	/*	Limpa a lista ligada nomes dos produtos, deixa as variáveis, para não ocupar memória desnecessariamente	*/
	partial_clear_config(&config);

	/* SIGNAL HANDLING	*/
	sigdelset (&set, SIGINT);
	sigdelset (&set, SIGUSR1);
	sigprocmask (SIG_SETMASK, &set, NULL);
	/*	Apanha o SIGINT e SIGUSR1	*/
	signal(SIGINT, sigint_handler_sim); /*	CTRL+C handler	*/
	signal(SIGUSR1, sigusr1_handler);	/*	SIGUSR1 handler	*/

	/*Vai abastecendo os armazens*/

	armazem_a_abastecer = 1;
	while(1)
	{
		seed = get_random_int();
		sleep_time = config.t_abast;

		/*Proteger sleep() contra interrupções de sinais*/
		do {
			sleep_time = sleep( (int) round(((double)sleep_time/(double)config.uni_tempo) ) );
		} while(sleep_time);

		tmp_msg = cria_msg_abastecimento(armazem_a_abastecer,
													escolhe_produto(armazem_a_abastecer, seed, config.armazens),
													config.qtd_abast
												);

		envia_mensagem(msgQ_id, tmp_msg);

		armazem_a_abastecer = (armazem_a_abastecer+1)%(config.num_armazens+1);
		if (armazem_a_abastecer == 0)
			++armazem_a_abastecer;
	}

	return 0;
}

void sigusr1_handler(int n)
{
	signal (SIGUSR1, SIG_IGN);

	show_est = ON;
	pthread_mutex_lock(&mutex_su1);
	pthread_cond_signal(&cond_su1);
	pthread_mutex_unlock(&mutex_su1);

	signal (SIGUSR1, sigusr1_handler);

	return;
}

void *rotina_thread_est(void *arg)
{
	Estatistica *shm_addr_est = (Estatistica *)arg;

	/*	Set that thread can get canceled at any time	*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	while (1)
	{
		pthread_mutex_lock(&mutex_su1);
		while (show_est == OFF)
			pthread_cond_wait(&cond_su1, &mutex_su1);

		/*	Imprime estatisticas para o ecrã	*/

		sem_wait(sem_est);
		sem_wait(sem_log);

		printf("\nA mostrar estatisticas: \n\n");
		printf("Numero total de encomendas: %u\n", shm_addr_est->total_encomendas);
		printf("Numero total de produtos carregados: %u\n", shm_addr_est->total_produtos_carregados);
		printf("Numero total de encomendas entregues: %u\n", shm_addr_est->total_encomendas_entregues);
		printf("Numero total de produtos entregues: %u\n", shm_addr_est->total_produtos_entregues);
		printf("Tempo de entrega medio de uma encomenda: %.2f\n\n", shm_addr_est->tempo_medio);

		sem_post(sem_log);
		sem_post(sem_est);

		show_est = OFF;

		pthread_mutex_unlock(&mutex_su1);
	}
}

char *escolhe_produto(int id_arm, int seed, Armazem *ptr)
{
	int n, i;
	Produto_Qtd *tmp;

	while(ptr->id != id_arm)
		ptr = ptr->next;

	srandom(seed);
	n = ((int)random()) % ptr->num_produtos;
	/*TESTAR SE ISTO DÁ BEM*/

	tmp = ptr->produtos;
	for (i=0; i<n; ++i)
		tmp = tmp->next;

	return tmp->nome;
}


MsgBuffer *cria_msg_abastecimento(int id_arm, char *prod, int qtd_abast)
{
	MsgBuffer *msgS;
	msgS = (MsgBuffer *)malloc(sizeof(MsgBuffer));

	msgS->mtype = id_arm;
	sprintf(msgS->text, "%s %s %d", ABASTECIMENTO, prod, qtd_abast);

	return msgS;
}



void envia_mensagem(int queue_id, MsgBuffer *msg)
{
	int status;

	do
	{
		status = msgsnd(queue_id, (void *)msg, (sizeof(*msg)-sizeof(long)), 0);
	} while(status);
}



int cria_msg_queue()
{
	key_t key;

	if ((key = ftok(KEYS_FILE, MQID)) == -1)
		return -1;

	return msgget(key, IPC_CREAT|0600);

}

int get_random_int()
{
	time_t seconds;
	struct tm curr_time;
	seconds = time(NULL);
	curr_time = *localtime(&seconds);
	return curr_time.tm_sec;
}

void log_ecra(char *s)
{
	FILE *fp;
	time_t seconds;
	struct tm curr_time;
	int status;

	/*	FICA COM SEMÁFORO; verifica se sem_wait() não foi interrompido por um sinal*/
	do
	{
		status = sem_wait(sem_log);
	} while(status == -1);

	fp = fopen(LOG_FILE, "a");

	seconds = time(NULL);
	curr_time = *localtime(&seconds);

	fprintf(fp, "%02d:%02d:%02d %s\n", curr_time.tm_hour, curr_time.tm_min, curr_time.tm_sec, s);

	fclose(fp);

	printf("%02d:%02d:%02d %s\n", curr_time.tm_hour, curr_time.tm_min, curr_time.tm_sec, s);

	sem_post(sem_log);

	return;
}


void setup_thread(Ponteiros *ponteiros, Config *config, Armazem *a, Estatistica *e, Stock *s, NomeProduto *n, pid_t *arm, pid_t central)
{
	ponteiros->config = config;
	ponteiros->shm_addr_arm = a;
	ponteiros->shm_addr_est = e;
	ponteiros->shm_addr_stock = s;
	ponteiros->shm_addr_nomes_produtos = n;
	ponteiros->armazens = arm;
	ponteiros->central = central;
}

void *terminacao_controlada(void *arg)
{
	int i, id;
	Ponteiros *ponteiros;
	key_t key;
	char buffer[MAX_CHARS];

	ponteiros = (Ponteiros *)arg;

	pthread_mutex_lock(&mutex);

	while(flag == OFF)
		pthread_cond_wait(&cond, &mutex);

	/*	Destruir thread das estatísticas	*/
	pthread_cancel(tid_est);
	pthread_join(tid_est, NULL);

	/*Destruir mutex e cond_var*/
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex_su1);
	pthread_cond_destroy(&cond_su1);

	/*	Espera por Central	*/
	waitpid(ponteiros->central, NULL, 0);

	/*	Por esta altura a Central já disse aos armazéns para sair, vamos esperar por eles	*/
	/*	A Central já destruiu a message queue	*/

	for (i=1; i<=ponteiros->config->num_armazens; ++i)
	{
		waitpid(*(ponteiros->armazens + i), NULL, 0);
		sprintf(buffer, "Armazem #%d terminou, PID [%d]", i, *(ponteiros->armazens + i));
		log_ecra(buffer);
	}

	/*	Destroi as shared memory	*/
	shmdt(ponteiros->shm_addr_arm);
	shmdt(ponteiros->shm_addr_est);
	shmdt(ponteiros->shm_addr_stock);
	shmdt(ponteiros->shm_addr_nomes_produtos);


	for (i=ID_SHM_ARM; i<=ID_SHM_NOMES_PRODUTOS; ++i)
	{
		key = ftok(KEYS_FILE, i);
		id = shmget(key, 0, 0);
		shmctl(id, IPC_RMID, NULL);
	}

	log_ecra("Fim do programa");

	//	DESTRUIR semáforos
	sem_close(sem_est);
	sem_close(sem_log);
	//sem_close(sem_term);
	sem_close(sem_stock);


	sem_unlink(SEMSTOCK);
	sem_unlink(SEMEST);
	sem_unlink(SEMLOG);
	sem_unlink(SEMTERM);


	/*	As listas em config já foram limpas anteriormente	*/

	exit(0);
}

void sigint_handler_sim(int signum)
{
	signal(signum, SIG_IGN); /*Ignorar dentro do handler porque pode ser acionado enquanto o handler está a processar o sinal anterior*/

	printf("\n CTRL+C pressed. Simulation is shutting down\n\n");

	/*Liga a variável de condição*/
	pthread_mutex_lock(&mutex);
	flag = ON;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
	pthread_join(tid, NULL);
}

void set_shm_nomes_produtos(NomeProduto *shm, Config *config)
{
	int i;
	NomeProduto *tmp;

	tmp = config->nomes_produtos;
	for (i=0; i<config->qtd_produtos; ++i)
	{
		strcpy(shm[i].nome, tmp->nome);
		shm[i].next = NULL;
		tmp = tmp->next;
	}

	strcpy(shm[i].nome, "\0");
	shm[i].next = NULL;
}

void *create_shm_nomes_produtos(int n)
{
	key_t key;
	int id;

	if ((key = ftok(KEYS_FILE, ID_SHM_NOMES_PRODUTOS)) == -1)
		return (void *)(-1);

	if ((id = shmget(key, (n+1)*sizeof(NomeProduto), IPC_CREAT|0600)) == -1)
		return (void *)(-1);

	return shmat(id, NULL, 0);

}


void partial_clear_config(Config *c)
/*
	Limpa as listas ligadas, deixa as variaveis
*/
{
	NomeProduto *np;

	while(c->nomes_produtos)
	{
		np = c->nomes_produtos;
		c->nomes_produtos = c->nomes_produtos->next;
		free(np);
	}
}


/*Funçao privada	*/
int soma_produtos_total(Armazem *p)
/*	Vai contar quantos tipos de produtos há em cada armazém e somar
* ex: se existem apenas dois armazens A e B e A tem 5 produtos e B tem 7, então
* a funçao vai devolver 5+7=12
*/
{
	int c;
	c = 0;
	while(p)
	{
		c += p->num_produtos;
		p = p->next;
	}

	return c;
}

void *create_shm_stock(int n)
{
	key_t key;
	int id;

	if ((key = ftok(KEYS_FILE, ID_SHM_STOCK)) == -1)
		return (void *)(-1);

	if ((id = shmget(key, (n+1)*sizeof(Stock), IPC_CREAT|0600)) == -1)
		return (void *)(-1);

	return shmat(id, NULL, 0);
}

void set_shm_stock(Stock *addr, Armazem *armazens)
{
	Produto_Qtd *tmp;
	int i, id_atual;

	i = 0;

	while (armazens)
	{
		id_atual = armazens->id;
		tmp = armazens->produtos;
		while (tmp)
		{
			addr[i].id_armazem = id_atual;
			strcpy(addr[i].nome_produto, tmp->nome);
			addr[i].qtd = tmp->qtd;

			++i;
			tmp = tmp->next;
		}
		armazens = armazens->next;
	}
	addr[i].id_armazem = 0;
}

void set_shm_arm(Armazem *shm_addr, Armazem *armazens)
{
	int i;
	i = 0;

	while(armazens)
	{
		copia_armazem(shm_addr+i, armazens);
		++i;
		armazens = armazens->next;
	}
	shm_addr[i].id = 0;

	return;
}

void copia_armazem(Armazem *para, Armazem *de)
{
	strcpy(para->nome_armazem, de->nome_armazem);
	para->x = de->x;
	para->y = de->y;
	para->num_produtos = de->num_produtos;
	para->id = de->id;
	para->produtos = NULL;
	para->next = NULL;
}


void set_shm_est(Estatistica *p)
{
	p->total_encomendas = 0;
	p->total_produtos_carregados = 0;
	p->total_encomendas_entregues = 0;
	p->total_produtos_entregues = 0;
	p->tempo_medio = 0.0;
}

void *create_shm_estatistica()
{
	key_t key;
	int shm_id;

	key = ftok(KEYS_FILE, ID_SHM_EST);
	if(key == -1)
		return (void *)(-1);

	shm_id = shmget(key, sizeof(Estatistica), IPC_CREAT|0600);
	if (shm_id == -1)
		return (void *)(-1);


	return shmat(shm_id, NULL, 0);
}


int read_config(char *nome_ficheiro, Config *config)

{
	char str_buffer[MAX_CHARS], *token;
	NomeProduto *tmp_nome_produto;
	Armazem *tmp_armazem;
	Produto_Qtd *tmp_prod_qtd;
	FILE *fp;
	int i, conta;

	config->nomes_produtos = NULL;
	config->armazens = NULL;

	if ( !(fp = fopen(nome_ficheiro, "r")))
	{
		perror("Erro");
		return -1;
	}

	/*	Ler x y	*/
	fscanf(fp, "%d, %d\n", &config->x, &config->y);
	if (config->x <= 0 || config->y <= 0)
		return -1;

	/*	Criar lista ligada com nomes dos produtos*/
	fgets(str_buffer, MAX_CHARS, fp);
	conta = 0;
	token = strtok(str_buffer, ", ");
	while (token)
	{
		++conta;
		if (!(tmp_nome_produto = (NomeProduto *)malloc(sizeof(NomeProduto))))
		{
			perror("Erro");
			return -1;
		}
		strcpy(tmp_nome_produto->nome, token);
		tmp_nome_produto->next = config->nomes_produtos;
		config->nomes_produtos = tmp_nome_produto;
		token = strtok(NULL, ", ");
	}
	config->nomes_produtos->nome[strlen(config->nomes_produtos->nome)-1] = '\0';
	config->qtd_produtos = conta;

	/*	Ler mais dados	*/
	fscanf(fp, "%d\n%d, %d, %d", &config->num_drones, &config->t_abast,
											&config->qtd_abast, &config->uni_tempo);
	if (config->num_drones <= 0 || config->t_abast <= 0 || config->qtd_abast <= 0 || config->uni_tempo <= 0)
		return -1;

	/*	Ler numero de armazéns*/
	fscanf(fp, "%d\n", &config->num_armazens);
	if (config->num_armazens <= 0)
		return -1;

	for (i=1; i<=config->num_armazens; i++)
	{
		if (!(tmp_armazem = (Armazem *)malloc(sizeof(Armazem))))
		{
			perror("Erro");
			return -1;
		}
		tmp_armazem->id = i;
		tmp_armazem->num_produtos = 0;
		tmp_armazem->produtos = NULL;
		tmp_armazem->next = config->armazens;
		config->armazens = tmp_armazem;

		fscanf(fp, "%s ", tmp_armazem->nome_armazem);
		fscanf(fp, "xy: %d, %d prod: ", &tmp_armazem->x, &tmp_armazem->y);
		if (tmp_armazem->x < 0 || tmp_armazem->x > config->x || tmp_armazem->y < 0 || tmp_armazem->y > config->y)
		{
			printf("Coordenadas de armazem #%d invalidas\n", i);
			return -1;
		}

		fgets(str_buffer, MAX_CHARS, fp);
		token = strtok(str_buffer, ", ");
		while (token)
		{
			if (!(tmp_prod_qtd = (Produto_Qtd *)malloc(sizeof(Produto_Qtd))))
			{
				perror("Erro");
				return -1;
			}
			if (notin(token, config->nomes_produtos))
			{
				token = strtok(NULL, ", ");
				token = strtok(NULL, ", ");
				free(tmp_prod_qtd);
			}
			else
			{
				strcpy(tmp_prod_qtd->nome, token);
				token = strtok(NULL, ", ");
				tmp_prod_qtd->qtd = (int) strtol(token, NULL, 10);
				tmp_prod_qtd->next = tmp_armazem->produtos;
				tmp_armazem->produtos = tmp_prod_qtd;
				token = strtok(NULL, ", ");
				++tmp_armazem->num_produtos;
			}
		}
	}
	fclose(fp);
	return 0;
}

int notin(char *nome_prod, NomeProduto *prod)
{
	NomeProduto *tmp;
	tmp = prod;
	while(tmp)
	{
		if (!strcmp(nome_prod, tmp->nome))
			return 0;
		tmp = tmp->next;
	}
	return 1;
}


void *create_shm_armazens(int num_armazens)
{
	key_t shm_key;
	int shm_id;

	if ((shm_key = ftok(KEYS_FILE, ID_SHM_ARM)) == -1)
		return (void *)(-1);

	if ((shm_id = shmget(shm_key, (num_armazens+1)*sizeof(Armazem), IPC_CREAT|0600)) == -1)
		return (void *)(-1);

	return shmat(shm_id, NULL, 0);
}


int create_file(char *nome)
{
	FILE *fp;

	if (!(fp = fopen(nome, "w")))
		return -1;

	if (fclose(fp) == EOF)
		return -1;

	return 0;
}
