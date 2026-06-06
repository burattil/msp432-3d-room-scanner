// Written by Luca Burattini
// Modified May 29, 2026

// FINAL PROJECT CODE

#include <stdint.h>
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
	PLL_Init();  // PLL
	SysTick_Init();  // SysTick
	onboardLEDs_Init();  // LEDs
	I2C_Init();  // I2C
	UART_Init();  // UART
	PortH_Init();  // Motor
	PortJ_Init();  // Push buttons
	PortJ_Interrupt_Init();	// Interrupt routine
	Sensor_Init();  // ToF Sensor
	
	while(1){}
		
	Stop_Ranging();
}