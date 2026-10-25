/* at: dir_sdi 0x08031628, dir_next 0x080316C2, dir_alloc 0x080317CE, ld_clust 0x0803182E */
/* at: st_clust 0x08031854 */
/* at: dir_read 0x08031878 */
/* at: dir_find 0x080318EC */
/* at: dir_register 0x08031958 */
/* at: dir_remove 0x08031994 */
/* at: get_fileinfo 0x080319B4 */
/* at: create_name 0x08031A40, follow_path 0x08031B0C, get_ldnumber 0x08031BAE, check_fs 0x08031BF6, find_volume 0x08031C6A, validate 0x08031F72, f_mount 0x08031FAE, f_open 0x08032004, f_read 0x080321F2, f_write 0x0803239E */
/* at: FatFs 0x200004E8, Fsid 0x200004EC */
/* at: f_sync 0x0803257C */
/* at: f_close 0x0803261C, f_lseek 0x0803263E, f_opendir 0x080327C2, f_closedir 0x08032832, f_readdir 0x0803284A */
/* at: f_stat 0x08032898, f_getfree 0x080328D6, f_truncate 0x080329D2 */
/* at: f_unlink 0x08032A84, f_mkdir 0x08032B2E */
/* at: f_rename 0x08032C8C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* FatFs - Generic FAT file system module R0.12c. ChaN's licence notice, kept as it asks:

   Copyright (C) 2017, ChaN, all right reserved.

   FatFs module is an open source software. Redistribution and use of FatFs in
   source and binary forms, with or without modification, are permitted provided
   that the following condition is met:

   1. Redistributions of source code must retain the above copyright notice,
      this condition and the following disclaimer.

   This software is provided by the copyright holder and contributors "AS IS"
   and any warranties related to this software are DISCLAIMED.
   The copyright owner or contributors be NOT LIABLE for any damages caused
   by use of this software. */
/* FatFs R0.12c (ChaN), ff.c, second part: directories, names, mounting and the f_ API
   (dir_sdi to f_rename, in ff.c's order, with the ExCvt table).
   Job in this firmware: the FAT file system on a USB stick (the USB port in host mode).
   The application (src/app/usb/usbh_usr.c) mounts "0:", reads DSPStore.bin and ARMStore.bin
   (the firmware updates usbh_usr.c passes to the DSP, the ARM and the battery BMS) and the text
   files SN.txt and MSN.txt, and deletes the update files with f_unlink when it has used them.
   It never writes or creates a file: f_write, f_mkdir, f_rename and the rest are in the image
   only because ff.c was built as one section, so armlink keeps all of it.
   Configuration seen in the code (src/include/fatfs_types.h, docs/matching.md round 4):
   _FS_READONLY 0, _FS_MINIMIZE 0, _USE_LFN 0 (8.3 names only), _FS_RPATH 0, _FS_EXFAT 0,
   _FS_LOCK 0, _USE_FASTSEEK 0, _FS_TINY 0 (each FIL has its own 512-byte buffer), _MAX_SS 512,
   one volume, code page 437 (ExCvt), no f_mkfs, f_chmod, f_utime, f_findfirst or string
   functions. get_fattime returns 0. The disk layer is usbh_msc_fatfs.c.
   Changes from R0.12c: none found. The functions follow ChaN's source in order and shape.
   match: ff.c is two files here, ff_p01.c (old parts 1-8) and ff_p09.c (old parts 9-21),
   named after the first old part each holds (docs/readability.md, step 4). Joined, armcc knows
   ld_word and st_word when it compiles ld_clust, st_clust and get_fileinfo and saves fewer
   registers in them than the image does. So in the vendor's build those callers did not see the
   callees' definitions in the same compile, and one file does not give the bytes.
   name: original (map/filenames.tsv). ChaN's function names. R0.12c and not an earlier R0.12:
   create_chain tests free_clst <= n_fatent - 2, and f_lseek moves fptr before following the chain. */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/ff.h"
#include "../../include/fatfs_types.h"
#include "../../include/usb.h"

/* Set a directory object to entry param_2 (cluster and sector of that entry). */
int dir_sdi(int param_1, int param_2)
{
  int v6;
  int v7;
  int v8;

  v6 = *(int *)param_1;
  if ((uint)param_2 >= 0x200000 || (param_2 & 0x1f) != 0) {
    return 2;
  }
  *(int *)(param_1 + 0x10) = param_2;
  v7 = *(int *)(param_1 + 8);
  if (v7 == 0 && *(uchar *)v6 >= 3) {
    v7 = *(int *)(v6 + 0x24);
  }
  if (v7 == 0) {
    if ((uint)*(ushort *)(v6 + 8) <= (uint)param_2 >> 5) {
      return 2;
    }
    *(int *)(param_1 + 0x18) = *(int *)(v6 + 0x24);
  }
  else {
    v8 = *(ushort *)(v6 + 0xa) << 9;
    while ((uint)param_2 >= v8) {
      v7 = get_fat(param_1, v7);
      if (v7 + 1 == 0) {
        return 1;
      }
      if ((uint)v7 < 2 || (uint)*(int *)(v6 + 0x14) <= v7) {
        return 2;
      }
      param_2 = param_2 - v8;
    }
    *(int *)(param_1 + 0x18) = clust2sect(v6, v7);
  }
  *(int *)(param_1 + 0x14) = v7;
  if (*(int *)(param_1 + 0x18) == 0) {
    return 2;
  }
  *(int *)(param_1 + 0x18) = *(int *)(param_1 + 0x18) + ((uint)param_2 >> 9);
  *(int *)(param_1 + 0x1c) = v6 + 0x30 + (param_2 & 0x1ff);
  return 0;
}


/* Move to the next directory entry; with param_2 set, stretch the directory when needed.
   Returns FR_NO_FILE (4) at the end. */
