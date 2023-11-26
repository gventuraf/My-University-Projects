/****************************************************
*                                                   *
*			Operating Systems Project               *
*                                                   *
*			Submission Date: 08/12/2018             *
*             										*
*                                                   *
*****************************************************/


#include "header.h"

int uni_tempo, get_out, get_out, x_max, y_max;
unsigned int drones_id_counter;
long counter_num_enc;
sem_t *sem_stock, *sem_est, *sem_log, *sem_term;

pthread_mutex_t mutex_term, mutex_enc;
pthread_cond_t cond_term, cond_enc;
pthread_t tid_term;

int main(int argc, char **argv)
{
	int num_drones, t_abast, fd_pipe, count, r;

	Armazem 		*addr_shm_arm;
	Stock 		*addr_shm_stock;
	NomeProduto	*addr_shm_nomes_produtos;
	Estatistica *addr_shm_est;
	Drone drones;
	sigset_t set;
	char command[MAX_CHARS], buffer[2*MAX_CHARS];
	fd_set rfds;
	struct timeval timeout;
	struct lista_encomendas encomendas_suspensas, *ant, *atual;
	struct ponteiros_central term_struct;

	/* SIGNAL HANDLING (setup)*/
	sigfillset (&set);
	pthread_sigmask (SIG_SETMASK, &set, NULL);

	/*	Initialize variables	*/
	drones_id_counter = count = 0;
	get_out = false;
	counter_num_enc = LONG_MAX;
	get_out = OFF;

	encomendas_suspensas.next = NULL;

	pthread_mutex_init(&mutex_term, NULL);
	pthread_cond_init(&cond_term, NULL);
	pthread_mutex_init(&mutex_enc, NULL);
	pthread_cond_init(&cond_enc, NULL);

	/*	Create named pipe	*/
	if ((mkfifo(PIPE_NAME, 0600) < 0))
	{
		perror("Erro");
		return -1;
	}


	ler_inteiros(argv[1], &num_drones, &uni_tempo, &t_abast);


	/*	Attach todas as shm (armazens, stock, estatisticas, nomes_produtos)	*/

	if ((addr_shm_arm = (Armazem *)attach_shm(ID_SHM_ARM)) == (void *)(-1))
	{
		printf("Erro no attach da shm dos armazens na central\n");
		return -1;
	}
	if ((addr_shm_stock = (Stock *)attach_shm(ID_SHM_STOCK)) == (void *)(-1))
	{
		printf("Erro no attach da shm do stock na central\n");
		return -1;
	}
	if ((addr_shm_est = (Estatistica *)attach_shm(ID_SHM_EST)) == (void *)(-1))
	{
		printf("Erro no attach da shm das estatisticas na central\n");
		return -1;
	}
	if ((addr_shm_nomes_produtos = (NomeProduto *)attach_shm(ID_SHM_NOMES_PRODUTOS)) == (void *)(-1))
	{
		printf("Erro no attach da shm dos nomes dos produtos na central\n");
		return -1;
	}


	/*	Semáforos	*/
	sem_log = sem_open(SEMLOG, 0);
	sem_est = sem_open(SEMEST, 0);
	sem_stock = sem_open(SEMSTOCK, 0);
	sem_term = sem_open(SEMTERM, 0);

	/*	Criação e Inicialização da lista ligada 'drones', e começo das threads*/
	drones.next = NULL;
	if (acrescenta_drones(&drones.next, num_drones, addr_shm_est))
	{
		printf("Erro a criar drones\n");
		return -1;
	}

	if ((fd_pipe = open(PIPE_NAME, O_RDWR)) == -1)
	{
		perror("Erro: ");
		return -1;
	}

	/*	Criação e inicialização da thread que vai terminar a Central	*/
	setup_thread_term_central(&term_struct, addr_shm_arm, addr_shm_est, addr_shm_stock, addr_shm_nomes_produtos, &drones, fd_pipe, &encomendas_suspensas);

	pthread_create(&tid_term, NULL, sigint_handler_central, (void *)&term_struct);


	/********	Pronto para receber encomendas	***********/

	timeout.tv_sec = t_abast;
	timeout.tv_usec= 0;

	while (true)
	{
		sigfillset (&set);
		sigdelset(&set, SIGINT);
		pthread_sigmask(SIG_SETMASK, &set, NULL);
		signal(SIGINT, sigint_handler);

		/*	Setup para select()	*/
		FD_ZERO(&rfds);
		FD_SET(fd_pipe, &rfds);

		/* IF o timeout do select() chegou ao fim na iteração anterior THEN faz-lhe reset */
		if (timeout.tv_sec < 1)
		{
			timeout.tv_sec = t_abast;
			timeout.tv_usec = 0;
		}

		if (!select(fd_pipe+1, &rfds, NULL, NULL, &timeout))
		{
			// novo abastecimento, vai ver encomendas suspensas

			ant = &encomendas_suspensas;
			atual = ant->next;

			while (atual)
			{
				if (try_resend_enc(atual, addr_shm_stock, addr_shm_arm, addr_shm_est, drones.next) == OK)
				{
					ant->next = atual->next;
					free(atual);
					atual = ant->next;
				}
				else
				{
					ant = atual;
					atual = atual->next;
				}
			}

			continue;
		}

		/*START: bloqueia todos os sinais, "em especial" o SIGINT	*/
		sigfillset (&set);
		pthread_sigmask (SIG_SETMASK, &set, NULL);
		/*	END	*/

		/*	Read the command into 'command'	*/
		get_command(fd_pipe, command);	//printf("%s\n", command);

		r = avalia_comando(command, addr_shm_nomes_produtos);

		switch (r)
		{
			case CDRONE:

				if ((r = set_drones(command, &drones.next, &num_drones, addr_shm_est)) > 0)
					sprintf(buffer, "Numero de drones aumentado em %d, para %d", r, num_drones);
				else
					sprintf(buffer, "Numero de drones reduzido em %d, para %d", abs(r), num_drones);

				log_ecra(buffer);
				break;

			case CORDER:

				nova_encomenda(command, addr_shm_stock, addr_shm_arm, addr_shm_est, drones.next, &encomendas_suspensas.next);
		}
	}

	return 0;
}

