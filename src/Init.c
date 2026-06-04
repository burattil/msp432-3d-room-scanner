#include <stdint.h>
#include "Init.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "SysTick.h"
#include "VL53L1X_api.h"

// FUNCTIONS FOR GENERAL BOARD INITIALIZATIONS

// Give clock to Port J and initalize PJ[1:0] as Digital Input GPIO
void PortJ_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;  // Activate clock for Port J
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R8) == 0){};  // Allow time for clock to stabilize
  GPIO_PORTJ_DIR_R &= ~0x03;  // Make PJ[1:0] input 
  GPIO_PORTJ_DEN_R |= 0x03;  // Enable digital I/O on PJ[1:0]
	
	GPIO_PORTJ_PCTL_R &= ~0x000000FF;  //? Configure PJ1 as GPIO 
	GPIO_PORTJ_AMSEL_R &= ~0x03;  //??Disable analog functionality on PJ[1:0]		
	GPIO_PORTJ_PUR_R |= 0x03;  //	Enable weak pull up resistor on PJ[1:0]
}

// Initialize Port H as an output for the stepper motor
void PortH_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;  // Activate the clock for Port H
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};  // Allow time for clock to stabilize
		
	GPIO_PORTH_DIR_R = 0b00001111;  // Enable PH[3:0] as outputs													
	GPIO_PORTH_DEN_R = 0b00001111;  // Enable PH[3:0] as digital pins
		
	return;
}

// Initialize the I2C
void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;  // Activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;  // Activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};  // Ready?

  GPIO_PORTB_AFSEL_R |= 0x0C;  // 3) enable alt funct on PB2,3       0b00001100
  GPIO_PORTB_ODR_R |= 0x08;  // 4) enable open drain on PB3 only

  GPIO_PORTB_DEN_R |= 0x0C;  // 5) enable digital I/O on PB2,3

  // 6) configure PB2,3 as I2C
		
	GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;  // TED
  I2C0_MCR_R = I2C_MCR_MFE;  // 9) master function enable
  I2C0_MTPR_R = 0b0000000000000101000000000111011;  // 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
		
	return;
}

// XSHUT    
// This pin is an active-low shutdown input; the board pulls it up to VDD to enable the sensor by default. 
// Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
  GPIO_PORTG_DIR_R |= 0x01;  // Make PG0 out
  GPIO_PORTG_DATA_R &= 0b11111110;  // PG0 = 0
  FlashAllLEDs();
  SysTick_Wait10ms(10);
  GPIO_PORTG_DIR_R &= ~0x01;  // Make PG0 input (HiZ)  
}