uint dir_next(int * param_1, int param_2)
{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;

  iVar3 = *param_1;
  uVar4 = param_1[4] + 0x20;
  if ((param_1[6] == 0) || (uVar4 >= 0x200000)) {
    return 4;
  }
  if ((uVar4 & 0x1ff) == 0) {
    param_1[6] = param_1[6] + 1;
    if (param_1[5] == 0) {
      if ((uint)*(ushort *)(iVar3 + 8) <= uVar4 >> 5) {
        param_1[6] = 0;
        return 4;
      }
    }
    else if ((*(ushort *)(iVar3 + 10) - 1 & uVar4 >> 9) == 0) {
      uVar1 = get_fat(param_1,param_1[5]);
      if (uVar1 <= 1) {
        return 2;
      }
      if (uVar1 == 0xffffffff) {
        return 1;
      }
      if (*(uint *)(iVar3 + 0x14) <= uVar1) {
        if (param_2 == 0) {
          param_1[6] = 0;
          return 4;
        }
        uVar1 = create_chain(param_1,param_1[5]);
        if (uVar1 == 0) {
          return 7;
        }
        if (uVar1 == 1) {
          return 2;
        }
        if (uVar1 == 0xffffffff) {
          return 1;
        }
        if ((sync_window(iVar3)) != 0) {
          return 1;
        }
        mem_set((uchar *)(iVar3 + 0x30),0,0x200);
        for (uVar5 = 0, *(int *)(iVar3 + 0x2c) = (clust2sect(iVar3,uVar1)); uVar5 < *(ushort *)(iVar3 + 10); uVar5 = uVar5 + 1, *(int *)(iVar3 + 0x2c) = *(int *)(iVar3 + 0x2c) + 1) {
          *(uchar *)(iVar3 + 3) = 1;
          if ((sync_window(iVar3)) != 0) {
            return 1;
          }
        }
        *(uint *)(iVar3 + 0x2c) = *(int *)(iVar3 + 0x2c) - uVar5;
      }
      param_1[5] = uVar1;
      param_1[6] = (clust2sect(iVar3,uVar1));
    }
  }
  param_1[4] = uVar4;
  param_1[7] = iVar3 + 0x30 + (uVar4 & 0x1ff);
  return 0;
}


/* Find param_2 free consecutive entries in a directory, stretching it if needed. */
int dir_alloc(int param_1, int param_2)
{
  int v5;
  int v7;
  int v8;

  v8 = *(int *)param_1;
  v5 = dir_sdi(param_1, 0);
  if (v5 == 0) {
    v7 = 0;
    do {
      v5 = move_window(v8, *(int *)(param_1 + 0x18));
      if (v5 != 0) {
        break;
      }
      if (*(uchar *)(*(int *)(param_1 + 0x1c)) == 0xe5 || *(uchar *)(*(int *)(param_1 + 0x1c)) == 0) {
        if (++v7 == param_2) break;
      }
      else {
        v7 = 0;
      }
      v5 = dir_next(param_1, 1);
    } while (v5 == 0);
  }
  if (v5 == 4) {
    v5 = 7;
  }
  return v5;
}


/* First cluster of a directory entry (high word only on FAT32). */
int ld_clust(int param_1, int param_2)
{
  int v6;

  v6 = ld_word(param_2 + 0x1a);
  if (*(uchar *)param_1 == 3) {
    v6 = v6 | ld_word(param_2 + 0x14) << 0x10;
  }
  return v6;
}

/* Store the first cluster in a directory entry. */
void st_clust(char * param_1, int param_2, uint param_3)
{

  st_word((uchar *)(param_2 + 0x1a),param_3 & 0xffff);
  if (*param_1 == '\x03') {
    st_word((uchar *)(param_2 + 0x14),param_3 >> 0x10);
  }
  return;
}

/* Read the next used entry of a directory (f_readdir). */
int dir_read(int param_1, int param_2)
{
  int v5;
  uchar v6;
  uchar v7;
  int v9;

  v5 = 4;
  v9 = *(int *)param_1;
  while (*(int *)(param_1 + 0x18) != 0) {
    v5 = move_window(v9, *(int *)(param_1 + 0x18));
    if (v5 != 0) {
      break;
    }
    v6 = *(uchar *)(*(int *)(param_1 + 0x1c));
    if (v6 == 0) {
      v5 = 4;
      break;
    }
    v7 = *(uchar *)(*(int *)(param_1 + 0x1c) + 0xb) & 0x3f;
    *(uchar *)(param_1 + 6) = v7;
    if (v6 != 0xe5 && v6 != 0x2e && v7 != 0xf) {
      if (((v7 & -33) == 8) == param_2) {
        break;
      }
    }
    v5 = dir_next(param_1, 0);
    if (v5 != 0) {
      break;
    }
  }
  if (v5 != 0) {
    *(int *)(param_1 + 0x18) = 0;
  }
  return v5;
}

/* Find the entry whose 8.3 name is in the object's fn[]. */
int dir_find(int param_1)
{
  int v5;
  uchar v6;
  int v7;

  v7 = *(int *)param_1;
  v5 = dir_sdi(param_1, 0);
  if (v5 != 0) {
    return v5;
  }
  do {
    v5 = move_window(v7, *(int *)(param_1 + 0x18));
    if (v5 != 0) {
      break;
    }
    v6 = *(uchar *)(*(int *)(param_1 + 0x1c));
    if (v6 == 0) {
      v5 = 4;
      break;
    }
    *(uchar *)(param_1 + 6) = *(uchar *)(*(int *)(param_1 + 0x1c) + 0xb) & 0x3f;
    if ((*(uchar *)(*(int *)(param_1 + 0x1c) + 0xb) & 8) == 0 && mem_cmp(*(int *)(param_1 + 0x1c), param_1 + 0x20, 0xb) == 0) {
      break;
    }
    v5 = dir_next(param_1, 0);
  } while (v5 == 0);
  return v5;
}

/* Create a new entry with the name in fn[]. */
int dir_register(int param_1)
{
  int v5;
  int v6;

  v6 = *(int *)param_1;
  v5 = dir_alloc(param_1, 1);
  if (v5 == 0) {
    v5 = move_window(v6, *(int *)(param_1 + 0x18));
    if (v5 == 0) {
      mem_set(*(int *)(param_1 + 0x1c), 0, 0x20);
      mem_cpy(*(int *)(param_1 + 0x1c), param_1 + 0x20, 0xb);
      *(uchar *)(v6 + 3) = 1;
    }
  }
  return v5;
}

/* Mark the current entry deleted (0xE5). */
int dir_remove(int * param_1)
{
  int iVar1;
  int iVar2;

  iVar2 = *param_1;
  iVar1 = move_window(iVar2,param_1[6]);
  if (iVar1 == 0) {
    *(uchar *)param_1[7] = 0xe5;
    *(uchar *)(iVar2 + 3) = 1;
  }
  return iVar1;
}

/* Fill a FILINFO from the current entry: the 8.3 name with its dot, attributes,
   size, date and time. */
void get_fileinfo(int param_1, int param_2)
{
  int v6;
  int v7;
  int v8;
  uchar v9;

  *(uchar *)(param_2 + 9) = 0;
  if (*(int *)(param_1 + 0x18) == 0) {
    return;
  }
  v6 = 0;
  v7 = v6;
  while ((uint)v6 < 0xb) {
    v9 = *(uchar *)(*(int *)(param_1 + 0x1c) + v6++);
    if (v9 == 0x20) {
      continue;
    }
    if (v9 == 5) {
      v9 = 0xe5;
    }
    if (v6 == 9) {
      *(uchar *)(param_2 + 9 + v7++) = 0x2e;
    }
    *(uchar *)(param_2 + 9 + v7++) = v9;
  }
  *(uchar *)(param_2 + 9 + v7) = 0;
  *(uchar *)(param_2 + 8) = *(uchar *)(*(int *)(param_1 + 0x1c) + 0xb);
  *(int *)param_2 = ld_dword(*(int *)(param_1 + 0x1c) + 0x1c);
  v8 = ld_dword(*(int *)(param_1 + 0x1c) + 0x16);
  *(ushort *)(param_2 + 6) = v8;
  *(ushort *)(param_2 + 4) = (uint)v8 >> 0x10;
  return;
}

