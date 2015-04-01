#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define ADVERTISING_PACKET "10 02 01 1a 0c ff 18 01 48 45 4c 4c 4f 57 4f 52 4c 44"


extern int dev_id; /*device id that is to be used in the server. initialized in bluetooth.c*/
extern struct hci_dev_info di;
extern int dev_ctl;


int bluetooth_init();
int get_dev_id(int ctl);
int bluetooth_adv_start(int ctl, int hdev);
int bluetooth_adv_stop(int ctl, int hdev);
int send_adv_package();
void bring_up(int ctl, int hdev);
void bring_down(int ctl, int hdev);
void set_adv_packet(int dev_id, const char* adv_string);

