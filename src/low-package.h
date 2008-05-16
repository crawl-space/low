
#ifndef _LOW_PACKAGE_H_
#define _LOW_PACKAGE_H_

typedef struct _LowPackage {
    const char *name;
    const char *version;
    const char *release;
    const char *arch;
    const char *epoch;
} LowPackage;

typedef struct _LowPackageIter {
       LowPackage *pkg;
} LowPackageIter;

#endif /* _LOW_PACKAGE_H_ */
