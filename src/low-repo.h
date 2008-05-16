#include <glib.h>

#ifndef _LOW_REPO_H_
#define _LOW_REPO_H_

typedef struct _LowRepo {
    char * id;
    char * name;
    gboolean enabled;
} LowRepo;

#endif /* _LOW_REPO_H__ */
