#include "server.h"


void handle_request(int socket, char* buffer, int length){

	printf("Intra in handle request.\n");
	if(!strncmp(buffer, "PING", length)){
		printf("Received PING sending ACK.\n");	
		write(socket, "ACK", 4);

	}else {


	}

}