/* FatFs: upper case conversion of the extended characters, code page 437 (_EXCVT) */
static const uchar ExCvt[128] = {
  0x80, 0x9A, 0x45, 0x41, 0x8E, 0x41, 0x8F, 0x80, 0x45, 0x45, 0x45, 0x49, 0x49, 0x49, 0x8E, 0x8F,
  0x90, 0x92, 0x92, 0x4F, 0x99, 0x4F, 0x55, 0x55, 0x59, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
  0x41, 0x49, 0x4F, 0x55, 0xA5, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
  0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
  0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* Turn the next path segment into an upper-case 8.3 name in fn[] (ExCvt for codes
   above 0x7F) and move *path on. */
int create_name(void *dp, const char **path)
{
  uchar c, *sfn;
  uint ni, si, i;
  const char *p;

  p = *path; sfn = ((FF_DIR *)dp)->fn;
  mem_set(sfn, ' ', 11);
  si = i = 0; ni = 8;
  for (;;) {
    c = (uchar)p[si++];
    if (c <= ' ') break;
    if (c == '/' || c == '\\') {
      while (p[si] == '/' || p[si] == '\\') si++;
      break;
    }
    if (c == '.' || i >= ni) {
      if (ni == 11 || c != '.') return FR_INVALID_NAME;
      i = 8; ni = 11;
      continue;
    }
    if (c >= 0x80) {
      c = ExCvt[c - 0x80];
    }
    {
      if (chk_chr("\"*+,:;<=>\?[]|\x7F", c)) return FR_INVALID_NAME;
      if (c >= 'a' && c <= 'z') c -= 0x20;
      sfn[i++] = c;
    }
  }
  *path = p + si;
  if (i == 0) return FR_INVALID_NAME;

  if (sfn[0] == 0xE5) sfn[0] = 0x05;
  sfn[11] = (c <= ' ') ? 0x04 : 0;

  return FR_OK;
}

/* Walk a path from the root, one segment at a time, to the entry it names. */
int follow_path(void *dp, const char *path)
{
  int res;
  uchar ns;
  FFOBJID *obj = &((FF_DIR *)dp)->obj;
  FATFS *fs = obj->fs;

  {
    while (*path == '/' || *path == '\\') path++;
    obj->sclust = 0;
  }

  if ((uint)*path < ' ') {
    ((FF_DIR *)dp)->fn[11] = 0x80;
    res = dir_sdi(dp, 0);
  } else {
    for (;;) {
      res = create_name(dp, &path);
      if (res != FR_OK) break;
      res = dir_find(dp);
      ns = ((FF_DIR *)dp)->fn[11];
      if (res != FR_OK) {
        if (res == FR_NO_FILE) {
          if (0 && (ns & 0x20)) {
            if (!(ns & 0x04)) continue;
            ((FF_DIR *)dp)->fn[11] = 0x80;
            res = FR_OK;
          } else {
            if (!(ns & 0x04)) res = FR_NO_PATH;
          }
        }
        break;
      }
      if (ns & 0x04) break;
      if (!(obj->attr & 0x10)) {
        res = FR_NO_PATH; break;
      }
      {
        obj->sclust = ld_clust(fs, fs->win + ((FF_DIR *)dp)->dptr % 512);
      }
    }
  }

  return res;
}

/* Logical drive number from a "0:" prefix; with one volume only 0 is valid. */
int get_ldnumber(const char **path)
{
  const char *tp, *tt;
  uint i;
  int vol = -1;

  if (*path) {
    for (tt = *path; (uint)*tt >= (0 ? ' ' : '!') && *tt != ':'; tt++) ;
    if (*tt == ':') {
      tp = *path;
      i = *tp++ - '0';
      if (i < 10 && tp == tt) {
        if (i < 1) {
          vol = (int)i;
          *path = ++tt;
        }
      }
      return vol;
    }
    vol = 0;
  }
  return vol;
}

/* Classify a sector: 0 a FAT boot sector, 2 a boot sector without FAT, 3 not a boot
   sector, 4 a disk error (1, exFAT, cannot occur with _FS_EXFAT 0). */
uchar check_fs(FATFS *fs, uint sect)
{
  fs->wflag = 0; fs->winsect = 0xFFFFFFFF;
  if (move_window(fs, sect) != FR_OK) return 4;

  if (ld_word(fs->win + 510) != 0xAA55) return 3;

  if (fs->win[0] == 0xE9 || (fs->win[0] == 0xEB && fs->win[0 + 2] == 0x90)) {
    if ((ld_dword(fs->win + 54) & 0xFFFFFF) == 0x544146) return 0;
    if (ld_dword(fs->win + 82) == 0x33544146) return 0;
  }
  return 2;
}


/* Check the volume is mounted, mount it if not: disk_initialize, find the FAT
   boot sector (whole disk or first partition), read the BPB and FSInfo. */
int find_volume(const char **path, FATFS **rfs, uchar mode)
{
  uchar fmt, *pt;
  int vol;
  int stat;
  uint bsect, fasize, tsect, sysect, nclst, szbfat, br[4];
  ushort nrsv;
  FATFS *fs;
  uint i;

  *rfs = 0;
  vol = get_ldnumber(path);
  if (vol < 0) return FR_INVALID_DRIVE;

  fs = ((FATFS **)((uchar *)&FatFs))[vol];
  if (!fs) return FR_NOT_ENABLED;

  *rfs = fs;

  mode &= (uchar)~0x01;
  if (fs->fs_type) {
    stat = disk_status(fs->drv);
    if (!(stat & 0x01)) {
      if (!0 && mode && (stat & 0x04)) {
        return FR_WRITE_PROTECTED;
      }
      return FR_OK;
    }
  }

  fs->fs_type = 0;
  fs->drv = (uchar)(vol);
  stat = disk_initialize(fs->drv);
  if (stat & 0x01) {
    return FR_NOT_READY;
  }
  if (!0 && mode && (stat & 0x04)) {
    return FR_WRITE_PROTECTED;
  }
  bsect = 0;
  fmt = check_fs(fs, bsect);
  if (fmt == 2 || (fmt < 2 && 0 != 0)) {
    for (i = 0; i < 4; i++) {
      pt = fs->win + (446 + i * 16);
      br[i] = pt[4] ? ld_dword(pt + 8) : 0;
    }
    i = 0;
    if (i) i--;
    do {
      bsect = br[i];
      fmt = bsect ? check_fs(fs, bsect) : 3;
    } while (0 == 0 && fmt >= 2 && ++i < 4);
  }
  if (fmt == 4) return FR_DISK_ERR;
  if (fmt >= 2) return FR_NO_FILESYSTEM;

  {
    if (ld_word(fs->win + 11) != 512) return FR_NO_FILESYSTEM;

    fasize = ld_word(fs->win + 22);
    if (fasize == 0) fasize = ld_dword(fs->win + 36);
    fs->fsize = fasize;

    fs->n_fats = fs->win[16];
    if (fs->n_fats != 1 && fs->n_fats != 2) return FR_NO_FILESYSTEM;
    fasize *= fs->n_fats;

    fs->csize = fs->win[13];
    if (fs->csize == 0 || (fs->csize & (fs->csize - 1))) return FR_NO_FILESYSTEM;

    fs->n_rootdir = ld_word(fs->win + 17);
    if (fs->n_rootdir % ((uint)512 / 32)) return FR_NO_FILESYSTEM;

    tsect = ld_word(fs->win + 19);
    if (tsect == 0) tsect = ld_dword(fs->win + 32);

    nrsv = ld_word(fs->win + 14);
    if (nrsv == 0) return FR_NO_FILESYSTEM;

    sysect = nrsv + fasize + fs->n_rootdir / ((uint)512 / 32);
    if (tsect < sysect) return FR_NO_FILESYSTEM;
    nclst = (tsect - sysect) / fs->csize;
    if (nclst == 0) return FR_NO_FILESYSTEM;
    fmt = 3;
    if (nclst <= 0xFFF5) fmt = 2;
    if (nclst <= 0xFF5) fmt = 1;

    fs->n_fatent = nclst + 2;
    fs->volbase = bsect;
    fs->fatbase = bsect + nrsv;
    fs->database = bsect + sysect;
    if (fmt == 3) {
      if (ld_word(fs->win + 42) != 0) return FR_NO_FILESYSTEM;
      if (fs->n_rootdir) return FR_NO_FILESYSTEM;
      fs->dirbase = ld_dword(fs->win + 44);
      szbfat = fs->n_fatent * 4;
    } else {
      if (fs->n_rootdir == 0) return FR_NO_FILESYSTEM;
      fs->dirbase = fs->fatbase + fasize;
      szbfat = (fmt == 2) ?
        fs->n_fatent * 2 : fs->n_fatent * 3 / 2 + (fs->n_fatent & 1);
    }
    if (fs->fsize < (szbfat + (512 - 1)) / 512) return FR_NO_FILESYSTEM;

    fs->last_clst = fs->free_clst = 0xFFFFFFFF;
    fs->fsi_flag = 0x80;
    if (fmt == 3
      && ld_word(fs->win + 48) == 1
      && move_window(fs, bsect + 1) == FR_OK)
    {
      fs->fsi_flag = 0;
      if (ld_word(fs->win + 510) == 0xAA55
        && ld_dword(fs->win + 0) == 0x41615252
        && ld_dword(fs->win + 484) == 0x61417272)
      {
        fs->free_clst = ld_dword(fs->win + 488);
        fs->last_clst = ld_dword(fs->win + 492);
      }
    }
  }
  fs->fs_type = fmt;
  fs->id = ++(*(ushort *)Fsid);
  return FR_OK;
}


/* Check that a file or directory object is still valid for its volume. */
int validate(int param_1, int param_2)
{
  int v6;

  if (param_1 == 0 || *(int *)param_1 == 0 || *(uchar *)(*(int *)param_1) == 0 || *(ushort *)(*(int *)param_1 + 6) != *(ushort *)(param_1 + 4) || (disk_status(*(uchar *)(*(int *)param_1 + 1)) & 1) != 0) {
    *(int *)param_2 = 0;
    v6 = 9;
  }
  else {
    *(int *)param_2 = *(int *)param_1;
    v6 = 0;
  }
  return v6;
}


/* Register (or with a null object, unregister) the file system object of a drive.
   usbh_usr.c mounts "0:" with delayed mounting (opt 0). */
int f_mount(int param_1, int param_2, int param_3)
{
  int v4;
  int v5;
  int v7;
  int local_4;

  local_4 = param_2;
  v5 = get_ldnumber(&local_4);
  if (v5 < 0) {
    return 0xb;
  }
  v4 = (&FatFs)[v5];
  if (v4 != 0) {
    *(uchar *)v4 = 0;
  }
  if (param_1 != 0) {
    *(uchar *)(param_1) = 0;
  }
  (&FatFs)[v5] = param_1;
  if (param_1 == 0 || param_3 != 1) {
    return 0;
  }
  v7 = find_volume(&param_2, &param_1, 0);
  return v7;
}

/* Open or create a file. The firmware only opens files to read (mode 1, FA_READ). */
int f_open(FF_FIL *fp, const char *path, uchar mode)
{
  int res;
  FF_DIR dj;
  FATFS *fs;
  uint dw, cl, bcs, clst, sc;
  uint ofs;

  if (!fp) return FR_INVALID_OBJECT;

  mode &= 0 ? 0x01 : 0x01 | 0x02 | 0x08 | 0x04 | 0x10 | 0x30 | 0x20;
  res = find_volume(&path, &fs, mode);
  if (res == FR_OK) {
    dj.obj.fs = fs;
    res = follow_path(&dj, path);
    if (res == FR_OK) {
      if (dj.fn[11] & 0x80) {
        res = FR_INVALID_NAME;
      }
    }
    if (mode & (0x08 | 0x10 | 0x04)) {
      if (res != FR_OK) {
        if (res == FR_NO_FILE) {
          res = dir_register(&dj);
        }
        mode |= 0x08;
      }
      else {
        if (dj.obj.attr & (0x01 | 0x10)) {
          res = FR_DENIED;
        } else {
          if (mode & 0x04) res = FR_EXIST;
        }
      }
      if (res == FR_OK && (mode & 0x08)) {
        dw = get_fattime();
        {
          st_dword(dj.dir + 14, dw);
          st_dword(dj.dir + 22, dw);
          dj.dir[11] = 0x20;
          cl = ld_clust(fs, dj.dir);
          st_clust(fs, dj.dir, 0);
          st_dword(dj.dir + 28, 0);
          fs->wflag = 1;
          if (cl) {
            dw = fs->winsect;
            res = remove_chain(&dj.obj, cl, 0);
            if (res == FR_OK) {
              res = move_window(fs, dw);
              fs->last_clst = cl - 1;
            }
          }
        }
      }
    }
    else {
      if (res == FR_OK) {
        if (dj.obj.attr & 0x10) {
          res = FR_NO_FILE;
        } else {
          if ((mode & 0x02) && (dj.obj.attr & 0x01)) {
            res = FR_DENIED;
          }
        }
      }
    }
    if (res == FR_OK) {
      if (mode & 0x08)
        mode |= 0x40;
      fp->dir_sect = fs->winsect;
      fp->dir_ptr = dj.dir;
    }
    if (res == FR_OK) {
      {
        fp->obj.sclust = ld_clust(fs, dj.dir);
        fp->obj.objsize = ld_dword(dj.dir + 28);
      }
      fp->obj.fs = fs;
      fp->obj.id = fs->id;
      fp->flag = mode;
      fp->err = 0;
      fp->sect = 0;
      fp->fptr = 0;
      mem_set(fp->buf, 0, 512);
      if ((mode & 0x20) && fp->obj.objsize > 0) {
        fp->fptr = fp->obj.objsize;
        bcs = (uint)fs->csize * ((uint)512);
        clst = fp->obj.sclust;
        for (ofs = fp->obj.objsize; res == FR_OK && ofs > bcs; ofs -= bcs) {
          clst = get_fat(&fp->obj, clst);
          if (clst <= 1) res = FR_INT_ERR;
          if (clst == 0xFFFFFFFF) res = FR_DISK_ERR;
        }
        fp->clust = clst;
        if (res == FR_OK && ofs % ((uint)512)) {
          if ((sc = clust2sect(fs, clst)) == 0) {
            res = FR_INT_ERR;
          } else {
            fp->sect = sc + (uint)(ofs / ((uint)512));
            if (disk_read(fs->drv, fp->buf, fp->sect, 1) != 0) res = FR_DISK_ERR;
          }
        }
      }
    }
  }
  if (res != FR_OK) fp->obj.fs = 0;
  return res;
}

/* Read from a file through its own sector buffer, or straight into buff for whole
   sectors. */
int f_read(FF_FIL *fp, void *buff, uint btr, uint *br)
{
  int res;
  FATFS *fs;
  uint clst, sect;
  uint remain;
  uint rcnt, cc, csect;
  uchar *rbuff = (uchar *)buff;

  *br = 0;
  res = validate(&fp->obj, &fs);
  if (res != FR_OK || (res = (FRESULT)fp->err) != FR_OK) return res;
  if (!(fp->flag & 0x01)) return FR_DENIED;
  remain = fp->obj.objsize - fp->fptr;
  if (btr > remain) btr = (uint)remain;

  for ( ;  btr;
    rbuff += rcnt, fp->fptr += rcnt, *br += rcnt, btr -= rcnt) {
    if (fp->fptr % ((uint)512) == 0) {
      csect = (uint)(fp->fptr / ((uint)512) & (fs->csize - 1));
      if (csect == 0) {
        if (fp->fptr == 0) {
          clst = fp->obj.sclust;
        } else {
          {
            clst = get_fat(&fp->obj, fp->clust);
          }
        }
        if (clst < 2) { fp->err = (uchar)(FR_INT_ERR); return FR_INT_ERR; };
        if (clst == 0xFFFFFFFF) { fp->err = (uchar)(FR_DISK_ERR); return FR_DISK_ERR; };
        fp->clust = clst;
      }
      sect = clust2sect(fs, fp->clust);
      if (!sect) { fp->err = (uchar)(FR_INT_ERR); return FR_INT_ERR; };
      sect += csect;
      cc = btr / ((uint)512);
      if (cc) {
        if (csect + cc > fs->csize) {
          cc = fs->csize - csect;
        }
        if (disk_read(fs->drv, rbuff, sect, cc) != 0) { fp->err = (uchar)(FR_DISK_ERR); return FR_DISK_ERR; };
        if ((fp->flag & 0x80) && fp->sect - sect < cc) {
          mem_cpy(rbuff + ((fp->sect - sect) * ((uint)512)), fp->buf, ((uint)512));
        }
        rcnt = ((uint)512) * cc;
        continue;
      }
      if (fp->sect != sect) {
        if (fp->flag & 0x80) {
          if (disk_write(fs->drv, fp->buf, fp->sect, 1) != 0) { fp->err = (uchar)(FR_DISK_ERR); return FR_DISK_ERR; };
          fp->flag &= (uchar)~0x80;
        }
        if (disk_read(fs->drv, fp->buf, sect, 1) != 0) { fp->err = (uchar)(FR_DISK_ERR); return FR_DISK_ERR; };
      }
      fp->sect = sect;
    }
    rcnt = ((uint)512) - (uint)fp->fptr % ((uint)512);
    if (rcnt > btr) rcnt = btr;
    mem_cpy(rbuff, fp->buf + fp->fptr % ((uint)512), rcnt);
  }
  return FR_OK;
}


/* Write to a file. No callers in the image. */
int f_write(int param_1, int param_2, int param_3, int param_4)
{
  int v6;
  int v7;
  int v8;
  uint v9;
  uint v10;
  int local_4[2];
  volatile int local_0;

  local_0 = param_2;
  *(int *)param_4 = 0;
  local_4[1] = validate(param_1, local_4);
  if (local_4[1] != 0 || (local_4[1] = *(uchar *)(param_1 + 0x11)) != 0) {
    return local_4[1];
  }
  if ((*(uchar *)(param_1 + 0x10) & 2) == 0) {
    return 7;
  }
  if ((!0 || **(uchar **)param_1 != 4) && (uint)(*(int *)(param_1 + 0x14) + param_3) < *(int *)(param_1 + 0x14)) {
    param_3 = -1 - *(int *)(param_1 + 0x14);
  }
  while (param_3 != 0) {
    if ((*(ushort *)(param_1 + 0x14) & 0x1ff) == 0) {
      v10 = *(ushort *)(local_4[0] + 0xa) - 1 & (uint)*(int *)(param_1 + 0x14) >> 9;
      if (v10 == 0) {
        if (*(int *)(param_1 + 0x14) == 0) {
          v7 = *(int *)(param_1 + 8);
          if (v7 == 0) {
            v7 = create_chain(param_1, 0);
          }
        }
        else {
          v7 = create_chain(param_1, *(int *)(param_1 + 0x18));
        }
        if (v7 == 0) {
          break;
        }
        if (v7 == 1) {
          *(uchar *)(param_1 + 0x11) = 2;
          return 2;
        }
        if (v7 + 1 == 0) {
          *(uchar *)(param_1 + 0x11) = 1;
          return 1;
        }
        *(int *)(param_1 + 0x18) = v7;
        if (*(int *)(param_1 + 8) == 0) {
          *(int *)(param_1 + 8) = v7;
        }
      }
      if ((*(uchar *)(param_1 + 0x10) & 0x80) != 0) {
        if (disk_write(*(uchar *)(local_4[0] + 1), param_1 + 0x28, *(int *)(param_1 + 0x1c), 1) != 0) {
          *(uchar *)(param_1 + 0x11) = 1;
          return 1;
        }
        *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) & 0x7f;
      }
      v6 = clust2sect(local_4[0], *(int *)(param_1 + 0x18));
      if (v6 == 0) {
        *(uchar *)(param_1 + 0x11) = 2;
        return 2;
      }
      v6 = v6 + v10;
      v9 = (uint)param_3 >> 9;
      if (v9 != 0) {
        if (v10 + v9 > *(ushort *)(local_4[0] + 0xa)) {
          v9 = *(ushort *)(local_4[0] + 0xa) - v10;
        }
        if (disk_write(*(uchar *)(local_4[0] + 1), local_0, v6, v9) != 0) {
          *(uchar *)(param_1 + 0x11) = 1;
          return 1;
        }
        if ((uint)(*(int *)(param_1 + 0x1c) - v6) < v9) {
          mem_cpy(param_1 + 0x28, local_0 + (*(int *)(param_1 + 0x1c) - v6 << 9), 0x200);
          *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) & 0x7f;
        }
        v8 = v9 << 9;
        goto L_46;
      }
      else {
        if (*(int *)(param_1 + 0x1c) != v6 && (uint)*(int *)(param_1 + 0x14) < *(int *)(param_1 + 0xc) && disk_read(*(uchar *)(local_4[0] + 1), param_1 + 0x28, v6, 1) != 0) {
          *(uchar *)(param_1 + 0x11) = 1;
          return 1;
        }
        *(int *)(param_1 + 0x1c) = v6;
      }
    }
    v8 = 0x200 - (*(ushort *)(param_1 + 0x14) & 0x1ff);
    if ((uint)v8 > param_3) {
      v8 = param_3;
    }
    mem_cpy((*(ushort *)(param_1 + 0x14) & 0x1ff) + (param_1 + 0x28), local_0, v8);
    *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) | 0x80;
