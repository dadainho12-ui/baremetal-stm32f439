# baremetal-stm32f439

STM32F439ZI 기반 Bare-Metal Embedded C 학습기록입니다.

## Development Environment

- Board: NUCLEO-F439ZI
- MCU: STM32F439ZIT6
- Core: ARM Cortex-M4
- IDE: STM32CubeIDE
- Language: C

- ## Goal

HAL 라이브러리에 의존하지 않고 STM32의 메모리 맵과
레지스터를 직접 제어하면서 MCU의 동작 원리를 이해하는 것을 목표로 한다.
(Baremetal Embedded C Programming 책 참고)


## Contents

- GPIO
- RCC / Clock
- UART
- Timer
- Interrupt / NVIC
- ADC
- SPI
- I2C
