# Chapter 2 — 메모리 주소로 레지스터를 구성하고 LED 제어하기

STM32의 메모리 맵과 레지스터 주소를 확인하고,
HAL 라이브러리 없이 GPIO 레지스터에 직접 접근하여 LD1 LED를 켰다.

- 교재: Bare-Metal Embedded C Programming, Chapter 2
- 보드: NUCLEO-F439ZI
- MCU: STM32F439ZIT6 / Arm Cortex-M4
- 개발 환경: STM32CubeIDE
- 제어 대상: 보드 내장 LD1 LED, PB0

> 교재의 NUCLEO-F411 예제를 NUCLEO-F439ZI에 맞게 변경했다.
> 교재의 PA5 대신 PB0을 사용한다.

## 1. 실습 목표

- 메모리 맵과 주변장치 레지스터의 관계 이해
- 기준 주소와 오프셋으로 레지스터 주소 계산
- 포인터를 이용한 메모리 매핑 레지스터 접근
- RCC를 통한 GPIO 클럭 활성화
- 비트 연산으로 GPIO 출력 모드 설정 및 LED 제어

## 2. 참고 문서

| 문서 | 확인한 내용 |
|---|---|
| RM0090 Reference Manual | 메모리 맵, RCC·GPIO 레지스터 주소와 비트 기능 |
| UM1974 User Manual 및 보드 회로도 | 보드의 LD1 LED와 MCU 핀 연결 |

MCU의 레지스터 기능은 Reference Manual에서,
보드의 LED 연결은 User Manual과 회로도에서 확인했다.

## 3. 메모리 맵과 레지스터 주소

STM32는 주변장치의 제어 레지스터를 특정 메모리 주소에 배치한다.
CPU는 해당 주소를 읽거나 써서 주변장치를 제어한다.

레지스터 주소는 다음과 같이 계산할 수 있다.

레지스터 주소 = 주변장치 기준 주소 + 레지스터 오프셋

| 항목 | 주소 또는 오프셋 |
|---|---|
| Peripheral 기준 주소 | `0x40000000` |
| AHB1 영역 오프셋 | `0x00020000` |
| AHB1 영역 기준 주소 | `0x40020000` |
| GPIOB 오프셋 | `0x0400` |
| GPIOB 기준 주소 | `0x40020400` |
| RCC 오프셋 | `0x3800` |
| RCC 기준 주소 | `0x40023800` |

실습에서 사용한 레지스터는 다음과 같다.

| 레지스터 | 오프셋 | 최종 주소 | 역할 |
|---|---|---|---|
| RCC_AHB1ENR | `0x30` | `0x40023830` | AHB1 주변장치 클럭 활성화 |
| GPIOB_MODER | `0x00` | `0x40020400` | GPIOB 핀 모드 설정 |
| GPIOB_ODR | `0x14` | `0x40020414` | GPIOB 출력 데이터 설정 |

오프셋은 기준 주소로부터 얼마나 떨어져 있는지를 나타낸다.

## 4. 포인터로 레지스터 접근

다음과 같은 매크로로 특정 주소의 레지스터에 접근했다.

```c
#define RCC_AHB1ENR \
    (*(volatile unsigned int *)(RCC_BASE + AHB1ENR_OFFSET))
```

| 표현 | 의미 |
|---|---|
| `RCC_BASE + AHB1ENR_OFFSET` | 레지스터 주소 계산 |
| `(volatile unsigned int *)` | 해당 주소를 레지스터 접근용 포인터로 변환 |
| `*` | 포인터가 가리키는 주소의 값을 읽거나 쓰기 |
| `volatile` | 레지스터 접근을 컴파일러가 임의로 생략하거나 재사용하지 않도록 지정 |

이 프로젝트의 Arm GCC 환경에서 `unsigned int`는 32비트이며,
대상 레지스터도 32비트이다.

주소 상수의 `UL`은 `unsigned long`,
비트 연산에서 사용하는 `1U`의 `U`는 `unsigned int`를 의미한다.

## 5. GPIOB 클럭 활성화

GPIOB를 사용하기 전에 RCC에서 GPIOB 클럭을 활성화했다.

```c
#define GPIOBEN (1U << 1)

RCC_AHB1ENR |= GPIOBEN;
```

`RCC_AHB1ENR`의 bit 1이 GPIOB 클럭 활성화 비트이다.

주변장치를 사용하지 않을 때 클럭을 차단하여 소비 전력을 줄이는 방식을
클럭 게이팅이라고 한다.

