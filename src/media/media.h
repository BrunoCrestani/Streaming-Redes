#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

#ifndef __MEDIA__
#define __MEDIA__

typedef struct media {
    off_t     size;    /* total size, in bytes */
    time_t    last_modified;   /* time of last modification */
} Media;

Media* media_new();
int media_stat(const char *path, Media *media);
char* media_to_string(Media *media);

#endif

