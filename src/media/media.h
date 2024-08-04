#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

#ifndef __MEDIA__
#define __MEDIA__

typedef struct media {
    uid_t     user_id;     /* user ID of owner */
    gid_t     group_id;     /* group ID of owner */
    off_t     size;    /* total size, in bytes */
    time_t    last_access;   /* time of last access */
    time_t    last_modified;   /* time of last modification */
    time_t    last_status_change;   /* time of last status change */
} Media;

Media* media_new();
int media_stat(const char *path, Media *media);

#endif

