#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "SysTick.h"

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

// Configure what happens when a PORT M interrupt occurs
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