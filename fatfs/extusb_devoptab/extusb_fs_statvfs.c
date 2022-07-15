#include "extusb_devoptab.h"

int
__extusb_fs_statvfs(struct _reent *r,
                    const char *path,
                    struct statvfs *buf) {
    //TODO: look up in FATFS struct
    r->_errno = ENOSYS;
    return -1;
}