L_46:;
    local_0 = local_0 + v8;
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + v8;
    *(int *)(param_1 + 0xc) = (uint)*(int *)(param_1 + 0x14) > *(int *)(param_1 + 0xc) ? *(int *)(param_1 + 0x14) : *(int *)(param_1 + 0xc);
    *(int *)param_4 = *(int *)param_4 + v8;
    param_3 = param_3 - v8;
  }
  *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) | 0x40;
  return 0;
}

/* Flush a written file: its buffer, and the size, cluster and time in its directory
   entry. */
int f_sync(int param_1)
{
  int v5;
  int v6;
  int v7;
  int local_0;

  v6 = validate(param_1, &local_0);
  if (v6 == 0 && (*(uchar *)(param_1 + 0x10) & 0x40) != 0) {
    if ((*(uchar *)(param_1 + 0x10) & 0x80) != 0) {
      if (disk_write(*(uchar *)(local_0 + 1), param_1 + 0x28, *(int *)(param_1 + 0x1c), 1) != 0) {
        return 1;
      }
      *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) & 0x7f;
    }
    v7 = get_fattime();
    v6 = move_window(local_0, *(int *)(param_1 + 0x20));
    if (v6 == 0) {
      v5 = *(int *)(param_1 + 0x24);
      *(uchar *)(v5 + 0xb) = *(uchar *)(v5 + 0xb) | 0x20;
      st_clust(*(int *)param_1, v5, *(int *)(param_1 + 8));
      st_dword(v5 + 0x1c, *(int *)(param_1 + 0xc));
      st_dword(v5 + 0x16, v7);
      st_word(v5 + 0x12, 0);
      *(uchar *)(local_0 + 3) = 1;
      v6 = sync_fs(local_0);
      *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) & 0xbf;
    }
  }
  return v6;
}

