// clang-format off
/* RTOS task 0 (every 20 ms): the operating state machine.
   Each state is a function that runs until it picks the next state (0xCF0D).
   at: task0_state_machine 0x3EDDA0

   Job: clear the state words once (state_init), then call the function of the current op_state
   for ever. Entry point: task0_state_machine (created by main). Data: none of its own.
   Neighbours: op_states.c (the states). Its period is 20 ticks of 1 ms; state5_grid_pv and
   state6_grid_batt set it to 11 for one wait while they switch to EPS.
   name: descriptive (map/filenames.tsv: src/task0_b.c). */
// clang-format on
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "state.h"

/* RTOS task 0: state_init, then run the state op_state names (OP_* in state.h); an unknown
   state runs state0_idle. */
void task0_state_machine(void) {
    state_init();
    for (;;) {
        switch (op_state) {
        case OP_START:
            state1_start();
            break;
        case OP_FAULT:
            state3_fault();
            break;
        case OP_UPDATE:
            state4_update();
            break;
        case OP_GRID_PV:
            state5_grid_pv();
            break;
        case OP_GRID_BATT:
            state6_grid_batt();
            break;
        case OP_EPS_PV:
            state7_eps_pv();
            break;
        case OP_EPS_BATT:
            state8_eps_batt();
            break;
        default:
            state0_idle();
            break;
        }
    }
}
