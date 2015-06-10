#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>

#include "comm/bluetooth.h"

void intHandler(int signal)
{
	bluetooth_adv_stop(dev_ctl,dev_id);
	printf("Stopping advertising...\n");
	bluetooth_finalize();
	exit(0);
}

void start_main_loop_rfcomm(){
	
	int result = -1;
	int i = 0;
	fd_set afds;
	fd_set rfds;
	
	struct timeval tv;
	int clientRfSock;

	struct sockaddr_rc rem_addr = {0};	
	socklen_t rfcommConnInfoLen;
	socklen_t sockAddrLen;
	bdaddr_t clientBdAddr;
	
	printf("Starting main loop and listening for rfcomm sockets..\n");
	while(1){
		FD_ZERO(&afds);
		FD_SET(rfcomm_socket, &afds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		result = select(FD_SETSIZE, &afds, NULL, NULL, &tv);
/*
		if(result == -1 ){
			printf("Could not select socket.\n");
			break;
		}else if(result && FD_ISSET(rfcomm_socket, &afds)){
			sockAddrLen = sizeof(rem_addr);
			clientRfSock = accept(rfcomm_socket, (struct sockaddr *) &rem_addr, &sockAddrLen);

			baswap(&clientBdAddr, &rem_addr.rc_bdaddr);
			printf("accept %s\n", batostr(&clientBdAddr));

			while(1){
				FD_ZERO(&rfds);
				FD_SET(0, &rfds);
				FD_SET(clientRfSock, &rfds);

                                tv.tv_sec = 1;
                                tv.tv_usec = 0;

                                result = select(clientRfSock + 1, &rfds, NULL, NULL, &tv);
					
				char buffer[1024];
				int read_bytes = read(rfcomm_socket, buffer, 1024);
				printf("%s\n", buffer);

                                if(result){
                                        printf("We got liftoff.\n");
                                }else{
					printf("Something went wrong.\n");
				}


			}
		}
*/


		if(result < 0){
			printf("select doesn't work.\n");
			break;
		}

		for(i = 0; i < FD_SETSIZE; ++i)
			if(FD_ISSET(i, &afds))
			{
				if ( i == rfcomm_socket)
				{
					/*Connection Request on original socket */
					sockAddrLen = sizeof(rem_addr);
					clientRfSock = accept(rfcomm_socket, (struct sockaddr *) &rem_addr, &sockAddrLen);
					if(clientRfSock < 0)
						{printf("Error accepting a socket."); break;}
					FD_SET(clientRfSock, &afds);
				}
				else
				{
					/*Data arriving on already connected socket*/
					char buffer[1024];
        	                        int read_bytes = read(i, buffer, 1024);
	                                printf("%s\n", buffer);
					close(i);
				}

			}
	}

}


void start_main_loop(){

	int result = -1;

	fd_set afds;
	fd_set rfds;
	struct timeval tv;
	int clientL2capSock;
	struct l2cap_conninfo l2capConnInfo;
	socklen_t l2capConnInfoLen;
	socklen_t sockAddrLen;	
	bdaddr_t clientBdAddr;
	int hciHandle;

	while(1){
		FD_ZERO(&afds);
		FD_SET(l2cap_socket, &afds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		result = select(l2cap_socket, &afds, NULL, NULL, &tv);
		
		if(result == -1){
			printf("Could not select socket.\n");
			break;
		}else if(result && FD_ISSET(l2cap_socket, &afds)){
			sockAddrLen = sizeof(sockAddr);

			clientL2capSock = accept(l2cap_socket, (struct sockaddr *) &sockAddr, &sockAddrLen);
			
			baswap(&clientBdAddr, &sockAddr.l2_bdaddr);
			printf("accept %s\n", batostr(&clientBdAddr));

			l2capConnInfoLen = sizeof(l2capConnInfo);
			getsockopt(clientL2capSock, SOL_L2CAP, L2CAP_CONNINFO, &l2capConnInfo, &l2capConnInfoLen);
			hciHandle = l2capConnInfo.hci_handle;


			while(1){
				FD_ZERO(&rfds);
				FD_SET(0, &rfds);
				FD_SET(clientL2capSock, &rfds);
				
				tv.tv_sec = 1;
				tv.tv_usec = 0;

				result = select(clientL2capSock + 1, &rfds, NULL, NULL, &tv);

				if(result){
					printf("We got liftoff.\n");
				}

			}
		}
	}
}

int main()
{
	//register closinf signal
	signal(SIGINT, intHandler);
	signal(SIGKILL, intHandler);
//	signal(SIGHUP, intHandler);

	
	//init and start bluetooth le for advertising
	bluetooth_init();
	printf("Starting advertising...\n");
	bluetooth_adv_start(dev_ctl, dev_id);
	//sleep(10);
	
	start_main_loop_rfcomm();


	bluetooth_adv_stop(dev_ctl, dev_id);
	printf("Stopping advertising...\n");
	bluetooth_finalize();
	return 0;
}
