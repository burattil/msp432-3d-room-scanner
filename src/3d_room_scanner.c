// Written by Luca Burattini
// Modified May 29, 2026

// FINAL PROJECT CODE

#include <stdint.h>
//#include <math.h>
#include <string.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
#include "vl53l1_platform_2dx4.h"
#include "Init.h"
#include "Interrupts.h"
#include "ToFSensor.h"

int main(void) {
	// Initializations
	I2C_Init();							// I2C
	SysTick_Init();					// SysTick
	onboardLEDs_Init();			// LEDs
	PortE_Init();						// Keypad output
	PortH_Init();						// Motor
	PortM_Init(); 					// Keypad inputs
	PortM_Interrupt_Init();	// Interrupt routine
	PLL_Init();							// PLL
	UART_Init(); 						// UART
	Sensor_Init();					// ToF Sensor
	Begin_Scanning(); 			// Begin scanning the keypad
	
	while(1){}
		
	Stop_Ranging();
}