#include <stdint.h>
#include "Interrupts.h"
#include "tm4c1294ncpdt.h"
#include "SysTick.h"
#include "ToFSensor.h"

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

// Enable Port M Interrupts
void PortJ_Interrupt_Init(void){
	
		GPIO_PORTJ_IS_R = 0;  // (Step 1) PJ[1:0] is Edge-sensitive 
		GPIO_PORTJ_IBE_R = 0;  // PJ[1:0] is not triggered by both edges 
		GPIO_PORTJ_IEV_R = 0;  // PJ[1:0] is falling edge event 
		GPIO_PORTJ_ICR_R = 0x03;  // Clear interrupt flag by setting proper bit in ICR register
		GPIO_PORTJ_IM_R = 0x03;  // Arm interrupt on PJ[1:0] by setting proper bit in IM register
    
		NVIC_EN1_R = 0x00080000;  // (Step 2) Enable interrupt 51 in NVIC (which is in Register EN1)
	
		NVIC_PRI12_R = 0xA0000000;  // (Step 4) Set interrupt priority to 5

		EnableInt();  // (Step 3) Enable Global Interrupt. lets go!
}

// Configure what happens when a PORT M interrupt occurs
void GPIOJ_IRQHandler(void){
	
	// Create an if-statement that will get the measurements if '1' is pressed
	if((GPIO_PORTJ_DATA_R & 0b00000001) == 0){
		// Only run if the measurements have been enabled
		if(Is_Enabled())
			Measure();
	
		// Acknowledge flag by setting proper bit in ICR register
		GPIO_PORTJ_ICR_R = 0x01;
	}
	
	// If-statement that allows measurements to be sent, thus enabling the entire process
	if((GPIO_PORTJ_DATA_R & 0b00000010) == 0){
		// Call enable function in ToFSensor
		Measurements_Enable();
		
		// Acknowledge flag by setting proper bit in ICR register
		GPIO_PORTJ_ICR_R = 0x02;
	}
}