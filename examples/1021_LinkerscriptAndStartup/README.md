# Chapter 4 — 링커 스크립트와 스타트업 파일 작성

링커 스크립트와 C 언어 스타트업 파일을 직접 작성하고,
GNU 툴체인으로 빌드하여 보드의 LD1 LED 깜빡임을 확인했다.
추가로 전역변수를 사용해 초기값이 정상적으로 준비되는지 확인했다.

- 실습일: 2026-10-21
- 교재: Bare-Metal Embedded C Programming, Chapter 4
- 보드: NUCLEO-F439ZI
- MCU: STM32F439ZIT6 / Arm Cortex-M4
- LED: LD1, PB0
- 개발 환경: Windows, STM32CubeIDE 1.18.1, Git Bash
- 도구: GNU Tools for STM32 13.3.1, STM32CubeProgrammer CLI

> 실습 경로는 `examples/1021_LinkerscriptAndStartup`이다.
> 교재의 NUCLEO-F411 예제를 F439ZI의 메모리 구성과 PB0 LED에 맞게 변경했다.

## 1. 실습 목표

- Flash와 SRAM의 메모리 구성 이해
- 링커 스크립트로 코드와 데이터의 배치 지정
- 벡터 테이블과 Reset_Handler 직접 작성
- .data 복사와 .bss의 0 초기화 과정 이해
- 직접 작성한 파일만으로 컴파일·링크·업로드 수행

## 2. 실습 파일 구성

| 파일 | 역할 |
|---|---|
| main.c | GPIO 설정, 초기값 검사, LED 깜빡임 |
| stm32_ls.ld | 메모리 영역과 섹션 배치 지정 |
| stm32f439_startup.c | 벡터 테이블과 메모리 초기화 |
| README.md | 실습 과정과 결과 기록 |

기존 CubeIDE 프로젝트의 스타트업 파일과 링커 스크립트 대신
직접 작성한 파일을 사용했다.

## 3. 메모리 구성 확인

보드 매뉴얼 UM1974에서 NUCLEO-F439ZI에 탑재된 MCU가
STM32F439ZIT6임을 확인했다.

이후 데이터시트 DS9484와 레퍼런스 매뉴얼 RM0090에서
메모리 용량과 주소를 확인했다.

| 영역 | 시작 주소 | 마지막 주소 | 용량 |
|---|---|---|---:|
| Flash | 0x08000000 | 0x081FFFFF | 2048KB |
| SRAM1 | 0x20000000 | 0x2001BFFF | 112KB |
| SRAM2 | 0x2001C000 | 0x2001FFFF | 16KB |
| SRAM3 | 0x20020000 | 0x2002FFFF | 64KB |

SRAM1·2·3은 주소가 연속되어 있어 192KB의 단일 영역으로 지정했다.
별도 주소 영역에 있는 64KB CCM RAM은 이번 실습에서 사용하지 않았다.

참고한 문서의 위치:

- UM1974 Rev 11, 7쪽 Table 1: 보드와 MCU 확인
- DS9484 Rev 14, 14쪽: 제품군 메모리 사양
- RM0090 Rev 22, 68쪽 및 71쪽 Table 4: 메모리 구성과 주소
- RM0090 Rev 22, 377~381쪽 Table 63: 벡터 테이블

## 4. 링커 스크립트 작성

### 4.1 진입점과 메모리 영역

```ld
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 192K
}
```

- `ENTRY`: ELF 파일의 진입점 지정
- `ORIGIN`: 메모리 영역의 시작 주소
- `LENGTH`: 사용할 메모리 영역의 크기
- `r`, `w`, `x`: 읽기·쓰기·실행 속성

실제 리셋 시 실행할 함수 주소는 벡터 테이블에도 등록한다.

### 4.2 초기 스택 위치

```ld
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

__max_heap_size = 0x200;
__max_stack_size = 0x400;
```

`_estack`은 `0x20030000`으로 계산된다.
SRAM의 마지막 바이트 주소인 `0x2002FFFF` 바로 다음 주소이며,
스택은 사용하면서 낮은 주소 방향으로 자란다.

힙과 스택 크기 심볼은 각각 512바이트와 1024바이트로 정의했다.
현재 스크립트에서는 이 값으로 공간을 예약하거나 사용량을 제한하지 않는다.

### 4.3 섹션 배치

| 섹션 | 내용 | 배치 |
|---|---|---|
| .text | 벡터 테이블, 실행 코드, 읽기 전용 데이터 | Flash |
| .data | 초기값이 있는 전역·정적 변수 | 실행 위치 SRAM, 초기값 저장 위치 Flash |
| .bss | 0으로 초기화할 전역·정적 변수 | SRAM |