void sigint_handler(int ignore)
{
	get_out = true;
	pthread_mutex_lock(&mutex_term);
	pthread_cond_signal(&cond_term);
	pthread_mutex_unlock(&mutex_term);
	pthread_join(tid_term, NULL);
}

void setup_thread_term_central(struct ponteiros_central *p, Armazem *arm, Estatistica *est, Stock *stock, NomeProduto *np, Drone *drones, int fd, struct lista_encomendas *le)
{
	p->shm_arm = arm;
	p->shm_est = est;
	p->shm_stock = stock;
	p->shm_np = np;
	p->drones = drones;
	p->fd_pipe = fd;
	p->encomendas_suspensas = le;
}

void *sigint_handler_central(void *arg)
{
		int i;
		char command[MAX_CHARS*3], msg_erro[2*MAX_CHARS], c;
		Drone *drones_tmp;
		struct lista_encomendas *enc_tmp;
		struct ponteiros_central *ponteiros = (struct ponteiros_central *)arg;

		pthread_mutex_lock(&mutex_term);
		if (get_out == false)
			pthread_cond_wait(&cond_term, &mutex_term);

		/*	Escreve todas as encomendas no pipe para o log	*/

			/* Configura o file descriptor para não bloquear */
		if (fcntl(ponteiros->fd_pipe, F_SETFL, fcntl(ponteiros->fd_pipe, F_GETFL, 0) | O_NONBLOCK) == -1)
			exit(-1);

		i = 0; c = 'a';
		while (read(ponteiros->fd_pipe, &c, 1) > 0)
		{
			if (c != '\n')
				command[i++] = c;
			else
			{
				command[i] = '\0';
				i = 0;

				/* escreve para o log	*/
				strcpy(msg_erro, "Comando nao validado nem executado: ");
				strcat(msg_erro, command);
				escreve_log(msg_erro);
			}
		}

		/*	Escreve as encomendas na fila de espera para o log	e limpa malloc()'s feitos	*/

		while (ponteiros->encomendas_suspensas->next)
		{
			enc_tmp = ponteiros->encomendas_suspensas->next;

			sprintf(command, "Encomenda '%s' estava em fila de espera", enc_tmp->encomenda->nome_encomenda);

			escreve_log(command);

			ponteiros->encomendas_suspensas->next = enc_tmp->next;
			free(enc_tmp->encomenda);
			free(enc_tmp);
		}

		/*	Cleanup drones	*/		// ARRANJAR!!!! está mal
		get_out = ON;

		while (ponteiros->drones->next)
		{
			drones_tmp = ponteiros->drones->next;

			pthread_mutex_lock(&drones_tmp->mutex);

			while (drones_tmp->estado == OCUPADO)
				pthread_cond_wait(&drones_tmp->cond, &drones_tmp->mutex);

			while (drones_tmp->estou_numa_base == false)
				pthread_cond_wait(&drones_tmp->cond, &drones_tmp->mutex);

			/*	O drone agora está a tentar dar lock ao mutex para ir "dormir" à espera de uma encomenda	*/
			drones_tmp->estado = OCUPADO;
			drones_tmp->encomenda = NULL;

				/* envia sinal para o caso em que o drone nunca trabalhou */
			pthread_cond_signal(&drones_tmp->cond);

			pthread_mutex_unlock(&drones_tmp->mutex);

			pthread_join(drones_tmp->tid, NULL);

			pthread_mutex_destroy(&drones_tmp->mutex);
			pthread_cond_destroy(&drones_tmp->cond);

			ponteiros->drones->next = drones_tmp->next;
			free(drones_tmp);
		}

		/*	Diz aos armazéns que é para sair	*/
		sem_wait(sem_term);

		/*	Destruir imediatamente message queue*/
		msgctl(get_msgQid(), IPC_RMID, NULL);


		/*	Dettach shared memories deste processo	*/
		shmdt(ponteiros->shm_arm);
		shmdt(ponteiros->shm_est);
		shmdt(ponteiros->shm_stock);
		shmdt(ponteiros->shm_np);

		/*	Destruir semáforos apenas para este processo	*/
		sem_close(sem_stock);
		sem_close(sem_est);
		sem_close(sem_log);
		sem_close(sem_term);

		close(ponteiros->fd_pipe);
		/*	Destruir pipe	*/
		unlink(PIPE_NAME);

		exit(0);
}

