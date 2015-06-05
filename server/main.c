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
	
	start_main_loop();



	bluetooth_adv_stop(dev_ctl, dev_id);
	printf("Stopping advertising...\n");
	bluetooth_finalize();
	return 0;
}
