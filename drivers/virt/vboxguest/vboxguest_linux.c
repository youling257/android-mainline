/*
 * vboxguest linux pci driver, char-dev and input-device code,
 *
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/vbox_utils.h>
#include "vboxguest_core.h"

/** The device name. */
#define DEVICE_NAME		"vboxguest"
/** The device name for the device node open to everyone. */
#define DEVICE_NAME_USER	"vboxuser"
/** VirtualBox PCI vendor ID. */
#define VBOX_VENDORID		0x80ee
/** VMMDev PCI card product ID. */
#define VMMDEV_DEVICEID		0xcafe

/** Mutex protecting the global vbg_gdev pointer used by vbg_get/put_gdev. */
static DEFINE_MUTEX(vbg_gdev_mutex);
/** Global vbg_gdev pointer used by vbg_get/put_gdev. */
static struct vbg_dev *vbg_gdev;

static int vbg_misc_device_open(struct inode *inode, struct file *filp)
{
	struct vbg_session *session;
	struct vbg_dev *gdev;
	int ret;

	/* misc_open sets filp->private_data to our misc device */
	gdev = container_of(filp->private_data, struct vbg_dev, misc_device);

	ret = vbg_core_open_session(gdev, &session, false);
	if (ret)
		return -ENOMEM;

	filp->private_data = session;
	return 0;
}

static int vbg_misc_device_user_open(struct inode *inode, struct file *filp)
{
	struct vbg_session *session;
	struct vbg_dev *gdev;
	int ret;

	/* misc_open sets filp->private_data to our misc device */
	gdev = container_of(filp->private_data, struct vbg_dev,
			    misc_device_user);

	ret = vbg_core_open_session(gdev, &session, true);
	if (ret)
		return ret;

	filp->private_data = session;
	return 0;
}

/**
 * Close device.
 *
 * @param   inode	Pointer to inode info structure.
 * @param   filp	Associated file pointer.
 */
static int vbg_misc_device_close(struct inode *inode, struct file *filp)
{
	vbg_core_close_session(filp->private_data);
	filp->private_data = NULL;
	return 0;
}

/**
 * Device I/O Control entry point.
 *
 * @returns -ENOMEM or -EFAULT for errors inside the ioctl callback; 0
 * on success, or a positive VBox status code on vbox guest-device errors.
 *
 * @param   filp	Associated file pointer.
 * @param   req		The request specified to ioctl().
 * @param   arg		The argument specified to ioctl().
 */
static long vbg_misc_device_ioctl(struct file *filp, unsigned int req,
				  unsigned long arg)
{
	struct vbg_session *session = filp->private_data;
	size_t returned_data_size, size = _IOC_SIZE(req);
	void *buf, *buf_free = NULL;
	u8 stack_buf[32];
	int rc, ret = 0;

	/*
	 * The largest ioctl req is a hgcm-call with 32 function arguments,
	 * which is 16 + 32 * 16 bytes. However the variable length buffer
	 * passed to VMMDevReq_LogString may be longer. Allow logging up to
	 * 1024 bytes (minus header size).
	 */
	if (size > 1024)
		return -E2BIG;

	/*
	 * For small amounts of data being passed we use a stack based buffer
	 * except for VMMREQUESTs where the data must not be on the stack.
	 */
	if (size <= sizeof(stack_buf) &&
		    _IOC_NR(req) != _IOC_NR(VBOXGUEST_IOCTL_VMMREQUEST(0))) {
		buf = stack_buf;
	} else {
		/* __GFP_DMA32 for VBOXGUEST_IOCTL_VMMREQUEST */
		buf_free = buf = kmalloc(size, GFP_KERNEL | __GFP_DMA32);
		if (!buf)
			return -ENOMEM;
	}

	if (copy_from_user(buf, (void *)arg, size)) {
		ret = -EFAULT;
		goto out;
	}

	rc = vbg_core_ioctl(session, req, buf, size, &returned_data_size);
	if (rc < 0) {
		/* Negate the Vbox status code to make it positive. */
		ret = -rc;
		goto out;
	}

	if (returned_data_size > size) {
		vbg_debug("%s: too much output data %zu > %zu\n",
			  __func__, returned_data_size, size);
		returned_data_size = size;
	}
	if (returned_data_size > 0) {
		if (copy_to_user((void *)arg, buf, returned_data_size) != 0)
			ret = -EFAULT;
	}

out:
	kfree(buf_free);

	return ret;
}

