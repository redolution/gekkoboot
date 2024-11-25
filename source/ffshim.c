#include "fatfs/ff.h"

#include "fatfs/diskio.h"

#include "ffshim.h"

DISC_INTERFACE *iface = NULL;

DSTATUS
disk_status(BYTE pdrv) {
	(void) pdrv;

	if (iface == NULL) {
		return STA_NOINIT;
	} else if (!iface->isInserted(iface)) {
		return STA_NOINIT;
	}

	return 0;
}

DSTATUS
disk_initialize(BYTE pdrv) {
	(void) pdrv;

	if (iface == NULL) {
		goto noinit;
	}

	if (!iface->startup(iface)) {
		goto noinit;
	}

	if (!iface->isInserted(iface)) {
		goto shutdown;
	}

	return 0;

shutdown:
	iface->shutdown(iface);
noinit:
	iface = NULL;
	return STA_NOINIT;
}

DRESULT
disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
	(void) pdrv;

	if (iface == NULL) {
		return RES_NOTRDY;
	}

	if (iface->readSectors(iface, sector, count, buff)) {
		return RES_OK;
	} else {
		return RES_ERROR;
	}
}
