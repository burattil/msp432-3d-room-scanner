#include <stdint.h>
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
uint8_t measurements = 0;

// Variables used to gather measurements and send them, respectively
char distances[MEASUREMENTS][32];
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
	FlashLED1(10);
	// Clear interrupt has to be called to enable next interrupt
	status = VL53L1X_ClearInterrupt(dev); 
	FlashLED2(10);
  // Initialize the sensor with the default setting
  status = VL53L1X_SensorInit(dev); FlashLED3(10);
	Status_Check("SensorInit", status); FlashLED4(10);

  // Enable ranging
  status = VL53L1X_StartRanging(dev); FlashLED3(10);
	
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

// Get the measurements with the sensor
void Get_Measurements(void){
	// Go for as many times as MEASUREMENTS
	for(int i = 0; i < MEASUREMENTS; i++){
		// Get the distance measured 32 times
		for(int j = 0; j < 32; j++){
			Sensor_Wait();
		
			// Read the data values from ToF sensor
			status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
			status = VL53L1X_GetDistance(dev, &Distance);					// The measured distance value

			// Ensure that the measurement is valid before storing it
			if(!RangeStatus){
				// Signal that data was valid
				FlashLED3(5);

				// Store the data in the array
				distances[i][j] = Distance;
			}
			
			// If the measurement was invalid
			else{
				// Store a garbage value that can be caught
				distances[i][j] = 'g';
			}
			
			// Clear interrupt has to be called to enable next interrupt (measurement)
			status = VL53L1X_ClearInterrupt(dev);				
			
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
		}
		
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
	}	

	// Switch the measurements variable to true
	measurements = 1;
	
	return;
}

// Send the measurements via UART
void Send_Measurements(void){
	// Send as many times as MEASUREMENTS
	for(int i = 0; i < MEASUREMENTS; i++){
		// Send the distance measured 32 times
		for(int j = 0; j < 32; j++){
			// Save each value in a buffer string
			sprintf(dist, "%u\n", distances[i][j]);
			
			// Send the value one character at a time 
			for(int h = 0; h < strlen(dist); h++){
				// When it is sent out, flash status LED PF0
				FlashLED4(1);
				
				UART_OutChar(dist[h]);
			}
		}
		
		SysTick_Wait10ms(5);
	}
	
	return;
}

// Stop ranging function that is required in the main code
void Stop_Ranging(void){
	VL53L1X_StopRanging(dev);
	
	return;
}

// Function to determine if the measurements have been taken (used in the interrupt code)
int Measurements_Taken(void){
	if(measurements == 1) return 1;
	else return 0;
}