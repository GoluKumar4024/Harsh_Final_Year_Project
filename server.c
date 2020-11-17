#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>	// for open
#include <unistd.h> // for close
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>

#define SERVERPORT 8989
#define BUFSIZE 4096
#define SOCKETERR (-1)
#define SERVER_BCKLOG 10

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

struct client
{
	int socket, pairsock;
	char mode;
};

char **arr[100];
int ques_cnt[100];
char titles[2048] = "";

void *handle_connection(void *client_socket);
int check(int exp, const char *msg);
void handle_single(int client_socket);
void handle_admin(int client_socket);
void handle_groups(int client_socket);
void sendlist(int client_socket, int ques_cnt[], char titles[], char **arr[]);
void add(char str[], char titles[], char **arr[], int ques_cnt[]);

void add_to_db(char buffer[])
{
	add(buffer, titles, arr, ques_cnt);
}

void sanitize(char s[])
{
	int len = strlen(s);
	len--;
	for (int i = 0; i < len; i++)
	{
		if (s[i] == '$')
		{
			s[i] = '\n';
		}
	}
}

int countChars(char *s, const char *c)
{
	return *s == '\0'
			   ? 0
			   : countChars(s + 1, c) + (*s == *c);
}

size_t string_parser(char *str, char ***word_array, const char *delim)
{

	int init_size = strlen(str);
	size_t sz = countChars(str, delim);
	sz++;

	char *ptr = strtok(str, delim);

	*word_array = malloc(sz * sizeof(char *));
	int i = 0;

	while (ptr != NULL)
	{
		(*word_array)[i] = (char *)malloc(strlen(ptr));
		strcpy((*word_array)[i], ptr);
		ptr = strtok(NULL, delim);
		i++;
	}

	return sz;
}

void showlist(int ques_cnt[], char titles[], char **arr[])
{
	if (strcmp(titles, "") == 0)
	{
		printf("\n");
		return;
	}
	char **topics = NULL;
	int topic_count = string_parser(titles, &topics, ",");
	printf("question counts: ");

	for (int topic = 0; topic < topic_count; topic++)
	{
		printf("%d, ", ques_cnt[topic]);
	}

	for (int topic = 0; topic < topic_count; topic++)
	{
		printf("\n\ntopic %s:\n\n", topics[topic]);
		for (int i = 0; i < ques_cnt[topic]; i++)
		{
			printf("question %d: %s\n", i, arr[topic][i]);
		}
	}
}

int main(int argc, char **argv)
{
	//admin vars
	memset(ques_cnt, 0, sizeof(ques_cnt));
	for (int i = 0; i < 100; i++)
	{
		arr[i] = malloc(100 * sizeof(char *));
	}

	int server_socket, addr_size;
	SA_IN server_addr, client_addr;
	struct client clients[100];

	check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Socket creation failed");
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(SERVERPORT);

	check((bind(server_socket, (SA *)&server_addr, sizeof(server_addr))), "Bind failed");

	check((listen(server_socket, SERVER_BCKLOG)), "Listen failed");

	while (true)
	{
		printf("Waiting for conns...\n");

		addr_size = sizeof(SA_IN);
		int client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *)&addr_size);
		if (client_socket < 0)
		{
			printf("accept failed\n");
		}
		printf("connected!\n");

		pthread_t t;
		int *pclient = malloc(sizeof(int));
		*pclient = client_socket;
		pthread_create(&t, NULL, handle_connection, pclient);
		// handle_connection(client_socket);

		printf("done looping\n");
	}

	return 0;
}

int check(int exp, const char *msg)
{
	if (exp == SOCKETERR)
	{
		perror(msg);
		exit(1);
	}
	return exp;
}

void *handle_connection(void *p_client_socket)
{
	int client_socket = *((int *)p_client_socket);
	free(p_client_socket);
	char client_message[100];
	char response[100];
	char end[100] = "E";

	//recieve init
	recv(client_socket, client_message, 2000, 0);
	printf("init user: %s\n", client_message);

	// Send init
	char *message = malloc(sizeof(client_message) + 20);
	strcpy(message, "Hello Client : ");
	strcat(message, client_message);
	strcat(message, "\n");
	strcpy(response, message);
	free(message);
	send(client_socket, response, sizeof(response), 0);

	memset(response, 0, sizeof(response));
	strcpy(response, "got it\n");

	while (strcmp(client_message, end) != 0)
	{
		memset(client_message, 0, sizeof(client_message));
		recv(client_socket, client_message, 2000, 0);
		printf("recieved mode: %s\n", client_message);

		if (strcmp(client_message, end) == 0)
		{
			memset(response, 0, sizeof(response));
			strcpy(response, "end\0");
		}

		if (strcmp(client_message, "I") == 0)
		{
			handle_single(client_socket);
		}

		if (strcmp(client_message, "A") == 0)
		{
			handle_admin(client_socket);
		}

		if (strcmp(client_message, "G") == 0)
		{
			handle_groups(client_socket);
		}
	}

	printf("Exit socket \n");
	close(client_socket);
	return NULL;
}

