#include "filesystem.h"
#include <errno.h>
#include <fat.h>
#include <ogc/system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const DISC_INTERFACE *wiface = NULL;
const char *
get_errno_message(int _errno);

FS_RESULT
fs_mount(const DISC_INTERFACE *iface_) {
	wiface = iface_;
	if (!fatMountSimple("sd", wiface)) {
		return FS_NOT_READY;
	}
	return FS_OK;
}

void
fs_unmount() {
	fatUnmount("sd");
	wiface->shutdown();
	wiface = NULL;
}

void
fs_get_volume_label(const char *path, char *label) {
	strcpy(label, "<wiisd>");
}

FS_RESULT
_fs_read_file(void **contents_, const char *path, int is_string) {
	char full_path[strlen(path) + 5];
	sprintf(full_path, "sd:/%s", path);

	errno = 0;
	FILE *file = fopen(full_path, "rb");
	if (!file) {
		if (errno == ENOENT) {
			kprintf("File not found\n");
			return FS_NO_FILE;
		}
		kprintf("->> !! Failed to open file: %s\n", get_errno_message(errno));
		return FS_INT_ERR;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

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
	errno = 0;
	if (!fread(contents, size, 1, file)) {
		kprintf("->> !! Failed to read file: %s\n", get_errno_message(errno));
		return FS_INT_ERR;
	}

	fclose(file);

	// Ensure files read as strings end with NUL character.
	if (is_string) {
		// This is safe because we malloc an extra byte above if reading as string.
		((char *) contents)[size] = '\0';
	}

	*contents_ = contents;
	return FS_OK;
}

#define NUM_ERRNO_MSGS 35
const char *errno_msgs[NUM_ERRNO_MSGS] = {
	/*OK      ( 0)*/ "OK",
	/*EPERM   ( 1)*/ "Operation not permitted",
	/*ENOENT  ( 2)*/ "No such file or directory",
	/*ESRCH   ( 3)*/ "No such process",
	/*EINTR   ( 4)*/ "Interrupted system call",
	/*EIO     ( 5)*/ "I/O error",
	/*ENXIO   ( 6)*/ "No such device or address",
	/*E2BIG   ( 7)*/ "Argument list too long",
	/*ENOEXEC ( 8)*/ "Exec format error",
	/*EBADF   ( 9)*/ "Bad file number",
	/*ECHILD  (10)*/ "No child processes",
	/*EAGAIN  (11)*/ "Try again",
	/*ENOMEM  (12)*/ "Out of memory",
	/*EACCES  (13)*/ "Permission denied",
	/*EFAULT  (14)*/ "Bad address",
	/*ENOTBLK (15)*/ "Block device required",
	/*EBUSY   (16)*/ "Device or resource busy",
	/*EEXIST  (17)*/ "File exists",
	/*EXDEV   (18)*/ "Cross-device link",
	/*ENODEV  (19)*/ "No such device",
	/*ENOTDIR (20)*/ "Not a directory",
	/*EISDIR  (21)*/ "Is a directory",
	/*EINVAL  (22)*/ "Invalid argument",
	/*ENFILE  (23)*/ "File table overflow",
	/*EMFILE  (24)*/ "Too many open files",
	/*ENOTTY  (25)*/ "Not a typewriter",
	/*ETXTBSY (26)*/ "Text file busy",
	/*EFBIG   (27)*/ "File too large",
	/*ENOSPC  (28)*/ "No space left on device",
	/*ESPIPE  (29)*/ "Illegal seek",
	/*EROFS   (30)*/ "Read-only file system",
	/*EMLINK  (31)*/ "Too many links",
	/*EPIPE   (32)*/ "Broken pipe",
	/*EDOM    (33)*/ "Math argument out of domain of func",
	/*ERANGE  (34)*/ "Math result not representable",
};
const char *
get_errno_message(int _errno) {
	if (_errno < 0 || _errno >= NUM_ERRNO_MSGS) {
		return "Unknown";
	}
	return errno_msgs[_errno];
}