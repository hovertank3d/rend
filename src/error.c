#include "error.h"

#include <string.h>
#include <stdio.h>

mrerror mrerror_new(const char const *message)
{
    static char msgbuf[2][512];
    static int msgidx = 0;

    mrerror ret;

    strcpy(msgbuf[msgidx], message);
    ret.err = 1;
    ret.msg = msgbuf[msgidx];

    msgidx ^= 1;

    return ret;
}

mrerror nilerr()
{
    return (mrerror){0};
}