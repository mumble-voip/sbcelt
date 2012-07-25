#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG
# define debugf(fmt, ...) \
        do { \
                fprintf(stderr, "libsbcelt:%s():%u: " fmt "\n", \
                    __FILE__, __LINE__, ## __VA_ARGS__); \
                fflush(stderr); \
        } while (0)
#else
# define debugf(s, ...) do{} while (0)
#endif

#endif
