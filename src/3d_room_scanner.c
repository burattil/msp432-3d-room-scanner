/*  Final Project Code, Luca Burattini

            Written by Tom Doyle
            Updated by  Hafez Mousavi Garmaroudi
            Last Update: March 17, 2026
						Updated by Luca Burattini
*/

#include <stdint.h>
#include <math.h>
#include <string.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable
#define MAXRETRIES              5           // number of receive attempts before giving up

uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

// Create variables that will start/stop the motor and measuring
volatile int motor = 0, acquire = 0;

// Create a variable to determine if the motor has done a 360
volatile int rotationCount = 0;

// Enable interrupts
void EnableInt(void)
{    __asm("    cpsie   i\n");
}

// Disable interrupts
void DisableInt(void)
{    __asm("    cpsid   i\n");
}

// Low power wait
void WaitForInt(void)
{    __asm("    wfi\n");
}

// Initialize Port M as an input
void PortM_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;                 	// Activate the clock for Port M
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R11) == 0){};     	  // Allow time for clock to stabilize
		
	GPIO_PORTM_DIR_R = 0b00000000;       								      	// Enable PM0 and PM1 as inputs 
  GPIO_PORTM_DEN_R = 0b00000111;															// Enable PM0 and PM1 as digital pins
	GPIO_PORTM_PUR_R = 0b00000111;															// Enable the pull-up resistors for PM0 and PM1

	return;
}

// Enable Port M Interrupts
void PortM_Interrupt_Init(void){
	
		GPIO_PORTM_IS_R = 0;     												// (Step 1) PM[1:0] is Edge-sensitive (Interrupt Sense)
		GPIO_PORTM_IBE_R = 0;   												// PM[1:0] is not triggered by both edges (Interrupt Both Edges)
		GPIO_PORTM_IEV_R = 0;   												// PM[1:0] is falling edge event (Interrupt Event)
		GPIO_PORTM_ICR_R = 0x07;    										// Clear interrupt flag by setting proper bit in ICR register (Interrupt Clear Register)
		GPIO_PORTM_IM_R = 0x07;     										// Arm interrupt (enable interrupts) on PM[1:0] by setting proper bit in IM register (Interrupt Mask)
    
		NVIC_EN2_R = 0x0000100;           							// (Step 2) Enable interrupt 72 in NVIC (which is in Register EN2)
	
		NVIC_PRI12_R = 0xA0000000; 											// (Step 4) Set interrupt priority to 5

		EnableInt();																		// (Step 3) Enable Global Interrupt. lets go!
}

// Configure what happens when a PORTJ interrupt occurs
void GPIOM_IRQHandler(void){
	
	// Create an if-statement that will toggle the on variable if '1' is pressed
	if((GPIO_PORTM_DATA_R & 0b00000001) == 0)
	{
			// Toggle the variable 
			motor ^= 1;
		
			// Change the spinCount back to 0
			rotationCount = 0;
		
			// Acknowledge flag by setting proper bit in ICR register
			GPIO_PORTM_ICR_R = 0x01;
	}
	
	// Create an if-statement that will toggle the on variable if '2' is pressed
	if((GPIO_PORTM_DATA_R & 0b00000010) == 0)
	{
			// Toggle the variable
			acquire ^= 1;
		
			// Acknowledge flag by setting proper bit in ICR register
			GPIO_PORTM_ICR_R = 0x02;
	}
	
	// Create an if-statement that will run the clock-bus display code (use AD3)
	if((GPIO_PORTM_DATA_R & 0b00000100) == 0)
	{
			// Make it go forever
			while(1)
			{
					// Turn the output on for 500ms
					GPIO_PORTL_DATA_R = 0b00000001;
					SysTick_Wait10ms(50);
					
					// Turn the output off for 500ms
					GPIO_PORTL_DATA_R = 0b00000000;
					SysTick_Wait10ms(50);
			}
	}
}

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

// Initialize Port H as an output
void PortH_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;            // Activate the clock for Port H
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};		// Allow time for clock to stabilize
		
	GPIO_PORTH_DIR_R = 0b00001111;											// Enable PH[3:0] as outputs													
	GPIO_PORTH_DEN_R = 0b00001111;											// Enable PH[3:0] as digital pins
		
	return;
}

void PortE_Init(void){	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;		              // Activate the clock for Port E
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R4) == 0){};	      // Allow time for clock to stabilize
  
	GPIO_PORTE_DIR_R = 0b00000001;														// Enable PE0 as output
	GPIO_PORTE_DEN_R = 0b00000001;                        		// Enable PE0 as digital pin
	return;
	}

//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}


