#ifndef INTERRUPTS_H
#define INTERRUPTS_H

void EnableInt(void);
void DisableInt(void);
void WaitForInt(void);

void PortJ_Interrupt_Init(void);
void GPIOJ_IRQHandler(void);

#endif