/* Close a file (f_sync first, then invalidate the object). */
int f_close(int param_1)
{
  int v4;
  int local_0;

  v4 = f_sync(param_1);
  if (v4 == 0) {
    v4 = validate(param_1, &local_0);
    if (v4 == 0) {
      *(int *)param_1 = 0;
    }
  }
  return v4;
}


/* Move the file pointer, following the cluster chain; on a file open for writing it
   can stretch the file. No callers. */
int f_lseek(int param_1, int param_2)
{
  int v5;
  int v7;
  int v8;
  int v9;
  int v10;
  int local_0;

  v10 = validate(param_1, &local_0);
  if (v10 == 0) {
    v10 = *(uchar *)(param_1 + 0x11);
  }
  if (v10 != 0) {
    return v10;
  }
  if ((uint)*(int *)(param_1 + 0xc) < param_2 && (*(uchar *)(param_1 + 0x10) & 2) == 0) {
    param_2 = *(int *)(param_1 + 0xc);
  }
  v9 = *(int *)(param_1 + 0x14);
  v8 = 0;
  *(int *)(param_1 + 0x14) = v8;
  if (param_2 != 0) {
    v7 = *(ushort *)(local_0 + 0xa) << 9;
    if (v9 != 0 && (uint)(param_2 - 1) / v7 >= (uint)(v9 - 1) / v7) {
      *(int *)(param_1 + 0x14) = v9 - 1 & ~(v7 - 1);
      param_2 = param_2 - *(int *)(param_1 + 0x14);
      v5 = *(int *)(param_1 + 0x18);
    }
    else {
      v5 = *(int *)(param_1 + 8);
      if (v5 == 0) {
        v5 = create_chain(param_1, 0);
        if (v5 == 1) {
          *(uchar *)(param_1 + 0x11) = 2;
          return 2;
        }
        if (v5 + 1 == 0) {
          *(uchar *)(param_1 + 0x11) = 1;
          return 1;
        }
        *(int *)(param_1 + 8) = v5;
      }
      *(int *)(param_1 + 0x18) = v5;
    }
    if (v5 != 0) {
      while ((uint)param_2 > v7) {
        param_2 = param_2 - v7;
        *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + v7;
        if ((*(uchar *)(param_1 + 0x10) & 2) != 0) {
          if (0 && (uint)*(int *)(param_1 + 0x14) > *(int *)(param_1 + 0xc)) {
            *(int *)(param_1 + 0xc) = *(int *)(param_1 + 0x14);
            *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) | 0x40;
          }
          v5 = create_chain(param_1, v5);
          if (v5 == 0) {
            param_2 = 0;
            break;
          }
        }
        else {
          v5 = get_fat(param_1, v5);
        }
        if (v5 + 1 == 0) {
          *(uchar *)(param_1 + 0x11) = 1;
          return 1;
        }
        if ((uint)v5 <= 1 || (uint)*(int *)(local_0 + 0x14) <= v5) {
          *(uchar *)(param_1 + 0x11) = 2;
          return 2;
        }
        *(int *)(param_1 + 0x18) = v5;
      }
      *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + param_2;
      if ((param_2 & 0x1ff) != 0) {
        v8 = clust2sect(local_0, v5);
        if (v8 == 0) {
          *(uchar *)(param_1 + 0x11) = 2;
          return 2;
        }
        v8 = v8 + ((uint)param_2 >> 9);
      }
    }
  }
  if ((uint)*(int *)(param_1 + 0x14) > *(int *)(param_1 + 0xc)) {
    *(int *)(param_1 + 0xc) = *(int *)(param_1 + 0x14);
    *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) | 0x40;
  }
  if ((*(ushort *)(param_1 + 0x14) & 0x1ff) != 0 && *(int *)(param_1 + 0x1c) != v8) {
    if ((*(uchar *)(param_1 + 0x10) & 0x80) != 0) {
      if (disk_write(*(uchar *)(local_0 + 1), param_1 + 0x28, *(int *)(param_1 + 0x1c), 1) != 0) {
        *(uchar *)(param_1 + 0x11) = 1;
        return 1;
      }
      *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) & 0x7f;
    }
    if (disk_read(*(uchar *)(local_0 + 1), param_1 + 0x28, v8, 1) != 0) {
      *(uchar *)(param_1 + 0x11) = 1;
      return 1;
    }
    *(int *)(param_1 + 0x1c) = v8;
  }
  return v10;
}


