// Written by Luca Burattini
// Modified May 29, 2026

// FINAL PROJECT CODE

#include <stdint.h>
#include <math.h>
#include <string.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
#include "Init.h"
#include "Interrupts.h"

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
	I2C_Init();							// I2C
	onboardLEDs_Init();			// LEDs
	PortE_Init();						// Keypad output
	PortH_Init();						// Motor
	PortM_Init(); 					// Keypad inputs
	PortM_Interrupt_Init();	// Interrupt routine
	PLL_Init();							// PLL
	SysTick_Init();					// SysTick
	UART_Init(); 						// UART
	
	// Output a low to the first row (PE0)
	GPIO_PORTE_DATA_R =  0b11111110;
	
	// 1 Wait for device booted
	while(sensorState==0)
	{
			status = VL53L1X_BootState(dev, &sensorState);
			SysTick_Wait10ms(10);
  }
	
	// lear interrupt has to be called to enable next interrupt
	status = VL53L1X_ClearInterrupt(dev); 
	
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

