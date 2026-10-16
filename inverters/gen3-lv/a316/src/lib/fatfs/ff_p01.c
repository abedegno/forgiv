/* at: ld_word 0x0803083C */
/* at: ld_dword 0x08030848 */
/* at: st_word 0x08030860 */
/* at: st_dword 0x0803086C */
/* at: mem_cpy 0x08030884, mem_set 0x0803089E */
/* at: mem_cmp 0x080308B0 */
/* at: chk_chr 0x080308D4 */
/* at: sync_window 0x080308E8, move_window 0x0803093A, sync_fs 0x0803096E, clust2sect 0x080309F4, get_fat 0x08030A0E, put_fat 0x08030AEE, remove_chain 0x08030C26, create_chain 0x08030CC0 */
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
/* FatFs R0.12c (ChaN), ff.c, first part: the byte helpers, the FAT window and the cluster
   chain functions (ld_word to create_chain, in ff.c's order).
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
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/ff.h"
#include "../../include/fatfs_types.h"
#include "../../include/usb.h"

/* Load a little-endian 16-bit value. match: declared ushort (FatFs WORD) for all of ff.c. */
ushort ld_word(uchar *p)
{
  return 256 * p[1] | p[0];
}

/* Load a little-endian 32-bit value. */
uint ld_dword(uchar *p)
{
  return 256 * (256 * (256 * p[3] | p[2]) | p[1]) | p[0];
}

/* Store a 16-bit value little-endian. */
uchar *st_word(uchar *p, int v)
{
  *p++ = v;
  v >>= 8;
  *p++ = v;
  return p;
}

/* Store a 32-bit value little-endian. */
uchar *st_dword(uchar *p, uint v)
{
  *p++ = v;
  v >>= 8;
  *p++ = v;
  v >>= 8;
  *p++ = v;
  v >>= 8;
  *p++ = v;
  return p;
}

/* Byte copy (FatFs does not use the C library). */
void mem_cpy(void *d, void *s, int n)
{
  uchar *dp = d;
  uchar *sp = s;
  if (n) do { *dp++ = *sp++; } while (--n);
}

/* Byte fill. */
void mem_set(void *d, uchar v, int n)
{
  uchar *dp = d;
  do { *dp++ = v; } while (--n);
}

/* Byte compare: 0 when equal. */
int mem_cmp(void *a, void *b, int n)
{
  uchar *p = a;
  uchar *q = b;
  int r = 0;
  do { r = *p++ - *q++; } while (--n && r == 0);
  return r;
}

/* Non-zero if character param_2 is in the string param_1. */
uchar chk_chr(byte * param_1, uint param_2)
{

  for (; (*param_1 != 0 && (*param_1 != param_2)); param_1 = param_1 + 1) {
  }
  return *param_1;
}

/* Write the volume's sector window back if it is dirty, and the second FAT copy too. */
int sync_window(int param_1)
{
  int v5;
  int v6;
  int v7;
  
  v7 = 0;
  if (*(uchar *)(param_1 + 3) != 0) {
    v5 = *(int *)(param_1 + 0x2c);
    if (disk_write(*(uchar *)(param_1 + 1), param_1 + 0x30, v5, 1) != 0) {
      v7 = 1;
    }
    else {
      *(uchar *)(param_1 + 3) = 0;
      if ((uint)(v5 - *(int *)(param_1 + 0x20)) < *(int *)(param_1 + 0x18)) {
        v6 = *(uchar *)(param_1 + 2);
        while ((uint)v6 >= 2) {
          v5 = v5 + *(int *)(param_1 + 0x18);
          disk_write(*(uchar *)(param_1 + 1), param_1 + 0x30, v5, 1);
          v6--;
        }
      }
    }
  }
  return v7;
}


/* Load sector param_2 into the volume's window, writing the old one back first. */
int move_window(int param_1, int param_2)
{
  int v6;
  
  v6 = 0;
  if (*(int *)(param_1 + 0x2c) != param_2) {
    v6 = sync_window(param_1);
    if (v6 == 0) {
      if (disk_read(*(uchar *)(param_1 + 1), param_1 + 0x30, param_2, 1) != 0) {
        param_2 = -1;
        v6 = 1;
      }
      *(int *)(param_1 + 0x2c) = param_2;
    }
  }
  return v6;
}


