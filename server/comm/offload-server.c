#include "bluetooth.h"
#include "offload-server.h"

struct gatt_offload_adapter {
	struct btd_adapter	*adapter;
	GSList			*sdp_handles;
};

static GSList *adapters = NULL;

static void gatt_offload_adapter_free(struct gatt_offload_adapter *gadapter)
{
	while (gadapter->sdp_handles != NULL) {
		uint32_t handle = GPOINTER_TO_UINT(gadapter->sdP-handles->data);
		attrib_free_sdp(gadapter->adapter, handle);
		gadapter->sdp_handles = g_slist_remove(gadapter->sdp_handles, gadapter->sdp_handles->data);
	}
	
	if(gadapter->adapter != NULL)
		btd_adapter_unref(gadapter->adapter);

	g_free(gadapter);
}


static int adapter_cmp(gconstpointer a, gconstpointer b)
{
        const struct gatt_offload_adapter *gatt_adapter = a;
        const struct btd_adapter *adapter = b;

        if (gatt_adapter->adapter == adapter)
                return 0;

        return -1;
}

static gboolean register_offload_service(struct btd_adapter *adapter){

	bt_uuid_t uuid;
	
	bt_uuid32_create(&uuid, OFFLOAD_SVC_UUID);

	return gatt_service_add(adapter, GATT_PRIM_SVC_UUID, &uuid);
}

static int gatt_offload_adapter_probe(struct btd_adapter *adapter)
{
	struct gatt_offload_adapter *gadapter;

	gadapter = g_new0(struct gatt_offload_adapter, 1);
	gadapter->adapter = btd_adapter_ref(adapter);

	if(!register_offload_service(adapter)){
		printf("Offload service could not be registered.\n");
		return -EIO;
	}

	adapters = g_slist_append(adapters, gadapter);

	return 0;
}


static void gatt_offload_adapter_remove(struct btd_adapter *adapter)
{
	struct gatt_offload_adapter *gadapter;
	GSList *l;

	l = g_slist_find_custom(adapters, adapter, adapter_cmp);
	if( l == NULL)
		return;

	gadapter = l->data;
	adapters = g_slist_remove(adapters, gadapter);
	gatt_offload_adapter_free(gadapter);
}

static struct btd_adapter_driver gatt_offload_adapter_driver = {
	.name = "OFFLOAD SERVICE ADAPTER DRIVER",
	.probe = gatt_offload_adapter_probe,
	.remove = gatt_example_adapter_remove
};

static int gatt_offload_init(void)
{
	return btd_register_adapter_driver(&gatt_offload_adapter_driver);
}

static int gatt_offload_exit(void)
{
	return btd_unregister_adapter_driver(&gatt_example_adapter_driver);
}

BLUETOOTH_PLUGIN_DEFINE(gatt_offload, VERSION, BLUETOOTH_PLUGIN_PRIORITY_LOW,
				gatt_offload_init, gatt_offload_exit)
