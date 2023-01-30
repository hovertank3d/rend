#ifndef __MRERROR_H__
#define __MRERROR_H__

typedef struct mrerror {
    int err;
    char *msg;

} mrerror;

mrerror mrerror_new(const char *message);
mrerror nilerr();

#endif
