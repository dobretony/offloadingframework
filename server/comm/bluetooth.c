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

int dev_id = -1;
//static struct hci_dev_info di;


static int dev_info(int s, int dev_id, long arg)
{
        struct hci_dev_info di = { .dev_id = dev_id };
        char addr[18];

        if (ioctl(s, HCIGETDEVINFO, (void *) &di))
                return 0;

        ba2str(&di.bdaddr, addr);
        printf("\t%s\t%s\n", di.name, addr);
        return 0;
}



/*Here we check if Bluetooth is enabled, enable it and then initialize the adverising parameters.*/
int bluetooth_init()
{
	int opt, ctl, i, cmd = 0;
	/* Open HCI socket - check to see if bluetooth exists. */
        if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
                perror("Can't open HCI socket.");
                exit(1);
        }

	dev_id = get_dev_id(ctl);
	printf("%i ", dev_id);

}


int get_dev_id(int ctl)
{
	struct hci_dev_list_req *dl;
        struct hci_dev_req *dr;
        int i;
	struct hci_dev_info di;

        if (!(dl = malloc(HCI_MAX_DEV * sizeof(struct hci_dev_req) +
                sizeof(uint16_t)))) {
                perror("Can't allocate memory");
                exit(1);
        }
        dl->dev_num = HCI_MAX_DEV;
        dr = dl->dev_req;

        if (ioctl(ctl, HCIGETDEVLIST, (void *) dl) < 0) {
                perror("Can't get device list");
                exit(1);
        }

	if(dl->dev_num != 0){//take the first one of the list.
                di.dev_id = dr->dev_id;
                if (ioctl(ctl, HCIGETDEVINFO, (void *) &di) < 0)
                {
			perror("Could not get device info.");
			exit(1);
		}
                return dr->dev_id;
        }else{
		perror("No hardware device was found.");
		exit(1);
	}

}


/*This should be started on a different thread. Always send adveritising packets.*/
int bluetooth_adv_loop()
{


}


int send_adv_package()
{



}


void cmd_up(int ctl, int hdev, char *opt)
{
        /* Start HCI device */
        if (ioctl(ctl, HCIDEVUP, hdev) < 0) {
                if (errno == EALREADY)
                        return;
                fprintf(stderr, "Can't init device hci%d: %s (%d)\n",
                                                hdev, strerror(errno), errno);
                exit(1);
        }
}

void cmd_down(int ctl, int hdev, char *opt)
{
        /* Stop HCI device */
        if (ioctl(ctl, HCIDEVDOWN, hdev) < 0) {
                fprintf(stderr, "Can't down device hci%d: %s (%d)\n",
                                                hdev, strerror(errno), errno);
                exit(1);
        }
}

/*Send arbitrary hci commands, taken from hcitool.*/
/*This is used to set the adveritisng data.*/
void cmd_cmd(int dev_id, int argc, char **argv)
{
        unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr = buf;
        struct hci_filter flt;
        hci_event_hdr *hdr;
        int i, opt, len, dd;
        uint16_t ocf;
        uint8_t ogf;

//        for_each_opt(opt, cmd_options, NULL) {
//                switch (opt) {
//                default:
//                        printf("%s", cmd_help);
//                        return;
//                }
//       }
//       helper_arg(2, -1, &argc, &argv, cmd_help);

        if (dev_id < 0)
                dev_id = hci_get_route(NULL);

        errno = 0;
        ogf = strtol(argv[0], NULL, 16);
        ocf = strtol(argv[1], NULL, 16);
        if (errno == ERANGE || (ogf > 0x3f) || (ocf > 0x3ff)) {
//                printf("%s", cmd_help);
                return;
        }

        for (i = 2, len = 0; i < argc && len < (int) sizeof(buf); i++, len++)
                *ptr++ = (uint8_t) strtol(argv[i], NULL, 16);

        dd = hci_open_dev(dev_id);
        if (dd < 0) {
                perror("Device open failed");
                exit(EXIT_FAILURE);
        }

        /* Setup filter */
        hci_filter_clear(&flt);
        hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
        hci_filter_all_events(&flt);
        if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
                perror("HCI filter setup failed");
                exit(EXIT_FAILURE);
        }

        printf("< HCI Command: ogf 0x%02x, ocf 0x%04x, plen %d\n", ogf, ocf, len);
//        hex_dump("  ", 20, buf, len); fflush(stdout);

        if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
                perror("Send failed");
                exit(EXIT_FAILURE);
        }

        len = read(dd, buf, sizeof(buf));
	if (len < 0) {
                perror("Read failed");
                exit(EXIT_FAILURE);
        }

        hdr = (void *)(buf + 1);
        ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
        len -= (1 + HCI_EVENT_HDR_SIZE);

        printf("> HCI Event: 0x%02x plen %d\n", hdr->evt, hdr->plen);
//        hex_dump("  ", 20, ptr, len); fflush(stdout);

        hci_close_dev(dd);
}