int try_resend_enc(struct lista_encomendas *enc, Stock *stock, Armazem *armazens, Estatistica *shm_est, Drone *drones)
// return OK se atribuiu encomenda ao drone
{
	char buffer_aux[2*MAX_CHARS];
	int nao_ha_drones, nao_ha_stock, first_time, i, k;
	Drone *tmp_drone;

	double dist_min, tmp_dist;
	Armazem *arm_escolhido;
	Drone *drone_escolhido;
	Stock *stock_escolhido;

	nao_ha_stock = nao_ha_drones = first_time = true;
	dist_min = DBL_MAX;

	sem_wait(sem_stock);

	for (i=0; stock[i].id_armazem != 0; ++i)
	{

		if (!strcmp(stock[i].nome_produto, enc->encomenda->nome_produto) && stock[i].qtd >= enc->encomenda->qtd )
		{
			/* este armazem tem stock	*/
			nao_ha_stock = false;

			/*	procura o armazem	*/
			for (k=0; armazens[k].id != stock[i].id_armazem; ++k)
				;
			/*	armazém encontrado	*/

			/*	percorre a lista de drones	*/
			do
			{
				if (first_time == false)
				{
					/*	libertar o semáforo para os armazens conseguirem processar
						os abastecimentos. O stock pode alterar entretanto,
						mas é garantido que nunca diminui, logo, não há problema*/
					sem_post(sem_stock);

					pthread_mutex_lock(&mutex_enc);
					pthread_cond_wait(&cond_enc, &mutex_enc);
					pthread_mutex_unlock(&mutex_enc);

					sem_wait(sem_stock);
				}

				tmp_drone = drones;

				while (tmp_drone)
				{
					pthread_mutex_lock(&tmp_drone->mutex);

					if (tmp_drone->estado == DESOCUPADO)
					{
						/*	Existe pelo menos um drone desocupado	*/
						nao_ha_drones = false;

						tmp_dist = get_distancia_percurso(tmp_drone->x, tmp_drone->y, armazens[k].x, armazens[k].y, enc->encomenda->x_final, enc->encomenda->y_final);

						if (tmp_dist < dist_min)
						{
							dist_min = tmp_dist;
							arm_escolhido = armazens + k;
							drone_escolhido = tmp_drone;
							stock_escolhido = stock + i;
						}
					}
					pthread_mutex_unlock(&tmp_drone->mutex);

					tmp_drone = tmp_drone->next;
				}

				first_time = false;

			} while (nao_ha_drones == true);
		}
	}

	sem_post(sem_stock);

	/*	Caso em que nenhum armazém tem stock	*/
	if (nao_ha_stock == true)
	{
		//sprintf(buffer_aux, "Encomenda '%s' foi novamente suspensa por falta de stock\n", enc->encomenda->nome_encomenda);

		//log_ecra(buffer_aux);

		return OK+1; // return something other than OK
	}

	/*	Atribui encomenda ao drone	*/

	/*	Reserva produtos	*/
	sem_wait(sem_stock);
	stock_escolhido->qtd -= enc->encomenda->qtd;
	sem_post(sem_stock);

	/*	Atribui encomenda ao drone	*/
	pthread_mutex_lock(&drone_escolhido->mutex);

	enc->encomenda->id_arm = arm_escolhido->id;
	enc->encomenda->x_arm = arm_escolhido->x;
	enc->encomenda->y_arm = arm_escolhido->y;

	drone_escolhido->encomenda = enc->encomenda;

	drone_escolhido->estado = OCUPADO;

	pthread_mutex_unlock(&drone_escolhido->mutex);

	pthread_cond_signal(&drone_escolhido->cond);
	/*	Feito	*/

	/*	Notifica que uma encomenda foi atribuída a um drone	*/
	sprintf(buffer_aux, "Encomenda '%s' foi retomada e atribuida ao drone %u", enc->encomenda->nome_encomenda, drone_escolhido->id);
	log_ecra(buffer_aux);

	/*	Atualiza shared memory das estatisticas	*/
	sem_wait(sem_est);
	++shm_est->total_encomendas;
	sem_post(sem_est);

	return OK;
}


