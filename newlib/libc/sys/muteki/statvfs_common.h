#ifndef __SYS_MUTEKI_STATVFS_COMMON_H__
#define __SYS_MUTEKI_STATVFS_COMMON_H__
#include <sys/statvfs.h>
int __statvfs_drive(char drive, struct statvfs *buf);
#endif // __SYS_MUTEKI_STATVFS_COMMON_H__
