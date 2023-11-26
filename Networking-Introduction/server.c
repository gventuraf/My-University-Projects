/*******************************************************************
**																   **
**    Project: Introduction to Computer Networking		           **
**                                                                 **
**    Submission Date: December 6, 2018             		       **
**                                                                 **
*******************************************************************/

/*		Aplicação: Servidor		*/


#include "header.h"

pthread_mutex_t mutex_ips, mutex_data, mutex_subscribers[NUM_PUBLIC_FIELDS];

pthread_t termination_thread;
pthread_mutex_t mutex_term; pthread_cond_t cond_term;
int shutdown_app;

int main(int argc, char **argv)
{
	int i, sfd, comsock; /*	listening socket and communication socket	*/
	struct sockaddr_in saddr, caddr; /*	server and client address	*/
	int port, caddr_len;
	ClientData *data;
	ClientAddress *ips, *ca_tmp;
	ThreadID *tids;	/*	Threads ids*/
	struct aux *aux;
	struct subscribers_list *subs_list;
	pthread_t subscriptions_thread; // thread responsible for ALL client's subscriptions
	sigset_t signal_set;
	struct pointers pointers;

	/*	Ignore all signals	*/
	sigfillset(&signal_set);
	sigprocmask(SIG_SETMASK, &signal_set, NULL);

	/*	Check for errors	*/
	if (argc != 2)
	{
		printf("Error\n\n\tSyntax: \"server.out port_number\"\n\n");
		return 0;
	}

	/*	Initialize global	*/
	shutdown_app = OFF;

	/*	Create socket	*/
	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error on socket creation\n");
		return 0;
	}

	/*	Initialize saddr	*/
	saddr.sin_family = AF_INET;

	/*	Check if port number is invalid	*/
	port = (int)strtol(argv[1], NULL, 10);
	if (port <= 1024 || port > 65536)
	{
		printf("Error: Invalid port number.\n");
		close(sfd);
		return 0;
	}
	/*	<=1024 are for specific apps
	**	<65536 > 16 bits
	*/
	/*	End port-checking	*/
	saddr.sin_port = htons((short)port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/*	Initialization DONE	*/

	/*	Bind	*/
	if (bind(sfd, (struct sockaddr *)&saddr, (socklen_t)sizeof(saddr)) == -1)
	{
		printf("Error on bind()\n");
		close(sfd);
		return 0;
	}

	/*	Create headers for 'data' and 'ips', so their "initial" addresses never change	*/
	data = (ClientData *)malloc(sizeof(ClientData));
	data->next = NULL;
	init_data(data);

	ips = (ClientAddress *)malloc(sizeof(ClientAddress));
	ips->next = NULL;

	/*	Get ISABELA's data	*/
	if (get_data(data))
	{
		printf("Error on get_data()\n");
		return -1;
	}
	printf("DONE GETTING DATA FROM ISABELA\n\n");


	/*	Initialize mutexes	*/
	pthread_mutex_init(&mutex_ips, NULL);
	pthread_mutex_init(&mutex_data, NULL);
	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)
		pthread_mutex_init(mutex_subscribers+i, NULL);

	/*	Create and nitialize 'subs_list'	*/
	init_subs_list(&subs_list);


	/*	For the threads	*/
	aux = (struct aux *)malloc(sizeof(struct aux));
	aux->data = data;
	aux->ips = ips;
	aux->subs_list = subs_list;

	tids = NULL;

	/*	Create 'subscriptions_thread'	*/
	if (pthread_create(&subscriptions_thread, NULL, subs_thread_routine, (void *)aux))
	{
		printf("Error creating 'subscriptions_thread'\n");
		return -1;
	}


	if (listen(sfd, QUEUE_MAX_LEN) == -1)
	{
		printf("Error on listen()\n");
		close(sfd);
		return 0;
	}

	/*	Signal Handling AND termination_thread setup and creation */
	pointers.data = data; pointers.tids = tids; pointers.aux = aux; pointers.subs_list = subs_list; pointers.listening_socket = sfd;

	pthread_mutex_init(&mutex_term, NULL);
	pthread_cond_init(&cond_term, NULL);

	pthread_create(&termination_thread, NULL, termination_routine, (void *)&pointers);

	sigdelset(&signal_set, SIGINT);
	sigprocmask(SIG_SETMASK, &signal_set, NULL);
	signal(SIGINT, sigint_handler);
	/*	END	*/

	while(1)
	{
		caddr_len = sizeof(caddr);
		comsock = accept(sfd, (struct sockaddr *)&caddr, (socklen_t *)&caddr_len);

		if (comsock == -1)
		{
			printf("Error on accept()\n");
			close(sfd);
			return 0;
		}

		if (!(ca_tmp = add2ips(ips, &caddr)))
		{
			printf("Error on add2ips()\n");
			close(comsock);
			continue;
		}
		// FALTA as threads saberem os "seus" socket_file_descriptors
		// Talvez a struct info4thread possa ter um campo int para o socket_fd ou data e ips
		// podem ser globais...
		if (go_go_thread(&tids, aux, comsock, ca_tmp))
		{
			printf("Error on go_go_thread()\n");
			close(comsock);
		}
	}

	return 0;
}

