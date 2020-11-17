#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>	// for open
#include <unistd.h> // for close
#include <pthread.h>
#include <ctype.h>

char *userid;
int clientSocket;
char *rightAns;
char *explain;

void handle_single();
void handle_admin();
void handle_groups();
void put_question();

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

void *clientThread(void *arg)
{
	char message[1000];
	char buffer[1024];
	char end[] = "end";

	memset(message, 0, sizeof(message));
	memset(buffer, 0, sizeof(buffer));

	//send init
	strcpy(message, userid);
	printf("%s\n", message);
	if (send(clientSocket, message, strlen(message), 0) < 0)
	{
		printf("Send failed\n");
	}

	//recieve init
	if (recv(clientSocket, buffer, 1024, 0) < 0)
	{
		printf("Receive failed\n");
	}
	//Print init
	printf("%s\n", buffer);

	while (strcmp(buffer, end) != 0)
	{
		memset(message, 0, sizeof(message));
		memset(buffer, 0, sizeof(buffer));
		printf("Select Mode:\n- 'I' for individual mode\n- 'G' for group mode\n- 'A' for admon mode\n- 'E' to exit\n");
		scanf("%s", message);

		//send user choice of mode
		if (send(clientSocket, message, strlen(message), 0) < 0)
		{
			printf("Send failed\n");
		}

		if (strcmp(message, "I") == 0)
		{
			handle_single();
		}

		if (strcmp(message, "A") == 0)
		{
			handle_admin();
		}

		if (strcmp(message, "G") == 0)
		{
			// handle_groups();
		}
	}

	close(clientSocket);
	printf("exiting\n");
	return NULL;
}

int main(int *argc, char **argv)
{
	//userid
	userid = argv[1];

	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	// Create the socket.
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);
	//Configure settings of the server address
	// Address family is Internet
	serverAddr.sin_family = AF_INET;
	//Set port number, using htons function
	serverAddr.sin_port = htons(8989);
	//Set IP address to localhost
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
	//Connect the socket to the server using the address
	addr_size = sizeof serverAddr;
	connect(clientSocket, (struct sockaddr *)&serverAddr, addr_size);

	clientThread(NULL);
	printf("done with %d\n", 0);
	return 0;
}

void handle_single()
{
	char buffer[2048];
	char **word_array = NULL;
	char userChoice[] = "n";
	char userAns[100];

	while (strcmp(userChoice, "r") != 0)
	{
		printf("\n\nPick a topic from the following:\n");

		//get topics and print
		memset(buffer, 0, sizeof(buffer));
		if (recv(clientSocket, buffer, 2048, 0) < 0)
		{
			printf("Receive failed\n");
		}
		size_t n = string_parser(buffer, &word_array, ",");
		int a = 'a';
		for (size_t i = 0; i < n; i++)
		{
			printf("%c %s\n", (int)(a + i), word_array[i]);
		}
		for (size_t i = 0; i < n; i++)
		{
			free(word_array[i]);
		}
		free(word_array);

		//get input of topic
		memset(buffer, 0, sizeof(buffer));
		scanf("%s", buffer);
		//send topic
		if (send(clientSocket, buffer, strlen(buffer), 0) < 0)
		{
			printf("Send topic failed\n");
		}

		//receive question
		memset(buffer, 0, sizeof(buffer));
		if (recv(clientSocket, buffer, 2048, 0) < 0)
		{
			printf("Receive failed\n");
		}
		//parse question
		n = string_parser(buffer, &word_array, ",");
		explain = word_array[n - 1];
		rightAns = word_array[n - 2];

		//print ques
		printf("\n%s\n", word_array[n - 3]);
		printf("Enter your answer: ");
		scanf("%s", userAns);
		if (strcmp(userAns, rightAns) == 0)
		{
			printf("\nCorrect answer. The explanation is below:\n");
		}
		else
		{
			printf("\nWrong answer. The explanation is below:\n");
		}
		printf("%s\n\n\n", explain);
		//free memory
		for (size_t i = 0; i < n; i++)
		{
			free(word_array[i]);
		}
		free(word_array);

		printf("To attempt another question enter 'n'. To return to main menu enter 'r'\n*************************************\n\n");
		memset(userChoice, 0, sizeof(userChoice));
		scanf("%s", userChoice);
		if (send(clientSocket, userChoice, 1, 0) < 0)
		{
			printf("send loop condition fail\n");
		}
	}
}

void handle_admin()
{
	char again[1] = "a";
	int numques;
	char buff[2048];
	while (again[0] != 'r')
	{
		printf("- Press a to add question\n- Press b to add questions in bulk\n- Press 's' to show all questions\n- Press 'r' to go to main menu\n");
		scanf("%s", again);

		// send user choice
		memset(buff, 0, sizeof(buff));
		strcpy(buff, again);
		if (send(clientSocket, buff, strlen(buff), 0) < 0)
		{
			printf("choice sending failed\n");
		}

		// check option
		if (again[0] == 'r')
		{
			return;
		}
		else if (again[0] == 'a')
		{
			memset(buff, 0, sizeof(buff));
			scanf("%s", buff);
			if (send(clientSocket, buff, strlen(buff), 0) < 0)
			{
				printf("question send failed\n");
			}
		}
		else if (again[0] == 'b')
		{
			//send message to add in bulk
			scanf("%d", &numques);
			for (int i = 0; i < numques; i++)
			{
				memset(buff, 0, sizeof(buff));
				scanf("%s", buff);
				if (send(clientSocket, buff, strlen(buff), 0) < 0)
				{
					printf("bulk send failed at ques %d\n", i);
				}
			}
		}
		else if (again[0] == 's')
		{
			//send message to show all questions
			//recieve number of lines
			memset(buff, 0, sizeof(buff));
			if (recv(clientSocket, buff, 2048, 0) < 0)
			{
				printf("Receive failed\n");
			}
			numques = atoi(buff);
			printf("received: %d lines\n", numques);
			//for lines
			for (int i = 0; i < numques; i++)
			{
				memset(buff, 0, sizeof(buff));
				//recieve and print line
				if (recv(clientSocket, buff, 2048, 0) < 0)
				{
					printf("Receive failed at line %d\n", i);
				}
				else
				{
					printf("received: %s\n", buff);
				}
			}
		}

		else
		{
			printf("Please enter valid choice\n");
		}
	}
}
