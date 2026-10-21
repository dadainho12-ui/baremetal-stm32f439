\# Chapter 4 — 링커 스크립트와 스타트업 파일 작성



\## 실습 목적



STM32CubeIDE가 제공하는 기본 스타트업 파일과 링커 스크립트 대신

직접 작성한 파일로 펌웨어를 빌드하고 실행한다.



\- Flash와 SRAM의 메모리 배치 이해

\- 링커 스크립트 작성

\- 벡터 테이블 구성

\- Reset\_Handler에서 전역변수 초기화

\- GNU Arm 툴체인으로 컴파일 및 링크

\- ST-LINK를 통한 업로드와 LED 동작 확인



\## 개발 환경



| 항목 | 내용 |

|---|---|

| 교재 | Bare-Metal Embedded C Programming, Chapter 4 |

| 보드 | NUCLEO-F439ZI |

| MCU | STM32F439ZIT6 / Arm Cortex-M4 |

| 터미널 | Windows Git Bash |

| 컴파일러 | arm-none-eabi-gcc 13.3.1 |

| 업로드 도구 | STM32CubeProgrammer CLI |

| LED | LD1 / PB0 |



교재는 NUCLEO-F411 기준이며, 메모리 용량과 LED 핀 등을 F439ZI에 맞게 변경했다.



\## 파일 구성



| 파일 | 역할 |

|---|---|

| main.c | GPIOB 설정, 변수 초기값 검사, LD1 깜빡임 |

| stm32\_ls.ld | 메모리 영역과 출력 섹션 배치 |

| stm32f439\_startup.c | 벡터 테이블, 기본 예외 처리, Reset\_Handler |

| README.md | 실습 과정과 결과 기록 |



빌드 시 생성되는 파일:



\- main.o

\- stm32f439\_startup.o

\- firmware.elf

\- firmware.map



\## 1. 메모리 구성



| 영역 | 시작 주소 | 마지막 주소 | 용량 |

|---|---|---|---:|

| Flash | 0x08000000 | 0x081FFFFF | 2048KB |

| SRAM1 | 0x20000000 | 0x2001BFFF | 112KB |

| SRAM2 | 0x2001C000 | 0x2001FFFF | 16KB |

| SRAM3 | 0x20020000 | 0x2002FFFF | 64KB |



SRAM1·2·3은 주소가 연속되어 있으므로 192KB의 단일 영역으로 지정했다.

별도 주소 영역에 있는 64KB CCM RAM은 이번 실습에서 사용하지 않았다.



```ld

MEMORY

{

&#x20;   FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K

&#x20;   SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 192K

}

```



초기 스택 포인터는 SRAM 끝 경계 주소로 설정했다.



```ld

\_estack = ORIGIN(SRAM) + LENGTH(SRAM);

```



계산 결과는 0x20030000이다.

SRAM의 마지막 바이트 주소인 0x2002FFFF 바로 다음 주소이며,

스택은 사용하면서 낮은 주소 방향으로 증가한다.



참고 문서:



\- UM1974 Rev 11, 7쪽 Table 1: 보드에 탑재된 MCU 확인

\- DS9484 Rev 14, 14쪽: 제품군 메모리 사양

\- RM0090 Rev 22, 68쪽 및 71쪽 Table 4: SRAM 구성과 메모리 주소

\- RM0090 Rev 22, 377\~381쪽 Table 63: F439 벡터 테이블



\## 2. 링커 스크립트



\### 진입점



```ld

ENTRY(Reset\_Handler)

```



ELF 파일의 진입점을 Reset\_Handler로 지정한다.

실제 리셋 시 실행 위치는 벡터 테이블에도 등록한다.



\### 출력 섹션



| 섹션 | 내용 | 배치 |

|---|---|---|

| .text | 벡터 테이블, 실행 코드, 읽기 전용 데이터 | Flash |

| .data | 초기값이 있는 전역·정적 변수 | 실행 위치 SRAM, 초기값 저장 위치 Flash |

| .bss | 0으로 초기화할 전역·정적 변수 | SRAM |



\### VMA와 LMA



\- VMA: 실행 중 사용하는 주소

\- LMA: 초기 내용을 저장해두는 주소



```ld

.data :

{

&#x20;   . = ALIGN(4);

&#x20;   \_sdata = .;

&#x20;   \*(.data)

&#x20;   . = ALIGN(4);

&#x20;   \_edata = .;

} > SRAM AT> FLASH



\_sidata = LOADADDR(.data);

```



`> SRAM`은 VMA를, `AT> FLASH`는 LMA를 지정한다.

`\_sidata`는 .data 초기값의 실제 Flash 주소를 기록한다.



\### 주요 심볼



| 심볼 | 의미 |

|---|---|

| \_estack | 초기 스택 포인터 값 |

| \_etext | .text 끝 경계 |

| \_sidata | .data의 Flash 저장 주소 |

| \_sdata / \_edata | SRAM의 .data 시작·끝 경계 |

| \_sbss / \_ebss | SRAM의 .bss 시작·끝 경계 |



`\_\_max\_heap\_size`와 `\_\_max\_stack\_size`도 정의했다.

단, 현재 스크립트에서는 이 값으로 공간을 예약하거나 사용량을 제한하지 않는다.



\## 3. 스타트업 파일



\### 벡터 테이블



\- 첫 번째 값: 초기 스택 포인터

