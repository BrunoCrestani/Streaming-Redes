#include <stdlib.h>
#include <string.h>
#include "media.h"

Media *media_new()
{
  Media *media = malloc(sizeof(Media));
  memset(media, 0, sizeof(Media));
  if (media == NULL)
  {
    return NULL;
  }
  return media;
}

int media_stat(const char *path, Media *media)
{
  struct stat st;
  if (stat(path, &st) == -1)
  {
    return -1;
  }
  media->user_id = st.st_uid;
  media->group_id = st.st_gid;
  media->size = st.st_size;
  media->last_access = st.st_atime;
  media->last_modified = st.st_mtime;
  media->last_status_change = st.st_ctime;
  return 0;
}