/* Open a directory. No callers. */
int f_opendir(int param_1, int param_2)
{
  int v4;
  int v5;
  int local_4;

  if (param_1 == 0) {
    return 9;
  }
  v5 = param_1;
  v4 = find_volume(&param_2, &local_4, 0);
  if (v4 == 0) {
    *(int *)v5 = local_4;
    v4 = follow_path(param_1, param_2);
    if (v4 == 0) {
      if ((((uchar *)(param_1 + 0x20))[0xb] & 0x80) == 0) {
        if ((*(uchar *)(v5 + 6) & 0x10) != 0) {
          *(int *)(v5 + 8) = ld_clust(local_4, *(int *)(param_1 + 0x1c));
        }
        else {
          v4 = 5;
        }
      }
      if (v4 == 0) {
        *(ushort *)(v5 + 4) = *(ushort *)(local_4 + 6);
        v4 = dir_sdi(param_1, 0);
      }
    }
    if (v4 == 4) {
      v4 = 5;
    }
  }
  if (v4 != 0) {
    *(int *)v5 = 0;
  }
  return v4;
}


/* Close a directory. No callers. */
int f_closedir(int * param_1)
{
  int iVar1;
  int iStack_10;

  iVar1 = validate(param_1,&iStack_10);
  if (iVar1 == 0) {
    *param_1 = 0;
  }
  return iVar1;
}


