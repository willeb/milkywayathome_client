/* ************************************************************************** */
/* UTIL: various useful routines and functions. */
/* */
/* Copyright (c) 1993 by Joshua E. Barnes, Honolulu, HI. */
/* It's free because it's yours. */
/* ************************************************************************** */

#include "stdinc.h"
#include "real.h"
#include "vectmath.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>

#ifndef HZ
#  include <time.h>
#  define HZ CLK_TCK
#endif

void error(char*, ...);

extern double drand48(void);            /* should be in math.h */

/*  * XRANDOM: generate floating-point random number.
 */

real xrandom(real xl, real xh)
{
    return (xl + (xh - xl) * drand48());
}

/*  * RSQR: compute x*x.
 */

real rsqr(real x)
{
    return (x * x);
}

/*  * DISTV: subtract vectors and return distance between.
 */

real distv(vector v, vector u)
{
    real s, d;
    int n = NDIM;

    s = 0.0;
    while (--n >= 0)
    {
        d = (*v++) - (*u++);
        s += d * d;
    }
    return (rsqrt(s));
}

/*  * SCANOPT: scan char* of the form "word1,word2,..." for match. Warning:
 * words must be separated by exactly one comma -- no spaces allowed!
 */

bool scanopt(char* opt, char* key)
{
    char* op, *kp;

    op = (char*) opt;               /* start scan of ps.options */
    while (*op != NULL)             /* loop over words in opt */
    {
        kp = (char*) key;           /* start at front of key */
        while ((*op != ',' ? *op : NULL)    /* loop while this word */
                == *kp)            /* ...matches text of key */
        {
            if (*kp++ == NULL)          /* at end of key word? */
                return (TRUE);          /* keyword found */
            op++;               /* else advance ptrs */
        }
        while (*op != NULL && *op++ != ',') /* loop till end of word, */
            continue;               /* passing "," at end */
    }
    return (FALSE);             /* keyword not found */
}

/*  * CPUTIME: compute CPU time in minutes.
 */

real cputime()
{
    struct tms buffer;

    if (times(&buffer) == -1)
        error("times() call failed\n");
    return (buffer.tms_utime / (60.0 * HZ));
}

/*  * ALLOCATE: memory allocation with error checking.
 */

void* allocate(nb)
int nb;
{
    void* mem;

    mem = (void*) calloc(nb, 1);        /* calloc zeros memory */
    if (mem == NULL)
        error("allocate: not enuf memory (%d bytes)\n", nb);
    return (mem);
}

/*  * ERROR: print error message and exit.
 */

void error(char* msg, ...)
{
    va_list args;

    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    exit(-1);                   /* quit with error status */
}

/*  * EPRINTF: print error message, but don't exit.
 */

void eprintf(char* msg, ...)
{
    va_list args;

    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

