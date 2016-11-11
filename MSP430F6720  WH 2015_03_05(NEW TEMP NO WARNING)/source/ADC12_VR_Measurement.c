#include "msp430x54x.h"
#define RSPIN   (0x04)           //P11.2
#define RWPIN   (0x02)           //P11.1
#define EPIN    (0x01)           //P11.0



unsigned char XPOS,YPOS;

void delay(unsigned int t)
{  unsigned int i,j;
   for(i=0;i<t;i++)
      for(j=0;j<10;j++) ;
        
}

void lcdwaitidle(void)
{  P10DIR=0x00;
   P11OUT &=~RSPIN;
   P11OUT |=RWPIN;
   P11OUT |=EPIN;
   while((P10IN&0x80)==0x80);
   P11OUT &=~EPIN;
   P10DIR |=0Xff;
}

void lcdwcn(unsigned char c)
{  P11OUT &=~RSPIN;
   P11OUT &=~RWPIN;
   P10OUT=c;
   P11OUT |=EPIN;
   _NOP();
   P11OUT &=~EPIN;
}

void lcdwc(unsigned char c)
{  lcdwaitidle();
   lcdwcn(c);
}

void lcdwd(unsigned char d)
{  lcdwaitidle();
   P11OUT |=RSPIN;
   P11OUT &=~RWPIN;
   P10OUT=d;
   P11OUT |=EPIN;
   _NOP();
   P11OUT &=~EPIN;
}

void lcdpos(void)
{  XPOS&=0x0f;
   YPOS&=0x03;
   if(YPOS==0x00)
      lcdwc(XPOS|0x80);
   else if(YPOS==0x01)
      lcdwc((XPOS+0x40)|0x80);
}

void lcdreset()
{  delay(150); lcdwcn(0x38);
   delay(50); lcdwcn(0x38);
   delay(50); lcdwcn(0x38);
   lcdwc(0x38);
   lcdwc(0x08);
   lcdwc(0x01);
   lcdwc(0x06);
   lcdwc(0x0c);
}

void lcdfill(unsigned char n)
{
   for(YPOS=0;YPOS<2;YPOS++)
      for(XPOS=0;XPOS<16;XPOS++)
      {  lcdpos();
         lcdwd(n);  }
}

void lcdwda(unsigned char x,unsigned char y,unsigned char *s)
{  YPOS=y;
   for(XPOS=x;XPOS<16;XPOS++)
   {  lcdpos();
      lcdwd(*s);
      s++;
      delay(400);
   }
}
volatile unsigned int results;           
char text[16]={"voltage=0.000   "};
int   v[4]={0},i=0,s=0;
void main(void)
{
   
   WDTCTL = WDTPW+WDTHOLD;   
   P5DIR |= BIT5;     
   
   P9DIR |= BIT6+BIT7;
   P9DS |=BIT6+BIT7;
   P9OUT &=~BIT7; //K
   P9OUT |= BIT6; //A
   //
   P10OUT =0;
   P11OUT =0;
   P10DIR |=0XFF;
   P11DIR |=0XFF;
      
   P11OUT &=~EPIN;
  
  //UART
  P5SEL = BIT6+BIT7;                        // P5.6,7 = USCI_A0 TXD/RXD
  UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA1BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA1BR1 = 0x00;                           //
  UCA1MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  
  //ADC
  ADC12CTL0 = ADC12ON+ADC12SHT02+ADC12REFON+ADC12REF2_5V;
                                                        // Internal ref = 2.5V
  ADC12CTL1 = ADC12SHP;         // enable sample timer
  ADC12CTL2 = ADC12RES_2+ADC12REFOUT; //12BIT,REF OUT continuous
  ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_15;  // ADC i/p ch A15 
  ADC12IE = 0x001;                          // ADC_IFG upon conv result-ADCMEMO
  __delay_cycles(35);                       // 35us delay to allow Ref to settle
                                                           // based on default DCO frequency.
                                                           // See Datasheet for typical settle time.
  P5SEL |= BIT0;                            // REF out
  ADC12CTL0 |= ADC12ENC;
  lcdreset(); 
  //TIMER
  TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
  TA0CCR0 = 32768;                          // 1s
  TA0CTL = TASSEL_1 + MC_1;  //  ACLK, up mode
  while(1)
  { 
     
    _BIS_SR(LPM0_bits + GIE);
  } 
 
}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
  s++;
  if(s==5)
  {
   s=0;
   P5OUT ^= BIT5;
   ADC12CTL0 |= ADC12SC;                 // Start convn - software trigger
      
   __bis_SR_register(LPM4_bits + GIE);     // Enter LPM4, Enable interrupts
  }
   TA0CCR0 += 32768;   // Add Offset to CCR0 01111111111111111
    
}
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                     // Vector  6:  ADC12IFG0
  
       results = ADC12MEM0;                    // Move results, IFG is cleared
       
       v[0] =(int)(2.5 *( results /4095.0));
       v[1] =(int)((25 * (results /4095.0))-v[0]*10);
       v[2] =(int)((250 * (results /4095.0))-v[0]*100-v[1]*10);
       v[3] =(int)((2500 * (results /4095.0))-v[0]*1000-v[1]*100-v[2]*10);
       text[8]=v[0]+0x30;
       text[10]=v[1]+0x30;
       text[11]=v[2]+0x30;
       text[12]=v[3]+0x30;
       lcdwda(0,0,text);
       for(i=0;i<16;i++){
         while (!(UCA1IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
         UCA1TXBUF = text[i];                  
       }   
      __bic_SR_register_on_exit(LPM4_bits);   // Exit active CPU, SET BREAKPOINT HERE
      
  case  8: break;                                  // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
    
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break; 
  }  
}