void handle_single(int client_socket)
{
	char topics[2048] = "anal,cumshots,masturbation";
	char ques[1000] = "this is a sample question\na. option 1\nb.option 2\n,right,explain is no possibly";
	char client_msg[2048] = "n";

	while (strcmp(client_msg, "r") != 0)
	{
		if (send(client_socket, topics, strlen(topics), 0) < 0)
		{
			printf("Sending topics failed\n");
		}

		memset(client_msg, 0, sizeof(client_msg));
		if (recv(client_socket, client_msg, 2048, 0) < 0)
		{
			printf("topic recieve fail\n");
		}

		printf("topic chosen: %s\n", client_msg);
		if (send(client_socket, ques, strlen(ques), 0) < 0)
		{
			printf("Sending question failed\n");
		}

		memset(client_msg, 0, sizeof(client_msg));
		if (recv(client_socket, client_msg, 2048, 0) < 0)
		{
			printf("loop condition recieve fail\n");
		}
	}
}

void handle_admin(int client_socket)
{
	printf("Got in admin\n");
	char buffer[2048];
	char again[2] = "";
	int ques;
	while (again[0] != 'r')
	{
		memset(again, 0, sizeof(again));
		if (recv(client_socket, again, 2, 0) < 0)
		{
			printf("loop condition recieve fail\n");
		}
		printf("admin recieve %s\n", again);

		if (again[0] == 'r')
		{
			printf("exit admin\n");
			return;
		}
		else if (again[0] == 'a')
		{
			printf("one ques\n");
			memset(buffer, 0, sizeof(buffer));
			if (recv(client_socket, buffer, 2048, 0) < 0)
			{
				printf("question recieve fail\n");
			}

			printf("recieved: %s\n", buffer);
			add_to_db(buffer);
		}
		else if (again[0] == 'b')
		{
			printf("bulk ques\n");
			memset(buffer, 0, sizeof(buffer));
			if (recv(client_socket, buffer, 2048, 0) < 0)
			{
				printf("question recieve fail\n");
			}
			ques = atoi(buffer);
			for (int i = 0; i < ques; i++)
			{
				memset(buffer, 0, sizeof(buffer));
				if (recv(client_socket, buffer, 2048, 0) < 0)
				{
					printf("question recieve fail\n");
				}
				add_to_db(buffer);
				printf("added question: %s", buffer);
				showlist(ques_cnt, titles, arr);
			}
		}

		else if (again[0] == 's')
		{
			printf("show\n");
			sendlist(client_socket, ques_cnt, titles, arr);
		}
	}
}

void sendlist(int client_socket, int ques_cnt[], char titles[], char **arr[])
{

	int lines;
	char buffer[2048] = "";
	if (strcmp(titles, "") == 0)
	{
		strcpy(buffer, "0");
		if (send(client_socket, buffer, strlen(buffer), 0) < 0)
		{
			printf("Sending number of lines failed\n");
		}
		return;
	}
	char **topics = NULL;
	int topic_count = string_parser(titles, &topics, ",");
	lines = topic_count;

	for (int topic = 0; topic < topic_count; topic++)
	{
		lines += ques_cnt[topic];
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", lines);

	if (send(client_socket, buffer, strlen(buffer), 0) < 0)
	{
		printf("Sending number of lines failed\n");
	}
	for (int topic = 0; topic < topic_count; topic++)
	{
		// printf("topic: ");
		memset(buffer, 0, sizeof(buffer));
		strcpy(buffer, topics[topic]);
		printf("buffer is now: %s\n", buffer);

		if (send(client_socket, buffer, strlen(buffer), 0) < 0)
		{
			printf("Sending topic %d failed\n", topic);
		}
		for (int i = 0; i < ques_cnt[topic]; i++)
		{
			memset(buffer, 0, sizeof(buffer));
			strcpy(buffer, arr[topic][i]);
			printf("buffer is now: %s\n", buffer);

			if (send(client_socket, buffer, strlen(buffer), 0) < 0)
			{
				printf("Sending topic %d question %d failed\n", topic, i);
			}
		}
	}
}

void add(char str[], char titles[], char **arr[], int ques_cnt[])
{

	// parse question
	sanitize(str);
	char **ques = NULL;
	string_parser(str, &ques, ",");

	int flg = 0;
	int topic_count = 0;
	char db_entry[2048] = "";
	strcpy(db_entry, ques[1]);
	strcat(db_entry, ",");
	strcat(db_entry, ques[2]);
	strcat(db_entry, ",");
	strcat(db_entry, ques[3]);

	// parse available topics
	if (strcmp(titles, "") != 0) //titles != ""
	{
		char **topics = NULL;
		char title_cop[2048];

		strcpy(title_cop, titles);
		topic_count = string_parser(title_cop, &topics, ",");

		//if topic already present, add to index
		for (int i = 0; i < topic_count; i++)
		{
			if (strcmp(topics[i], ques[0]) == 0)
			{
				flg = 1;
				(arr[i])[ques_cnt[i]] = (char *)malloc(strlen(db_entry));
				strcpy((arr[i])[ques_cnt[i]], db_entry);
				ques_cnt[i]++;
			}
		}

		for (size_t i = 0; i < topic_count; i++)
		{
			free(topics[i]);
		}
		free(topics);
	}

	//else add topic, add new index
	if (flg == 0)
	{
		if (strcmp(titles, "") != 0)
		{
			strcat(titles, ",");
		}
		strcat(titles, ques[0]);
		arr[topic_count] = malloc(100 * sizeof(char *));
		(arr[topic_count])[ques_cnt[topic_count]] = (char *)malloc(strlen(db_entry));
		strcpy((arr[topic_count])[ques_cnt[topic_count]], db_entry);
		ques_cnt[topic_count]++;
	}

	for (size_t i = 0; i < 4; i++)
	{
		free(ques[i]);
	}
	free(ques);
}

void handle_groups(int client_socket){

	
}
