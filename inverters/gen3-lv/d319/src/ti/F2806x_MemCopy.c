/* F2806x_MemCopy.c: TI's MemCopy (F2806x_MemCopy.c, controlSUITE), unchanged, in the boot
   loader. boot_copy_run uses it to copy the update routine to RAM at 0x0400.
   Entry point: MemCopy. Data: none. Neighbours: boot/boot_csm.c.
   Like src/app/util/util.c this file was built by a later compiler than the application (16.3 ..
   17.9 give these words; every 15.x reloads the end address into ACC in the loop, or with
   --opt_for_speed=0/1 copies with PREAD). The boot loader's other files come out the same with
   either compiler.
   name: original (map/filenames.tsv): TI's file and function name.
   at: MemCopy 0x3F52BB */
#include "F2806x_Device.h"

/* TI's: copies the words from SourceAddr up to SourceEndAddr to DestAddr. */
void MemCopy(Uint16 *SourceAddr, Uint16* SourceEndAddr, Uint16* DestAddr)
{
    while(SourceAddr < SourceEndAddr)
    {
       *DestAddr++ = *SourceAddr++;
    }
    return;
}