/** The file_operations structures. */
static const struct file_operations vbg_misc_device_fops = {
	.owner			= THIS_MODULE,
	.open			= vbg_misc_device_open,
	.release		= vbg_misc_device_close,
	.unlocked_ioctl		= vbg_misc_device_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= vbg_misc_device_ioctl,
#endif
};
static const struct file_operations vbg_misc_device_user_fops = {
	.owner			= THIS_MODULE,
	.open			= vbg_misc_device_user_open,
	.release		= vbg_misc_device_close,
	.unlocked_ioctl		= vbg_misc_device_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= vbg_misc_device_ioctl,
#endif
};

/**
 * Called when the input device is first opened.
 *
 * Sets up absolute mouse reporting.
 */
static int vbg_input_open(struct input_dev *input)
{
	struct vbg_dev *gdev = input_get_drvdata(input);
	u32 feat = VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE | VMMDEV_MOUSE_NEW_PROTOCOL;
	int ret;

	ret = vbg_core_set_mouse_status(gdev, feat);
	if (ret)
		return ret;

	return 0;
}

/**
 * Called if all open handles to the input device are closed.
 *
 * Disables absolute reporting.
 */
static void vbg_input_close(struct input_dev *input)
{
	struct vbg_dev *gdev = input_get_drvdata(input);

	vbg_core_set_mouse_status(gdev, 0);
}

/**
 * Creates the kernel input device.
 *
 * @returns 0 on success, negated errno on failure.
 */
static int vbg_create_input_device(struct vbg_dev *gdev)
{
	struct input_dev *input;

	input = devm_input_allocate_device(gdev->dev);
	if (!input)
		return -ENOMEM;

	input->id.bustype = BUS_PCI;
	input->id.vendor = VBOX_VENDORID;
	input->id.product = VMMDEV_DEVICEID;
	input->open = vbg_input_open;
	input->close = vbg_input_close;
	input->dev.parent = gdev->dev;
	input->name = "VirtualBox mouse integration";

	input_set_abs_params(input, ABS_X, VMMDEV_MOUSE_RANGE_MIN,
			     VMMDEV_MOUSE_RANGE_MAX, 0, 0);
	input_set_abs_params(input, ABS_Y, VMMDEV_MOUSE_RANGE_MIN,
			     VMMDEV_MOUSE_RANGE_MAX, 0, 0);
	input_set_capability(input, EV_KEY, BTN_MOUSE);
	input_set_drvdata(input, gdev);

	gdev->input = input;

	return input_register_device(gdev->input);
}

static ssize_t host_version_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct vbg_dev *gdev = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", gdev->host_version);
}

static ssize_t host_features_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct vbg_dev *gdev = dev_get_drvdata(dev);

	return sprintf(buf, "%#x\n", gdev->host_features);
}

static DEVICE_ATTR_RO(host_version);
static DEVICE_ATTR_RO(host_features);

/**
 * Does the PCI detection and init of the device.
 *
 * @returns 0 on success, negated errno on failure.
 */