int main(void) {
	// Constants used for initializations, etc.
  uint8_t byteData, sensorState=0;
  uint16_t Distance;
  uint8_t RangeStatus;
  uint8_t dataReady;
	
	// Variable initializations
	int zBreak = 0;
	char dist[16];

	// Initializations
	PLL_Init();							// PLL
	SysTick_Init();					// SysTick
	onboardLEDs_Init();			// LEDs
	I2C_Init();							// I2C
	UART_Init(); 						// UART
	PortH_Init();						// Motor
	PortE_Init();						// Keypad output
	PortM_Init(); 					// Keypad inputs
	PortM_Interrupt_Init();	// Interrupt routine
	
	// Output a low to the first row (PE0)
	GPIO_PORTE_DATA_R =  0b11111110;
	
	// 1 Wait for device booted
	while(sensorState==0)
	{
			status = VL53L1X_BootState(dev, &sensorState);
			SysTick_Wait10ms(10);
  }
	
	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
	
  /* 2 Initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);

  // 4 What is missing -- refer to API flow chart
  status = VL53L1X_StartRanging(dev);   // This function has to be called to enable the ranging
	
	
	while(1)
	{
			// If the motor is not on, do nothing but make it check again
			if(!motor) continue;
		
			// If the motor variable is on, go through the entire process
			zBreak = 0;
			rotationCount = 0;
			
			// Get the Distance Measures 32 times
			for(int i = 0; i < 32; i++) 
			{
					// If the motor is off during this step, break the loop
					if(!motor) break;
						
					// IF YOU AREN'T SUPPOSED TO MEASURE AT ALL, MOVE IF-STATEMENT TYPE TO THE TOP
					
					// 5 wait until the ToF sensor's data is ready, but also make sure nothing else has changed
					while (dataReady == 0 && motor)
					{
							status = VL53L1X_CheckForDataReady(dev, &dataReady);
							VL53L1_WaitMs(dev, 5);
					}
					dataReady = 0;
					
					// If you are stopped while checking if the sensor's data is ready, break out of the for-loop and go back to the top
					if(!motor) break;
					
					//7 read the data values from ToF sensor
					status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
					status = VL53L1X_GetDistance(dev, &Distance);					//The Measured Distance value

					// If the RangeStatus is equal to 0, flash LED PF4 and send the data (potentially)
					if(!RangeStatus)
					{
							FlashLED3(5);
			
							// Only acquire the data if the acquire is set. Here the motor won't stop.
							if(acquire)
							{
									// Convert the value in Distance to a text with \n
									sprintf(dist, "%u", Distance);
									
									// Create a for-loop to send each bit at a time
									for(int h = 0; h < strlen(dist); h++)
									{
											// When it is sent out, flash LED PF0
											FlashLED4(1);
										
											UART_OutChar(dist[h]);
									}
									
									// Send it so it knows to stop
									UART_OutChar('\n');
							}
					}
					
					// If the RangeStatus is 1 (invalid), send garbage data so Python can recognize it
					else
					{
							UART_OutChar('g');
							UART_OutChar('\n');
					}
					
					status = VL53L1X_ClearInterrupt(dev); /* 8 clear interrupt has to be called to enable next interrupt*/				
					
					// Rotate the motor 11.25 deg
					for(int j = 0; j < 16; j++)
					{
							// Rotate counter-clockwise that many steps
							GPIO_PORTH_DATA_R = 0b00001001;
							SysTick_Wait10us(140);											
							GPIO_PORTH_DATA_R = 0b00001100;													
							SysTick_Wait10us(140);
							GPIO_PORTH_DATA_R = 0b00000110;													
							SysTick_Wait10us(140);
							GPIO_PORTH_DATA_R = 0b00000011;													
							SysTick_Wait10us(140);
						
							// Check to make sure that the motor hasn't been turned off
							if(!motor)
							{
									// Set the zBreak variable to break the outer-for-loop
									zBreak = 1;	
						
									break;
							}
					}
						
					// If the zBreak is set in this for-loop, it needs two exits
					if(zBreak) break;
					
					// Wait a second
					SysTick_Wait10ms(50);
					
					// Make sure the button hasn't been pressed in that time
					if(!motor) break;
					
					// Add one to the rotationCount;
					rotationCount++;
					
					// If 45 deg have been done
					if(rotationCount % 4 == 0)
					{
							// Blink the other status LED
							FlashLED2(1);
					}
			}
			
			// Create it so it will only go after a full rotation
			if(motor && !zBreak)
			{
					// Do a full rotation to avoid force from the wires
					for(int k = 0; k < 512; k++)
					{
							GPIO_PORTH_DATA_R = 0b00000011;
							SysTick_Wait10us(140);	
							GPIO_PORTH_DATA_R = 0b00000110;													
							SysTick_Wait10us(140);
							GPIO_PORTH_DATA_R = 0b00001100;													
							SysTick_Wait10us(140);
							GPIO_PORTH_DATA_R = 0b00001001;
							SysTick_Wait10us(140);	
						
							// Make sure the motor hasn't been pressed again
							if(!motor) break;
					}
					
					// If it reaches to this point in the code, turn the motor off
					motor = 0;
			}
	}
	
	VL53L1X_StopRanging(dev);
}

