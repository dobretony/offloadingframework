#include<stdio.h>
#include<stdlib.h>

#include "comm/bluetooth.h"

int main()
{
	bluetooth_init();

	printf("Starting advertising...\n");
	bluetooth_adv_start(dev_ctl, dev_id);
	sleep(10);
	bluetooth_adv_stop(dev_ctl, dev_id);
	printf("Stopping advertising...\n");

	bluetooth_finalize();

	return 0;
}
