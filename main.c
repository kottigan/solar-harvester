//******************************************************************************
//  MSP430F2211 Solar Demo
//
//  Description:
//
//  ACLK = LFXTAL 32kHz, MCLK = DCO 8MHz, SMCLK = DCO 1MHz
//
//                MSP430F2211
//             -----------------
//         /|\|              XIN|-
//          | |                 | 32kHz
//          --|RST          XOUT|-
//            |                 |
//      Vin-->|P1.6/CA06    P1.0|--> LED = pulse 1Hz
//
//
//   Michael Zwerg
//   March 2013
//******************************************************************************

#include <msp430g2211.h>

/***********************************/
/*   Init WDT as delay counter     */
/***********************************/

volatile unsigned int wdt_tick_cnt = 0;
volatile unsigned int delay_cnt    = 0;

void init_wdta ( void )
{
  WDTCTL = WDTPW + WDTHOLD;               // hold WDT
  IE1 &= ~WDTIE;                          // disable WDT interrupt
}

void delay_1s (unsigned int delay_value )
{
  if (delay_value > 0)
  {
    wdt_tick_cnt = 0;                     // init tick counter
    delay_cnt = delay_value;              // set delay value
    IFG1 &= ~WDTIFG;                      // clear all pending interrupts
    IE1 |= WDTIE;                         // enable WDT interrupt
    WDTCTL = WDT_ADLY_1000;               // start WDT in interrupt mode
    __bis_SR_register(LPM3_bits + GIE);   // Enter LPM3, enable interrupts
  }
}

void delay_250ms (unsigned int delay_value )
{
  if (delay_value > 0)
  {
    wdt_tick_cnt = 0;                     // init tick counter
    delay_cnt = delay_value;              // set delay value
    IFG1 &= ~WDTIFG;                      // clear all pending interrupts
    IE1 |= WDTIE;                         // enable WDT interrupt
    WDTCTL = WDT_ADLY_250;                // start WDT in interrupt mode
    __bis_SR_register(LPM3_bits + GIE);   // Enter LPM3, enable interrupts
  }
}

void delay_1ms (unsigned int delay_value )
{
  if (delay_value > 0)
  {
    wdt_tick_cnt = 0;                     // init tick counter
    delay_cnt = delay_value<<1;           // set delay value
    IFG1 &= ~WDTIFG;                      // clear all pending interrupts
    IE1 |= WDTIE;                         // enable WDT interrupt
    WDTCTL = WDT_MDLY_0_5;                // start WDT in interrupt mode
    __bis_SR_register(LPM0_bits + GIE);   // Enter LPM0, enable interrupts
  }
}


#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void)
{
  wdt_tick_cnt++;
  if (wdt_tick_cnt == delay_cnt)              // if delay reached
  {
    WDTCTL = WDTPW + WDTHOLD;                 // stop WDTA
    IE1  &= ~WDTIE;                           // disable WDT interrupt
    __bic_SR_register_on_exit(LPM3_bits);     // wakeup from LPM
  }
}

/***********************************/
/*   Init Clock system             */
/***********************************/

void init_cs ( void )
{
  P2SEL |= 0xC0;                            // Select XT1 (P2.6 & P2.7)
  do  {                                     // Loop until XT1,XT2 & DCO stabilizes
    BCSCTL3 &= ~(LFXT1OF);                  // Clear fault flags
    IFG1 &= ~OFIFG;                         // Clear fault flags
    delay_250ms(1);                         // wait 250ms
  } while (IFG1&OFIFG);                     // Test oscillator fault flag
  if (CALBC1_1MHZ==0xFF)					// If calibration constant erased
  {
    while(1);                               // do not load, trap CPU!!
  }
  DCOCTL = 0;                               // Select lowest DCOx and MODx settings
  BCSCTL1 = CALBC1_1MHZ;                    // Set DCO to 1MHz
  DCOCTL =  CALDCO_1MHZ;
}

/***********************************/
/*   Init COMPA module             */
/***********************************/

void init_comp ( void )
{
	CAPD   = CAPD6;							// disable digital function on COMP pin
	CACTL2 = P2CA3+P2CA2+CAF;               // assign P1.6/CA6 as input
}

char comp ( void )
{
	char caout;
	CACTL1 = CAON+CAREF0;                   // enable COMPA
	caout = CACTL2 & CAOUT;
	CACTL1 = 0;
	return (caout);
}

/***********************************/
/*   Init Port module              */
/***********************************/

void init_port ( void )
{
  // all (unused) port pins to LOW
  P1OUT = 0;
  P2OUT = 0;

  // all (unused) port pins to OUTPUT
  P1DIR = 0xFF;
  P2DIR = 0xFF;

}

void init_port_sw2 ( void )
{
  // configure P1.3 as SW2
  P1DIR &= ~(BIT3);
  P1REN |=  (BIT3);
  P1OUT |=  (BIT3);
  P1IFG  =  0xFF;
  P1IFG  =  0x00;
  P1IE  |=  (BIT3);
}

#pragma vector = PORT1_VECTOR
__interrupt void PRT1_ISR(void)
{
  P1IFG = 0;
  __bic_SR_register_on_exit(LPM4_bits);     // wakeup from LPM
}

/***********************************/
/*   MAIN                          */
/***********************************/

void main( void )
{
  // initialize all system relevant modules like
  // Clock System, Power Managment Module, I/O-Ports
  init_wdta ();
  init_port ();
  init_cs ();
  init_port_sw2 ();
  init_comp ();

//  __bis_SR_register(LPM4_bits+GIE);   // Enter LPM3, enable interrupts

  while (1)
  {
  	 delay_250ms(1);
     if (comp()==0)
	 {
       P1OUT |= 0x01;
       delay_1ms(200);
       P1OUT &= ~0x01;
	 }
  }
}

