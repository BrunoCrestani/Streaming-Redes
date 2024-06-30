#ifndef __MEDIA__
#define __MEDIA__

#define MAX_MEDIA_NAME 63

typedef struct media {
  char name[MAX_MEDIA_NAME];
  unsigned int duration;
  unsigned int startPosition;
  unsigned int endPosition;
} media;

#endif