void sigint_handler(int n)
{
	shutdown_app = ON;
	pthread_mutex_lock(&mutex_term);
	pthread_cond_signal(&cond_term);
	pthread_mutex_unlock(&mutex_term);
	pthread_join(termination_thread, NULL);
}

void *termination_routine(void *arg)
{
	struct pointers *pointers = (struct pointers *)arg;
	ThreadID *tids_tmp;
	ClientData *data_tmp;
	int i;

	pthread_mutex_lock(&mutex_term);
	while (shutdown_app == OFF)
		pthread_cond_wait(&cond_term, &mutex_term);

	/*****	Cleanup time	********/

	/*	Cancel threads and free() 'tids' and subscribers_list (almost)	*/
	while (pointers->tids)
	{
		tids_tmp = pointers->tids;
		pointers->tids = pointers->tids->next;
		pthread_cancel(tids_tmp->tid);
		thread_cleanup_routine(tids_tmp->info);
		pthread_join(tids_tmp->tid, NULL);
	}

	/*	Clean subs_list (the list inside this one is already cleaned above)	*/
	free(pointers->subs_list);


	/*	free() 'data' ('ips' already cleaned by threads)	*/
	while (pointers->data)
	{
		data_tmp = pointers->data;
		pointers->data = pointers->data->next;
		free(data_tmp);
	}

	free(pointers->aux);

	/*	Destroy mutex and condition variables	*/

	pthread_mutex_destroy(&mutex_ips);
	pthread_mutex_destroy(&mutex_data);

	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)
		pthread_mutex_destroy(mutex_subscribers+i);

	pthread_mutex_destroy(&mutex_term);
	pthread_cond_destroy(&cond_term);

	close(pointers->listening_socket);

	printf("\n\n***SERVER GOING OFFLINE***\n\n");

	exit(0);
}

void *client_handler(void *arg)
{
	struct info4thread *info;
	info = (struct info4thread *)arg;
	char buffer[BUFFER_SIZE];
	int nread, get_out;

	/*	Set that thread can get cancelled anytime	*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	/* Ask for ID AND check if it exists	*/
	do{

		write(info->sfd, "Please insert your ID (type 'exit' to leave): ", strlen("Please insert your ID (type 'exit' to leave): ")+1);

		if ((nread = read(info->sfd, buffer, BUFFER_SIZE)) == 0)
			continue;

		if (!strcmp(buffer, "exit"))
		{
			thread_cleanup_routine(info);
			pthread_exit(NULL);
		}

		if (!(info->client_data_info = search_for_client_data(info->aux->data->next, buffer)))
		{

			printf("Read: %s\n", buffer);
			write(info->sfd, ERR_IDINV, strlen(ERR_IDINV)+1);
			printf("ID INVALIDO\n");
		}
		else
			write(info->sfd, "OK", strlen("OK")+1);

	} while (!info->client_data_info && nread);


	/*	Check if client disconnected	*/
	if (!nread)
	{
		thread_cleanup_routine(info);
		pthread_exit(NULL);
	}

	/*	Client is still here AND his ID is valid	*/

	/*	Read the port number where the client is listening (to eventually send subscriptions)	*/
	read(info->sfd, buffer, BUFFER_SIZE);

	/*	Save it	*/
	pthread_mutex_lock(&mutex_ips);

	info->client_network_info->server_port = (int)strtol(buffer, NULL, 10);

	/*	Add ID to the ips list	(there's no need to lock the mutex_ips)	*/
	strcpy(info->client_network_info->id, buffer);

	pthread_mutex_unlock(&mutex_ips);

	/*	Now we have:
	*1) a pointer to the client data structure in info->client_data_info
	*2) a pointer to the client address structure in info->client_network_info

	***************	Let's wait for commands	*****************/

	do{
		if ((nread = read(info->sfd, buffer, BUFFER_SIZE)) == 0) continue;

		/*	Read command and act accordingly	*/
		printf("Just received: %s\n", buffer);
		get_out = process_client_command(buffer, info);

	} while(nread && !get_out);

	/*	CLEANUP	*/
	thread_cleanup_routine(info);
	pthread_exit(NULL);
}

