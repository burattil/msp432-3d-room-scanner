#ifndef INTERRUPTS_H
#define INTERRUPTS_H

void EnableInt(void);
void DisableInt(void);
void WaitForInt(void);


void PortM_Interrupt_Init(void);
void GPIOM_IRQHandler(void);

#endif