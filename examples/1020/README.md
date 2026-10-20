# Chapter 3 — GNU 툴체인을 이용한 수동 빌드 및 업로드

STM32CubeIDE가 자동으로 수행하던 컴파일·링크·업로드 과정을
PowerShell에서 직접 실행하고, 보드의 LD1 LED 깜빡임을 확인했다.

- 실습일: 2026-10-20
- 교재: Bare-Metal Embedded C Programming, Chapter 3
- 보드: NUCLEO-F439ZI
- MCU: STM32F439ZIT6 / Arm Cortex-M4
- LED: LD1, PB0
- 개발 환경: Windows, STM32CubeIDE 1.18.1, PowerShell
- 도구: GNU Tools for STM32 13.3.1, OpenOCD, GDB

> 실제 실습은 `examples/0930`에서 수행했으며, 완료 후 `examples/1020`으로 정리했다.
> 아래 경로와 출력 파일 이름은 실습 당시의 `0930`을 기준으로 한다.
> `1020`에서 재현할 때는 프로젝트 설정과 생성된 빌드 명령에 맞게 경로를 조정해야 한다.

## 1. 실습 목표

- C 소스와 시작 코드가 오브젝트 파일로 변환되는 과정 이해
- 오브젝트 파일을 링크하여 ELF 실행 파일 생성
- 실행 파일의 메모리 사용량 확인 및 BIN 변환
- OpenOCD와 GDB로 보드에 연결하여 프로그램 기록·실행

## 2. LED 코드 변경

2장의 LED 계속 켜기 코드를 주기적으로 반전하도록 변경했다.
GPIOB 클럭 활성화와 PB0 출력 모드 설정은 기존 코드를 유지했다.

```c
while (1)
{
    GPIOB_OD_R ^= LED_PIN;

    for (volatile int i = 0; i < 100000; i++)
    {
        // 눈으로 깜빡임을 확인하기 위한 지연
    }
}
```

- `LED_PIN`: `(1U << 0)`, PB0 선택
- `^=`: PB0 출력 상태 반전
- 지연 반복문: LED 상태 전환 사이에 대기 시간 추가
- `volatile`: 최적화로 지연 반복문이 제거되는 것을 방지

이 지연 방식은 정확한 시간 측정을 위한 타이머 방식은 아니다.

## 3. 도구 실행 환경 설정

PowerShell에서 GCC를 명령 이름으로 실행할 수 있도록 현재 세션의 PATH에
툴체인 `bin` 폴더를 추가했다.

```powershell
$toolchainBin = "C:\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin"

$env:Path = $toolchainBin + ";" + $env:Path

arm-none-eabi-gcc --version
```

확인한 GCC 버전: `13.3.1`

PATH 설정은 현재 PowerShell 창에서만 적용된다.

## 4. 수동 컴파일

먼저 CubeIDE에서 기존 프로젝트를 Clean 후 Build하여 명령을 확인했다.
이후 LED 코드를 수정·저장하고, 기존 `Debug` 폴더에서 컴파일 명령을 직접 실행했다.

```powershell
cd "C:\Git\baremetal-stm32f439\examples\0930\Debug"
```

### 4.1 시작 코드 어셈블

```powershell
arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"Startup/startup_stm32f439zitx.d" -MT"Startup/startup_stm32f439zitx.o" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "Startup/startup_stm32f439zitx.o" "../Startup/startup_stm32f439zitx.s"
```

시작 코드는 어셈블리로 작성되어 있으므로 전처리·어셈블 과정을 거쳐 `.o`가 된다.

### 4.2 C 소스 컴파일

```powershell
arm-none-eabi-gcc "../Src/main.c" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F4 -DSTM32F439ZITx -DNUCLEO_F439ZI -c -I../Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Src/main.d" -MT"Src/main.o" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "Src/main.o"

arm-none-eabi-gcc "../Src/syscalls.c" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F4 -DSTM32F439ZITx -DNUCLEO_F439ZI -c -I../Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Src/syscalls.d" -MT"Src/syscalls.o" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "Src/syscalls.o"

arm-none-eabi-gcc "../Src/sysmem.c" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F4 -DSTM32F439ZITx -DNUCLEO_F439ZI -c -I../Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Src/sysmem.d" -MT"Src/sysmem.o" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "Src/sysmem.o"
```

C 파일은 전처리 → 컴파일 → 어셈블 과정을 거쳐 `.o`가 된다.
`-c` 옵션을 사용하므로 링크는 수행하지 않는다.

각 명령 실행 직후 다음 값이 `0`인지 확인했다.

```powershell
$LASTEXITCODE
```

기존 코드의 FPU 초기화 관련 경고가 있었으나 컴파일은 성공했다.
현재 LED 코드는 정수 연산만 사용한다.

## 5. 링크

생성한 오브젝트 파일들을 링커 스크립트에 따라 배치하고 `0930.elf`를 생성했다.

