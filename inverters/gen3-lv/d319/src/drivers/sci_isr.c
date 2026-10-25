/* sci_isr.c: the SCI interrupts, PIE group 9 (9.1 SCI-A RX, 9.2 SCI-A TX, 9.3 SCI-B RX,
   9.4 SCI-B TX; init_system puts them in the vector table). A receive error resets the port,
   a received byte goes to sci_rx, a transmit interrupt sends the next byte (sci_tx). Each one
   acknowledges group 9. The receive error handling is the same code as at the top of sci_rx,
   so an error is handled twice.

   Entry points: scib_rx_isr, scib_tx_isr, scia_rx_isr, scia_tx_isr. Data: none.
   Neighbours: sci.c, drivers/device_init.c (the vectors and PIEIER9).
   name: descriptive (map/filenames.tsv), name kept.
   at: scib_rx_isr 0x3F122A .. scia_tx_isr 0x3F12F0 */
#include "F2806x_Device.h"
#include "F2806x_Sci.h"
#include "F2806x_PieCtrl.h"
#include "sci.h"
#include "util.h"

/* SCI-B receive (PIE 9.3): the BMS bus. */
interrupt void scib_rx_isr(void)
{
    if (ScibRegs.SCIRXST.bit.RXERROR) {
        ScibRegs.SCICTL1.all = 3;
        ScibRegs.SCICTL2.all = 3;
        ScibRegs.SCICTL1.bit.RXERRINTENA = 1;
        ScibRegs.SCICTL1.all = 0x23;
    }
    if (ScibRegs.SCIRXST.bit.RXRDY)
        sci_rx(1);
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

/* SCI-B transmit (PIE 9.4). */
interrupt void scib_tx_isr(void)
{
    sci_tx(1);
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

/* SCI-A receive (PIE 9.1): the ARM link. A receive error also abandons the frame being sent. */
interrupt void scia_rx_isr(void)
{
    if (SciaRegs.SCIRXST.bit.RXERROR) {
        SciaRegs.SCICTL1.all = 3;
        SciaRegs.SCICTL2.all = 3;
        SciaRegs.SCICTL1.bit.RXERRINTENA = 1;
        sci_tx_state[0].pos = 0;
        sci_tx_done = 1;
        SciaRegs.SCICTL1.all = 0x23;
        sci_tx_state[0].busy = 0;
    }
    if (SciaRegs.SCIRXST.bit.RXRDY)
        sci_rx(0);
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

/* SCI-A transmit (PIE 9.2). */
interrupt void scia_tx_isr(void)
{
    sci_tx(0);
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}