static int vbg_pci_probe(struct pci_dev *pci, const struct pci_device_id *id)
{
	struct device *dev = &pci->dev;
	resource_size_t io, io_len, mmio, mmio_len;
	VMMDevMemory *vmmdev;
	struct vbg_dev *gdev;
	int ret;

	gdev = devm_kzalloc(dev, sizeof(*gdev), GFP_KERNEL);
	if (!gdev)
		return -ENOMEM;

	ret = pci_enable_device(pci);
	if (ret != 0) {
		vbg_err("vboxguest: Error enabling device: %d\n", ret);
		return ret;
	}

	ret = -ENODEV;

	io = pci_resource_start(pci, 0);
	io_len = pci_resource_len(pci, 0);
	if (!io || !io_len) {
		vbg_err("vboxguest: Error IO-port resource (0) is missing\n");
		goto err_disable_pcidev;
	}
	if (devm_request_region(dev, io, io_len, DEVICE_NAME) == NULL) {
		vbg_err("vboxguest: Error could not claim IO resource\n");
		ret = -EBUSY;
		goto err_disable_pcidev;
	}

	mmio = pci_resource_start(pci, 1);
	mmio_len = pci_resource_len(pci, 1);
	if (!mmio || !mmio_len) {
		vbg_err("vboxguest: Error MMIO resource (1) is missing\n");
		goto err_disable_pcidev;
	}

	if (devm_request_mem_region(dev, mmio, mmio_len, DEVICE_NAME) == NULL) {
		vbg_err("vboxguest: Error could not claim MMIO resource\n");
		ret = -EBUSY;
		goto err_disable_pcidev;
	}

	vmmdev = devm_ioremap(dev, mmio, mmio_len);
	if (!vmmdev) {
		vbg_err("vboxguest: Error ioremap failed; MMIO addr=%p size=%d\n",
			(void *)mmio, (int)mmio_len);
		goto err_disable_pcidev;
	}

	/* Validate MMIO region version and size. */
	if (vmmdev->u32Version != VMMDEV_MEMORY_VERSION ||
	    vmmdev->u32Size < 32 || vmmdev->u32Size > mmio_len) {
		vbg_err("vboxguest: Bogus VMMDev memory; u32Version=%08x (expected %08x) u32Size=%d (expected <= %d)\n",
			vmmdev->u32Version, VMMDEV_MEMORY_VERSION,
			vmmdev->u32Size, (int)mmio_len);
		goto err_disable_pcidev;
	}

	gdev->io_port = io;
	gdev->mmio = vmmdev;
	gdev->dev = dev;
	gdev->misc_device.minor = MISC_DYNAMIC_MINOR;
	gdev->misc_device.name = DEVICE_NAME;
	gdev->misc_device.fops = &vbg_misc_device_fops;
	gdev->misc_device_user.minor = MISC_DYNAMIC_MINOR;
	gdev->misc_device_user.name = DEVICE_NAME_USER;
	gdev->misc_device_user.fops = &vbg_misc_device_user_fops;

	ret = vbg_core_init(gdev, VMMDEV_EVENT_MOUSE_POSITION_CHANGED);
	if (ret)
		goto err_disable_pcidev;

	ret = vbg_create_input_device(gdev);
	if (ret) {
		vbg_err("vboxguest: Error creating input device: %d\n", ret);
		goto err_vbg_core_exit;
	}

	ret = devm_request_irq(dev, pci->irq, vbg_core_isr, IRQF_SHARED,
			       DEVICE_NAME, gdev);
	if (ret) {
		vbg_err("vboxguest: Error requesting irq: %d\n", ret);
		goto err_vbg_core_exit;
	}

	ret = misc_register(&gdev->misc_device);
	if (ret) {
		vbg_err("vboxguest: Error misc_register %s failed: %d\n",
			DEVICE_NAME, ret);
		goto err_vbg_core_exit;
	}

	ret = misc_register(&gdev->misc_device_user);
	if (ret) {
		vbg_err("vboxguest: Error misc_register %s failed: %d\n",
			DEVICE_NAME_USER, ret);
		goto err_unregister_misc_device;
	}

	mutex_lock(&vbg_gdev_mutex);
	if (!vbg_gdev)
		vbg_gdev = gdev;
	else
		ret = -EBUSY;
	mutex_unlock(&vbg_gdev_mutex);

	if (ret) {
		vbg_err("vboxguest: Error more then 1 vbox guest pci device\n");
		goto err_unregister_misc_device_user;
	}

	pci_set_drvdata(pci, gdev);
	device_create_file(dev, &dev_attr_host_version);
	device_create_file(dev, &dev_attr_host_features);

	vbg_info("vboxguest: misc device minor %d, IRQ %d, I/O port %x, MMIO at %p (size %d)\n",
		 gdev->misc_device.minor, pci->irq, gdev->io_port,
		 (void *)mmio, (int)mmio_len);

	return 0;

err_unregister_misc_device_user:
	misc_deregister(&gdev->misc_device_user);
err_unregister_misc_device:
	misc_deregister(&gdev->misc_device);
err_vbg_core_exit:
	vbg_core_exit(gdev);
err_disable_pcidev:
	pci_disable_device(pci);

	return ret;
}