void nova_encomenda(char *command, Stock *stock, Armazem *armazens, Estatistica *shm_est, Drone *drones, struct lista_encomendas **suspensas)
{
	Encomenda *nova_enc;
	char buffer_aux[2*MAX_CHARS];
	int nao_ha_drones, nao_ha_stock, first_time, i, k;
	Drone *tmp_drone;

	double dist_min, tmp_dist;
	Armazem *arm_escolhido;
	Drone *drone_escolhido;
	Stock *stock_escolhido;

	nao_ha_stock = nao_ha_drones = true;
	dist_min = DBL_MAX;

	if (!(nova_enc = (Encomenda *)malloc(sizeof(Encomenda))))
		return;

	/*	Inicializa nova encomenda	*/
	sscanf(command, "ORDER %s prod: %[^,], %u to: %u, %u", nova_enc->nome_encomenda,nova_enc->nome_produto, &nova_enc->qtd, &nova_enc->x_final, &nova_enc->y_final);

	nova_enc->id = counter_num_enc--;

	/*	Notifica que chegou nova encomenda	*/
	sprintf(buffer_aux, "Nova Encomenda '%s' recebida [0x%lX]", nova_enc->nome_encomenda,nova_enc->id);
	log_ecra(buffer_aux);

	/*	Procura por armazens que consigam satisfazer o pedido  i.e. tenham stock suficiente	*/

	sem_wait(sem_stock);

	for (i=0; stock[i].id_armazem != 0; ++i)
	{
		first_time = true;

		if (!strcmp(stock[i].nome_produto, nova_enc->nome_produto) && stock[i].qtd >= nova_enc->qtd )
		{
			/* este armazem tem stock	*/
			nao_ha_stock = false;

			/*	procura o armazem	*/
			for (k=0; armazens[k].id != stock[i].id_armazem; ++k)
				;
			/*	armazém encontrado	*/

			/*	percorre a lista de drones	*/
			do
			{
				if (first_time == false)
				{
					/*	libertar o semáforo para os armazens conseguirem processar
						os abastecimentos. O stock pode alterar entretanto,
						mas é garantido que nunca diminui, logo, não há problema*/
					sem_post(sem_stock);

					pthread_mutex_lock(&mutex_enc);
					pthread_cond_wait(&cond_enc, &mutex_enc);
					pthread_mutex_unlock(&mutex_enc);

					sem_wait(sem_stock);
				}

				tmp_drone = drones;

				while (tmp_drone)
				{
					pthread_mutex_lock(&tmp_drone->mutex);

					if (tmp_drone->estado == DESOCUPADO)
					{
						/*	Existe pelo menos um drone desocupado	*/
						nao_ha_drones = false;

						tmp_dist = get_distancia_percurso(tmp_drone->x, tmp_drone->y, armazens[k].x, armazens[k].y, nova_enc->x_final, nova_enc->y_final);

						if (tmp_dist < dist_min)
						{
							dist_min = tmp_dist;
							arm_escolhido = armazens + k;
							drone_escolhido = tmp_drone;
							stock_escolhido = stock + i;
						}
					}
					pthread_mutex_unlock(&tmp_drone->mutex);

					tmp_drone = tmp_drone->next;
				}

				first_time = false;

			} while (nao_ha_drones == true);
		}
	}

	sem_post(sem_stock);

	/*	Caso em que nenhum armazém tem stock	*/
	if (nao_ha_stock == true)
	{
		if (!add_enc_suspensa(suspensas, nova_enc))
		{
			sprintf(buffer_aux, "Encomenda %s suspensa por falta de stock", nova_enc->nome_encomenda);
			log_ecra(buffer_aux);
		}
		else
		{
			sprintf(buffer_aux, "ERRO ao guardar Encomenda %s suspensa por falta de stock", nova_enc->nome_encomenda);
			log_ecra(buffer_aux);
		}
		return;
	}

	/*	Atribui encomenda ao drone escolhido	*/

	/*	Reserva produtos	*/
	sem_wait(sem_stock);
	stock_escolhido->qtd -= nova_enc->qtd;
	sem_post(sem_stock);

	/*	Atribui encomenda ao drone	*/
	pthread_mutex_lock(&drone_escolhido->mutex);

	nova_enc->id_arm = arm_escolhido->id;
	nova_enc->x_arm = arm_escolhido->x;
	nova_enc->y_arm = arm_escolhido->y;

	drone_escolhido->encomenda = nova_enc;

	drone_escolhido->estado = OCUPADO;

	pthread_mutex_unlock(&drone_escolhido->mutex);

	pthread_cond_signal(&drone_escolhido->cond);
	/*	Feito	*/

	/*	Notifica que uma encomenda foi atribuída a um drone	*/
	sprintf(buffer_aux, "Encomenda '%s' atribuida ao drone %u", nova_enc->nome_encomenda, drone_escolhido->id);
	log_ecra(buffer_aux);

	/*	Atualiza shared memory das estatisticas	*/
	sem_wait(sem_est);
	++shm_est->total_encomendas;
	sem_post(sem_est);

	return;
}

