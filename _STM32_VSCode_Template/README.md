# STM32 + VS Code 새 프로젝트 가이드

전역(모든 프로젝트 공통) 설정은 이미 VS Code 사용자 설정에 들어 있어서,
**CubeMX로 CMake 프로젝트를 생성해 VS Code로 열기만 하면** 빌드/다운로드/디버깅이 바로 됩니다.
이 폴더는 프로젝트별 선택 사항을 위한 템플릿입니다.

## 새 프로젝트 시작 절차

1. **CubeMX에서 프로젝트 생성**
   - Project Manager → Toolchain/IDE: **CMake** 선택 → GENERATE CODE
2. **VS Code에서 폴더 열기** (`파일 → 폴더 열기`)
3. 하단 상태바에서 CMake 프리셋 **Debug** 선택 (처음 한 번)
4. 끝. 아래 "일상 사용법"대로 사용

## 일상 사용법 (모든 프로젝트 공통)

| 작업 | 방법 |
|---|---|
| 빌드 | `Ctrl+Shift+P` → Tasks: Run Task → **STM32: Build (Debug)** (또는 상태바 Build 버튼) |
| 다운로드만 (ST-LINK) | Tasks: Run Task → **STM32: Flash (ST-LINK SWD)** — 자동 빌드 후 플래시 |
| 다운로드만 (USB DFU) | BOOT0=High로 리셋 후 → **STM32: Flash (USB DFU)** |
| 디버깅 | `F5` → "**STM32 전역: ST-LINK 디버그 (ST 공식)**" — 자동 빌드→플래시→main 정지 |
| 실행 중 보드 관찰 | 디버그 드롭다운에서 "**STM32 전역: 실행 중인 보드에 연결 (Attach)**" |

- 처음 한 번 "launch target" 선택을 물어보면 프로젝트 이름(실행 파일)을 선택하면 됩니다.
- 디버깅 중: `F10` 줄 단위 실행, `F11` 함수 진입, `F5` 계속, 줄 번호 왼쪽 클릭으로 브레이크포인트.

## 프로젝트별 선택 사항 (이 템플릿에서 복사)

### `.vscode/settings.json` — clangd 인텔리센스 (권장)
코드 자동완성/정의 이동 품질이 좋아집니다. 이 폴더의 `.vscode/settings.json`을 프로젝트에 복사하세요.

### `.vscode/extensions.json` — 확장 추천
다른 PC나 팀원과 공유할 때 필요한 확장을 자동 추천합니다. 그대로 복사하면 됩니다.

### 주변장치 레지스터 뷰 (SVD) — 프로젝트별 launch.json 필요
전역 디버그 구성은 MCU를 모르므로 SVD가 연결되지 않습니다.
디버그 중 GPIO/SPI/TIM 레지스터를 보고 싶으면 프로젝트에 `.vscode/launch.json`을 만들고
전역 구성을 복사한 뒤 한 줄만 추가하세요:

```jsonc
// ST 공식(stlinkgdbtarget) 구성이면:
"svdPath": "C:/Users/user/AppData/Local/stm32cube/packs/STMicroelectronics/<시리즈_dfp>/<버전>/SVD/<MCU>.svd"
// Cortex-Debug 구성이면:
"svdFile": "위와 동일 경로"
```

SVD 파일 위치: `C:\Users\user\AppData\Local\stm32cube\packs\STMicroelectronics\` 아래 시리즈별 DFP 폴더.
없는 시리즈는 VS Code의 STM32 확장 → Pack Manager에서 해당 시리즈 DFP를 설치하면 생깁니다.

## 툴 버전이 바뀌면 (번들 업데이트 시)

전역 설정에 아래 번들 경로가 버전 포함으로 하드코딩되어 있습니다.
ST 확장이 번들을 업데이트해서 빌드/디버그가 갑자기 안 되면 이 경로들의 버전을 갱신하세요.

- 수정할 파일:
  - `C:\Users\user\AppData\Roaming\Code\User\settings.json` (cmake.environment, terminal env, launch 구성)
  - `C:\Users\user\AppData\Roaming\Code\User\tasks.json`
- 번들 위치: `C:\Users\user\AppData\Local\stm32cube\bundles\` (폴더명 = 버전)
  - gnu-tools-for-stm32(gcc), gnu-gdb-for-stm32(gdb), ninja, cmake, stlink-gdbserver, programmer

## 문제 해결

- **"No ST-Link detected"**: USB 케이블/드라이버 확인. `STM32_Programmer_CLI -l`로 프로브 인식 확인 (VS Code 터미널에서 바로 실행 가능)
- **DFU 모드 인식 안 됨**: BOOT0=High 상태로 전원 리셋했는지, USB 케이블이 데이터 지원인지 확인
- **cmake/컴파일러 못 찾음**: VS Code 재시작(터미널 환경 변수는 새 터미널부터 적용) 후 `Ctrl+Shift+P` → "CMake: Delete Cache and Reconfigure"