void thread_cleanup_routine(struct info4thread *info)
{
	printf("Client OUT\n");
	close(info->sfd);
	remove_client(info);
	free(info);
}


int process_client_command(char *buffer, struct info4thread *info)
{
	int ncmd;

	/*	Did the client asked for the menu?	*/
	if (!strcmp(buffer, MENU))
	{
		send_menu(info->sfd);
		return 0;
	}

	/*	Convert command to an integer	*/
	ncmd = (int)strtol(buffer, NULL, 10);

	/*	Check if 'ncmd' is valid	*/
	if (ncmd<1 || ncmd>4)
	{
		write(info->sfd, ERR_COMINV, strlen(ERR_COMINV)+1);
		return 0;
	}

	switch (ncmd)
	{
		case 1:	handle_first_cmd(info); break;
		case 2:	send_group_data(info); break;
		case 3:	manage_subscriptions(info); break;
		case 4:	return 1;
	}

	return 0;
}

void send_menu(int s)
{
	char big_buffer[BIG_BUFFER_SIZE];
	big_buffer[0] = '\0';

	sprintf(big_buffer, "\n\n******************MENU****************\n\n");
	strcat(big_buffer, "\t1) See my data\n\t2) See group data\n\t3) Manage my subscriptions\n\t4) Close the app\n");
	write(s, big_buffer, strlen(big_buffer)+1);
	return;
}

void handle_first_cmd(struct info4thread 	*info)
{
	char big_buffer[BIG_BUFFER_SIZE];
	int nread;

	sprintf(big_buffer, "1) Type 'all' to see all your data\n\n2) Type the corresponding keyword to a certain field of data to see only that information\n\n3) Type 'menu' to go back to the main menu\n");

	write(info->sfd, big_buffer, strlen(big_buffer)+1);
	nread = read(info->sfd, big_buffer, BIG_BUFFER_SIZE);

	if (nread <= 0 || !strcmp(big_buffer, MENU))
		return;

	if (!strcmp(big_buffer, "all"))
	{
		send_all_user_data(info);
		return;
	}

	if (send_user_field(big_buffer, info))
		write(info->sfd, ERR_COMINV, strlen(ERR_COMINV)+1);

	return;
}

void send_all_user_data(struct info4thread *info)
{
	char big_buffer[BIG_BUFFER_SIZE], bbuffer[BIG_BUFFER_SIZE];

	big_buffer[0] = '\0';

	sprintf(bbuffer, "\nID: %s\n", info->client_data_info->id);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Type: %s\n", info->client_data_info->type);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Activity: %s\n", info->client_data_info->activity);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Location: %s\n", info->client_data_info->location);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Department: %s\n", info->client_data_info->department);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Calls duration: %.2f\n", info->client_data_info->calls_duration);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Calls made: %.0f\n", info->client_data_info->calls_made);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Calls missed: %.0f\n", info->client_data_info->calls_made);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Calls received: %.0f\n", info->client_data_info->calls_rcv);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Messages received: %.0f\n", info->client_data_info->sms_rcv);
	strcat(big_buffer, bbuffer);

	sprintf(bbuffer, "Messages sent: %.0f\n", info->client_data_info->sms_sent);
	strcat(big_buffer, bbuffer);


	write(info->sfd, big_buffer, strlen(big_buffer)+1);
	return;
}

