#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "media.h"

/*
 * Verify if it is an mp4 file or not
 */
unsigned int isMp4(const char* fileName){

  // Looks for the content after the "."
  const char *fileType = strrchr(fileName, '.');
  return fileType && strcmp(fileType, ".mp4") == 0;
}



media* createMedia(media* media, char* mediaPath){

  strncpy(media->name, mediaPath, MAX_MEDIA_NAME - 1);
  media->name[MAX_MEDIA_NAME - 1] = '\0';

  struct stat st;

  if(stat(mediaPath, &st) == 0){
    media->size = st.st_size;
  } else {
    media->size = 0;
  }

  media->duration = 0;

  return media;
}

/*
 * List and fill the media array
 */
media** listMp4Files(const char *dirName, int *count){
  DIR* dir;
  struct dirent *entry;
  media** mediaList = NULL;
  int i = 0;

  // Case of null dir
  dir = opendir(dirName);
  if (dir == NULL){
    perror("opendir");
    return NULL;
  }

  while((entry = readdir(dir)) != NULL){
    if (entry->d_type == DT_REG && isMp4(entry->d_name)){
      i++;
      mediaList = realloc(mediaList, i * sizeof(media*));
      mediaList[i - 1] = malloc(sizeof(media));
  
      char filePath[512];
      snprintf(filePath, sizeof(filePath), "%s%s", dirName, entry->d_name);

      createMedia(mediaList[i - 1], filePath);
    }
  }
  
  closedir(dir);
  *count = i;
  return mediaList; 
}
