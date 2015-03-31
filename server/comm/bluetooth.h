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


extern int dev_id; /*device id that is to be used in the server. initialized in bluetooth.c*/
int bluetooth_init();
int get_dev_id(int ctl);
int bluetooth_adv_loop();
int send_adv_package();
void cmd_up(int ctl, int hdev, char* opt);
void cmd_down(int ctl, int hdev, char* opt);
void cmd_cmd(int dev_id, int argc, char** argv);

