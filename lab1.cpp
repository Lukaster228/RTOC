#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

char * TEXT_PATH;// "/home/bill/study/qnx/text.txt"
char * ENCRYPT_PATH; // "/home/bill/study/qnx/encrypt.txt"
char * DECRYPT_PATH; //"/home/bill/study/qnx/decrypt.txt"
int PTHREAD_NUM; 

pthread_barrier_t bar;



typedef struct keygen_params
{
	int a;
	int m;
	int c;
	int X0;
}keygen_params;

typedef struct crypt_params
{
	char* msg;
	size_t size;
	char* key;
	char* outText;
}crypt_params;

void killTask(char* msg);
void* keyGeneration(void* param);
void* encryption(void* param);


int main(int argc, char *argv[])
{
	for (int i =0; i < argc; i++)
		std::cout << argv[i] << std::endl;

	int len = strlen(argv[1]);
	TEXT_PATH = new char[len];
	TEXT_PATH = argv[1];

	len = strlen(argv[2]);
	ENCRYPT_PATH = new char[len];
	ENCRYPT_PATH = argv[2];

	len = strlen(argv[3]);
	DECRYPT_PATH = new char[len];
	DECRYPT_PATH = argv[3];
	
	char *CMD = new char[1];
	CMD = argv[8];

	PTHREAD_NUM = sysconf(_SC_NPROCESSORS_ONLN);
	pthread_t keygenThread, cryptThread[PTHREAD_NUM];
	keygen_params keygenParams;
	crypt_params cryptParams[PTHREAD_NUM];
	int textFd = 0;
	int encryptFd = 0;
	int decryptFd = 0;
	char *buffer = NULL;
	char *key = NULL;
	struct stat sb;

	memset(&keygenParams, 0, sizeof(keygenParams));
	memset(&cryptParams, 0, sizeof(cryptParams));

	pthread_barrier_init(&bar, NULL, PTHREAD_NUM+1);

	if ((textFd = open(TEXT_PATH, O_RDONLY)) < 0)
		killTask("[-]open failed");

	if (stat(TEXT_PATH, &sb) < 0)
		killTask("[-]stat failed");

	printf("TXT size:%d\n", sb.st_size);
	buffer = new char[sb.st_size+1];
	assert(buffer != NULL);

	keygenParams.a = atoi(argv[4]);
	keygenParams.m = atoi(argv[5]);
	keygenParams.c = atoi(argv[6]);
	keygenParams.X0 = atoi(argv[7]);

	if (pthread_create(&keygenThread, NULL, &keyGeneration, &keygenParams) < 0)
		killTask("[-]pthread_create");

	if (pthread_join(keygenThread, (void**)&key) < 0)
		killTask("[-]failed to wait for keyGeneration thread to finish");

	printf("KEY:[%d]%s\n",strlen(key), key);

	if (read(textFd, buffer, sb.st_size) != sb.st_size)
		killTask("[-]read failed");

	printf("%s\n", buffer);

	int bytesForEachThread = static_cast<int>(sb.st_size / PTHREAD_NUM);
	int remainderForLast = sb.st_size % PTHREAD_NUM;

	int offset = 0;
	for (int i = 0; i < PTHREAD_NUM; i++)
	{
			if (i == PTHREAD_NUM-1)
				bytesForEachThread += remainderForLast;

			cryptParams[i].key = new char[strlen(key)+1];
			assert(cryptParams[i].key != NULL);
			memccpy(cryptParams[i].key, key + offset, bytesForEachThread,sizeof(char)*bytesForEachThread);

			cryptParams[i].msg = new char[bytesForEachThread];
			assert(cryptParams[i].msg != NULL);
			memccpy(cryptParams[i].msg, buffer + offset, bytesForEachThread,sizeof(char)*bytesForEachThread);
			offset += bytesForEachThread;

			cryptParams[i].size = bytesForEachThread;

			if (pthread_create(&cryptThread[i], NULL, &encryption, &cryptParams[i]) < 0)
				killTask("[-]create encryption thread failed");
	}

	pthread_barrier_wait(&bar);
	
	if ((encryptFd = open(ENCRYPT_PATH, O_CREAT | O_RDWR, 0777)) < 0)
		killTask("[-]open encrypt file failed");
	for (int i = 0; i < PTHREAD_NUM; i++)
		if (write(encryptFd, cryptParams[i].outText, sb.st_size !=  sb.st_size))
			killTask("[-]write to encrypt.txt failed");

	printf("ENCRYPTED:");
	for (int i = 0; i < PTHREAD_NUM; i++)
		printf("%s", cryptParams[i].outText);
		printf("\n");
	if (*CMD == 'D'){
	for (int i = 0; i < PTHREAD_NUM; i++)
	{
		memccpy(cryptParams[i].msg, cryptParams[i].outText, sb.st_size,sb.st_size);
		memset(cryptParams[i].outText, 0 , sizeof(cryptParams[i].outText));
		if (pthread_create(&cryptThread[i], NULL, &encryption, &cryptParams[i]) < 0)
			killTask("[-]create decrypt thread failed");
	}

	if ((decryptFd = open(DECRYPT_PATH, O_CREAT | O_RDWR, 0777)) < 0)
			killTask("[-]open decrypt file failed");
	for (int i = 0; i < PTHREAD_NUM; i++)
		if (write(decryptFd, cryptParams[i].outText, sb.st_size !=  sb.st_size))
			killTask("[-]write to decrypt.txt failed");

	pthread_barrier_wait(&bar);
	}
	printf("RESULT   :");
	for (int i = 0; i < PTHREAD_NUM; i++)
			printf("%s", cryptParams[i].outText);
	printf("\n");

	pthread_barrier_destroy(&bar);
	close(textFd);
	close(encryptFd);
	close(decryptFd);
	for (int i = 0; i < PTHREAD_NUM; i++)
	{
		delete[] cryptParams[i].key;
		delete[] cryptParams[i].outText;
		delete[] cryptParams[i].msg;
	}
	delete[] buffer;
	delete[] key;
	return EXIT_SUCCESS;
}

void killTask(char* msg)
{
	perror(msg);
	exit(-1);
}

void* keyGeneration(void* param)
{
	keygen_params* keygenParams = (keygen_params*)param;
	struct stat sb;
	char* key;

	if (stat(TEXT_PATH, &sb) < 0)
		killTask("[-]stat failed");
	key = new char[sb.st_size];

	key[0] = keygenParams->X0;
	for (int i = 0; i < sb.st_size - 1; i++)
		key[i + 1] = (keygenParams->a * key[i] + keygenParams->c) % keygenParams->m;

	pthread_exit(key);
}

void* encryption(void* param)
{
	crypt_params* cryptParams = (crypt_params*)param;
	cryptParams->outText = new char[cryptParams->size + 1];

	for (size_t i = 0; i < cryptParams->size; i++)
		cryptParams->outText[i] = cryptParams->msg[i] ^ cryptParams->key[i];

	pthread_barrier_wait(&bar);
	pthread_exit(0);
}