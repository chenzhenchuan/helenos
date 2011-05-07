#include <errno.h>
#include <str_error.h>
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>
#include "vhcd.h"

#define list_foreach(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)

vhc_transfer_t *vhc_transfer_create(usb_address_t address, usb_endpoint_t ep,
    usb_direction_t dir, usb_transfer_type_t tr_type,
    ddf_fun_t *fun, void *callback_arg)
{
	vhc_transfer_t *result = malloc(sizeof(vhc_transfer_t));
	if (result == NULL) {
		return NULL;
	}
	link_initialize(&result->link);
	result->address = address;
	result->endpoint = ep;
	result->direction = dir;
	result->transfer_type = tr_type;
	result->setup_buffer = NULL;
	result->setup_buffer_size = 0;
	result->data_buffer = NULL;
	result->data_buffer_size = 0;
	result->ddf_fun = fun;
	result->callback_arg = callback_arg;
	result->callback_in = NULL;
	result->callback_out = NULL;

	usb_log_debug2("Created transfer %p (%d.%d %s %s)\n", result,
	    address, ep, usb_str_transfer_type_short(tr_type),
	    dir == USB_DIRECTION_IN ? "in" : "out");

	return result;
}

static bool is_set_address_transfer(vhc_transfer_t *transfer)
{
	if (transfer->endpoint != 0) {
		return false;
	}
	if (transfer->transfer_type != USB_TRANSFER_CONTROL) {
		return false;
	}
	if (transfer->direction != USB_DIRECTION_OUT) {
		return false;
	}
	if (transfer->setup_buffer_size != sizeof(usb_device_request_setup_packet_t)) {
		return false;
	}
	usb_device_request_setup_packet_t *setup = transfer->setup_buffer;
	if (setup->request_type != 0) {
		return false;
	}
	if (setup->request != USB_DEVREQ_SET_ADDRESS) {
		return false;
	}

	return true;
}

int vhc_virtdev_add_transfer(vhc_data_t *vhc, vhc_transfer_t *transfer)
{
	fibril_mutex_lock(&vhc->guard);

	link_t *pos;
	bool target_found = false;
	list_foreach(pos, &vhc->devices) {
		vhc_virtdev_t *dev = list_get_instance(pos, vhc_virtdev_t, link);
		fibril_mutex_lock(&dev->guard);
		if (dev->address == transfer->address) {
			if (target_found) {
				usb_log_warning("Transfer would be accepted by more devices!\n");
				goto next;
			}
			target_found = true;
			list_append(&transfer->link, &dev->transfer_queue);
		}
next:
		fibril_mutex_unlock(&dev->guard);
	}

	fibril_mutex_unlock(&vhc->guard);

	if (target_found) {
		return EOK;
	} else {
		return ENOENT;
	}
}

static int process_transfer_local(vhc_transfer_t *transfer,
    usbvirt_device_t *dev, size_t *actual_data_size)
{
	int rc;

	if (transfer->transfer_type == USB_TRANSFER_CONTROL) {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_control_read(dev,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_control_write(dev,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size);
		}
	} else {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_data_in(dev, transfer->transfer_type,
			    transfer->endpoint,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_data_out(dev, transfer->transfer_type,
			    transfer->endpoint,
			    transfer->data_buffer, transfer->data_buffer_size);
		}
	}

	return rc;
}

static int process_transfer_remote(vhc_transfer_t *transfer,
    int phone, size_t *actual_data_size)
{
	int rc;

	if (transfer->transfer_type == USB_TRANSFER_CONTROL) {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_control_read(phone,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_control_write(phone,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size);
		}
	} else {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_data_in(phone, transfer->endpoint,
			    transfer->transfer_type,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_data_out(phone, transfer->endpoint,
			    transfer->transfer_type,
			    transfer->data_buffer, transfer->data_buffer_size);
		}
	}

	return rc;
}


int vhc_transfer_queue_processor(void *arg)
{
	vhc_virtdev_t *dev = arg;
	fibril_mutex_lock(&dev->guard);
	while (dev->plugged) {
		if (list_empty(&dev->transfer_queue)) {
			fibril_mutex_unlock(&dev->guard);
			async_usleep(10 * 1000);
			fibril_mutex_lock(&dev->guard);
			continue;
		}

		vhc_transfer_t *transfer = list_get_instance(dev->transfer_queue.next,
		    vhc_transfer_t, link);
		list_remove(&transfer->link);
		fibril_mutex_unlock(&dev->guard);

		int rc = EOK;
		size_t data_transfer_size = 0;
		if (dev->dev_phone > 0) {
			rc = process_transfer_remote(transfer, dev->dev_phone,
			    &data_transfer_size);
		} else if (dev->dev_local != NULL) {
			rc = process_transfer_local(transfer, dev->dev_local,
			    &data_transfer_size);
		} else {
			usb_log_warning("Device has no remote phone nor local node.\n");
			rc = ESTALL;
		}

		usb_log_debug2("Transfer %p processed: %s.\n",
		    transfer, str_error(rc));

		fibril_mutex_lock(&dev->guard);
		if (rc == EOK) {
			if (is_set_address_transfer(transfer)) {
				usb_device_request_setup_packet_t *setup
				    = transfer->setup_buffer;
				dev->address = setup->value;
				usb_log_debug2("Address changed to %d\n",
				    dev->address);
			}
		}
		if (rc == ENAK) {
			// FIXME: this will work only because we do
			// not NAK control transfers but this is generally
			// a VERY bad idea indeed
			list_append(&transfer->link, &dev->transfer_queue);
		}
		fibril_mutex_unlock(&dev->guard);

		if (rc != ENAK) {
			usb_log_debug2("Transfer %p ended: %s.\n",
			    transfer, str_error(rc));
			if (transfer->direction == USB_DIRECTION_IN) {
				transfer->callback_in(transfer->ddf_fun, rc,
				    data_transfer_size, transfer->callback_arg);
			} else {
				assert(transfer->direction == USB_DIRECTION_OUT);
				transfer->callback_out(transfer->ddf_fun, rc,
				    transfer->callback_arg);
			}
			free(transfer);
		}

		async_usleep(1000 * 100);
		fibril_mutex_lock(&dev->guard);
	}

	fibril_mutex_unlock(&dev->guard);

	// TODO - destroy pending transfers

	return EOK;
}