int send_user_field(char *big_buffer, struct info4thread *info)
{
	if (!strcmp(big_buffer, "id"))
	{
		sprintf(big_buffer, "\nID: %s\n", info->client_data_info->id);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "type"))
	{
		sprintf(big_buffer, "\nType: %s\n", info->client_data_info->type);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "activity"))
	{
		sprintf(big_buffer, "\nActivity: %s\n", info->client_data_info->activity);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "location"))
	{
		sprintf(big_buffer, "\nLocation: %s\n", info->client_data_info->location);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "department"))
	{
		sprintf(big_buffer, "\nDepartment: %s\n", info->client_data_info->department);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "cd"))
	{
		sprintf(big_buffer, "\nCalls duration: %.2f\n", info->client_data_info->calls_duration);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "cma"))
	{
		sprintf(big_buffer, "\nCalls made: %.0f\n", info->client_data_info->calls_made);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "cmi"))
	{
		sprintf(big_buffer, "\nCalls missed: %.0f\n", info->client_data_info->calls_made);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "cr"))
	{
		sprintf(big_buffer, "\nCalls received: %.0f\n", info->client_data_info->calls_rcv);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "mr"))
	{
		sprintf(big_buffer, "\nMessages received: %.0f\n", info->client_data_info->sms_rcv);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	else if (!strcmp(big_buffer, "ms"))
	{
		sprintf(big_buffer, "\nMessages sent: %.0f\n", info->client_data_info->sms_sent);
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
		return 0;
	}

	/*	Invalid command	*/
	return 1;
}


void send_group_data(struct info4thread *info)
{
	char buf[BIG_BUFFER_SIZE], tmp[BUFFER_SIZE];

	strcpy(buf, "\n***Group Info***\n\n");

	sprintf(tmp, "\tCalls duration: %.2f\n", info->aux->data->calls_duration);
	strcat(buf, tmp);

	sprintf(tmp, "\tCalls made: %.2f\n", info->aux->data->calls_made);
	strcat(buf, tmp);

	sprintf(tmp, "\tCalls missed: %.2f\n", info->aux->data->calls_made);
	strcat(buf, tmp);

	sprintf(tmp, "\tCalls received: %.2f\n", info->aux->data->calls_rcv);
	strcat(buf, tmp);

	sprintf(tmp, "\tMessages received: %.2f\n", info->aux->data->sms_rcv);
	strcat(buf, tmp);

	sprintf(tmp, "\tMessages sent: %.2f\n", info->aux->data->sms_sent);
	strcat(buf, tmp);

	write(info->sfd, buf, strlen(buf)+1);

	return;
}

void manage_subscriptions(struct info4thread *info)
{
	char big_buffer[BIG_BUFFER_SIZE];
	int ncmd;

	sprintf(big_buffer, "1) See all your subscriptions\n2) Make a subscription to a field\n3) Remove a subscription\n4) Go back to menu\n");

	write(info->sfd, big_buffer, strlen(big_buffer)+1);
	read(info->sfd, big_buffer, BIG_BUFFER_SIZE);

	ncmd = (int)strtol(big_buffer, NULL, 10);

	if (ncmd<1 || ncmd >4)
	{
		write(info->sfd, ERR_COMINV, strlen(ERR_COMINV)+1);
		return;
	}

	if (ncmd == 4)
		return;

	if (ncmd != 1)
	{
		write(info->sfd, "Which field? ", strlen("Which field? ")+1);
		read(info->sfd, big_buffer, BIG_BUFFER_SIZE);
	}

	switch (ncmd)
	{
		case 1: show_subscriptions(info); break;
		case 2: subscribe_to_field(info, big_buffer); break;
		case 3:
					if (remove_subscription(info, big_buffer))
						write(info->sfd, "Removal successfull\n", strlen("Removal successfull\n")+1);
					else
						write(info->sfd, "There was nothing to remove\n", strlen("There was nothing to remove\n")+1);
	}

	return;
}

