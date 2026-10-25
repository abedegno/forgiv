/* ff.h: FatFs R0.12c (src/lib/fatfs, ff.c) as built into A316, used by usb/usbh_usr.c for the
   update files on a USB stick.
   name: original (FatFs). */
#ifndef FF_H
#define FF_H

#include "firmware_types.h"
#include "usb_otg_types.h"
#include "usbh_types.h"
#include "fatfs_types.h"

/* data */
extern uchar Stat;
extern ushort f_ac1; /* not FatFs: IR13 grid frequency (power_flow_update); declared here by the
                         header generator */
extern int FatFs;
extern uchar Fsid[];

/* functions */
uchar check_fs(FATFS *fs, uint sect);
uchar chk_chr(byte *param_1, uint param_2);
uint clust2sect(int param_1, uint param_2);
uint create_chain(void *obj, uint clst);
int create_name(void *dp, const char **path);
int dir_alloc(int param_1, int param_2);
int dir_find(int param_1);
uint dir_next(int *param_1, int param_2);
int dir_read(int param_1, int param_2);
int dir_register(int param_1);
int dir_remove(int *param_1);
int dir_sdi(int param_1, int param_2);
int f_close(int param_1);
int f_closedir(int *param_1);
int f_getfree(int param_1, int param_2, int param_3);
int f_lseek(int param_1, int param_2);
int f_mkdir(int param_1);
int f_mount(int param_1, int param_2, int param_3);
int f_open(FF_FIL *fp, const char *path, uchar mode);
int f_opendir(int param_1, int param_2);
int f_read(FF_FIL *fp, void *buff, uint btr, uint *br);
int f_readdir(int *param_1, uint *param_2);
int f_rename(const char *path_old, const char *path_new);
int f_stat(int param_1, int param_2);
int f_sync(int param_1);
int f_truncate(int param_1);
int f_unlink(int param_1);
int f_write(int param_1, int param_2, int param_3, int param_4);
int find_volume(const char **path, FATFS **rfs, uchar mode);
int follow_path(void *dp, const char *path);
uint get_fat(void *obj, uint clst);
uint get_fattime();
void get_fileinfo(int param_1, int param_2);
int get_ldnumber(const char **path);
int ld_clust(int param_1, int param_2);
uint ld_dword(uchar *p);
ushort ld_word(uchar *p);
int mem_cmp(void *a, void *b, int n);
void mem_cpy(void *d, void *s, int n);
void mem_set(void *d, uchar v, int n);
int move_window(int param_1, int param_2);
int put_fat(void *fs_, uint clst, uint val);
int remove_chain(int param_1, int param_2, int param_3);
void st_clust(char *param_1, int param_2, uint param_3);
uchar *st_dword(uchar *p, uint v);
uchar *st_word(uchar *p, int v);
int sync_fs(char *param_1);
int sync_window(int param_1);
int validate(int param_1, int param_2);

#endif
