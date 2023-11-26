/****************************************************
*                                                   *
*			Operating Systems Project               *
*                                                   *
*			Submission Date: 08/12/2018             *
*             										*
*                                                   *
*****************************************************/


#include "header.h"

sem_t *sem_est, *sem_stock, *sem_log, *sem_term;

int main(int argc, char **argv)
{
	unsigned int id;
	Stock *shm_stock;
	Estatistica *shm_est;
	sigset_t set;
	MsgBuffer msg_buf;
	char *token, buffer[MAX_CHARS];
	int uni_tempo, qtd, sval, mq_id;

	/* SIGNAL HANDLING (start)*/
	sigfillset (&set);
	sigprocmask (SIG_SETMASK, &set, NULL);
	/* SIGNAL HANDLING (finish)*/

	/*	Lê argv	*/
	token = strtok(argv[1], " ");
	id = (unsigned int)strtol(token, NULL, 10);
	token = strtok(NULL, " ");
	uni_tempo = strtol(token, NULL, 10);

	/*	Attach da shm do stock e da shm das estatísticas*/

	if ((shm_stock = (Stock *)attach_shm(ID_SHM_STOCK)) == (void *)(-1))
	{
		printf("Erro a juntar shm_stock no armazem #%d\n", id);
		return -1;
	}

	if ((shm_est = (Estatistica *)attach_shm(ID_SHM_EST)) == (void *)(-1))
	{
		printf("Erro a juntar shm_est no armazem #%d\n", id);
		return -1;
	}

	/*	Semáforos	*/
	sem_est = sem_open(SEMEST, 0);
	sem_stock = sem_open(SEMSTOCK, 0);
	sem_term = sem_open(SEMTERM, 0);
	sem_log = sem_open(SEMLOG, 0);

	/*TESTE-->*/ /*"Attach" Message Queue*/
	if ((mq_id = get_msgQid()) == -1)
	{
		perror("Erro");
		return -1;
	}

	/*****	Recebe encomendas	********/

	sem_getvalue(sem_term, &sval);
	while(sval > 0)
	{
		if (msgrcv(mq_id, (void *)&msg_buf, (size_t)(sizeof(msg_buf)-sizeof(long)), (long)id, 0) == -1)
		{
			sem_getvalue(sem_term, &sval);
			continue;
		}

		/*	Avalia mensagem	*/
		if (avalia_msg(&msg_buf) == MSG_DRONE)
		{
			qtd = take_care_of_drone(&msg_buf, mq_id, uni_tempo);

			//atualiza shm_est com o total_produtos_carregados
			atualiza_shm_est(shm_est, qtd);
		}
		else{
			take_care_of_abast(&msg_buf, shm_stock);

			sprintf(buffer, "Armazem %u recebeu novo stock", id);
			log_ecra(buffer);
		}

		sem_getvalue(sem_term, &sval);
	}

	terminar_armazem(shm_stock, shm_est, sem_term);

	return 0;
}

void terminar_armazem(Stock *shm_stock, Estatistica *shm_est, sem_t *sem_term)
{
	/*	Dettach memórias partilhadas	*/
	shmdt(shm_stock);
	shmdt(shm_est);

	/*	Limpar os 4 semáforos	*/
	sem_close(sem_est);
	sem_close(sem_log);
	sem_close(sem_term);
	sem_close(sem_stock);

	return;
}

int avalia_msg (MsgBuffer* msg){
	//Avalia o tipo de mensagem, se é ABASTECIMENTO OU DRONE
	char copia[MSGMAX], *token_info;
	strcpy(copia, msg->text);

	token_info = strtok (copia, " ");

	if (strcmp(token_info, DRONE) == 0)
		return MSG_DRONE;

	return (MSG_DRONE+1);
}


int take_care_of_drone(MsgBuffer* msg, int mqid, int uni_tempo)
{
	char *token;
	int qtd;
	long id_enc;

	token = strtok (msg->text, " ");

	token = strtok (NULL, " ");
	id_enc = (long)strtol(token, NULL, 10);

	token = strtok(NULL, " ");
	qtd = (int)strtol(token, NULL, 10);

	sleep(uni_tempo * qtd); //Tempo que demora a tratar da encomenda

	msg -> mtype = (long)id_enc;
	msgsnd(mqid, (void*)msg, sizeof(*msg) - sizeof(long), 0);

	return qtd;
}

void take_care_of_abast(MsgBuffer* msg, Stock *shm_stock)
{
	char *token, prod_name[MAX_CHARS];
	int qtd_abast, i;
	unsigned int id_armz;

	token = strtok(msg->text, " ");

	token = strtok(NULL, " ");
	strcpy(prod_name, token);

	token = strtok(NULL, " ");
	qtd_abast = (int)strtol(token, NULL, 10);

	id_armz = (unsigned int)msg->mtype;

	//VOU ATUALIZAR A SHM DO STOCK
	sem_wait(sem_stock);

	i = 0;
	while (shm_stock[i].id_armazem != id_armz || strcmp(shm_stock[i].nome_produto, prod_name) != 0)
		++i;

	shm_stock[i].qtd += qtd_abast;

	sem_post(sem_stock);

	return;
}

void atualiza_shm_est(Estatistica* shm_est, int qtd){

	sem_wait (sem_est);

	shm_est -> total_produtos_carregados += qtd;

	sem_post (sem_est);
}
