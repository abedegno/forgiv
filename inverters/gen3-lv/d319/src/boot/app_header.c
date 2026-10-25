#include "boot.h"
#include "util.h"
/* app_header.c: the application's header words, which boot_main and the ARM link read.
     version    0x013F = 319 at the start of the application's flash (0x3E8000). The DSP also
                sends it to the ARM on page 3 (bytes 28-29), and page 1 bytes 31-32 carry the same
                number as a constant. The ARM compares it with DSPStore.bin's version.
     app_valid  0x00AA at 0x3F3FFC, the last words of sector B. boot_main starts the application
                only when this word is 0xAA. An erase ('W') clears it, and the 'D' block
                that covers 0x3F3FFC writes it again.
   Each is a section of its own, placed by the link.
   Entry points: none. Neighbours: boot_main.asm, boot_flash_loader.c, app/comm/arm_link.c.
   name: descriptive (map/filenames.tsv: was app_ver.c). */
#pragma DATA_SECTION(version, "version")
const unsigned int version = 0x013F;	/* 319 */
#pragma DATA_SECTION(app_valid, "app_valid")
const unsigned int app_valid = 0x00AA;