```ld
.data :
{
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    . = ALIGN(4);
    _edata = .;
} > SRAM AT> FLASH

_sidata = LOADADDR(.data);
```

- VMA: 실행 중 사용하는 주소
- LMA: 초기 내용을 저장해두는 주소
- `> SRAM`: .data의 VMA 지정
- `AT> FLASH`: .data의 LMA 지정
- `LOADADDR(.data)`: .data의 실제 LMA 반환

교재는 `_etext`를 .data 복사 원본 주소로 사용한다.
이번에는 실제 저장 주소를 명확하게 참조하도록 `_sidata`를 추가했다.

## 5. 스타트업 파일 작성

### 5.1 링커 심볼 참조

```c
extern uint32_t _estack;
extern uint32_t _etext;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
```

링커 스크립트에서 정의한 심볼을 C 코드에서 참조한다.
새 변수를 만드는 것이 아니라, 주소를 취해 링커가 지정한 값을 사용한다.

### 5.2 벡터 테이블

| 배열 위치 | 내용 |
|---|---|
| 0 | 초기 스택 포인터 |
| 1 | Reset_Handler 주소 |
| 2~15 | 기본 예외 처리 주소와 예약 항목 |
| 16~106 | 주변장치 IRQ 0~90의 처리 주소 |

총 107개 항목이며, 크기는 428바이트이다.

```c
const uint32_t vector_tbl[]
    __attribute__((section(".isr_vector_tbl"), used)) =
{
    /* 초기 스택 값과 기본 예외 항목 */
    /* ... */

    [16 ... 106] = (uint32_t)&Default_Handler
};
```

위 코드는 구조 설명용이며 실제 파일에는 기본 예외 항목을 모두 작성했다.

주변장치 인터럽트는 이번 실습에서 사용하지 않으므로
GCC 범위 초기화 문법으로 Default_Handler에 연결했다.
추후 사용하려면 해당 테이블 항목을 실제 처리 함수 주소로 변경해야 한다.

### 5.3 기본 예외 처리

```c
void NMI_Handler(void)
    __attribute__((weak, alias("Default_Handler")));
```

기본 예외 처리 함수에는 weak와 alias 속성을 사용했다.
별도 구현이 없으면 Default_Handler를 사용하고,
같은 이름의 일반 함수를 정의하면 해당 함수를 우선 사용한다.

함수 선언은 벡터 테이블에서 해당 이름을 사용하기 전에 배치했다.

### 5.4 Reset_Handler

Reset_Handler는 다음 순서로 동작한다.

1. Flash의 .data 초기값을 SRAM으로 복사
2. SRAM의 .bss 영역을 0으로 초기화
3. main() 호출
4. main()이 반환하면 무한 반복

```c
void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    dst = &_sbss;

    while (dst < &_ebss)
    {
        *dst++ = 0U;
    }

    main();

    while (1)
    {
    }
}
```

uint32_t 포인터는 한 번 증가할 때 4바이트 이동한다.
끝 주소와 비교하여 초기화 범위를 제한했다.

## 6. 도구 실행 환경 설정

Git Bash에서 현재 세션의 PATH에 툴체인과 업로드 도구의
bin 폴더를 추가했다.

```bash
export PATH="/c/ST/STM32CubeIDE_1.18.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344/tools/bin:$PATH"

export PATH="/c/ST/STM32CubeIDE_1.18.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.100.202412061334/tools/bin:$PATH"

arm-none-eabi-gcc --version
```

확인한 GCC 버전: `13.3.1`

PATH 설정은 현재 Git Bash 창에서만 적용된다。
설치 버전이나 위치가 다르면 경로를 수정해야 한다.

## 7. 수동 컴파일 및 링크

실습 폴더에서 명령을 실행했다.

```bash
cd /c/Git/baremetal-stm32f439/examples/1021_LinkerscriptAndStartup
```

### 7.1 C 소스 컴파일

```bash
arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -std=gnu11 -O0 -ffreestanding main.c -o main.o

arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -std=gnu11 -O0 -ffreestanding stm32f439_startup.c -o stm32f439_startup.o
```

- `-c`: 링크 없이 오브젝트 파일 생성
- `-mcpu=cortex-m4 -mthumb`: 대상 CPU와 명령어 지정
- `-std=gnu11`: C11과 GCC 확장 문법 사용
- `-O0`: 최적화 없이 컴파일
- `-ffreestanding`: 독립 실행 환경임을 지정

