#include "server.h"


void handle_request(int socket, char* buffer, int length){


	if(strncmp(buffer, "PING", length)){
		
		write(socket, "ACK", 4);

	}else {


	}

}
