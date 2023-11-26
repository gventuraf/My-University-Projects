#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <sys/time.h>
#include <signal.h>

#define SMALL_BUFFER_SIZE 16	/*	max size of ip_address to string, with '\0'	*/
#define BUFFER_SIZE 100
#define BIG_BUFFER_SIZE 2048
#define MENU "menu"
#define ON 1
#define OFF 0
#define ERR_IDINV "INVALID_ID"
#define ERR_COMINV "INVALID_COMMAND"
#define QUEUE_MAX_LEN 10
#define NUM_PUBLIC_FIELDS 7
#define EXIT_CODE 4
#define SUBSCRIBE_TO_ALL "all"
#define REFRESH_TIME 20
#define MAKE_SUBSCRIPTION 14
#define REMOVE_SUBSCRIPTION 15

typedef struct sClientData{
	char id[BUFFER_SIZE], type[BUFFER_SIZE], activity[BUFFER_SIZE], location[BUFFER_SIZE];
	char department[BUFFER_SIZE];
	double calls_duration;
	double calls_made, calls_missed, calls_rcv, sms_rcv, sms_sent;
	struct sClientData *next;
} ClientData;

typedef struct sClientAddress{
	char id[BUFFER_SIZE];
	short port, server_port;
	char ip[SMALL_BUFFER_SIZE];
	struct sClientAddress *next;
} ClientAddress;

typedef struct sThreadID{
	pthread_t tid;
	struct info4thread *info;
	struct sThreadID *next;
} ThreadID;

struct aux{
	ClientData *data;
	ClientAddress *ips;
	struct subscribers_list *subs_list;
};

struct info4thread{
	int sfd; /*	socket file descriptor	*/
	ClientAddress *client_network_info;
	ClientData *client_data_info;
	struct aux *aux;
};

struct subscribers_list{
	char field[SMALL_BUFFER_SIZE];	/*	the subscribed field	*/
	struct address_capsule *subscribers;
};

struct address_capsule{
	ClientAddress *ptr;
	struct address_capsule *next;
};

struct pointers{
	ClientData *data;
	ThreadID *tids;
	struct aux *aux;
	struct subscribers_list *subs_list;
	int listening_socket;
};


int go_go_thread(ThreadID **tids, struct aux *aux, int sock, ClientAddress *client);
ClientAddress *add2ips(ClientAddress *ips, struct sockaddr_in *caddr);

ClientData* search_for_client_data(ClientData*, char*);
void thread_cleanup_routine(struct info4thread *info);
int process_client_command(char*, struct info4thread*);
void send_menu(int);
void send_group_data(struct info4thread *info);
void subscribe_to_field(struct info4thread *info, char *field);
void init_subs_list(struct subscribers_list **p);
int remove_subscription(struct info4thread *info, char *field);
void remove_client(struct info4thread *info);
void remove_all_client_subscriptions(struct info4thread *info);
int not_in(ClientAddress *client_network_info, struct address_capsule *subscribers);
int get_data(ClientData *);
void init_data(ClientData *data);
void handle_first_cmd(struct info4thread 	*info);
int send_user_field(char *big_buffer, struct info4thread *info);
void manage_subscriptions(struct info4thread *info);
void show_subscriptions(struct info4thread *info);
void send_all_user_data(struct info4thread *info);
void sigint_handler(int n);
void *termination_routine(void *arg);

void *mini_server(void *port_str);
int see_my_data_routine(int sfd, char *big_buffer);
int manage_subscriptions_routine(int sfd, char *big_buffer);
void sigpipe_handler(int n);

void *subs_thread_routine(void *arg);
void tell_subscribers(struct subscribers_list *subs_list, char *field);
ClientData *is_id_in(const char *new_id, ClientData *data);
void refresh_data(struct aux *aux);

/*TEMPORARIO*/
void preenche(ClientData *p, char *a, char *b, char *c, char *d, char *e, double f, double g, double h, double i, double j, double k);