## 6. PB0 출력 모드 설정

GPIO의 MODER 레지스터는 핀 하나당 2비트를 사용한다.
PB0의 모드는 bit 1과 bit 0으로 결정된다.

| MODER 비트 값 | 핀 모드 |
|---|---|
| `00` | 입력 |
| `01` | 일반 출력 |
| `10` | 대체 기능 |
| `11` | 아날로그 |

PB0을 출력으로 사용하기 위해 MODER0의 값을 `01`로 설정했다.

```c
GPIOB_MODE_R |=  (1U << 0);
GPIOB_MODE_R &= ~(1U << 1);
```

첫 번째 줄은 bit 0을 1로,
두 번째 줄은 bit 1을 0으로 만든다.

최종 결과:

```text
bit 1 = 0
bit 0 = 1
```

## 7. LED 켜기

GPIOB의 출력 데이터 레지스터에서 PB0에 해당하는 bit 0을 1로 설정했다.

```c
#define PIN0    (1U << 0)
#define LED_PIN PIN0

GPIOB_OD_R |= LED_PIN;
```

보드의 LD1 LED가 PB0에 연결되어 있으므로,
해당 출력을 HIGH로 설정하면 LED가 켜진다.

## 8. 전체 코드

```c
#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#define PERIPH_BASE         (0x40000000UL)
#define AHB1PERIPH_OFFSET   (0x00020000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + AHB1PERIPH_OFFSET)

#define GPIOB_OFFSET        (0x0400UL)
#define GPIOB_BASE          (AHB1PERIPH_BASE + GPIOB_OFFSET)

#define RCC_OFFSET          (0x3800UL)
#define RCC_BASE            (AHB1PERIPH_BASE + RCC_OFFSET)

#define AHB1ENR_OFFSET      (0x30UL)
#define RCC_AHB1ENR \
    (*(volatile unsigned int *)(RCC_BASE + AHB1ENR_OFFSET))

#define MODE_R_OFFSET       (0x00UL)
#define GPIOB_MODE_R \
    (*(volatile unsigned int *)(GPIOB_BASE + MODE_R_OFFSET))

#define OD_R_OFFSET         (0x14UL)
#define GPIOB_OD_R \
    (*(volatile unsigned int *)(GPIOB_BASE + OD_R_OFFSET))

#define GPIOBEN             (1U << 1)
#define PIN0                (1U << 0)
#define LED_PIN             PIN0

int main(void)
{
    // GPIOB 클럭 활성화
    RCC_AHB1ENR |= GPIOBEN;

    // PB0을 일반 출력 모드(01)로 설정
    GPIOB_MODE_R |=  (1U << 0);
    GPIOB_MODE_R &= ~(1U << 1);

    while (1)
    {
        // PB0 출력 HIGH: LD1 켜기
        GPIOB_OD_R |= LED_PIN;
    }
}
```

FPU 초기화 경고는 프로젝트의 부동소수점 설정과 관련된 경고이다.
이 실습의 GPIO 제어 코드는 부동소수점 연산을 사용하지 않는다.

## 9. 사용한 비트 연산

| 연산 | 기능 | 예시 |
|---|---|---|
| `1U << n` | n번 비트만 1인 마스크 생성 | `1U << 1` |
| `reg |= mask` | 선택한 비트를 1로 설정 | 클럭 활성화, LED 켜기 |
| `reg &= ~mask` | 선택한 비트를 0으로 설정 | GPIO 모드 비트 해제 |

다른 비트 값을 유지하면서 필요한 비트만 변경하는 방법을 익혔다.
단, 이 방식은 레지스터 읽기와 쓰기로 이루어지므로 원자적 연산은 아니다.

## 10. 실행 결과

STM32CubeIDE에서 빌드하고 보드에 업로드한 후,
LD1 LED가 계속 켜지는 것을 확인했다.

## 11. 배운 점

- 레지스터는 특정 메모리 주소에 배치된 하드웨어 제어 인터페이스이다.
- 기준 주소와 오프셋으로 원하는 레지스터의 주소를 계산할 수 있다.
- 포인터와 역참조를 이용해 레지스터를 읽고 쓸 수 있다.
- GPIO 사용 전 해당 포트의 클럭을 활성화해야 한다.
- MODER로 핀의 동작 모드를 설정하고 ODR로 출력 값을 제어할 수 있다.
- HAL 함수 없이도 레지스터 직접 접근으로 LED를 제어할 수 있다.