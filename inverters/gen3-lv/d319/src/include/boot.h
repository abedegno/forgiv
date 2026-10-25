/* boot.h: the boot loader (src/boot/) and the application header words it reads. The update
   protocol is described in boot_flash_loader.c and docs/subsystems/boot.md.
   name: descriptive. */
#ifndef BOOT_H
#define BOOT_H

#include "F2806x_Device.h"
#include "util.h"

/* The boot loader's update protocol (boot_flash_loader): frames 0x0A, length, command, data, a
 * 16-bit sum of everything before it; replies 0x0A, 0, code, sum. The ARM sends 'W', 'S', the
 * 'D' blocks and 'E'. */
#define BOOT_CMD_ADDR 'S' /* set the flash address */
#define BOOT_CMD_DATA                                                                              \
    'D' /* program 64 words at address + block * 64 (when that is below 0x3F4000) */
#define BOOT_CMD_ERASE                                                                             \
    'W' /* erase sectors B..H (Flash_Erase(0xFE)); sector A keeps the boot loader */
#define BOOT_CMD_HELLO 'F' /* answer only; meant as the first frame, but not enforced */
#define BOOT_CMD_END 'E'   /* answer, then raise GPIO27 and wait for a reset from outside */
#define BOOT_REPLY_OK 'O'
#define BOOT_REPLY_BAD_SUM 'C' /* any other code is the flash API's status */

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "F2806x_Device.h"

/* app_header.c */
extern const unsigned int version; /* 316 */

/* boot_csm.c */
void boot_copy_run(void);

/* boot_flash_loader.c */
void boot_flash_loader(void);

/* boot_update.c */
void boot_update(void);

/* defined by no source: the linker places it at its map address */
extern Uint16 BootRamfuncsLoadEnd[];   /* and end (RamfuncsLoadEnd) */
extern Uint16 BootRamfuncsLoadStart[]; /* the update routine's load start (RamfuncsLoadStart) */
extern Uint16 BootRamfuncsRunStart[];  /* where it runs (RamfuncsRunStart) */
/* ---- end of declarations ---- */

#endif
