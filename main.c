//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"

void System_Config(void);
void UART0_Config(void);
void UART0_SendChar();
char UART0_GetChar();
int main(void)
{
	System_Config();
	UART0_Config();
	while(1){

	}
	
}

void System_Config (void){
	SYS_UnlockReg(); // Unlock protected registers

	// enable clock sources
	CLK->PWRCON |= (0x01 << 0);
	while(!(CLK->CLKSTATUS & (1 << 0)));

	//PLL configuration starts
	CLK->PLLCON &= ~(1<<19); //0: PLL input is HXT
	CLK->PLLCON &= ~(1<<16); //PLL in normal mode
	CLK->PLLCON &= (~(0x01FF << 0));
	CLK->PLLCON |= 48;
	CLK->PLLCON &= ~(1<<18); //0: enable PLLOUT
	while(!(CLK->CLKSTATUS & (0x01 << 2)));
	//PLL configuration ends

	// CPU clock source selection
	CLK->CLKSEL0 &= (~(0x07 << 0));
	CLK->CLKSEL0 |= (0x02 << 0);
	//clock frequency division
	CLK->CLKDIV &= (~0x0F << 0);

	//UART0 Clock selection and configuration
	CLK->CLKSEL1 |= (0x03 << 24); // UART0 clock source is 22.1184 MHz
	CLK->CLKDIV &= ~(0x0F << 8); // clock divider is 1
	CLK->APBCLK |= (0x01 << 16); // enable UART0 clock

	SYS_LockReg();  // Lock protected registers
}

void UART0_Config(void) {
	// UART0 pin configuration. PB.1 pin is for UART0 TX; PB.0 pin is for UART0 RX
	PB->PMD &= ~(0x03 << 2);
	PB->PMD |= (0x01 << 2); // PB.1 is output pin
	PB->PMD &= ~(0x03 << 0); // PB.0 is input pin
	SYS->GPB_MFP |= (0x01 << 1); // GPB_MFP[1] = 1 -> PB.1 is UART0 TX pin
	SYS->GPB_MFP |= (0x01 << 0); // GPB_MFP[0] = 1 -> PB.0 is UART0 RX pin
	

	// UART0 operation configuration
	UART0->FCR |= (0x03 << 1); // clear both TX & RX FIFO
	UART0->FCR &= ~(0x0F << 16); // FIFO Trigger Level is 1 byte
	UART0->LCR &= ~(0x01 << 3); // no parity bit
	UART0->LCR &= ~(0x01 << 2); // one stop bit
	UART0->LCR |= (0x03 << 0); // 8 data bit

	//Baud rate config: BRD/A = 10, DIV_X_EN=0
	//--> Mode 0, Baud rate = UART_CLK/[16*(A+2)] = 22.1184 MHz/[16*(10+2)]= 115200 bps
	UART0->BAUD &= ~(0b11 << 28); // mode 0	
	UART0->BAUD &= ~(0xFFFF << 0);
	UART0->BAUD |= 10; 

	// UART0 Enable Interrupts
	UART0->IER |= (0b01 << 0); // Receive Data Available Interrupt Enable.	
	UART0->IER |= (0b01 << 1); // Transmit Holding Register Empty Interrupt Enable
	// UART Interrupt enabling and priority setting
	NVIC->ISER[0] |= 1 << 12;
	NVIC->IP[3] &= (~(0b11 << 6));
}

void UART02_IRQHandler(void){
	UART0_SendChar();

}

char UART0_GetChar(){
	while(1){
		if(!(UART0->FSR & (0x01 << 14))){ 
			return(UART0->RBR);
		}
	}
}

void UART0_SendChar(){
	int ch = UART0_GetChar();
	while(UART0->FSR & (0x01 << 23)); //wait until TX FIFO is not full
	UART0->THR = ch;
	if(ch == '\n'){								// \n is new line
		while(UART0->FSR & (0x01 << 23));
		UART0->THR = '\r';						// '\r' - Carriage return
	}
}


	
//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------