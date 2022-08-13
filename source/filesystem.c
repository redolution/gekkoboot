#include "filesystem.h"
#include <stdlib.h>
#include <ogc/system.h>

#ifndef DOLPHIN_BUILD
#include "fatfs/ff.h"
#include "ffshim.h"

FATFS fs;
FS_RESULT fs_mount(const DISC_INTERFACE *iface_)
{
    iface = iface_;
    return f_mount(&fs, "", 1);
}

void fs_unmount()
{
    f_unmount("");
    iface->shutdown();
    iface = NULL;
}

void fs_get_volume_label(const char *path, char *label)
{
    FRESULT res = f_getlabel(path, label, NULL);
    if (res != FR_OK)
    {
        *label = '\0';
    }
}

FS_RESULT _fs_read_file(void **contents_, const char *path, int is_string)
{
    FIL file;
    FRESULT result = f_open(&file, path, FA_READ);
    if (result != FR_OK)
    {
        if (result == FR_NO_FILE || result == FR_NO_PATH)
        {
            kprintf("File not found\n");
            return FS_NO_FILE;
        }
        kprintf("->> !! Failed to open file: %s\n", get_fs_result_message(result));
        return result;
    }

    size_t size = f_size(&file);
    if (size <= 0)
    {
        kprintf("->> !! File is empty\n");
        return FS_FILE_EMPTY;
    }
    kprintf("File size: %iB\n", size);

    // Malloc an extra byte if we are reading as a string incase we need to add NUL character.
    void *contents = malloc(size + (is_string ? 1 : 0));
    if (!contents)
    {
        kprintf("->> !! Couldn't allocate memory for file\n");
        return FS_NOT_ENOUGH_MEMORY;
    }

    kprintf("Reading file...\n");
    UINT _;
    result = f_read(&file, contents, size, &_);
    if (result != FR_OK)
    {
        kprintf("->> !! Failed to read file: %s\n", get_fs_result_message(result));
        return result;
    }

    f_close(&file);

    // Ensure files read as strings end with NUL character.
    if (is_string)
    {
        // This is safe because we malloc an extra byte above if reading as string.
        ((char *)contents)[size] = '\0';
    }

    *contents_ = contents;
    return FS_OK;
}

#else
#include <fat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

const DISC_INTERFACE *wiface = NULL;
const char *get_errno_message(int _errno);

FS_RESULT fs_mount(const DISC_INTERFACE *iface_)
{
    wiface = iface_;
    if (!fatMountSimple("sd", wiface))
    {
        return FS_NOT_READY;
    }
    return FS_OK;
}

void fs_unmount()
{
    fatUnmount("sd");
    wiface->shutdown();
    wiface = NULL;
}

void fs_get_volume_label(const char *path, char *label)
{
    strcpy(label, "<wiisd>");
}

FS_RESULT _fs_read_file(void **contents_, const char *path, int is_string)
{
    char full_path[strlen(path) + 5];
    sprintf(full_path, "sd:/%s", path);

    errno = 0;
    FILE *file = fopen(full_path, "rb");
    if (!file)
    {
        if (errno == ENOENT)
        {
            kprintf("File not found\n");
            return FS_NO_FILE;
        }
        kprintf("->> !! Failed to open file: %s\n", get_errno_message(errno));
        return FS_INT_ERR;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    if (size <= 0)
    {
        kprintf("->> !! File is empty\n");
        return FS_FILE_EMPTY;
    }
    kprintf("File size: %iB\n", size);

    // Malloc an extra byte if we are reading as a string incase we need to add NUL character.
    void *contents = malloc(size + (is_string ? 1 : 0));
    if (!contents)
    {
        kprintf("->> !! Couldn't allocate memory for file\n");
        return FS_NOT_ENOUGH_MEMORY;
    }

    kprintf("Reading file...\n");
    errno = 0;
    if (!fread(contents, size, 1, file))
    {
        kprintf("->> !! Failed to read file: %s\n", get_errno_message(errno));
        return FS_INT_ERR;
    }

    fclose(file);

    // Ensure files read as strings end with NUL character.
    if (is_string)
    {
        // This is safe because we malloc an extra byte above if reading as string.
        ((char *)contents)[size] = '\0';
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
const char *get_errno_message(int _errno)
{
    if (_errno < 0 || _errno >= NUM_ERRNO_MSGS)
    {
        return "Unknown";
    }
    return errno_msgs[_errno];
}
#endif

FS_RESULT fs_read_file(void **contents, const char *path)
{
    return _fs_read_file(contents, path, false);
}
FS_RESULT fs_read_file_string(const char **contents, const char *path)
{
    return _fs_read_file((void **)contents, path, true);
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
    /*FS_TIMEOUT             (15)*/ "Could not get a grant to access the volume within defined period",
    /*FS_LOCKED              (16)*/ "The operation is rejected according to the file sharing policy",
    /*FS_NOT_ENOUGH_CORE     (17)*/ "LFN working buffer could not be allocated",
    /*FS_TOO_MANY_OPEN_FILES (18)*/ "Number of open files > FF_FS_LOCK",
    /*FS_INVALID_PARAMETER   (19)*/ "Given parameter is invalid",
    /*FS_FILE_EMPTY          (20)*/ "File is empty",
    /*FS_NOT_ENOUGH_MEMORY   (21)*/ "Not enough memory",
};

const char *get_fs_result_message(FS_RESULT result)
{
    if (result < 0 || result >= NUM_FS_RESULT_MSGS)
    {
        return "Unknown";
    }
    return fs_result_msgs[result];
}