int add_enc_suspensa(struct lista_encomendas **lista, Encomenda *enc)
{
	struct lista_encomendas *nova, *ant, *atual;

	if (!(nova = (struct lista_encomendas *)malloc(sizeof(struct lista_encomendas))))
		return -1;
	nova->encomenda = enc;
	nova->next = NULL;

	/*	Adiciona nova encomenda ao fim da lista de encomendas suspensas	*/
	atual = *lista;
	ant = NULL;
	while (atual)
	{
		ant = atual;
		atual = atual->next;
	}
	if (!ant)
		*lista = nova;
	else
		ant->next = nova;

	return 0;
}

double get_distancia_percurso(double xi, double yi, unsigned int xm, unsigned int ym, unsigned int xf, unsigned int yf)
{
	return (distance((double)xi, (double)yi, (double)xm, (double)ym) + distance((double)xm, (double)ym, (double)xf, (double)yf));
}

int set_drones(char *command, Drone **drones, int *num_drones, Estatistica *shm_est)
{
	int n, n_atual;

	n_atual = *num_drones;
	sscanf(command, "DRONE SET %d", &n);

	if (n < n_atual)
	{
		//printf("A entrar em retira_drones()\n");
		retira_drones(drones, n_atual-n);
		//printf("A sair de retira_drones()\n");
	}
	else
		acrescenta_drones(drones, n-n_atual, shm_est);

	*num_drones = n;

	return (n - n_atual);
}