### 7.2 링크

```bash
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -nostdlib -T stm32_ls.ld main.o stm32f439_startup.o -Wl,-Map=firmware.map -o firmware.elf
```

- `-nostdlib`: 기본 스타트업 파일과 표준 라이브러리의 자동 연결 제외
- `-T`: 직접 작성한 링커 스크립트 지정
- `firmware.map`: 링크 결과의 메모리 배치 기록
- `firmware.elf`: 최종 실행 파일

### 링크 오류 해결

주석의 시작 문자 `/`가 빠져 링커 스크립트 문법 오류가 발생했다.

```text
stm32_ls.ld:18: ignoring invalid character `*' in script
stm32_ls.ld:18: syntax error
```

다음과 같이 수정한 후 링크에 성공했다.

```ld
/* Output sections */
```

## 8. 메모리 배치 확인 및 업로드

### 8.1 섹션과 벡터 테이블 확인

```bash
arm-none-eabi-objdump -h firmware.elf

arm-none-eabi-objdump -s -j .text firmware.elf
```

초기 LED 실습의 .text 크기는 620바이트였으며,
.data와 .bss는 모두 0바이트였다.

벡터 테이블에서는 다음 값을 확인했다.

| 항목 | 값 |
|---|---|
| 초기 스택 포인터 | 0x20030000 |
| Reset 벡터 값 | 0x08000213 |
| 기본 처리 함수 벡터 값 | 0x0800020D |

메모리는 리틀 엔디언으로 표시된다.
함수 벡터 값의 최하위 비트 1은 Thumb 상태를 나타낸다.

### 8.2 프로그램 기록 및 실행

보드를 ST-LINK USB로 연결하고 실행했다.

```bash
STM32_Programmer_CLI.exe -c port=SWD -w firmware.elf -v -rst
```

- `-c port=SWD`: SWD로 MCU 연결
- `-w`: 프로그램 기록
- `-v`: 기록 내용 검증
- `-rst`: MCU 리셋

교재의 OpenOCD 대신 STM32CubeProgrammer CLI를 사용했다.

실행 후 LD1이 깜빡이는 것을 확인했다.
지연 반복 횟수를 5,000,000회에서 500,000회로 줄인 후
깜빡임 간격이 짧아지는 것도 확인했다.

소프트웨어 반복문을 사용하는 지연이므로 정확한 시간은 보장하지 않는다.

## 9. 전역변수 초기화 추가 검증

main.c에 두 전역변수를 추가했다.

```c
volatile uint32_t data_test = 1234U;
volatile uint32_t bss_test;
```

초기값이 예상과 다르면 LED를 켠 상태로 정지하도록 했다.

```c
if ((data_test != 1234U) || (bss_test != 0U))
{
    GPIOB_ODR |= LED_PIN;

    while (1)
    {
    }
}
```

최종 섹션 배치:

| 섹션 | 크기 | VMA | LMA |
|---|---:|---|---|
| .text | 664 B | 0x08000000 | 0x08000000 |
| .data | 4 B | 0x20000000 | 0x08000298 |
| .bss | 4 B | 0x20000004 | 복사할 초기값 없음 |

.bss에는 LOAD 속성이 없으며, 표시되는 LMA는 초기값 복사에 사용하지 않는다.

업로드 후 LED가 계속 깜빡이는 것을 확인했다.
따라서 main() 진입 시 data_test는 1234, bss_test는 0으로 확인됐다.

이 검사는 초기값 확인 방식이며,
초기화 전 SRAM에 다른 값을 넣는 대조 실험은 수행하지 않았다.

## 10. 배운 점

- 링커 스크립트는 코드와 데이터의 메모리 배치를 지정한다.
- .data의 초기값 저장 위치와 실행 중 사용 위치는 다를 수 있다.
- 스타트업 코드는 main() 실행 전에 전역변수를 초기화한다.
- 벡터 테이블은 초기 스택 값과 예외·인터럽트 처리 주소를 제공한다.
- 링커 심볼을 사용해 C 코드에서 메모리 경계를 참조할 수 있다.
- 직접 작성한 스타트업 파일과 링커 스크립트로 LED 프로그램을 실행했다.

## 11. 저장소 관리

직접 작성한 소스 코드와 실습 문서를 관리한다.

- main.c
- stm32_ls.ld
- stm32f439_startup.c
- README.md

.o, .elf, .map 등의 빌드 결과물은 이번 커밋에서 제외했다.
README에 기록한 명령으로 다시 생성할 수 있다.