```powershell
arm-none-eabi-gcc --% -o "0930.elf" @"objects.list" -mcpu=cortex-m4 -T"C:\Git\baremetal-stm32f439\examples\0930\STM32F439ZITX_FLASH.ld" --specs=nosys.specs -Wl,-Map="0930.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
```

- `objects.list`: 링크할 오브젝트 파일 목록
- `-T`: 사용할 링커 스크립트 지정
- `0930.map`: 링크 결과의 메모리 배치 정보
- `--%`: 뒤의 인수를 PowerShell이 해석하지 않고 프로그램에 전달

이번 실습에서는 CubeIDE가 생성한 `objects.list`, 폴더 구조,
기존 링커 스크립트를 활용했다.

## 6. 크기 확인 및 BIN 변환

```powershell
arm-none-eabi-size 0930.elf
```

출력 결과:

```text
   text    data     bss     dec     hex filename
    772       0    1568    2340     924 0930.elf
```

| 항목 | 변경 전 | 변경 후 |
|---|---:|---:|
| text | 748 B | 772 B |
| data | 0 B | 0 B |
| bss | 1568 B | 1568 B |
| 합계 | 2316 B | 2340 B |

LED 반전과 지연 코드를 추가한 후 `text` 크기가 24바이트 증가했다.
위 합계는 섹션 크기의 합이며, ELF 파일 자체의 크기를 의미하지 않는다.

다음 명령으로 ELF를 순수 바이너리 형식으로 변환했다.

```powershell
arm-none-eabi-objcopy -O binary 0930.elf 0930.bin
```

변환 종료 코드 `0`을 확인했다. 실제 보드 기록에는 ELF 파일을 사용했다.

## 7. OpenOCD 실행

보드를 ST-LINK USB로 연결하고 CubeIDE의 디버그 세션을 종료한 뒤 실행했다.

```powershell
$openocdExe = "C:\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.100.202501161620\tools\bin\openocd.exe"

$openocdScripts = "C:\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.debug.openocd_2.3.100.202501240831\resources\openocd\st_scripts"

& $openocdExe -s $openocdScripts -f interface/stlink-dap.cfg -c "transport select dapdirect_swd" -f target/stm32f4x.cfg
```

정상 연결 로그:

```text
Info : STLINK V2J46M31 (API v2)
Info : [stm32f4x.cpu] Cortex-M4 r0p1 processor detected
Info : [stm32f4x.cpu] Examination succeed
Info : Listening on port 3333 for gdb connections
```

OpenOCD는 ST-LINK를 통해 MCU를 제어하고, GDB의 연결을 기다린다.
이 PowerShell 창은 실행 상태로 유지했다.

### 연결 오류 해결

처음에는 `interface/stlink.cfg`와 `hla_swd`를 사용했으나,
다음 경고가 반복되면서 설정 처리 오류가 발생했다.

```text
DEPRECATED! use 'swj_newdap' not 'hla newtap'
```

설치된 환경에서 `interface/stlink-dap.cfg`와 `dapdirect_swd` 조합으로
변경한 후 정상 연결됐다.

## 8. GDB 연결 및 프로그램 기록

두 번째 PowerShell 창을 열고 GDB를 실행했다.

```powershell
cd "C:\Git\baremetal-stm32f439\examples\0930\Debug"

& "C:\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin\arm-none-eabi-gdb.exe" 0930.elf
```

이후 아래 명령은 PowerShell이 아닌 `(gdb)` 프롬프트에서 입력했다.

```text
target remote localhost:3333
monitor reset init
monitor flash write_image erase 0930.elf
monitor reset init
monitor resume
```

Flash 기록 로그:

```text
auto erase enabled
wrote 16384 bytes from file 0930.elf in 0.787512s
```

실행 후 보드의 LD1 LED가 깜빡이는 것을 확인했다.

세션을 종료할 때는 GDB에서 다음 명령을 사용한다.

```text
detach
quit
```

이후 OpenOCD 창에서 `Ctrl+C`로 종료한다.

## 9. 배운 점

- GCC는 옵션에 따라 전처리·컴파일·어셈블·링크 도구를 실행한다.
- 시작 코드와 C 소스는 각각 오브젝트 파일로 변환된 후 링크된다.
- Makefile은 빌드 규칙을 담고, make는 그 규칙에 따라 필요한 명령을 실행한다.
- 이번에는 make 대신 개별 빌드 명령을 직접 실행했다.
- GDB는 디버깅 명령을 전달하고, OpenOCD는 ST-LINK를 통해 MCU를 제어한다.
- CubeIDE가 자동화하던 빌드와 업로드 과정을 터미널에서 직접 재현했다.

## 10. 저장소 관리

소스 코드, 프로젝트 설정, 실습 문서를 관리한다.
`Debug/`의 오브젝트 파일과 ELF·BIN 등 빌드 결과물은 Git 추적 대상에서 제외한다.