int avalia_comando(char *command, NomeProduto *produtos)
{
	char *token, msg_erro[2*MAX_CHARS], copia_comando[MAX_CHARS];

	strcpy(copia_comando, command);

	if ((token = strtok(copia_comando, " ")))
	{
		if (!strcmp(token, "ORDER"))
			if (is_order_valid(command, produtos))
				return CORDER;

		if (!strcmp(token, "DRONE"))
			if (is_drone_set_valid(command))
				return CDRONE;
	}

	sprintf(msg_erro, "Comando descartado: %s", command);
	escreve_log(msg_erro);

	return -1;
}

int is_order_valid(char *command, NomeProduto *produtos)
{
	char nome[MAX_CHARS], buf[MAX_CHARS];
	int qtd, x, y, i;

	strcpy(buf, command);

	/*	Lê os campos do comando	*/
	if (sscanf(command, "ORDER %s prod: %[^,], %d to: %d, %d", buf, nome, &qtd, &x, &y) == EOF)
		return 0;

	//printf("ORDER xx prod: %s, %d to: %d, %d\n", nome, qtd, x, y);

	if (qtd <= 0 || x < 0 || y < 0 || x > x_max || y > y_max)
		return 0;


	/*	Verifica se o produto existe	*/
	for (i=0; strcmp(produtos[i].nome, "\0"); ++i)
	{
		if (!strcmp(produtos[i].nome, nome))
			return 1;
	}
	return 0;
}

int is_drone_set_valid(char *command)
{
	int n;
	if (sscanf(command, "DRONE SET %d", &n) == EOF)
		return 0;
	return n<=0 ? 0 : 1;
}

/////////////////////////////////

void get_command(int fd, char *command)
{
	char c;
	int i;

	i = 0;
	c = 'a';

	// Ler carater a carater ate encontrar '\n'
	while (c != '\n' && c != '\0')
	{
		read(fd, &c, 1);
		if (c != '\n' && c != '\0')
			command[i++] = c;
	}
	command[i] = '\0';

	return;
}

void escreve_log(char *s)
{
	FILE *fp;
	time_t seconds;
	struct tm curr_time;

	/*	FICA COM SEMÁFORO; verifica se sem_wait() não foi interrompido por um sinal*/
	do
	{
		errno = 0;
		sem_wait(sem_log);

	} while(errno == EINTR);


	fp = fopen(LOG_FILE, "a");

	seconds = time(NULL);
	curr_time = *localtime(&seconds);

	fprintf(fp, "%02d:%02d:%02d %s\n", curr_time.tm_hour, curr_time.tm_min, curr_time.tm_sec, s);

	fclose(fp);
	sem_post(sem_log);

	return;
}

int acrescenta_drones(Drone **inicio, int num, Estatistica *shm_est)
{
	int i;
	int xy[NUM_BASES][2] = {{0,0}, {x_max,0}, {x_max,y_max}, {0,y_max}};
	Drone *tmp;

	for (i=1; i<=num; ++i)
	{
		if ( !(tmp = (Drone *)malloc(sizeof(Drone))) )
			return -1;

		tmp->next = *inicio;
		*inicio = tmp;

		/*	Estrutura criada, vamos preechê-la	*/

		tmp->x = xy[(i-1)%4][0];
		tmp->y = xy[(i-1)%4][1];
		tmp->id = ++drones_id_counter;
		tmp->estado = DESOCUPADO;
		tmp->encomenda = NULL;
		tmp->shm_est = shm_est;
		tmp->estou_numa_base = true;

		if (pthread_mutex_init(&tmp->mutex, NULL))
			return -1;
		if (pthread_cond_init(&tmp->cond, NULL))
			return -1;
		if (pthread_create(&tmp->tid, NULL, drone_start, (void *)tmp))
			return -1;
	}

	return 0;
}


