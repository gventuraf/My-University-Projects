/****************************************************
*                                                   *
*			Operating Systems Project               *
*                                                   *
*			Submission Date: 08/12/2018             *
*             										*
*                                                   *
*****************************************************/


#include "header.h"

extern sem_t *sem_log;

// armazem central
void log_ecra(char *s)
{
	FILE *fp;
	time_t seconds;
	struct tm curr_time;

	/*	FICA COM SEMÁFORO	*/
	sem_wait(sem_log);


	fp = fopen(LOG_FILE, "a");

	seconds = time(NULL);
	curr_time = *localtime(&seconds);

	fprintf(fp, "%02d:%02d:%02d %s\n", curr_time.tm_hour, curr_time.tm_min, curr_time.tm_sec, s);
	fclose(fp);

	printf("%02d:%02d:%02d %s\n", curr_time.tm_hour, curr_time.tm_min, curr_time.tm_sec, s);

	sem_post(sem_log);

	return;
}
// armazem central
void *attach_shm(int type)
{
	key_t key;
	int id;

	if ((key = ftok(KEYS_FILE, type)) == -1)
		return (void *)(-1);

	if ((id = shmget(key, 0, 0)) == -1)
		return (void *)(-1);


	return shmat(id, NULL, 0);
}

//	armazem e central
int get_msgQid()
{
	key_t key;

	key = ftok(KEYS_FILE, MQID);
	if (key == -1)
		return -1;
	return msgget(key, 0);
}