static void vbg_pci_remove(struct pci_dev *pci)
{
	struct vbg_dev *gdev = pci_get_drvdata(pci);

	mutex_lock(&vbg_gdev_mutex);
	vbg_gdev = NULL;
	mutex_unlock(&vbg_gdev_mutex);

	device_remove_file(gdev->dev, &dev_attr_host_features);
	device_remove_file(gdev->dev, &dev_attr_host_version);
	misc_deregister(&gdev->misc_device_user);
	misc_deregister(&gdev->misc_device);
	vbg_core_exit(gdev);
	pci_disable_device(pci);
}

struct vbg_dev *vbg_get_gdev(void)
{
	mutex_lock(&vbg_gdev_mutex);

	/*
	 * Note on success we keep the mutex locked until vbg_put_gdev(),
	 * this stops vbg_pci_remove from removing the device from underneath
	 * vboxsf. vboxsf will only hold a reference for a short while.
	 */
	if (vbg_gdev)
		return vbg_gdev;

	mutex_unlock(&vbg_gdev_mutex);
	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(vbg_get_gdev);

void vbg_put_gdev(struct vbg_dev *gdev)
{
	WARN_ON(gdev != vbg_gdev);
	mutex_unlock(&vbg_gdev_mutex);
}
EXPORT_SYMBOL(vbg_put_gdev);

/**
 * Callback for mouse events.
 *
 * This is called at the end of the ISR, after leaving the event spinlock, if
 * VMMDEV_EVENT_MOUSE_POSITION_CHANGED was raised by the host.
 *
 * @param   gdev	The device extension.
 */
void vbg_linux_mouse_event(struct vbg_dev *gdev)
{
	int rc;

	/* Report events to the kernel input device */
	gdev->mouse_status_req->mouseFeatures = 0;
	gdev->mouse_status_req->pointerXPos = 0;
	gdev->mouse_status_req->pointerYPos = 0;
	rc = vbg_req_perform(gdev, gdev->mouse_status_req);
	if (rc >= 0) {
		input_report_abs(gdev->input, ABS_X,
				 gdev->mouse_status_req->pointerXPos);
		input_report_abs(gdev->input, ABS_Y,
				 gdev->mouse_status_req->pointerYPos);
		input_sync(gdev->input);
	}
}

static const struct pci_device_id vbg_pci_ids[] = {
	{ .vendor = VBOX_VENDORID, .device = VMMDEV_DEVICEID },
	{}
};
MODULE_DEVICE_TABLE(pci,  vbg_pci_ids);

static struct pci_driver vbg_pci_driver = {
	.name		= DEVICE_NAME,
	.id_table	= vbg_pci_ids,
	.probe		= vbg_pci_probe,
	.remove		= vbg_pci_remove,
};

module_pci_driver(vbg_pci_driver);

MODULE_AUTHOR("Oracle Corporation");
MODULE_DESCRIPTION("Oracle VM VirtualBox Guest Additions for Linux Module");
MODULE_LICENSE("GPL");
