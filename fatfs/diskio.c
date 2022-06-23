/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include <iosuhax.h>

/* Definitions of physical drive number for each drive */
// #define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
// #define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		0	/* Example: Map USB MSD to physical drive 2 */


static int fsaFdUsb = -1;
static int usbFd = -1;


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	if (fsaFdUsb < 0 || usbFd < 0) {
		return STA_NOINIT;
	}
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
	// close so we can reinitialize if needed
	if (usbFd >= 0) {
		IOSUHAX_FSA_RawClose(fsaFdUsb, usbFd);
		usbFd = -1;
	}
	if (fsaFdUsb >= 0) {
		IOSUHAX_FSA_Close(fsaFdUsb);
		fsaFdUsb = -1;
		IOSUHAX_Close();
	}

	if (IOSUHAX_Open(NULL) < 0)
        return STA_NOINIT;

	fsaFdUsb = IOSUHAX_FSA_Open();
	if (fsaFdUsb < 0) {
		IOSUHAX_Close();
		return STA_NOINIT;
	}

	int res = IOSUHAX_FSA_RawOpen(fsaFdUsb, "/dev/usb02", &usbFd);
	if (res < 0) {
		IOSUHAX_FSA_Close(fsaFdUsb);
		IOSUHAX_Close();
		return STA_NOINIT;
	}

	if (usbFd < 0) return STA_NOINIT;

	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	int result;

	// sector size 512 bytes
	if (fsaFdUsb < 0 || usbFd < 0) return RES_NOTRDY;
	int res = IOSUHAX_FSA_RawRead(fsaFdUsb, buff, 512, count, sector, usbFd);
    if (res < 0) {
        return RES_ERROR;
    }

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here

		result = RAM_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_MMC :
		// translate the arguments here

		result = MMC_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_USB :
		// translate the arguments here

		result = USB_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	return RES_PARERR;
}

