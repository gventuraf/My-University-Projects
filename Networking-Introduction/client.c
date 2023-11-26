/*******************************************************************
**																   **
**    Project: Introduction to Computer Networking		           **
**                                                                 **
**    Submission Date: December 6, 2018             		       **
**                                                                 **
*******************************************************************/

/*		Aplicação: Cliente		*/


#include "header.h"

pthread_t tid_mini_server;
int sfd; /*	socket file descriptor	*/
int sock; /*	mini-server listening socket*/

int main(int argc, char **argv)
{
	struct sockaddr_in server_info; /*	structure with server's server_info*/
	char buffer[BUFFER_SIZE], big_buffer[BIG_BUFFER_SIZE];
	int port, myserver_port, show_menu, k;
	fd_set rfds;	/*	read file descriptor set*/
	struct timeval timeout;
	sigset_t signal_set;

	/*	Ignore all signals	*/
	sigfillset(&signal_set);
	sigdelset(&signal_set, SIGPIPE);
	sigprocmask(SIG_SETMASK, &signal_set, NULL);
	signal(SIGPIPE, sigpipe_handler);

	/*	Initialize buffer	*/
	buffer[0] = '\0';

	/*	Check for errors	*/
	if (argc != 4)
	{
		printf("Error!\n\n\tSyntax: \"client.out <server IP address> <server port> <client port>\"\n\n");
		return 0;
	}

	/*	Create tcp socket	*/
	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		printf("Error on socket creation\n");
		return 0;
	}

	/*	Initialize server_info struct*/
	server_info.sin_family = AF_INET;

	/*	Check if port number is invalid	*/
	port = (int)strtol(argv[2], NULL, 10);
	if (port <= 1024 || port > 65536)
	{
		printf("Error: Invalid port number\n");
		close(sfd);
		return 0;
	}
	/*	<=1024 are for specific apps
	**	65536 > 16 bits
	*/
	/*	End port-checking	*/

	server_info.sin_port = htons((short)port);

	if (!inet_aton(argv[1], &server_info.sin_addr))
	{
		printf("Invalid IP adress argv[1]\n");
		close(sfd);
		return 0;
	}
	/*Initialization done*/

	/*	Check if myserver_port is valid	*/
	myserver_port = (int)strtol(argv[3], NULL, 10);
	if (myserver_port <= 1024 || myserver_port > 65536)
	{
		printf("Error: invalid myserver port number\n");
		close(sfd);
		return 0;
	}
	/*END checking: is valid	*/

	/*	Connect to server	*/
	if (connect(sfd, (struct sockaddr *)&server_info, (socklen_t)sizeof(server_info)) == -1)
	{
		printf("Can't connect to server\n");
		close(sfd);
		return 0;
	}

	/*	All set-up! Can now talk to server	*/

	/*	Give server your user id	*/
	do
	{
		printf("\n");

		FD_ZERO(&rfds);
		FD_SET(sfd, &rfds);
		timeout.tv_sec = 3;	//wait-time is 3 seconds to prevent deadlock
		timeout.tv_usec = 0;

		if (select(sfd+1, &rfds, NULL, NULL, &timeout) > 0)
			read(sfd, buffer, BUFFER_SIZE);
		else
		{
			close(sfd);
			return 0;
		}

		printf("\n");

		printf("%s", buffer);
		scanf("%s", buffer);

		write(sfd, buffer, strlen(buffer)+1);

		if (!strcmp(buffer, "exit"))
		{
			close(sfd);
			return 0;
		}

		if (read(sfd, buffer, BUFFER_SIZE) <= 0)
		{
			close(sfd);
			return 0;
		}

		//fflush(stdout);

	} while(!strcmp(buffer, ERR_IDINV));

	/*	Let's launch the mini-server	*/
	pthread_create(&tid_mini_server, NULL, mini_server, (void *)argv[3]);

	/*	Server recognizes you, let's tell him the listening port number (myserver_port)	*/
	write(sfd, argv[3], strlen(argv[3])+1);

	/*	Clear shell	*/
	system("clear");

	/* Server accepted your id, please continue	*/
	show_menu = ON;
	while ( ((int)strtol(buffer, NULL, 10)) != EXIT_CODE)
	{
		system("clear");
		buffer[0] = '\0';
		/*	Present Menu	*/
		if (show_menu == ON)
		{
			show_menu = OFF;

			/*	Ask server to show menu	*/
			write(sfd, MENU, strlen(MENU)+1);

			if (read(sfd, big_buffer, BIG_BUFFER_SIZE) <= 0)
				kill(getpid(), SIGPIPE);

			/*	Present menu to client	*/
			printf("%s", big_buffer);
		}

		printf("\nInsert command (type 'menu' for menu): ");

		/*	Read client command	*/
		scanf("%s", buffer);
		k = (int)strtol(buffer, NULL, 10);


		/*	Was the command Exit?	*/
		if (k == EXIT_CODE)
		{
			write(sfd, buffer, strlen(buffer)+1);
			continue;
		}

		/*	Show menu?	*/
		if (!strcmp(buffer, MENU))
		{
			show_menu = ON;
			continue;
		}

		/*	Send command to server	*/
		write(sfd, buffer, strlen(buffer)+1);

		/*	Read server's response	*/
		if (read(sfd, big_buffer, BIG_BUFFER_SIZE) <= 0)
			kill(getpid(), SIGPIPE);

		/*	IF command is invalid THEN	*/
		if (!strcmp(big_buffer, ERR_COMINV))
		{
			printf("That's not a valid command OR wrong syntax\n");
			continue;
		}

		/*	If user chose command number one	*/
		if (k == 1)
		{
			if (see_my_data_routine(sfd, big_buffer))
			{
				show_menu = ON;
				continue;
			}
		}

		/*	If user chose command number three	*/
		else if (k == 3)
		{
			if (manage_subscriptions_routine(sfd, big_buffer))
			{
				show_menu = ON;
				continue;
			}
		}

		/*	If user chose comamnd number two	*/
		else
		{
			system("clear");
			printf("%s\n", big_buffer);
		}

		printf("Type <enter> to go on!");
		getchar(); getchar();

	}

	/*	Close app */
	kill(getpid(), SIGPIPE);
	return 0;
}

