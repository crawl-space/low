#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include "low-package-rpmdb.h"

union rpm_entry {
	void *p;
	char *string;
	char **list;
	uint_32 *flags;
	uint_32 integer;
};

static LowPackage *
low_package_rpmdb_new_from_header (Header header)
{
    union rpm_entry name, epoch, version, release, arch;
	int_32 type, count;

	headerGetEntry(header, RPMTAG_NAME, &type, &name.p, &count);
	headerGetEntry(header, RPMTAG_EPOCH, &type, &epoch.p, &count);
	headerGetEntry(header, RPMTAG_VERSION, &type, &version.p, &count);
	headerGetEntry(header, RPMTAG_RELEASE, &type, &release.p, &count);
	headerGetEntry(header, RPMTAG_ARCH, &type, &arch.p, &count);

	LowPackage *pkg = malloc (sizeof (LowPackage));
	
	pkg->name = name.string;
	pkg->epoch = epoch.string;
	pkg->version = version.string;
	pkg->release = release.string;
	pkg->arch = arch.string;

	return pkg;
}

LowPackageIter *
low_package_iter_next (LowPackageIter *iter)
{
	LowPackageIterRpmdb *iter_rpmdb = (LowPackageIterRpmdb *) iter;
	Header header = rpmdbNextIterator(iter_rpmdb->rpm_iter);

	if (header == NULL) {
		free (iter);
		return NULL;
	}

	free (iter->pkg);
	iter->pkg = low_package_rpmdb_new_from_header (header);

	return iter;
}
