#include <string.h>

#include "mem.h"
#include "module.h"
#include "syscalls.h"
#include "timer.h"
#include "types.h"
#include "usb2.h"
#include "usbglue.h"
#include "usbstorage.h"

/* Constants */
#define DEVLIST_MAXSIZE		8

/* Device handler */
static usbstorage_handle __usbfd;

/* Device info */
static u8   __lun[2] = {0, 0};
static u16  __vid = 0;
static u16  __pid = 0;

/* Variables */
static bool __mounted[2] = {false, false};
static bool __inited  = false;

extern u32 current_drive;

bool usbstorage_Startup(void)
{
	s32 ret;

	if (!__inited) {
		/* Initialize USB */
		ret = USB_Init();
		if (ret < 0)
			return false;

		/* Initialize USB Storage */
		ret = USBStorage_Initialize();
		if (ret < 0)
			return false;

		/* Set as initialized */
		__inited = true;
	}

	msleep(1);

	/* Setup message */
	os_message_queue_send(queuehandle, MESSAGE_MOUNT, 0);

	return true;
}

bool usbstorage_IsInserted(void)
{
	usb_device_entry *buffer;

	u8  device_count, i, j;
	u16 vid, pid;
	s32 maxLun;
	s32 retval;

	/* USB not inited */
	if (!__inited)
		return false;

	/* Allocate memory */
	buffer = Mem_Alloc(DEVLIST_MAXSIZE * sizeof(usb_device_entry));
	if (!buffer)
		return false;

	memset(buffer, 0, DEVLIST_MAXSIZE * sizeof(usb_device_entry));

	/* Get device list */
	retval = USB_GetDeviceList(buffer, DEVLIST_MAXSIZE, USB_CLASS_MASS_STORAGE, &device_count);
	if (retval < 0)
		goto err;

	usleep(100);

	if (__vid || __pid) {
		/* Search device */
		for(i = 0; i < device_count; i++) {
			vid = buffer[i].vid;
			pid = buffer[i].pid;

			if((vid == __vid) && (pid == __pid)) {
				/* Free memory */
				Mem_Free(buffer);

				usleep(50);

				return __mounted[current_drive];
			}
		}

		goto err;
	}

	/* Reset flag */
	for (i = 0; i < sizeof(__mounted)/sizeof(__mounted[0]); i++)
		__mounted[i] = false;

	for (i = 0; i < device_count; i++) {
		vid = buffer[i].vid;
		pid = buffer[i].pid;

		/* Wrong VID/PID */
		if (!vid || !pid)
			continue;

		/* Open device */
		retval = USBStorage_Open(&__usbfd, buffer[i].device_id, vid, pid);
		if (retval < 0)
			continue;

		/* Get LUN */
		maxLun = USBStorage_GetMaxLUN(&__usbfd);

		int found = 0;
		/* Mount LUN */
		for (j = 0; j < maxLun; j++) {
			retval = USBStorage_MountLUN(&__usbfd, j);

			if (retval == USBSTORAGE_ETIMEDOUT)
				break;

			if (retval < 0)
				continue;

			/* Set parameters */
			__mounted[found] = true;
			__lun[found] = j;
			__vid = vid;
			__pid = pid;

			if (++found >= sizeof(__mounted)/sizeof(__mounted[0]))
				break;
		}

		/* Device mounted - we don't care if the requested LUN exist or not, if LUN=1 is asked for but max LUN is 0, it's an error. */
		if (__vid || __pid)
			break;

		/* Close device */
		USBStorage_Close(&__usbfd);
	}

	/* No device mounted - if we have found zero drives __vid and __pid will be zeros, and we would have already closed the USB Storage above. No need to kill again. */
//	if (!__mounted)
//		goto err;

	goto out;

err:
	/* Close device */
	USBStorage_Close(&__usbfd);

	/* Clear parameters */
	__mounted[current_drive] = false;
	__lun[current_drive] = 0;
	__vid = 0;
	__pid = 0;

out:
	/* Free memory */
	Mem_Free(buffer);

	return __mounted[current_drive];
}

bool usbstorage_ReadSectors(u32 sector, u32 numSectors, void *buffer)
{
	s32 retval;

	if (!__mounted[current_drive])
		return false;

	/* Read sectors */
	retval = USBStorage_Read(&__usbfd, __lun[current_drive], sector, numSectors, buffer);

	return retval >= 0;
}

bool usbstorage_WriteSectors(u32 sector, u32 numSectors, const void *buffer)
{
	s32 retval;

	if (!__mounted[current_drive])
		return false;

	/* Write sectors */
	retval = USBStorage_Write(&__usbfd, __lun[current_drive], sector, numSectors, buffer);

	return retval >= 0;
}

bool usbstorage_ReadCapacity(u32 *sectorSz, u32 *numSectors)
{
	s32 retval;

	if (!__mounted[current_drive])
		return false;

	/* Read capacity */
	retval = USBStorage_ReadCapacity(&__usbfd, __lun[current_drive], sectorSz, numSectors);

	return retval >= 0;
}

bool usbstorage_Shutdown(void)
{
	__mounted[current_drive] = 0;
	int i;
	for (i=0; i<sizeof(__mounted)/sizeof(__mounted[0]); i++)
		if (__mounted[i])
			return true;
	/* Close device - only if all drives have been unmounted */
	if (__vid || __pid)
		USBStorage_Close(&__usbfd);
	__inited = false;
	__vid = 0;
	__pid = 0;
	return true;
}