\- 두 번째 값: Reset\_Handler 주소

\- 이후: 기본 예외 및 주변장치 인터럽트 처리 주소



Cortex-M4 기본 항목 16개와 F439 주변장치 IRQ 0\~90을 합쳐

107개 항목, 총 428바이트로 구성했다.



```c

\[16 ... 106] = (uint32\_t)\&Default\_Handler

```



주변장치 인터럽트는 이번 실습에서 사용하지 않으므로

모두 Default\_Handler로 연결했다.



이 구문은 GCC 확장 문법이다.

나중에 주변장치 인터럽트를 사용하려면 해당 테이블 항목을

실제 핸들러 주소로 변경해야 한다.



\### 기본 예외 처리



기본 예외 처리 함수에는 weak와 alias 속성을 사용했다.



```c

void NMI\_Handler(void)

&#x20;   \_\_attribute\_\_((weak, alias("Default\_Handler")));

```



별도 구현이 없으면 Default\_Handler를 사용하고,

같은 이름의 일반 함수를 정의하면 해당 함수를 우선 사용한다.



\### Reset\_Handler



1\. \_sidata에서 \_sdata로 .data 초기값 복사

2\. \_sbss부터 \_ebss 직전까지 0으로 초기화

3\. main() 호출

4\. main()이 반환하면 무한 반복



uint32\_t 포인터를 사용하므로 한 번에 4바이트씩 처리하며,

끝 주소와 비교해 초기화 범위를 제한했다.



\## 4. LED 제어



\- RCC\_AHB1ENR의 GPIOBEN 비트로 GPIOB 클럭 활성화

\- GPIOB\_MODER의 PB0 모드 비트를 01로 설정

\- GPIOB\_ODR의 bit 0을 XOR 연산으로 반전

\- 소프트웨어 지연으로 깜빡임 간격 조절



지연 반복 횟수를 5,000,000회에서 500,000회로 줄여

깜빡임 간격이 짧아지는 것을 확인했다.



빈 반복문을 이용한 지연이므로 정확한 시간은 보장하지 않는다.



\## 5. 컴파일 및 링크



컴파일러와 업로드 도구의 bin 폴더를 Git Bash PATH에 추가한 뒤 실행한다.



\### 컴파일



```bash

arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -std=gnu11 -O0 -ffreestanding main.c -o main.o

```



```bash

arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -std=gnu11 -O0 -ffreestanding stm32f439\_startup.c -o stm32f439\_startup.o

```



\### 링크



```bash

arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -nostdlib -T stm32\_ls.ld main.o stm32f439\_startup.o -Wl,-Map=firmware.map -o firmware.elf

```



`-nostdlib`를 사용해 기본 스타트업 파일과 표준 라이브러리의

자동 연결 없이 직접 작성한 코드로 링크했다.



\### 결과 확인



```bash

arm-none-eabi-objdump -h firmware.elf

```



```bash

arm-none-eabi-objdump -s -j .text firmware.elf

```



\### 업로드·검증·리셋



```bash

STM32\_Programmer\_CLI.exe -c port=SWD -w firmware.elf -v -rst

```



\## 6. 변수 초기화 추가 검증



다음 전역변수를 main.c에 추가했다.



```c

volatile uint32\_t data\_test = 1234U;

volatile uint32\_t bss\_test;

```



main()에서 초기값을 검사하고, 예상과 다르면 LED를 켠 상태로 정지하도록 했다.



```c

if ((data\_test != 1234U) || (bss\_test != 0U))

{

&#x20;   GPIOB\_ODR |= LED\_PIN;



&#x20;   while (1)

&#x20;   {

&#x20;   }

}

```



\### 최종 섹션 배치



| 섹션 | 크기 | VMA | LMA |

|---|---:|---|---|

| .text | 0x298 = 664바이트 | 0x08000000 | 0x08000000 |

| .data | 4바이트 | 0x20000000 | 0x08000298 |

| .bss | 4바이트 | 0x20000004 | 복사할 초기값 없음 |



.bss는 ALLOC 속성을 가지며 LOAD 속성은 없다.

objdump가 표시하는 .bss의 LMA는 초기값 복사에 사용하지 않는다.



\### 결과



\- .data와 .bss가 각각 4바이트로 생성됨을 확인

\- 업로드 후 초기값 검사를 통과하고 LD1이 깜빡임을 확인

\- main()에서 data\_test가 1234, bss\_test가 0임을 확인



단, 이 검사는 main() 진입 시 변수 값이 예상과 일치함을 확인하는 방식이다.

초기화 전 SRAM에 다른 값을 넣어 비교하는 대조 실험은 수행하지 않았다.



\## 7. 교재 대비 변경 사항



\- F411 대신 F439ZI의 Flash 2048KB, 일반 SRAM 192KB 적용

\- PA5 대신 PB0의 LD1 사용

\- .data 복사 원본은 \_etext 대신 LOADADDR(.data)로 구한 \_sidata 사용

\- 초기화 반복문은 4바이트 포인터와 끝 주소 비교 방식으로 작성

\- 벡터 테이블에 const와 used 속성 추가

\- 주변장치 IRQ는 GCC 범위 초기화로 Default\_Handler에 연결

\- OpenOCD 대신 STM32CubeProgrammer CLI로 업로드

\- 전역변수를 추가해 초기값 검사 수행