int see_my_data_routine(int sfd, char *server_reply)
{
	char buffer[BUFFER_SIZE], copy_server_reply[BIG_BUFFER_SIZE];

	system("clear");
	strcpy(copy_server_reply, server_reply);

	printf("\n%s\nCommand: ", server_reply);
	scanf("%s", buffer);

	write(sfd, buffer, strlen(buffer)+1);

	if (!strcmp(buffer, MENU))
		return 1;


	if (read(sfd, server_reply, BIG_BUFFER_SIZE) <= 0)
		kill(getpid(), SIGPIPE);

	system("clear");

	if (!strcmp(server_reply, ERR_COMINV))
	{
		printf("\nInvalid Command\n\n");
		return 0;
	}

	printf("%s\n", server_reply);

	return 0;
}

int manage_subscriptions_routine(int sfd, char *server_reply)
{
	char buffer[BUFFER_SIZE];
	int ncmd;

	system("clear");

	printf("\n%s\nCommand: ", server_reply);
	scanf("%s", buffer);

	write(sfd, buffer, strlen(buffer)+1);

	ncmd = (int)strtol(buffer, NULL, 10);

	if (ncmd == 4)	/*	User asked to see menu	*/
		return 1;

	if (read(sfd, server_reply, BIG_BUFFER_SIZE) <= 0)
		kill(getpid(), SIGPIPE);

	system("clear");

	if (!strcmp(server_reply, ERR_COMINV))
	{
		printf("\nInvalid Command\n\n");
		return 0;
	}

	if (ncmd == 1)
	{
		printf("\n%s\n", server_reply);
		return 0;
	}

	printf("\n%s", server_reply),
	scanf("%s", buffer);
	write(sfd, buffer, strlen(buffer)+1);
	//sleep(1);	/*To give the user time to see what happened	*/

	system("clear");

	if (read(sfd, server_reply, BIG_BUFFER_SIZE) <= 0)
		kill(getpid(), SIGPIPE);

	if (!strcmp(server_reply, ERR_COMINV))
	{
		printf("\nInvalid Command\n\n");
		return 0;
	}

	printf("\n%s\n", server_reply);

	return 0;
}






void *mini_server(void *port_str)
{
	int port, comsock;
	struct sockaddr_in saddr, caddr;
	int caddr_len;
	char buffer[BUFFER_SIZE];

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	/*	Create socket	*/
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		//printf("Error on socket creation\n");
		pthread_exit(NULL);
	}

	/*	Initialize saddr	*/
	saddr.sin_family = AF_INET;

	port = (int)strtol((char *)port_str, NULL, 10);
	saddr.sin_port = htons((short)port);

	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*	Initialization DONE	*/

	/*	Bind	*/
	if (bind(sock, (struct sockaddr *)&saddr, (socklen_t)sizeof(saddr)) == -1)
	{
		//printf("Error on bind()\n");
		close(sock);
		pthread_exit(NULL);
	}

	if (listen(sock, 1) == -1)
	{
		//printf("Error on listen()\n");
		close(sock);
		pthread_exit(NULL);
	}

	while(1)
	{
		caddr_len = sizeof(caddr);
		comsock = accept(sock, (struct sockaddr *)&caddr, (socklen_t *)&caddr_len);

		if (comsock == -1)
		{
			close(sock);
			pthread_exit(NULL);
		}

		if (read(comsock, buffer, BUFFER_SIZE) <= 0)
			kill(getpid(), SIGPIPE);

		close(comsock);
		printf("\n\n--------------------------------\n%s\n--------------------------------\nInsert command:\n\t", buffer);
	}
}

void sigpipe_handler(int n)
{
	/*	Close 'main' socket */
	close(sfd);

	/*	Close thread and it's socket	*/
	pthread_cancel(tid_mini_server);
	pthread_join(tid_mini_server, NULL);
	close(sock);

	system("clear");

	printf("\n\n**The app will close within 5 seconds!**\n");
	printf("\nPossible reasons:\n\t1) You chose to leave\n\t2) The server disconnected\n");
	sleep(5);

	system("clear");
	exit(0);
}
