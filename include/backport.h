#ifndef _BACKPORT_H
#define _BACKPORT_H

#include <linux/version.h>
#include <linux/usb.h>

#ifndef hid_to_usb_dev
#define	hid_to_usb_dev(hid_dev) \
	container_of(hid_dev->dev.parent->parent, struct usb_device, dev)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
extern void usbhid_submit_report(struct hid_device *hid, struct hid_report *report, unsigned char dir);
extern int usbhid_wait_io(struct hid_device *hid);


/*
 * ugly hack to guess if the device is really on the USB bus or if it is a
 * virtual one created by uhid.
 */
static inline __u16 hid_get_bus(struct hid_device *hdev)
{
	__u16 bus = hdev->bus;

#ifdef CONFIG_USB_HIDDEV
	if (!hdev->hiddev_hid_event)
		bus = BUS_VIRTUAL;
	pr_err("%s bus:%x %s:%d\n", __func__, bus, __FILE__, __LINE__);
#endif

	return bus;
}

/**
 * hid_hw_request - send report request to device
 *
 * @hdev: hid device
 * @report: report to send
 * @reqtype: hid request type
 */
static inline void hid_hw_request(struct hid_device *hdev,
				  struct hid_report *report, int reqtype)
{
	if (hid_get_bus(hdev) == BUS_USB) {
		switch (reqtype) {
		case HID_REQ_GET_REPORT:
			usbhid_submit_report(hdev, report, USB_DIR_IN);
			break;
		case HID_REQ_SET_REPORT:
			usbhid_submit_report(hdev, report, USB_DIR_OUT);
			break;
		}
	}
}

static inline int hid_set_idle(struct usb_device *dev, int ifnum, int report, int idle)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		HID_REQ_SET_IDLE, USB_TYPE_CLASS | USB_RECIP_INTERFACE, (idle << 8) | report,
		ifnum, NULL, 0, USB_CTRL_SET_TIMEOUT);
}

/**
 * hid_hw_idle - send idle request to device
 *
 * @hdev: hid device
 * @report: report to control
 * @idle: idle state
 * @reqtype: hid request type
 */
static inline int hid_hw_idle(struct hid_device *hdev, int report, int idle,
		int reqtype)
{
	if (hid_get_bus(hdev) == BUS_USB) {
		struct usb_device *dev = hid_to_usb_dev(hdev);
		struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
		struct usb_host_interface *interface = intf->cur_altsetting;
		int ifnum = interface->desc.bInterfaceNumber;

		if (reqtype != HID_REQ_SET_IDLE)
			return -EINVAL;

		return hid_set_idle(dev, ifnum, report, idle);
	}

	return 0;
}

/**
 * hid_hw_wait - wait for buffered io to complete
 *
 * @hdev: hid device
 */
static inline void hid_hw_wait(struct hid_device *hdev)
{
	if (hid_get_bus(hdev) == BUS_USB)
		usbhid_wait_io(hdev);
}
#endif

#endif