void retira_drones(Drone **inicio, int num)
{
	Drone *ant, *atual;
	int first_time;

	first_time = true;

	while(num)
	{
		atual = *inicio;
		ant = NULL;

		if (first_time == false)
		{
			pthread_mutex_lock(&mutex_enc);
			pthread_cond_wait(&cond_enc, &mutex_enc);
			pthread_mutex_unlock(&mutex_enc);
		}

		while(atual && num)
		{
			//printf("num = %d\n", num);
			pthread_mutex_lock(&atual->mutex);
			//printf("After lock\n");

			if (atual->estado == DESOCUPADO)
			{
				////////////////////////

				atual->encomenda = NULL;
				atual->estado = OCUPADO;
				pthread_mutex_unlock(&atual->mutex);
				pthread_cond_signal(&atual->cond);

				//printf("After signal\n");

				pthread_join(atual->tid, NULL);

				pthread_mutex_destroy(&atual->mutex);
				pthread_cond_destroy(&atual->cond);

				//printf("Antes do algoritmo\n");

				////////////////

				if (ant)
				{
					ant->next = atual->next;

					free(atual);
					atual = ant->next;
				}
				else
				{
					*inicio = (*inicio)->next;
					free(atual);
					atual = (*inicio);
				}
				--num;
			}
			else
			{
				pthread_mutex_unlock(&atual->mutex);
				ant = atual;
				atual = atual->next;
			}
		}
		first_time = false;
	}

	return;
}


void ler_inteiros(char *str, int *d, int *t, int *abast)
{
	char *token;
	token = strtok(str, " ");
	*d = (int)strtol(token, NULL, 10);
	token = strtok(NULL, " ");
	*t = (int)strtol(token, NULL, 10);
	token = strtok(NULL, " ");
	x_max = (int)strtol(token, NULL, 10);
	token = strtok(NULL, " ");
	y_max = (int)strtol(token, NULL, 10);
	token = strtok(NULL, " ");
	*abast = (int)strtol(token, NULL, 10);
}

void *drone_start(void *drone_info)
{
	Drone *info;
	int x_prox, y_prox, msgQ_id;
	MsgBuffer *tmp_msg;
	char buffer[2*MAX_CHARS];
	time_t partida, chegada;
	double tempo_percurso;

	/*Set cancellation type, para a thread poder ser cancelada a qualquer altura*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	info = (Drone *)drone_info;

	msgQ_id = get_msgQid();

	while (true)
	{
		if (info->estado == DESOCUPADO)
		{
			pthread_mutex_lock(&info->mutex);
			/* Espera por uma Encomenda	*/
			while (info->estado == DESOCUPADO)
				pthread_cond_wait(&info->cond, &info->mutex);

			if (!info->encomenda)
			{
				pthread_mutex_unlock(&info->mutex);
				pthread_exit(NULL);
			}

			pthread_mutex_unlock(&info->mutex);
		}

		/*	Aqui, temos que estado == OCUPADO, vamos verficar se encomenda==NULL, se sim, sai	*/

		if (!info->encomenda)
		{
			pthread_mutex_unlock(&info->mutex);
			pthread_exit(NULL);
		}

		info->estou_numa_base = false;

		partida = time(NULL);

		/*	Vai até ao armazém. Não pode ser interrompido*/

		/*	Nao preciso do lock, porque o drone está OCUPADO e ninguem lhe vai mexer nas coordenadas
			no máximo vao verificar o seu estado	*/

		while(move_towards(&info->x, &info->y, info->encomenda->x_arm, info->encomenda->y_arm))
			sleep(uni_tempo);

		/*	Drone chegou ao armazém	*/


		/*Criar e Enviar mensagem ao armazem a dizer que cheguei*/

		tmp_msg = cria_msg_drone(info->encomenda->id_arm, info->encomenda->id, info->encomenda->qtd);

		msgsnd(msgQ_id, (void *)tmp_msg, sizeof(*tmp_msg)-sizeof(long), 0);

		/*Esperar que carregamento complete*/
		/*não tenho que me preocupar em proteger esta call contra sinais*/
		msgrcv(msgQ_id, (void *)tmp_msg, sizeof(MsgBuffer)-sizeof(long), info->encomenda->id, 0);

		//printf("Mensagem enviada ao armazem\n");


		/*	Estou a ir até ao local destino da encomendas
		*	Demoro uma unidade de tempo
		*/
		sleep(uni_tempo);

		/*	CHEGUEI	*/

		chegada = time(NULL);
		tempo_percurso = get_double_from_time((int)difftime(chegada, partida));

		/*	Escrever para a log_ecra() que mais uma encomenda foi entregue	*/
		sprintf(buffer, "Encomenda '%s' entregue no destino pelo drone %u", info->encomenda->nome_encomenda, info->id);
		log_ecra(buffer); // argumentos

		/*	Atualizar a shm_est: total_encomenda_entregues e total_produtos_entregues	*/
		/*	Atualizar também tempo médio	*/
		sem_wait(sem_est);

		info->shm_est->total_produtos_entregues += info->encomenda->qtd;
		info->shm_est->tempo_medio = (info->shm_est->tempo_medio * info->shm_est->total_encomendas_entregues + tempo_percurso) / (info->shm_est->total_encomendas_entregues + 1);
		++info->shm_est->total_encomendas_entregues;

		sem_post(sem_est);

		/*Encomenda entregue*/
		free(info->encomenda);

		///
		pthread_mutex_lock(&info->mutex);

		info->estado = DESOCUPADO;

		/*	Avisa quem puder estar a esperar (caso da terminação)	*/
		pthread_cond_signal(&info->cond);

		pthread_mutex_unlock(&info->mutex);
		///

		////	Avisa que este drone ficou desocupado
				//(caso em que uma encomenda está à espera de um drone)
				//(caso em que se está à espera para remover drones)
			pthread_mutex_lock(&mutex_enc);
			pthread_cond_signal(&cond_enc);
			pthread_mutex_unlock(&mutex_enc);

		////


		/*Agora vou para a base mais próxima
		* Sempre atento se me é atribuída alguma encomenda
		*/

		/*Invocar funçao que devolve a base mais proxima  (x_prox, y_prox) */
		base_mais_proxima(info->x, info->y, &x_prox, &y_prox);

		pthread_mutex_lock(&info->mutex);

		while (move_towards(&info->x, &info->y, x_prox, y_prox) && (info->estado == DESOCUPADO))
		{
			pthread_mutex_unlock(&info->mutex);
			sleep(uni_tempo);
			pthread_mutex_lock(&info->mutex);
		}

		info->estou_numa_base = true;
		pthread_cond_signal(&info->cond);
		pthread_mutex_unlock(&info->mutex);
	}

	pthread_exit(NULL);
}

