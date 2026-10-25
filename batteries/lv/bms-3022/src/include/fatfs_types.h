/* fatfs_types.h: FatFs R0.12 objects as A316 lays them out (_FS_READONLY 0, _USE_LFN 0,
   _FS_EXFAT 0, _FS_LOCK 0, _USE_FASTSEEK 0, _FS_TINY 0, _MAX_SS 512). Field offsets checked
   against the image: FATFS.win at 0x30, DIR.fn at 0x20, FIL.buf at 0x28. Hand sources in
   build/hand use these.
   name: original (FatFs ff.h types). */
#ifndef FATFS_TYPES_H
#define FATFS_TYPES_H

typedef enum {
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
    FR_NOT_READY,
    FR_NO_FILE,
    FR_NO_PATH,
    FR_INVALID_NAME,
    FR_DENIED,
    FR_EXIST,
    FR_INVALID_OBJECT,
    FR_WRITE_PROTECTED,
    FR_INVALID_DRIVE,
    FR_NOT_ENABLED,
    FR_NO_FILESYSTEM,
    FR_MKFS_ABORTED,
    FR_TIMEOUT,
    FR_LOCKED,
    FR_NOT_ENOUGH_CORE,
    FR_TOO_MANY_OPEN_FILES,
    FR_INVALID_PARAMETER
} FRESULT;

typedef struct {
    unsigned char fs_type; /* 0x00 */
    unsigned char drv;
    unsigned char n_fats;
    unsigned char wflag;
    unsigned char fsi_flag;   /* 0x04 */
    unsigned short id;        /* 0x06 */
    unsigned short n_rootdir; /* 0x08 */
    unsigned short csize;     /* 0x0a */
    unsigned int last_clst;   /* 0x0c */
    unsigned int free_clst;   /* 0x10 */
    unsigned int n_fatent;    /* 0x14 */
    unsigned int fsize;       /* 0x18 */
    unsigned int volbase;     /* 0x1c */
    unsigned int fatbase;     /* 0x20 */
    unsigned int dirbase;     /* 0x24 */
    unsigned int database;    /* 0x28 */
    unsigned int winsect;     /* 0x2c */
    unsigned char win[512];   /* 0x30 */
} FATFS;

typedef struct {
    FATFS *fs;            /* 0x00 */
    unsigned short id;    /* 0x04 */
    unsigned char attr;   /* 0x06 */
    unsigned char stat;   /* 0x07 */
    unsigned int sclust;  /* 0x08 */
    unsigned int objsize; /* 0x0c */
} FFOBJID;

typedef struct {
    FFOBJID obj;          /* 0x00 */
    unsigned int dptr;    /* 0x10 */
    unsigned int clust;   /* 0x14 */
    unsigned int sect;    /* 0x18 */
    unsigned char *dir;   /* 0x1c */
    unsigned char fn[12]; /* 0x20 */
} FF_DIR;

typedef struct {
    FFOBJID obj;            /* 0x00 */
    unsigned char flag;     /* 0x10 */
    unsigned char err;      /* 0x11 */
    unsigned int fptr;      /* 0x14 */
    unsigned int clust;     /* 0x18 */
    unsigned int sect;      /* 0x1c */
    unsigned int dir_sect;  /* 0x20 */
    unsigned char *dir_ptr; /* 0x24 */
    unsigned char buf[512]; /* 0x28 */
} FF_FIL;

#endif
