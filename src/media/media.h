#ifndef __MEDIA__
#define __MEDIA__

#define MAX_MEDIA_NAME 63

typedef struct media {
  char name[MAX_MEDIA_NAME];
  unsigned int duration;
  unsigned int size;
} media;


/*
 * Verify if it is an mp4 file or not
 */
unsigned int isMp4(const char* filename);

/*
 * Fill the media struct with the content gotten
 * from the file
 */
media* createMedia(media* media, char* mediaName);

/*
 * List and fill the media array
 */
media** listMp4Files(const char *dir, int *count);

#endif