/* Flush the window and, on FAT32, update the FSInfo sector; then CTRL_SYNC the disk. */
int sync_fs(char * param_1)
{
  int iVar1;
  int iVar2;
  
  iVar1 = sync_window((int)param_1);
  if (iVar1 == 0) {
    if ((*param_1 == '\x03') && (param_1[4] == '\x01')) {
      mem_set(param_1 + 0x30,0,0x200);
      st_word(param_1 + 0x22e,0xaa55);
      st_dword(param_1 + 0x30,0x41615252);
      st_dword(param_1 + 0x214,0x61417272);
      st_dword(param_1 + 0x218,*(undefined4 *)(param_1 + 0x10));
      st_dword(param_1 + 0x21c,*(undefined4 *)(param_1 + 0xc));
      *(int *)(param_1 + 0x2c) = *(int *)(param_1 + 0x1c) + 1;
      disk_write((uint)(byte)param_1[1],param_1 + 0x30,*(undefined4 *)(param_1 + 0x2c),1);
      param_1[4] = '\0';
    }
    if ((disk_ioctl((uint)(byte)param_1[1],0,(undefined4 *)0x0)) != 0) {
      iVar1 = 1;
    }
  }
  return iVar1;
}

/* First sector of a cluster (0 for an invalid cluster). */
uint clust2sect(int param_1, uint param_2)
{
  param_2 -= 2;
  if (param_2 >= *(uint *)(param_1 + 0x14) - 2) return 0;
  return param_2 * *(ushort *)(param_1 + 0xa) + *(uint *)(param_1 + 0x28);
}

/* Read a FAT entry (FAT12, FAT16 or FAT32): the next cluster in the chain. */
uint get_fat(void *obj, uint clst)
{
  uint wc, bc;
  uint val;
  FATFS *fs = ((FFOBJID *)obj)->fs;

  if (clst < 2 || clst >= fs->n_fatent) {
    val = 1;
  } else {
    val = 0xFFFFFFFF;
    switch (fs->fs_type) {
    case 1:
      bc = (uint)clst; bc += bc / 2;
      if (move_window(fs, fs->fatbase + (bc / 512)) != 0) break;
      wc = fs->win[bc++ % 512];
      if (move_window(fs, fs->fatbase + (bc / 512)) != 0) break;
      wc |= fs->win[bc % 512] << 8;
      val = (clst & 1) ? (wc >> 4) : (wc & 0xFFF);
      break;
    case 2:
      if (move_window(fs, fs->fatbase + (clst / (512 / 2))) != 0) break;
      val = ld_word(fs->win + clst * 2 % 512);
      break;
    case 3:
      if (move_window(fs, fs->fatbase + (clst / (512 / 4))) != 0) break;
      val = ld_dword(fs->win + clst * 4 % 512) & 0x0FFFFFFF;
      break;
    default:
      val = 1;
    }
  }
  return val;
}

