#include "header.h"

void tell_subscribers(struct subscribers_list *subs_list, char *field)
{
	int i, sock;
	struct address_capsule *tmp_subscribers;
	struct sockaddr_in mini_server;
	char buffer[BUFFER_SIZE];

	for (i=0; i<NUM_PUBLIC_FIELDS; ++i)
	{
		if (strcmp(subs_list[i].field, field))
			continue;

		/*	Estamos na lista que queremos	*/

		tmp_subscribers = subs_list[i].subscribers;
		while(tmp_subscribers)
		{
			/*	START: Create a connection to "mini-server" waiting inside the client program	*/
			if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				tmp_subscribers = tmp_subscribers->next;
				continue;	/*	Houve um erro, mas vamos continuar para o próximo cliente	*/
			}
			mini_server.sin_family = AF_INET;
			mini_server.sin_port = htons(tmp_subscribers->ptr->server_port);
			inet_aton(tmp_subscribers->ptr->ip, &mini_server.sin_addr);
			/*	END 'START'*/

			if (connect(sock, (struct sockaddr *)&mini_server, (socklen_t)sizeof(mini_server)) == -1)
			{
				close(sock);
				continue;	/*	Houve um erro, mas vamos continuar para o próximo cliente	*/
			}

			sprintf(buffer, "ATENTION: a change was made to the field '%s' which you are subscribed to\n\n", field);
			write(sock, buffer, strlen(buffer)+1);
			close(sock);


			//printf("\n\nfield = '%s'\n\n", field);

			tmp_subscribers = tmp_subscribers->next;
		}

		/*	Não vale a pena continuar a pesquisa, já temos o que queríamos	*/
		return;
	}

	return;
}

ClientData *is_id_in(const char *new_id, ClientData *data)
{
	while (data)
	{
		if (!strcmp(new_id, data->id))
			return data;
		data = data->next;
	}
	return NULL;
}
