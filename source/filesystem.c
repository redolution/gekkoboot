#include "filesystem.h"
#include "fatfs/ff.h"
#include "ffshim.h"
#include <ogc/system.h>
#include <stdlib.h>

FATFS fs;
FS_RESULT
fs_mount(const DISC_INTERFACE *iface_) {
	iface = iface_;
	return f_mount(&fs, "", 1);
}

void
fs_unmount() {
	f_unmount("");
	iface->shutdown();
	iface = NULL;
}

void
fs_get_volume_label(const char *path, char *label) {
	FRESULT res = f_getlabel(path, label, NULL);
	if (res != FR_OK) {
		*label = '\0';
	}
}

FS_RESULT
_fs_read_file(void **contents_, const char *path, int is_string) {
	FIL file;
	FRESULT result = f_open(&file, path, FA_READ);
	if (result != FR_OK) {
		if (result == FR_NO_FILE || result == FR_NO_PATH) {
			kprintf("File not found\n");
			return FS_NO_FILE;
		}
		kprintf("->> !! Failed to open file: %s\n", get_fs_result_message(result));
		return result;
	}

	size_t size = f_size(&file);
	if (size <= 0) {
		kprintf("->> !! File is empty\n");
		return FS_FILE_EMPTY;
	}
	kprintf("File size: %iB\n", size);

	// Malloc an extra byte if we are reading as a string incase we need to add NUL character.
	void *contents = malloc(size + (is_string ? 1 : 0));
	if (!contents) {
		kprintf("->> !! Couldn't allocate memory for file\n");
		return FS_NOT_ENOUGH_MEMORY;
	}

	kprintf("Reading file...\n");
	UINT _;
	result = f_read(&file, contents, size, &_);
	if (result != FR_OK) {
		kprintf("->> !! Failed to read file: %s\n", get_fs_result_message(result));
		return result;
	}

	f_close(&file);

	// Ensure files read as strings end with NUL character.
	if (is_string) {
		// This is safe because we malloc an extra byte above if reading as string.
		((char *) contents)[size] = '\0';
	}

	*contents_ = contents;
	return FS_OK;
}

FS_RESULT
fs_read_file(void **contents, const char *path) {
	return _fs_read_file(contents, path, false);
}
FS_RESULT
fs_read_file_string(const char **contents, const char *path) {
	return _fs_read_file((void **) contents, path, true);
}

#define NUM_FS_RESULT_MSGS 22
const char *fs_result_msgs[NUM_FS_RESULT_MSGS] = {
	/*FS_OK                  ( 0)*/ "Succeeded",
	/*FS_DISK_ERR            ( 1)*/ "A hard error occurred in the low level disk I/O layer",
	/*FS_INT_ERR             ( 2)*/ "Assertion failed",
	/*FS_NOT_READY           ( 3)*/ "Device not ready",
	/*FS_NO_FILE             ( 4)*/ "Could not find the file",
	/*FS_NO_PATH             ( 5)*/ "Could not find the path",
	/*FS_INVALID_NAME        ( 6)*/ "The path name format is invalid",
	/*FS_DENIED              ( 7)*/ "Access denied due to prohibited access or directory full",
	/*FS_EXIST               ( 8)*/ "Access denied due to prohibited access",
	/*FS_INVALID_OBJECT      ( 9)*/ "The file/directory object is invalid",
	/*FS_WRITE_PROTECTED     (10)*/ "The physical drive is write protected",
	/*FS_INVALID_DRIVE       (11)*/ "The logical drive number is invalid",
	/*FS_NOT_ENABLED         (12)*/ "The volume has no work area",
	/*FS_NO_FILESYSTEM       (13)*/ "There is no valid FAT volume",
	/*FS_MKFS_ABORTED        (14)*/ "The f_mkfs() aborted due to any problem",
	/*FS_TIMEOUT             (15)*/
	"Could not get a grant to access the volume within defined period",
	/*FS_LOCKED              (16)*/
	"The operation is rejected according to the file sharing policy",
	/*FS_NOT_ENOUGH_CORE     (17)*/ "LFN working buffer could not be allocated",
	/*FS_TOO_MANY_OPEN_FILES (18)*/ "Number of open files > FF_FS_LOCK",
	/*FS_INVALID_PARAMETER   (19)*/ "Given parameter is invalid",
	/*FS_FILE_EMPTY          (20)*/ "File is empty",
	/*FS_NOT_ENOUGH_MEMORY   (21)*/ "Not enough memory",
};

const char *
get_fs_result_message(FS_RESULT result) {
	if (result < 0 || result >= NUM_FS_RESULT_MSGS) {
		return "Unknown";
	}
	return fs_result_msgs[result];
}
