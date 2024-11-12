#include "STM32L432KC.h"
#include <stm32l432xx.h>
#define USART_ID USART2_ID

#define encoderA PA5
#define encoderB PA8

volatile int direction;
volatile int interval;

int main(void) {
  // Configure flash and clock
  configureFlash();
  configureClock();

  // Initialize GPIO pins for inputs
  gpioEnable(GPIO_PORT_A);
  pinMode(encoderA, GPIO_INPUT); 
  GPIOA->PUPDR |= _VAL2FLD(GPIO_PUPDR_PUPD9, 0b01);
  pinMode(encoderB, GPIO_INPUT); 
  GPIOA->PUPDR |= _VAL2FLD(GPIO_PUPDR_PUPD6, 0b01);

  // Initialize timer
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
  initTIM2(TIM2);
  RCC->APB2ENR |= (1 << 17);
  initTIM(TIM16);

  // Enable SYSCFG clock domain in RCC
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // Configure EXTICR for PA5 and PA8
  SYSCFG->EXTICR[0] |= _VAL2FLD(SYSCFG_EXTICR2_EXTI5, 0b000); // Select PA5
  SYSCFG->EXTICR[0] |= _VAL2FLD(SYSCFG_EXTICR3_EXTI8, 0b000); // Select PA8

  // Enable interrupts globally
   __enable_irq();

  // Configure interrupt for PA5
  EXTI->IMR1 |= (1 << 5);   // Enable interrupt mask for PA5
  EXTI->RTSR1 &= ~(1 << 5);  // Disable rising edge trigger
  EXTI->FTSR1 |= (1 << 5);  // Enable falling edge trigger

  // Configure interrupt for PA8
  EXTI->IMR1 |= (1 << 8);   // Enable interrupt mask for PA8
  EXTI->RTSR1 &= ~(1 << 8);  // Disable rising edge trigger
  EXTI->FTSR1 |= (1 << 8);  // Enable falling edge trigger

  // Enable EXTI interrupt in NVIC
  //SYSCFG->EXTICR[1] &= ~(SYSCFG_EXTICR2_EXTI5);  // Clear the EXTI5 field
  //SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI8);  // Clear the EXTI5 field
  //SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI5_PA;  // EXTI5 connected to PA5
  //SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI8_PA;  // EXTI8 connected to PA8
  NVIC_EnableIRQ(EXTI9_5_IRQn);

  // Initialize USART
  USART_TypeDef *USART = initUSART(USART_ID, 9600);

  float period;
  float speed;

  while (1) {
        if (direction) {
            printf("clockwise \n");
        } else {
            printf("counter-clockwise \n");
        }
        
        if (interval > 50000) {
          printf("Speed 0 rev/s\n");
        } else {
            // Calculate speed in revolutions per second
            speed = 1 / 0.000001 / interval / 120.0 / 4.0;
            if (speed<0.3) {
                printf("Speed 0 rev/s\n");
            } else {
                printf("Speed  %f rev/s\n", speed);
            }
            
        }
      
        // Delay using Timer 2
        delay_millis(TIM2, 1000);  // Delay for 0.5s
    }
}

// Interrupt handler
void EXTI9_5_IRQHandler(void) {
    int a = digitalRead(encoderA);
    int b = digitalRead(encoderB);

    // Check if interrupt is triggered
    if (EXTI->PR1 & (1 << 5)) {
        // Clear interrupt flag for PA5
        EXTI->PR1 |= (1 << 5);

        // Determine direction and calculate time interval
        if ((a && b) || (!a && !b)) {
            interval = TIM16->CNT;  // Capture time between signals
            direction = 0;  // Set counter-clockwise
        }

        // Reset Timer 16 for next pulse
        TIM16->CNT = 0;
        TIM16->EGR |= (1 << 0);  // Generate update event
    }

    // Check if interrupt came from PA8
    if (EXTI->PR1 & (1 << 8)) {
        // Clear interrupt flag for PA8  
        EXTI->PR1 |= (1 << 8);   

        // Determine direction and calculate time interval
        if ((a && b) || (!a && !b)) {
            interval = TIM16->CNT;  // Capture time between signals
            direction = 1;  // Set clockwise
        }

        // Reset Timer 16 counter
        TIM16->CNT = 0;
        TIM16->EGR |= (1 << 0);  // Update event generation
    }

}