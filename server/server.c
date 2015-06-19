#include "server.h"
#include "aux/mandelbrot.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

struct thread_struct {
	char* buffer;
	int length;
	int socket;
};


void *launch_mandelbrot(void *ptr){
	
	char* buffer = ((struct thread_struct*)ptr)->buffer;
	int socket = ((struct thread_struct*)ptr)->socket;

	int widthValue = 0 , heightValue = 0;
	char widthParam[6], heightParam[7];
	char methodName[10];

	unsigned char* outbuffer = NULL;

	//parse the paramters;
//	sscanf(buffer, "%s;%s:%i;%s:%i", methodName, widthParam, &widthValue, heightParam, &heightValue);

	char* token = strtok(buffer, ";");

	strcpy(methodName, token);
	printf("Method Name: %s\n", methodName);

	token = strtok(NULL, ";");

	char* param1 = strdup(token);

	token = strtok(NULL, ";");

	char* param2 = strdup(token);

	token = strtok(param1, ":");
	strcpy(heightParam, token);
	token = strtok(NULL, ":");
	heightValue = atoi(token);

	printf("Param is: %s with value %i.\n", heightParam, heightValue);

	token = strtok(param2, ":");
	strcpy(widthParam, token);
	token = strtok(NULL, ":");
	widthValue = atoi(token);

	printf("Param is: %s with value %i.\n", widthParam, widthValue);
	
	mandelbrot(widthValue, heightValue, 1000, outbuffer);

	int size = widthValue * heightValue * 3 * sizeof(unsigned char);
	printf("The size is: %i.\n", size);
	printf("Result is: %s.\n", outbuffer);

	if(outbuffer == NULL)
		outbuffer = (unsigned char*) calloc(size, sizeof(unsigned char));

	memset(outbuffer, '-', size);

	if(outbuffer != NULL)
		write(socket, outbuffer, size);

	printf("Done sending result.\n");

	free(buffer);
	free(outbuffer);
	free(param1);
	free(param2);
}


void handle_request(int socket, char* buffer, int length){

//	char* copy_buffer = strdup(buffer);

	printf("Intra in handle request.\n");
	if(!strncmp(buffer, "PING", length)){
		printf("Received PING sending ACK.\n");	
		write(socket, "ACK", 4);

	}else if(!strncmp(buffer, "mandelbrot", 10)){

		pthread_t thread;
		int ret;
		printf("Directiva debug.");
		struct thread_struct params;
		params.buffer = strdup(buffer);
		params.socket = socket;
		params.length = length;

		ret = pthread_create(&thread, NULL, launch_mandelbrot, 	&params);
	}

}