void show_subscriptions(struct info4thread *info)
{
	int i;
	struct address_capsule *tmp;
	char buffer[BUFFER_SIZE], big_buffer[BIG_BUFFER_SIZE];

	big_buffer[0] = '\0';

	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)
	{
		pthread_mutex_lock(mutex_subscribers+i);
		tmp = info->aux->subs_list[i].subscribers;

		while (tmp && tmp->ptr != info->client_network_info)
			tmp = tmp->next;
		if (tmp)
		{
			sprintf(buffer, "Field: %s\n", info->aux->subs_list[i].field);
			strcat(big_buffer, buffer);
		}
		pthread_mutex_unlock(mutex_subscribers+i);
	}

	if (big_buffer[0] != '\0')
		write(info->sfd, big_buffer, strlen(big_buffer)+1);
	else
		write(info->sfd, "You have no subscriptions\n", strlen("You have no subscriptions\n")+1);

	return;
}

int remove_subscription(struct info4thread *info, char *field)
{
	int i;
	struct address_capsule *ant, *atual;

	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)
	{
		/*	Lock the specific mutex	*/
		pthread_mutex_lock(mutex_subscribers + i);

		if (strcmp(info->aux->subs_list[i].field, field))
		{
			pthread_mutex_unlock(mutex_subscribers + i);
			continue;
		}

		/*	Find our guy	*/
		ant = NULL;
		atual = info->aux->subs_list[i].subscribers;
		while (atual && atual->ptr != info->client_network_info)
		{
			ant = atual;
			atual = atual->next;
		}

		/*	there are no subscribers OR this guy was not subscribed	--> there's nothing to remove	*/
		if (!atual)
		{
			pthread_mutex_unlock(mutex_subscribers + i);
			return 0;
		}

		/*	the element we want to remove is in the begginnig of the list	*/
		if (!ant && atual)
		{
			info->aux->subs_list[i].subscribers = atual->next;
			free(atual);
		}

		/*	the element to remove is somewhere in the middle of the list*/
		else
		{
			ant->next = atual->next;
			free(atual);
		}

		pthread_mutex_unlock(mutex_subscribers + i);
	}

	return 1;
}

void remove_client(struct info4thread *info)
{
	ClientAddress *ant, *atual;

	/*	Remove client's subscriptions	*/
	remove_all_client_subscriptions(info);

	/*	Remove the client from the 'ips' list (don't forget there's a header)	*/

	pthread_mutex_lock(&mutex_ips);

	ant = info->aux->ips;
	atual = ant->next;

	/*	Search for the element before ours	*/
	while(atual && atual != info->client_network_info)
	{
		ant = atual;
		atual = atual->next;
	}

	/*	Just remove	*/
	ant->next = atual->next;
	free(atual);

	pthread_mutex_unlock(&mutex_ips);
	return;
}

void remove_all_client_subscriptions(struct info4thread *info)
{
	char fields[NUM_PUBLIC_FIELDS][SMALL_BUFFER_SIZE] = {
		"all", "cd", "cma", "cmi", "cr", "mr", "ms"
	};
	int i;

	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)	remove_subscription(info, fields[i]);
}


void subscribe_to_field(struct info4thread *info, char *field)
{
	int i;
	struct address_capsule *tmp;

	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)
	{
		pthread_mutex_lock(mutex_subscribers+i);

		if (strcmp(info->aux->subs_list[i].field, field))
		{
			pthread_mutex_unlock(mutex_subscribers+i);
			continue;
		}

		if (not_in(info->client_network_info, info->aux->subs_list[i].subscribers))
		{
			tmp = (struct address_capsule *)malloc(sizeof(struct address_capsule));
			if (tmp)
			{
				tmp->ptr = info->client_network_info;
				tmp->next = info->aux->subs_list[i].subscribers;
				info->aux->subs_list[i].subscribers = tmp;
				pthread_mutex_unlock(mutex_subscribers+i);

				/*	Tell client good news!	*/
				write(info->sfd, "Subscription was successfull!\n", strlen("Subscription was successfull!\n")+1);
				return;
			}
		}
		pthread_mutex_unlock(mutex_subscribers+i);

		write(info->sfd, "You were already subsribed!\n", strlen("You were already subsribed!\n")+1);
		return;
	}
	write(info->sfd, ERR_COMINV, strlen(ERR_COMINV)+1);
}

