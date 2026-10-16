// clang-format off
/* at: stub_noop_1 0x08024AA0, stub_noop_2 0x08024AA2 */
/* at: stub_noop_3 0x08024AA4, stub_noop_4 0x08024AA6 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* stubs.c: four empty functions. Nothing calls them and nothing points at them; armlink kept
   them because the project was linked without --split_sections. They may be placeholders left in
   a board file, or in nvic.c or panel_led.c, which they sit between.

   Entry points: stub_noop_1 .. stub_noop_4 (all unused). Data: none.
   Neighbours: nvic.c before, panel_led.c after.

   name: descriptive. Which original file held them is not known (map/filenames.tsv). Two parts
   joined, the joined file compiles to the same bytes. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/board.h"

/* Dead. Returns at once. */
void stub_noop_1() { return; }

/* Dead. Returns at once. */
void stub_noop_2() { return; }

/* Dead. Returns at once. */
void stub_noop_3() { return; }

/* Dead. Returns at once. */
void stub_noop_4() { return; }
