#include "bluetooth.h"

#define RFCOMM_CHANNEL 11

#define ATT_CID 4


int dev_id = -1;
struct hci_dev_info di;
int dev_ctl = -1;
int l2cap_socket = -1;
struct sockaddr_l2 sockAddr;
int rfcomm_socket = -1;
struct sockaddr_rc loc_addr = {0};

static int dev_info(int s, int dev_id)
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
	char* command;

	/* Open HCI socket - check to see if bluetooth exists. */
        if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
                perror("Can't open HCI socket.");
                exit(1);
        }

	dev_ctl = ctl;
	dev_id = get_dev_id(ctl);
	printf("Succesfully found a Bluetooth Interface:\n");
	dev_info(ctl, dev_id);
	
	//reset the interface in order to make sure it is on and stable
	bring_down(ctl, dev_id);
	bring_up(ctl, dev_id);
	printf("Succesfully restarted the interface.\n");

	//configure the advertisment packets that identify this server
	set_adv_packet(dev_id, ADVERTISING_PACKET);
	printf("Succesfully set the Advertising data to %s.\n", ADVERTISING_PACKET);

	register_service();

	//init the l2cap socket that will listen for LE connections
	//init_l2cap_sock(dev_id);	
	init_rfcomm_sock(dev_id);
}

int bluetooth_finalize()
{
	bring_down(dev_ctl,dev_id);
	close(l2cap_socket);
	close(rfcomm_socket);
}


int init_l2cap_sock(int dev_id)
{
	
	bdaddr_t daddr;

	l2cap_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	int hci_socket = hci_open_dev(dev_id);

	if(hci_read_bd_addr(hci_socket, &daddr, 1000) == -1){
		daddr = *BDADDR_ANY;
	}

//	struct sockaddr_l2 sockAddr;

	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.l2_family = AF_BLUETOOTH;
	sockAddr.l2_bdaddr = daddr;
	sockAddr.l2_cid = htobs(ATT_CID);
	sockAddr.l2_bdaddr_type = BDADDR_LE_PUBLIC;

	//bind the l2cap socket
	int result = bind(l2cap_socket, (struct sockaddr*)&sockAddr, sizeof(sockAddr));

	if(result == -1)
		printf("Binding L2CAP socket failed.\n");
	else
		printf("Successfuly binded L2CAP address.\n");

	//mark the l2cap socket to listen
	result = listen(l2cap_socket, 1);

	if(result == -1)
		printf("Listening to L2CAP socket failed.\n");
	else
		printf("Successfuly listening to L2CAP socket.\n");	

	return 0;
}

int init_rfcomm_sock(int dev_id)
{
	char buf[1024] = {0};

	int s, client, bytes_read;

	//alocate socket
	rfcomm_socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	
	//bind socket to port 1 of the first available local bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_bdaddr = *BDADDR_ANY;
	loc_addr.rc_channel = (uint8_t)RFCOMM_CHANNEL;

	printf("Trying to bind rfcomm socket.\n");
	int result = bind(rfcomm_socket, (struct sockaddr*)&loc_addr, sizeof(loc_addr));
	
	if(result == -1)
		printf("Could not bind RFCOMM socket.\n");
	else
		printf("Succesful.\n");

	printf("Trying to set listen on rfcomm_socket.");
	result = listen(rfcomm_socket, 1);

	if(result == -1)
		printf("Could not set rfcomm socekt to listen.\n");
	else
		printf("Successful.\n");
	

}


int start_l2cap_listen()
{

}



