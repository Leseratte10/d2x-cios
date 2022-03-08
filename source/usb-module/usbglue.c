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

// For a given USB port, scan for available drives
bool usbstorage_Scan(void)
{
	int j = 0;		// LUN to start scanning from
	int found = 0;	// "Port" to start scanning from
    if (current_drive > 0)
	{
		for (found = current_drive; found > 0; found--)
			if (0 != __mounted[found-1])
			{
				// This is where we left off the last time we last scanned for LUNs, and is where we start scanning this time.
				j = __lun[found-1] + 1;
				break;
			}
	}
	/* Get LUN */
	s32 maxLun = USBStorage_GetMaxLUN(&__usbfd);
	bool success = false;
	/* Mount LUN */
	for (; j < maxLun; j++) {
		s32 retval = USBStorage_MountLUN(&__usbfd, j);

		if (retval == USBSTORAGE_ETIMEDOUT)
			break;

		if (retval < 0)
			continue;

		/* Set parameters */
		__mounted[found] = true;
		__lun[found] = j;
		success = true;

		if (found++ > current_drive)
			break;
	}
	return success;
}

bool usbstorage_IsInserted(void)
{
	usb_device_entry *buffer;

	u8  device_count, i;
	u16 vid, pid;
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

				// We have seen the USB device before but we need to look for the exact drive (0 or 1) requested
				if (!__mounted[current_drive])
					usbstorage_Scan();
				return __mounted[current_drive];
			}
		}

		goto err;
	}

	/* Reset flag - USB device not previously seen so everything is unmounted */
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

		// If scan finds any drive, record as success by remembering vid/pid
		if (usbstorage_Scan())
		{
			__vid = vid;
			__pid = pid;
			/* Device mounted - we don't care if the requested LUN exist or not, if LUN=1 is asked for but max LUN is 0, it's an error. */
			break;
		}

		/* If scan finds nothing, close device */
		USBStorage_Close(&__usbfd);
	}

	/* Even if requested drive not mounted we must not kill the entire device. Just return failure. */
//	if (!__mounted)
//		goto err;

	goto out;

err:
	/* Close device */
	USBStorage_Close(&__usbfd);

	/* Clear parameters */
	for (i = 0; i < sizeof(__mounted)/sizeof(__mounted[0]); i++)
	{
		__mounted[i] = false;
		__lun[i] = 0;
	}
	__inited = false;
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