/* Read the next directory entry into a FILINFO. No callers. */
int f_readdir(int * param_1, uint * param_2)
{
  int iVar1;
  int iStack_18;

  iVar1 = validate(param_1,&iStack_18);
  if (iVar1 == 0) {
    if (param_2 == (uint *)0x0) {
      iVar1 = dir_sdi(param_1,0);
    }
    else {
      iVar1 = dir_read(param_1,0);
      if (iVar1 == 4) {
        iVar1 = 0;
      }
      if (iVar1 == 0) {
        get_fileinfo((int)param_1,param_2);
        iVar1 = dir_next(param_1,0);
        if (iVar1 == 4) {
          iVar1 = 0;
        }
      }
    }
  }
  return iVar1;
}

/* File status (FILINFO) of a path. No callers. */
int f_stat(int param_1, int param_2)
{
  int v4;
  uchar local_0[44];

  v4 = find_volume(&param_1, local_0, 0);
  if (v4 == 0) {
    v4 = follow_path(local_0, param_1);
    if (v4 == 0) {
      if ((local_0[43] & 0x80) != 0) {
        v4 = 6;
      }
      else {
        if (param_2 != 0) {
          get_fileinfo(local_0, param_2);
        }
      }
    }
  }
  return v4;
}


/* Number of free clusters, from FSInfo when valid or by scanning the FAT.
   usbh_usr.c calls it on the stick. */
int f_getfree(int param_1, int param_2, int param_3)
{
  int v4;
  int v5;
  int v6;
  int v7;
  int v8;
  int v10;
  int v11;
  int local_14;
  int local_4[4];

  v10 = find_volume(&param_1, &local_14, 0);
  if (v10 == 0) {
    *(int *)(param_3) = local_14;
    if ((uint)*(int *)(local_14 + 0x10) <= *(int *)(local_14 + 0x10 + 4) - 2) {
      *(int *)param_2 = *(int *)(local_14 + 0x10);
    }
    else {
      v4 = 0;
      if (*(uchar *)(local_14) == 1) {
        v5 = 2;
        local_4[0] = local_14;
        do {
          v6 = get_fat(local_4, v5);
          if (v6 + 1 == 0) {
            v10 = 1;
            break;
          }
          if (v6 == 1) {
            v10 = 2;
            break;
          }
          if (v6 == 0) {
            v4++;
          }
        } while ((uint)(v5 = v5 + 1) < *(int *)(local_14 + 0x14));
      }
      else {
        v5 = *(int *)(local_14 + 0x14);
        v11 = *(int *)(local_14 + 0x20);
        v7 = 0;
        v8 = v7;
        do {
          if (v7 == 0) {
            v10 = move_window(local_14, v11++);
            if (v10 != 0) {
              break;
            }
            v8 = local_14 + 0x30;
            v7 = 0x200;
          }
          if (*(uchar *)(local_14) == 2) {
            if (ld_word(v8) == 0) {
              v4++;
            }
            v8 = v8 + 2;
            v7 = v7 - 2;
          }
          else {
            if ((ld_dword(v8) & ~0xf0000000) == 0) {
              v4++;
            }
            v8 = v8 + 4;
            v7 = v7 - 4;
          }
        } while ((v5 = v5 - 1) != 0);
      }
      *(int *)param_2 = v4;
      *(int *)(local_14 + 0x10) = v4;
      *(uchar *)(local_14 + 4) = *(uchar *)(local_14 + 4) | 1;
    }
  }
  return v10;
}


/* Cut a file at its file pointer. No callers. */
int f_truncate(int param_1)
{
  int v5;
  int v6;
  int local_0;

  v5 = validate(param_1, &local_0);
  if (v5 != 0 || (v5 = *(uchar *)(param_1 + 0x11)) != 0) {
    return v5;
  }
  if ((*(uchar *)(param_1 + 0x10) & 2) == 0) {
    return 7;
  }
  if ((uint)*(int *)(param_1 + 0x14) < *(int *)(param_1 + 0xc)) {
    if (*(int *)(param_1 + 0x14) == 0) {
      v5 = remove_chain(param_1, *(int *)(param_1 + 8), 0);
      *(int *)(param_1 + 8) = 0;
    }
    else {
      v6 = get_fat(param_1, *(int *)(param_1 + 0x18));
      v5 = 0;
      if (v6 + 1 == 0) {
        v5 = 1;
      }
      if (v6 == 1) {
        v5 = 2;
      }
      if (v5 == 0 && (uint)*(int *)(local_0 + 0x14) > v6) {
        v5 = remove_chain(param_1, v6, *(int *)(param_1 + 0x18));
      }
    }
    *(int *)(param_1 + 0xc) = *(int *)(param_1 + 0x14);
    *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) | 0x40;
    if (v5 == 0 && (*(uchar *)(param_1 + 0x10) & 0x80) != 0) {
      if (disk_write(*(uchar *)(local_0 + 1), param_1 + 0x28, *(int *)(param_1 + 0x1c), 1) != 0) {
        v5 = 1;
      }
      else {
        *(uchar *)(param_1 + 0x10) = *(uchar *)(param_1 + 0x10) & 0x7f;
      }
    }
    if (v5 != 0) {
      *(uchar *)(param_1 + 0x11) = v5;
      return v5;
    }
  }
  return v5;
}