int get_dev_id(int ctl)
{
	struct hci_dev_list_req *dl;
        struct hci_dev_req *dr;
        int i;

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


int bluetooth_adv_start(int ctl, int hdev)
{
	struct hci_request rq;
	le_set_advertise_enable_cp advertise_cp;
	le_set_advertising_parameters_cp adv_params_cp;
	uint8_t status;
	int dd, ret;

	if (hdev < 0)
		hdev = hci_get_route(NULL);

	dd = hci_open_dev(hdev);
	if (dd < 0) {
		perror("Could not open device");
		exit(1);
	}

	memset(&adv_params_cp, 0, sizeof(adv_params_cp));
	adv_params_cp.min_interval = htobs(0x0800);
	adv_params_cp.max_interval = htobs(0x0800);
	adv_params_cp.advtype = atoi("0");
	adv_params_cp.chan_map = 7;

	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
	rq.cparam = &adv_params_cp;
	rq.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
	rq.rparam = &status;
	rq.rlen = 1;

	ret = hci_send_req(dd, &rq, 1000);
	if (ret < 0)
		goto done;

	memset(&advertise_cp, 0, sizeof(advertise_cp));
	advertise_cp.enable = 0x01;

	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
	rq.cparam = &advertise_cp;
	rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
	rq.rparam = &status;
	rq.rlen = 1;

	ret = hci_send_req(dd, &rq, 1000);

done:
	hci_close_dev(dd);

	if (ret < 0) {
		fprintf(stderr, "Can't set advertise mode on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
		exit(1);
	}

	if (status) {
		fprintf(stderr,
			"LE set advertise enable on hci%d returned status %d\n",
								hdev, status);
		exit(1);
	}

}

int bluetooth_adv_stop(int ctl, int hdev)
{
	struct hci_request rq;
	le_set_advertise_enable_cp advertise_cp;
	uint8_t status;
	int dd, ret;

	if (hdev < 0)
		hdev = hci_get_route(NULL);

	dd = hci_open_dev(hdev);
	if (dd < 0) {
		perror("Could not open device");
		exit(1);
	}

	memset(&advertise_cp, 0, sizeof(advertise_cp));

	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
	rq.cparam = &advertise_cp;
	rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
	rq.rparam = &status;
	rq.rlen = 1;

	ret = hci_send_req(dd, &rq, 1000);

	hci_close_dev(dd);

	if (ret < 0) {
		fprintf(stderr, "Can't set advertise mode on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
		exit(1);
	}

	if (status) {
		fprintf(stderr, "LE set advertise enable on hci%d returned status %d\n",
						hdev, status);
		exit(1);
	}

}


int send_adv_package()
{



}


void bring_up(int ctl, int hdev)
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

void bring_down(int ctl, int hdev)
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
void set_adv_packet(int dev_id, const char* adv_string)
{
        unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr = buf;
	char* token = NULL;
        struct hci_filter flt;
        hci_event_hdr *hdr;
        int i, opt, len, dd;
        uint16_t ocf = 0x0008; // this the advertising command
        uint8_t ogf = 0x08;

        if (dev_id < 0)
                dev_id = hci_get_route(NULL);

        //errno = 0;
        //ogf = strtol(argv[0], NULL, 16);
        //ocf = strtol(argv[1], NULL, 16);
        if ((ogf > 0x3f) || (ocf > 0x3ff)) {
                return;
        }

	char* temp;
	temp = calloc(strlen(adv_string)+1, sizeof(char));
	strcpy(temp, adv_string);  

	token = strtok(temp, " ");

	while(token){
		*ptr++ = (uint8_t) strtol(token, NULL, 16);
		token = strtok(NULL, " ");	
	}

//        for (i = 2, len = 0; i < argc && len < (int) sizeof(buf); i++, len++)
//                *ptr++ = (uint8_t) strtol(argv[i], NULL, 16);

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

        hci_close_dev(dd);
}

static int bt_string_to_uuid128(bt_uuid_t *uuid, const char *string)
{
	uint32_t data0, data4;
	uint16_t data1, data2, data3, data5;
	uint128_t n128, u128;
	uint8_t *val = (uint8_t *) &n128;

	if (sscanf(string, "%08x-%04hx-%04hx-%04hx-%08x%04hx",
				&data0, &data1, &data2,
				&data3, &data4, &data5) != 6)
		return -EINVAL;

	data0 = htonl(data0);
	data1 = htons(data1);
	data2 = htons(data2);
	data3 = htons(data3);
	data4 = htonl(data4);
	data5 = htons(data5);

	memcpy(&val[0], &data0, 4);
	memcpy(&val[4], &data1, 2);
	memcpy(&val[6], &data2, 2);
	memcpy(&val[8], &data3, 2);
	memcpy(&val[10], &data4, 4);
	memcpy(&val[14], &data5, 2);

	ntoh128(&n128, &u128);

	bt_uuid128_create(uuid, u128);

	return 0;
}


sdp_session_t *register_service()
{
	uint32_t service_uuid_int[] = {0, 0, 0, 0xABCD};
	uint8_t rfcomm_channel = RFCOMM_CHANNEL;
	const char *service_name = "Offload Service";
	const char *service_dsc = "And experimental offloading service.";
	const char *service_prov = "Offloading";

	bt_uuid_t serviceUUID;
	uuid_t root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid;
	sdp_list_t *l2cap_list = 0,
		   *rfcomm_list = 0,
		   *root_list = 0,
		   *proto_list = 0,
		   *access_proto_list = 0;

	sdp_data_t *channel =0, *psm =0;

	sdp_record_t *record = sdp_record_alloc();

	bt_string_to_uuid128(&serviceUUID, SERVICE_UUID);
	//service_uuid_int = serviceUUID.value.u128;
	// set the general service ID
	sdp_uuid128_create( &svc_uuid, &serviceUUID.value.u128 );
    	sdp_set_service_id( record, svc_uuid );

    	// make the service record publicly browsable
    	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
    	root_list = sdp_list_append(0, &root_uuid);
    	sdp_set_browse_groups( record, root_list );

    	// set l2cap information
    	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
    	l2cap_list = sdp_list_append( 0, &l2cap_uuid );
    	proto_list = sdp_list_append( 0, l2cap_list );

    	// set rfcomm information
    	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
    	channel = sdp_data_alloc(SDP_UINT8, &rfcomm_channel);
    	rfcomm_list = sdp_list_append( 0, &rfcomm_uuid );
    	sdp_list_append( rfcomm_list, channel );
    	sdp_list_append( proto_list, rfcomm_list );

    	// attach protocol information to service record
    	access_proto_list = sdp_list_append( 0, proto_list );
    	sdp_set_access_protos( record, access_proto_list );

    	// set the name, provider, and description
    	sdp_set_info_attr(record, service_name, service_prov, service_dsc);

	int err = 0;
    	sdp_session_t *session = 0;

    	// connect to the local SDP server, register the service record, and 
    	// disconnect
    	session = sdp_connect( BDADDR_ANY, BDADDR_LOCAL, SDP_RETRY_IF_BUSY );
    	err = sdp_record_register(session, record, 0);

    	// cleanup
    	sdp_data_free( channel );
    	sdp_list_free( l2cap_list, 0 );
    	sdp_list_free( rfcomm_list, 0 );
    	sdp_list_free( root_list, 0 );
    	sdp_list_free( access_proto_list, 0 );

	return session;
}