int not_in(ClientAddress *client_network_info, struct address_capsule *subscribers)
{
	struct address_capsule *tmp;
	tmp = subscribers;

	/*	We already have the mutex for this usbscribers list,
		because not_in was called by subscribe_to field	*/

	while(tmp)
	{
		if (client_network_info == tmp->ptr)
			return 0;
		tmp = tmp->next;
	}

	return 1;
}

void init_subs_list(struct subscribers_list **p)
{
	int i;
	char fields[NUM_PUBLIC_FIELDS][SMALL_BUFFER_SIZE] = {
		"all", "cd", "cma", "cmi", "cr", "mr", "ms"
	};

	*p = (struct subscribers_list *)malloc(sizeof(struct subscribers_list)*NUM_PUBLIC_FIELDS);

	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)
	{
		strcpy((*p)[i].field, fields[i]);
		(*p)[i].subscribers = NULL;
	}
	return;
}


ClientData* search_for_client_data(ClientData *data, char *id)
{
	pthread_mutex_lock(&mutex_data);
	while(data)
	{
		if (!strcmp(data->id, id))
		{
			pthread_mutex_unlock(&mutex_data);
			return data;
		}
		data = data->next;
	}

	pthread_mutex_unlock(&mutex_data);
	return NULL;
}

ClientAddress *add2ips(ClientAddress *ips, struct sockaddr_in *caddr)
{
	ClientAddress *tmp;

	if (!(tmp = (ClientAddress *)malloc(sizeof(ClientAddress))))
		return NULL;

	tmp->port = ntohs(caddr->sin_port);
	strcpy(tmp->ip, inet_ntoa(caddr->sin_addr));

	/*	Add tmp to ips list
	*	Don't forget the header
	*/
	pthread_mutex_lock(&mutex_ips);

	tmp->next = ips->next;
	ips->next = tmp;

	pthread_mutex_unlock(&mutex_ips);

	return tmp;
}

int go_go_thread(ThreadID **tids, struct aux *aux, int sock, ClientAddress *client)
{
	ThreadID *thrid;
	struct info4thread *i4t;

	thrid = (ThreadID *)malloc(sizeof(ThreadID));
	i4t = (struct info4thread *)malloc(sizeof(struct info4thread));

	if (!thrid || !i4t)
	{
		printf("Error on go_go_thread: malloc()\n");
		return -1;
	}

	/*	Add new thread to list
	**	Don't forget the header
	*/
	thrid->info = i4t;

	thrid->next = *tids;
	*tids = thrid;

	/*	Initialize struct to give to thread	*/
	i4t->sfd = sock;
	i4t->aux = aux;
	i4t->client_network_info = client;

	if (pthread_create(&thrid->tid, NULL, client_handler, (void *)i4t))
	{
		printf("Error on go_go_thread: pthread_create()\n");
		return -1;
	}

	return 0;
}

void init_data(ClientData *data)
{
	data->calls_duration = 0.0;
	data->calls_made = 0.0;
	data->calls_missed = 0.0;
	data->calls_rcv = 0.0;
	data->sms_rcv = 0.0;
	data->sms_sent = 0.0;
}


void *subs_thread_routine(void *arg)
{
	struct aux *aux;
	int i;

	aux = (struct aux *)arg;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	/*	De REFRESH_TIME em REFRESH_TIME minutos verifica se os dados mudaram	*/
		// 1) connect to isabela and re-initialize 'data' list

	// Check subscribers, if data changed: create socket, send message, destroy socket

	while(1)
	{
		sleep(REFRESH_TIME);
		//printf("REFRESHING THE DATA\n");

		/*	GET ALL the mutexes	*/
		pthread_mutex_lock(&mutex_data);
		pthread_mutex_lock(&mutex_ips);
		for(i=0; i<NUM_PUBLIC_FIELDS; ++i)
			pthread_mutex_lock(mutex_subscribers + i);

		refresh_data(aux);

		/*	RELEASE ALL the mutexes	*/
		pthread_mutex_unlock(&mutex_data);
		pthread_mutex_unlock(&mutex_ips);
		for(i=0; i<NUM_PUBLIC_FIELDS; ++i)
			pthread_mutex_unlock(mutex_subscribers + i);

		printf("DATA REFRESHED\n");
	}
}
