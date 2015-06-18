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

	int widthValue, heightValue;
	char widthParam[6], heightParam[7];
	char methodName[10];

	char* outbuffer = NULL;

	//parse the paramters;
	sscanf(buffer, "%s;%s:%i;%s:%i", methodName, widthParam, &widthValue, heightParam, &heightValue);
	mandelbrot(widthValue, heightValue, 1000, outbuffer);

	if(outbuffer != NULL)
		write(socket, outbuffer, widthValue * heightValue * sizeof(char));

	free(buffer);
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

		struct thread_struct params;
		params.buffer = strdup(buffer);
		params.socket = socket;
		params.length = length;

		ret = pthread_create(&thread, NULL, launch_mandelbrot, 	&params);
	}

}