MsgBuffer *cria_msg_drone(int id_arm, long id_enc, int qtd_abast)
{
	MsgBuffer *tmp;
	tmp = (MsgBuffer *)malloc(sizeof(MsgBuffer));

	tmp->mtype = id_arm;
	sprintf(tmp->text, "%s %ld %d", DRONE, id_enc, qtd_abast);

	/*Para teste*/
	//printf("%s\n", tmp->text);

	return tmp;
}


double get_double_from_time(int seconds)
{
	double r;
	int min, sec;

	r = 0;

	min = seconds%60;
	sec = (int)(seconds/60);

	if (sec < 10)
		r += min + (sec/10);
	else
		r += min + (sec/100);

	return r;
}

/*Funçao usada pelos drones.. PRIVADA*/
void base_mais_proxima(int x_drone, int y_drone, int *xb, int *yb)
{
	double dist, tmp;
	int i, base;
	int coord_bases[NUM_BASES][2] = {{0,0}, {x_max,0}, {x_max,y_max}, {0,y_max}};

	dist = -1.0;

	/**/
	for (i=0; i<NUM_BASES; ++i)
	{
		if ( (tmp = distance(x_drone, y_drone, coord_bases[i][0], coord_bases[i][1])) < dist && dist>0)
		{
			dist = tmp;
			base = i;
		}
		if (dist < 0)
		{
			dist = tmp;
			base = i;
		}
	}

	switch (base)
	{
		case 0: *xb = coord_bases[0][0];	*yb = coord_bases[0][1]; break;
		case 1: *xb = coord_bases[1][0];	*yb = coord_bases[1][1]; break;
		case 2: *xb = coord_bases[2][0];	*yb = coord_bases[2][1]; break;
		case 3: *xb = coord_bases[3][0];	*yb = coord_bases[3][1]; break;
	}
}
