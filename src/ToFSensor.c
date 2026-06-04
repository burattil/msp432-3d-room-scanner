#include <stdint.h>
#include <string.h>
#include "ToFSensor.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "vl53l1_platform_2dx4.h"

// FUNCTIONS FOR SENSOR INITIALIZATIONS/COMMANDS

uint16_t	dev = 0x29;	 // Address of the ToF sensor as an I2C slave peripheral

uint8_t sensorState = 0;
uint16_t Distance;
uint8_t RangeStatus;
uint8_t dataReady;
int status = 0;

// Variable to confirm that the measurements have been taken
uint8_t enable = 0;

// Variables used to gather measurements and send them, respectively
char dist[16];

// Create a variable to determine if the motor has done a 360
int rotationCount = 0;



// Initialization function
void Sensor_Init(void){
	// Wait for device booted
	while(sensorState == 0){
			status = VL53L1X_BootState(dev, &sensorState);
			SysTick_Wait10ms(10);
  }
	
	// Clear interrupt has to be called to enable next interrupt
	status = VL53L1X_ClearInterrupt(dev); 
	
  // Initialize the sensor with the default setting
  status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);
	
	FlashAllLEDs();

  // Enable ranging
  status = VL53L1X_StartRanging(dev);
	
	FlashAllLEDs();
	
	return;
}

// Helper function——Wait for the sensor to be measurement-ready again
void Sensor_Wait(void){
	while (dataReady == 0)
	{
			status = VL53L1X_CheckForDataReady(dev, &dataReady);
			VL53L1_WaitMs(dev, 5);
	}
	dataReady = 0;
	
	return;
}

// Helper function to rotate the sensor to its next measuring point
void Rotate_Next_Measurement(void){
	// Rotate the motor 11.25 deg counter-clockwise
	for(int j = 0; j < 16; j++)
	{
		GPIO_PORTH_DATA_R = 0b00001001;
		SysTick_Wait10us(140);											
		GPIO_PORTH_DATA_R = 0b00001100;													
		SysTick_Wait10us(140);
		GPIO_PORTH_DATA_R = 0b00000110;													
		SysTick_Wait10us(140);
		GPIO_PORTH_DATA_R = 0b00000011;													
		SysTick_Wait10us(140);
	}

	SysTick_Wait10ms(50);
	
	// Add one to the rotationCount;
	rotationCount++;
	
	// If 45 deg have been done
	if(rotationCount % 4 == 0)
	{
			// Blink status LED
			FlashLED2(1);
	}
	
	return;
}

// Helper function to fully rotate the sensor in the opposite direction
void Full_Rotation(void){
	// Do a full rotation clockwise to mitigate force from the wires
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
	}
	
	return;
}

// Helper function to send the measurements via UART
void Send_Measurement(uint16_t measurement){
	// Convert the value in Distance to text 
	sprintf(dist, "%u", measurement);
	
	// Create a for-loop to send each bit at a time
	for(int h = 0; h < strlen(dist); h++){
		// When it is sent out, flash LED4
		FlashLED4(1);
		
		UART_OutChar(dist[h]);
	}
	
	// Send a newline so it knows when to stop reading
	UART_OutChar('\n');
	
	return;
}

// Get the measurements with the sensor
void Measure(void){
	// Go for as many times as MEASUREMENTS
	for(int i = 0; i < XMEASUREMENTS; i++){
		// Get the distance measured 32 times
		for(int j = 0; j < 32; j++){
			Sensor_Wait();
		
			// Read the data values from ToF sensor
			status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
			status = VL53L1X_GetDistance(dev, &Distance);					// The measured distance value

			// Ensure that the measurement is valid before storing it
			if(!RangeStatus){
				// To show that the measurement is valid
				FlashLED3(1);
				SysTick_Wait10ms(10);
			
				Send_Measurement(Distance);
			}
				
			// If the measurement was invalid
			else{
				// Send a garbage value that can be caught
				UART_OutChar('g');
				UART_OutChar('\n');
			}
			
			// Clear interrupt has to be called to enable next interrupt
			status = VL53L1X_ClearInterrupt(dev);				
			
			// Rotate to the next point of measurement
			Rotate_Next_Measurement();
		}	

		// Rotate fully in the opposite direction to reduce wire tension
		Full_Rotation();
	}
	
	return;
}

// Stop ranging function that is required in the main code
void Stop_Ranging(void){
	VL53L1X_StopRanging(dev);
	
	return;
}

// Function to determine if the measurements can be sent (used in the interrupt code)
void Measurements_Enable(void){
	// Only execute if it hasn't already been done before
	if(enable == 0){
		enable = 1;
		
		FlashAllLEDs();
	}

	return;
}

// Function that returns true if enable is set, false if not
int Is_Enabled(void){
	if(enable == 1) return 1;
	else return 0;
}