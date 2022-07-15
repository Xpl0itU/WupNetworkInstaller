#include <sys/iosupport.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../devices.h"
#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

static devoptab_t
        extusb_fs_devoptab =
        {
                .name         = "extusb",
                .structSize   = sizeof(__extusb_fs_file_t),
                .open_r       = __extusb_fs_open,
                .close_r      = __extusb_fs_close,
                .write_r      = __extusb_fs_write,
                .read_r       = __extusb_fs_read,
                .seek_r       = __extusb_fs_seek,
                .fstat_r      = __extusb_fs_fstat,
                .stat_r       = __extusb_fs_stat,
                .link_r       = __extusb_fs_link,
                .unlink_r     = __extusb_fs_unlink,
                .chdir_r      = __extusb_fs_chdir,
                .rename_r     = __extusb_fs_rename,
                .mkdir_r      = __extusb_fs_mkdir,
                .dirStateSize = sizeof(__extusb_fs_dir_t),
                .diropen_r    = __extusb_fs_diropen,
                .dirreset_r   = __extusb_fs_dirreset,
                .dirnext_r    = __extusb_fs_dirnext,
                .dirclose_r   = __extusb_fs_dirclose,
                .statvfs_r    = __extusb_fs_statvfs,
                .ftruncate_r  = __extusb_fs_ftruncate,
                .fsync_r      = __extusb_fs_fsync,
                .deviceData   = NULL,
                .chmod_r      = __extusb_fs_chmod,
                .fchmod_r     = __extusb_fs_fchmod,
                .rmdir_r      = __extusb_fs_rmdir,
        };

static BOOL extusb_fs_initialised = FALSE;

int init_extusb_devoptab() {
    if (extusb_fs_initialised) {
        return 0;
    }

    extusb_fs_devoptab.deviceData = memalign(0x40, sizeof(FATFS));
    if (extusb_fs_devoptab.deviceData == NULL) {
        return -1;
    }

    char mountPath[0x80];
    sprintf(mountPath, "%d:", DEV_USB_EXT);

    int dev = AddDevice(&extusb_fs_devoptab);
    if (dev != -1) {
        setDefaultDevice(dev);

        // Mount the external USB drive
        FRESULT fr = f_mount(extusb_fs_devoptab.deviceData, mountPath, 1);

        if (fr != FR_OK) {
            free(extusb_fs_devoptab.deviceData);
            extusb_fs_devoptab.deviceData = NULL;
            return fr;
        }
        char workDir[0x86];
        // chdir to external USB root for general use
        strcpy(workDir, extusb_fs_devoptab.name);
        strcat(workDir, ":/");
        chdir(workDir);
        extusb_fs_initialised = true;
    } else {
        f_unmount(mountPath);
        free(extusb_fs_devoptab.deviceData);
        extusb_fs_devoptab.deviceData = NULL;
        return dev;
    }

    return 0;
}

int fini_extusb_devoptab() {
    if (!extusb_fs_initialised) {
        return 0;
    }

    int rc = RemoveDevice(extusb_fs_devoptab.name);
    if (rc < 0) {
        return rc;
    }

    char mountPath[0x80];
    sprintf(mountPath, "%d:", DEV_USB_EXT);
    rc = f_unmount(mountPath);
    if (rc != FR_OK) {
        return rc;
    }
    free(extusb_fs_devoptab.deviceData);
    extusb_fs_devoptab.deviceData = NULL;
    extusb_fs_initialised = false;

    return rc;
}

#ifdef __cplusplus
}
#endif