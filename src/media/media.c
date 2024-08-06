#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
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
  media->size = st.st_size;
  media->last_modified = st.st_mtime;

  return 0;
}

char* media_to_string(Media *media)
{
  char *str = malloc(63);
  memset(str, '\0', 63);
 
  // convert last modified to DD/MM/YYYY HH:MM:SS
  char last_modified[20];
  strftime(last_modified, 20, "%d/%m/%Y %H:%M:%S", localtime(&media->last_modified));

  sprintf(str, "%ld kB | %s", media->size / 1000, last_modified);
  return str;
}