/* Delete a file or an empty directory. usbh_usr.c deletes "0:DSPStore.bin" and
   "0:ARMStore.bin" with it.
   match: the __nop() after follow_path is ChaN's "if (_FS_RPATH && ...)" test, which
   compiles to a nop when _FS_RPATH is 0. */
int f_unlink(int param_1)
{
  int v4;
  int v5;
  uchar local_34[44];
  int local_8[11];
  int local_4;

  v5 = 0;
  v4 = find_volume(&param_1, &local_4, 2);
  *(int *)((uchar *)local_34) = local_4;
  if (v4 == 0) {
    v4 = follow_path(local_34, param_1);
    __nop();
    if (v4 == 0) {
      if ((local_34[43] & 0x80) != 0) {
        v4 = 6;
      }
      else {
        if ((local_34[6] & 1) != 0) {
          v4 = 7;
        }
      }
      if (v4 == 0) {
        v5 = ld_clust(local_4, *(int *)((uchar *)local_34 + 28));
        if ((local_34[6] & 0x10) != 0) {
          local_8[0] = local_4;
          local_8[2] = v5;
          v4 = dir_sdi(local_8, 0);
          if (v4 == 0) {
            v4 = dir_read(local_8, 0);
            if (v4 == 0) {
              v4 = 7;
            }
            if (v4 == 4) {
              v4 = 0;
            }
          }
        }
      }
      if (v4 == 0) {
        v4 = dir_remove(local_34);
        if (v4 == 0 && v5 != 0) {
          v4 = remove_chain(local_34, v5, 0);
        }
        if (v4 == 0) {
          v4 = sync_fs(local_4);
        }
      }
    }
  }
  return v4;
}


/* Create a directory with its "." and ".." entries. No callers. */
int f_mkdir(int param_1)
{
  int v4;
  int v5;
  int v6;
  int v7;
  int v8;
  int v9;
  int v10;
  int local_8[11];
  int local_4;

  v5 = find_volume(&param_1, &local_4, 2);
  local_8[0] = local_4;
  if (v5 == 0) {
    v5 = follow_path(local_8, param_1);
    if (v5 == 0) {
      v5 = 8;
    }
    if (0 && v5 == 4 && (*(uchar *)(local_8 + 6) & 0x20)) {
      v5 = 6;
    }
    if (v5 == 4) {
      v6 = create_chain(local_8, 0);
      local_8[3] = *(ushort *)(local_4 + 0xa) << 9;
      v5 = 0;
      if (v6 == 0) {
        v5 = 7;
      }
      if (v6 == 1) {
        v5 = 2;
      }
      if (v6 + 1 == 0) {
        v5 = 1;
      }
      if (v5 == 0) {
        v5 = sync_window(local_4);
      }
      v10 = get_fattime();
      if (v5 == 0) {
        v8 = clust2sect(local_4, v6);
        v4 = local_4 + 0x30;
        mem_set(v4, 0, 0x200);
        mem_set(v4, 0x20, 0xb);
        *(uchar *)v4 = 0x2e;
        *(uchar *)(v4 + 0xb) = 0x10;
        st_dword(v4 + 0x16, v10);
        st_clust(local_4, v4, v6);
        mem_cpy(v4 + 0x20, v4, 0x20);
        *(uchar *)(v4 + 0x21) = 0x2e;
        v9 = local_8[2];
        if (*(uchar *)(local_4) == 3 && *(int *)(local_4 + 0x24) == v9) {
          v9 = v5;
        }
        st_clust(local_4, v4 + 0x20, v9);
        v7 = *(ushort *)(local_4 + 0xa);
        while (v7 != 0) {
          *(int *)(local_4 + 0x2c) = v8;
          v8++;
          *(uchar *)(local_4 + 3) = 1;
          v5 = sync_window(local_4);
          if (v5 != 0) {
            break;
          }
          mem_set(v4, 0, 0x200);
          v7--;
        }
      }
      if (v5 == 0) {
        v5 = dir_register(local_8);
      }
      if (v5 == 0) {
        v4 = local_8[7];
        st_dword(v4 + 0x16, v10);
        st_clust(local_4, v4, v6);
        *(uchar *)(v4 + 0xb) = 0x10;
        *(uchar *)(local_4 + 3) = 1;
        if (v5 == 0) {
          v5 = sync_fs(local_4);
        }
      }
      else {
        remove_chain(local_8, v6, 0);
      }
    }
  }
  return v5;
}

/* Rename or move a file or directory, fixing the ".." entry of a moved directory.
   No callers. */
int f_rename(const char *path_old, const char *path_new)
{
  int res;
  FF_DIR djo, djn;
  FATFS *fs;
  uchar buf[0 ? 32 * 2 : 24], *dir;
  uint dw;

  get_ldnumber(&path_new);
  res = find_volume(&path_old, &fs, 0x02);
  if (res == FR_OK) {
    djo.obj.fs = fs;
    res = follow_path(&djo, path_old);
    if (res == FR_OK && (djo.fn[11] & (0x20 | 0x80))) res = FR_INVALID_NAME;
    if (res == FR_OK) {
      {
        mem_cpy(buf, djo.dir + 11, 21);
        mem_cpy(&djn, &djo, sizeof (FF_DIR));
        res = follow_path(&djn, path_new);
        if (res == FR_OK) {
          res = (djn.obj.sclust == djo.obj.sclust && djn.dptr == djo.dptr) ? FR_NO_FILE : FR_EXIST;
        }
        if (res == FR_NO_FILE) {
          res = dir_register(&djn);
          if (res == FR_OK) {
            dir = djn.dir;
            mem_cpy(dir + 13, buf + 2, 19);
            dir[11] = buf[0] | 0x20;
            fs->wflag = 1;
            if ((dir[11] & 0x10) && djo.obj.sclust != djn.obj.sclust) {
              dw = clust2sect(fs, ld_clust(fs, dir));
              if (!dw) {
                res = FR_INT_ERR;
              } else {
                res = move_window(fs, dw);
                dir = fs->win + 32 * 1;
                if (res == FR_OK && dir[1] == '.') {
                  st_clust(fs, dir, djn.obj.sclust);
                  fs->wflag = 1;
                }
              }
            }
          }
        }
      }
      if (res == FR_OK) {
        res = dir_remove(&djo);
        if (res == FR_OK) {
          res = sync_fs(fs);
        }
      }
    }
  }
  return res;
}
