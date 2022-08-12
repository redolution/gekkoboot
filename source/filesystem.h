#ifndef INC_FILESYSTEM_H
#define INC_FILESYSTEM_H
#include <ogc/disc_io.h>

// See ./fatfs/ff.h:276
typedef enum
{
    FS_OK = 0,              /* ( 0) Succeeded */
    FS_DISK_ERR,            /* ( 1) A hard error occurred in the low level disk I/O layer */
    FS_INT_ERR,             /* ( 2) Assertion failed */
    FS_NOT_READY,           /* ( 3) The physical drive cannot work */
    FS_NO_FILE,             /* ( 4) Could not find the file */
    FS_NO_PATH,             /* ( 5) Could not find the path */
    FS_INVALID_NAME,        /* ( 6) The path name format is invalid */
    FS_DENIED,              /* ( 7) Access denied due to prohibited access or directory full */
    FS_EXIST,               /* ( 8) Access denied due to prohibited access */
    FS_INVALID_OBJECT,      /* ( 9) The file/directory object is invalid */
    FS_WRITE_PROTECTED,     /* (10) The physical drive is write protected */
    FS_INVALID_DRIVE,       /* (11) The logical drive number is invalid */
    FS_NOT_ENABLED,         /* (12) The volume has no work area */
    FS_NO_FILESYSTEM,       /* (13) There is no valid FAT volume */
    FS_MKFS_ABORTED,        /* (14) The f_mkfs() aborted due to any problem */
    FS_TIMEOUT,             /* (15) Could not get a grant to access the volume within defined period */
    FS_LOCKED,              /* (16) The operation is rejected according to the file sharing policy */
    FS_NOT_ENOUGH_CORE,     /* (17) LFN working buffer could not be allocated */
    FS_TOO_MANY_OPEN_FILES, /* (18) Number of open files > FF_FS_LOCK */
    FS_INVALID_PARAMETER,   /* (19) Given parameter is invalid */
    FS_FILE_EMPTY,          /* (20) File is empty */
    FS_NOT_ENOUGH_MEMORY,   /* (21) Not enough memory to malloc file */
} FS_RESULT;
// Changes to this enum should also be made to fs_result_msgs in filesystem.c

FS_RESULT fs_mount(const DISC_INTERFACE *iface_);
void fs_unmount();
void fs_get_volume_label(const char *path, char *label);
FS_RESULT fs_read_file(void **contents, const char *path);
FS_RESULT fs_read_file_string(const char **contents, const char *path);
const char *get_fs_result_message(FS_RESULT result);

#endif
