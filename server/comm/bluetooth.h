#ifndef OFFLOAD_BLUETOOTH_HEADER
#define OFFLOAD_BLUETOOTH_HEADER

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
#include <bluetooth/sdp.h>
#include <bluetooth/sco.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/uuid.h>
//#include <bluetooth/l2cap.h>



#define ADVERTISING_PACKET "10 02 01 1a 0c ff 18 01 48 45 4c 4c 4f 57 4f 52 4c 44"
#define SERVICE_UUID "9ba5c4d1-0b68-4717-8f79-45aa6b418666"


struct sockaddr_l2 {
        sa_family_t l2_family;
        unsigned short l2_psm;
        bdaddr_t l2_bdaddr;
        unsigned short l2_cid;
        uint8_t l2_bdaddr_type;
};

#define L2CAP_CONNINFO  0x02
struct l2cap_conninfo {
  uint16_t       hci_handle;
  uint8_t        dev_class[3];
};

extern int dev_id; /*device id that is to be used in the server. initialized in bluetooth.c*/
extern struct hci_dev_info di;
extern int dev_ctl;
extern int l2cap_socket;
extern struct sockaddr_l2 sockAddr;
extern int rfcomm_socket;
extern struct sockaddr_rc loc_addr; 

int bluetooth_init();
int bluetooth_finalize();
int get_dev_id(int ctl);
int init_l2cap_sock(int dev_id);
int bluetooth_adv_start(int ctl, int hdev);
int bluetooth_adv_stop(int ctl, int hdev);
int send_adv_package();
void bring_up(int ctl, int hdev);
void bring_down(int ctl, int hdev);
void set_adv_packet(int dev_id, const char* adv_string);
int init_rfcomm_sock(int dev_id);
sdp_session_t *register_service();

#endif //OFFLOAD_BLUETOOTH_SERVER