/* Write a FAT entry. */
int put_fat(void *fs_, uint clst, uint val)
{
  uint bc;
  uchar *p;
  int res = FR_INT_ERR;

  if (clst >= 2 && clst < ((FATFS *)fs_)->n_fatent) {
    switch (((FATFS *)fs_)->fs_type) {
    case 1:
      bc = (uint)clst; bc += bc / 2;
      res = move_window(fs_, ((FATFS *)fs_)->fatbase + (bc / 512));
      if (res != FR_OK) break;
      p = ((FATFS *)fs_)->win + bc++ % 512;
      *p = (clst & 1) ? ((*p & 0x0F) | ((uchar)val << 4)) : (uchar)val;
      ((FATFS *)fs_)->wflag = 1;
      res = move_window(fs_, ((FATFS *)fs_)->fatbase + (bc / 512));
      if (res != FR_OK) break;
      p = ((FATFS *)fs_)->win + bc % 512;
      *p = (clst & 1) ? (uchar)(val >> 4) : ((*p & 0xF0) | ((uchar)(val >> 8) & 0x0F));
      ((FATFS *)fs_)->wflag = 1;
      break;
    case 2:
      res = move_window(fs_, ((FATFS *)fs_)->fatbase + (clst / (512 / 2)));
      if (res != FR_OK) break;
      st_word(((FATFS *)fs_)->win + clst * 2 % 512, (ushort)val);
      ((FATFS *)fs_)->wflag = 1;
      break;
    case 3:
      res = move_window(fs_, ((FATFS *)fs_)->fatbase + (clst / (512 / 4)));
      if (res != FR_OK) break;
      if (!0 || ((FATFS *)fs_)->fs_type != 4) {
        val = (val & 0x0FFFFFFF) | (ld_dword(((FATFS *)fs_)->win + clst * 4 % 512) & 0xF0000000);
      }
      st_dword(((FATFS *)fs_)->win + clst * 4 % 512, val);
      ((FATFS *)fs_)->wflag = 1;
      break;
    }
  }
  return res;
}


/* Free a cluster chain from param_2 on and mark the previous cluster as the end. */
int remove_chain(int param_1, int param_2, int param_3)
{
  int v4;
  int v5;
  int v8;
  
  v8 = 0;
  v4 = *(int *)param_1;
  if ((uint)param_2 < 2 || (uint)*(int *)(v4 + 0x14) <= param_2) {
    return 2;
  }
  if (param_3 != 0 && (!0 || *(uchar *)v4 != 4)) {
    v8 = put_fat(v4, param_3, -1);
    if (v8 != 0) {
      return v8;
    }
  }
  do {
    v5 = get_fat(param_1, param_2);
    if (v5 == 0) {
      break;
    }
    if (v5 == 1) {
      return 2;
    }
    if (v5 + 1 == 0) {
      return 1;
    }
    v8 = put_fat(v4, param_2, 0);
    if (v8 != 0) {
      return v8;
    }
    if ((uint)*(int *)(v4 + 0x10) < *(int *)(v4 + 0x10 + 4) - 2) {
      *(int *)(v4 + 0x10) = *(int *)(v4 + 0x10) + 1;
      *(uchar *)(v4 + 4) = *(uchar *)(v4 + 4) | 1;
    }
    param_2 = v5;
  } while ((uint)*(int *)(v4 + 0x14) > param_2);
  return 0;
}

/* Stretch a chain by one cluster, or start a new chain when clst is 0. Returns the
   new cluster, 0 when the disk is full, 1 or 0xFFFFFFFF on error. */
uint create_chain(void *obj, uint clst)
{
  uint cs, ncl, scl;
  int res;
  FATFS *fs = ((FFOBJID *)obj)->fs;

  if (clst == 0) {
    scl = fs->last_clst;
    if (scl == 0 || scl >= fs->n_fatent) scl = 1;
  }
  else {
    cs = get_fat(obj, clst);
    if (cs < 2) return 1;
    if (cs == 0xFFFFFFFF) return cs;
    if (cs < fs->n_fatent) return cs;
    scl = clst;
  }
  {
    ncl = scl;
    for (;;) {
      ncl++;
      if (ncl >= fs->n_fatent) {
        ncl = 2;
        if (ncl > scl) return 0;
      }
      cs = get_fat(obj, ncl);
      if (cs == 0) break;
      if (cs == 1 || cs == 0xFFFFFFFF) return cs;
      if (ncl == scl) return 0;
    }
    res = put_fat(fs, ncl, 0xFFFFFFFF);
    if (res == 0 && clst != 0) {
      res = put_fat(fs, clst, ncl);
    }
  }
  if (res == 0) {
    fs->last_clst = ncl;
    if (fs->free_clst <= fs->n_fatent - 2) fs->free_clst--;
    fs->fsi_flag |= 1;
  } else {
    ncl = (res == 1) ? 0xFFFFFFFF : 1;
  }
  return ncl;
}
