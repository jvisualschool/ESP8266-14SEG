# 📟 ESP8266 Dual 14-Segment LED + OLED Animation System

> **8-Digit 14-Segment LED (HT16K33 ×2) + 0.96" OLED (SSD1306) — 50 Creative Animations**

ESP8266 NodeMCU에서 두 개의 HT16K33 14-세그먼트 LED 모듈(총 8자리)과 SSD1306 OLED를 **3개의 독립 I2C 버스**로 동시 제어하는 애니메이션 프로젝트입니다.

---

## ✨ 주요 특징

- **트리플 독립 I2C 버스** — Hardware Wire(OLED) + Software Bit-bang ×2(14-Seg A/B)로 버스 잠김 원천 차단
- **8자리 14-세그먼트 디스플레이** — 동일 I2C 주소(0x70) 모듈 2개를 별도 버스로 분리 구동
- **50가지 창의적 애니메이션** — 텍스트 스크롤, 모스 부호, Knight Rider, DNA 나선 등
- **OLED 실시간 대시보드** — 현재 애니메이션 이름, 스텝, 프로그레스 바 동기 출력
- **180° 회전 모드** — `ROTATE_DISPLAYS` 플래그로 물리적 거꾸로 장착 지원
- **자동 감지** — 각 디스플레이 모듈 미연결 시 해당 모듈 출력만 스킵 (크래시 방지)

---

## 🛠 하드웨어 구성

### 부품 목록

| 부품 | 스펙 | 수량 |
|:---|:---|:---:|
| MCU | ESP8266 NodeMCU v2 (ESP-12E) | 1 |
| 14-Seg LED | 0.54" Quad Alphanumeric (HT16K33 드라이버) | **2** |
| OLED | 0.96" 128×64 I2C (SSD1306) | 1 |

### ⚡ 핀 배선도 (Pin Wiring)

```
ESP8266 NodeMCU v2 Pinout
┌──────────────────────────┐
│         (USB)            │
│                          │
│  D0 (GPIO16) ──── 14-Seg B SDA    │
│  D1 (GPIO5)  ──── 14-Seg A SCL    │
│  D2 (GPIO4)  ──── 14-Seg A SDA    │
│  D5 (GPIO14) ──── OLED SDA        │
│  D6 (GPIO12) ──── OLED SCL        │
│  D7 (GPIO13) ──── 14-Seg B SCL    │
│  3V3 ────────── VCC (전 모듈)     │
│  GND ────────── GND (전 모듈)     │
│                          │
└──────────────────────────┘
```

#### 상세 핀 매핑 테이블

| 버스 | 장치 | 신호 | NodeMCU 핀 | GPIO | I2C 주소 | 제어 방식 |
|:---:|:---|:---:|:---:|:---:|:---:|:---|
| **Bus A** | OLED (SSD1306) | SDA | **D5** | GPIO14 | `0x3C` | Hardware I2C (`Wire`) |
| | | SCL | **D6** | GPIO12 | | Hardware I2C (`Wire`) |
| **Bus B** | 14-Seg A (우측 4자리) | SCL | **D1** | GPIO5 | `0x70` | Software Bit-bang |
| | | SDA | **D2** | GPIO4 | | Software Bit-bang |
| **Bus C** | 14-Seg B (좌측 4자리) | SCL | **D7** | GPIO13 | `0x70` | Software Bit-bang |
| | | SDA | **D0** | GPIO16 | | Software Bit-bang |

#### 전원 연결

| 핀 | 연결 대상 | 비고 |
|:---|:---|:---|
| **3V3** | 모든 모듈 VCC | OLED, 14-Seg A, 14-Seg B 공통 |
| **3V3** | HT16K33 Vi2c | 로직 레벨 전원 (14-Seg 모듈) |
| **GND** | 모든 모듈 GND | 공통 접지 |

> [!WARNING]
> **HT16K33 모듈의 Vi2c 핀**을 반드시 3V3에 연결하세요. Vi2c가 미연결이면 I2C 통신이 실패합니다.

> [!IMPORTANT]
> 두 14-Seg 모듈은 **동일한 I2C 주소(0x70)**를 사용합니다. 하나의 버스에 함께 연결하면 주소 충돌이 발생하므로, 반드시 **물리적으로 분리된 버스(Bus B, Bus C)**에 각각 연결해야 합니다.

---

## 🏗 시스템 아키텍처

```
                    ESP8266 NodeMCU v2
                    ┌─────────────┐
                    │             │
     Bus A          │   Wire      │         D5(SDA), D6(SCL)
   (HW I2C) ───────│  (Hardware) │─────────── OLED SSD1306 @ 0x3C
                    │             │
     Bus B          │  Bit-bang   │         D1(SCL), D2(SDA)
   (SW I2C) ───────│  (Software) │─────────── 14-Seg A (Right) @ 0x70
                    │             │
     Bus C          │  Bit-bang   │         D7(SCL), D0(SDA)
   (SW I2C) ───────│  (Software) │─────────── 14-Seg B (Left)  @ 0x70
                    │             │
                    └─────────────┘
```

**왜 3개의 독립 버스인가?**

ESP8266에서 단일 I2C 버스로 여러 장치를 핀 스와핑하면 초기화 과정에서 **버스가 잠기거나** 화면이 꺼지는 문제가 발생합니다. 또한 두 HT16K33 모듈은 동일 주소(0x70)이므로 물리적 분리가 필수입니다.

### 디스플레이 자릿수 매핑 (좌→우 기준)

```
   ┌──── 14-Seg B (Bus C) ────┐ ┌──── 14-Seg A (Bus B) ────┐
   │ [0]  [1]  [2]  [3]       │ │ [4]  [5]  [6]  [7]       │
   └───────────────────────────┘ └───────────────────────────┘
     왼쪽                                              오른쪽
```

> `displayText8("ABCDEFGH")` 호출 시 → A(pos 0) ~ H(pos 7)이 좌→우 순서로 표시됩니다.

---

## 📂 프로젝트 구조

```
ESP8266-14Seg/
├── ESP8266-14Seg.ino   # 메인 펌웨어 (1032줄)
├── index.html          # 웹 기반 제어 UI
├── preview.html        # 브라우저 디스플레이 프리뷰 (CSS 시뮬레이터)
├── 14LED.JPG           # 하드웨어 참고 사진
├── 14Seg.png           # 14-세그먼트 레이아웃 참고 이미지
├── CLAUDE.md           # AI 어시스턴트 컨텍스트
└── README.md           # 이 문서
```

---

## 🚀 시작하기

### 1. 필수 라이브러리 설치

Arduino IDE → Sketch → Include Library → Manage Libraries:

| 라이브러리 | 용도 |
|:---|:---|
| **Adafruit GFX Library** | 그래픽 프리미티브 |
| **Adafruit SSD1306** | OLED 드라이버 |
| **Adafruit BusIO** | I2C 추상화 (종속성) |

> [!NOTE]
> HT16K33 14-세그먼트 제어는 코드 내에 포함된 **커스텀 Software I2C 클래스 (`HT16K33_SoftBus`)**를 사용하므로 추가 라이브러리가 필요 없습니다.

### 2. 업로드

1. ESP8266을 USB로 PC에 연결합니다.
2. `ESP8266-14Seg.ino` 파일을 Arduino IDE에서 엽니다.
3. 보드 설정: **`NodeMCU 1.0 (ESP-12E Module)`**
4. 업로드 속도: **115200**
5. 업로드 후 시리얼 모니터(115200 baud)를 열어 상태를 확인합니다.

### 3. 보드 매니저 설정 (최초 1회)

Arduino IDE에 ESP8266 보드가 없다면:
1. File → Preferences → Additional Board URLs에 추가:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
2. Tools → Board → Boards Manager → `esp8266` 검색 후 설치

---

## 🎬 50가지 애니메이션 목록

| # | 함수명 | 설명 |
|:---:|:---|:---|
| 1 | `anim_outerRingSweep` | 외곽 프레임 회전 |
| 2 | `anim_shootingStar` | 한 점이 좌→우로 이동 |
| 3 | `anim_binaryRain` | 0/1 이진수 비 |
| 4 | `anim_heartbeat` | 밝기 맥동 (심장 박동) |
| 5 | `anim_typewriter` | 글자 타이핑 효과 |
| 6 | `anim_wipeRight` | 좌→우 채우기/지우기 |
| 7 | `anim_verticalFill` | 위→아래 수직 채우기 |
| 8 | `anim_diagonalCross` | 대각선 X 패턴 |
| 9 | `anim_snake` | 뱀 이동 |
| 10 | `anim_explode` | 중앙 폭발 확산 |
| 11 | `anim_countdown` | 8→0 카운트다운 |
| 12 | `anim_scanline` | 수평 스캔라인 |
| 13 | `anim_starfield` | 무작위 별빛 |
| 14 | `anim_morse` | SOS 모스 부호 |
| 15 | `anim_raindrop` | 빗방울 낙하 |
| 16 | `anim_mirror` | 좌우 대칭 전개 |
| 17 | `anim_breathing` | 밝기 호흡 효과 |
| 18 | `anim_domino` | 순차 점등/소등 |
| 19 | `anim_wave` | 사인파 교대 |
| 20 | `anim_clockTick` | 시계 초침 이동 |
| 21 | `anim_pinwheel` | 바람개비 회전 |
| 22 | `anim_telegraph` | 전신 타전 점멸 |
| 23 | `anim_fire` | 불꽃 타오름 |
| 24 | `anim_matrixFall` | 매트릭스 낙하 |
| 25 | `anim_zenCircle` | 빈 원→꽉찬 원 |
| 26 | `anim_ticker` | 텍스트 스크롤 |
| 27 | `anim_pulseCenter` | 중앙→양측 맥동 |
| 28 | `anim_shuffle` | 세그먼트 랜덤 셔플 |
| 29 | `anim_crossfade` | 밝기 교차 페이드 |
| 30 | `anim_waterfall` | 우→좌 흐르는 물 |
| 31 | `anim_morseLove` | LOVE 모스 부호 |
| 32 | `anim_neonFlicker` | 형광등 깜빡임 |
| 33 | `anim_race` | 가속 질주 |
| 34 | `anim_buildUp` | 세그먼트 하나씩 켜기 |
| 35 | `anim_tearDown` | 세그먼트 하나씩 끄기 |
| 36 | `anim_bounce` | 양 끝 바운스 |
| 37 | `anim_loading` | 로딩 바 채우기 |
| 38 | `anim_musicNote` | 4/4 박자 리듬 |
| 39 | `anim_dnaHelix` | DNA 나선 패턴 |
| 40 | `anim_ripple` | 중앙 물결 파동 |
| 41 | `anim_clockFace` | 시계 외곽 회전 |
| 42 | `anim_zoomIn` | 바깥→중앙 줌인 |
| 43 | `anim_zoomOut` | 중앙→바깥 줌아웃 |
| 44 | `anim_ekg` | 심전도 파형 |
| 45 | `anim_pixelFade` | 독립 페이드 |
| 46 | `anim_strobe` | 스트로보 효과 |
| 47 | `anim_gravityFall` | 중력 낙하 쌓기 |
| 48 | `anim_knightRider` | Knight Rider 스캔 |
| 49 | `anim_aurora` | 오로라 패턴 |
| 50 | `anim_grandFinale` | 전체 점등 피날레 |

모든 애니메이션은 `loop()`에서 순차적으로 실행되며, 각 애니메이션 사이에 300ms 딜레이가 있습니다.

---

## 🧩 코드 구조 (개발자 참고)

### 핵심 클래스/함수

| 항목 | 설명 |
|:---|:---|
| `HT16K33_SoftBus` | 커스텀 Software I2C 클래스. `begin()` → `false`면 디바이스 미감지 |
| `displayText8(const char*)` | 8자리 문자열을 양쪽 모듈에 분배 출력 |
| `setAll8Raw(uint16_t[8])` | 8개 자릿수에 raw 세그먼트 데이터 직접 출력 |
| `setDigit(int pos, uint16_t val)` | 특정 위치(0~7)에 개별 세그먼트 설정 |
| `oledShow(name, step, total, detail)` | OLED 대시보드 갱신 (이름 + 프로그레스 바) |
| `rotateCharacter180(uint16_t v)` | 14-seg 비트를 180° 회전 변환 |
| `setBrightAll(uint8_t b)` | 양쪽 모듈 밝기 동시 설정 (0~15) |

### 14-Segment 비트 매핑

```
        ── A ──
       |╲ │ ╱|
       F  H I J  B
       |╱ │ ╲|
       G1 ── G2
       |╲ │ ╱|
       E  M L K  C
       |╱ │ ╲|
        ── D ──  (DP)
```

| 세그먼트 | 비트 | 값 | 설명 |
|:---:|:---:|:---:|:---|
| A | 0 | `0x0001` | 상단 수평 |
| B | 1 | `0x0002` | 우측 상단 수직 |
| C | 2 | `0x0004` | 우측 하단 수직 |
| D | 3 | `0x0008` | 하단 수평 |
| E | 4 | `0x0010` | 좌측 하단 수직 |
| F | 5 | `0x0020` | 좌측 상단 수직 |
| G1 | 6 | `0x0040` | 중앙 좌측 수평 |
| G2 | 7 | `0x0080` | 중앙 우측 수평 |
| H | 8 | `0x0100` | 대각선 좌상→중앙 |
| I | 9 | `0x0200` | 중앙 상단 수직 |
| J | 10 | `0x0400` | 대각선 우상→중앙 |
| K | 11 | `0x0800` | 대각선 우하→중앙 |
| L | 12 | `0x1000` | 중앙 하단 수직 |
| M | 13 | `0x2000` | 대각선 좌하→중앙 |
| DP | 14 | `0x4000` | 소수점 |

### 폰트 테이블

`alphafonttable[]`은 PROGMEM에 저장된 128개 ASCII 엔트리입니다. 각 값은 해당 문자를 표현하는 14비트 세그먼트 조합입니다.

### 컴파일 플래그

| 플래그 | 기본값 | 설명 |
|:---|:---:|:---|
| `ROTATE_DISPLAYS` | `true` | 180° 회전 모드. OLED: `setRotation(2)`, 14-Seg: 세그먼트 비트 교환 + 자릿수 역순 |

### 초기화 흐름 (`setup()`)

```
1. Serial.begin(115200)
2. Wire.begin(D5, D6)         → OLED용 Hardware I2C 초기화
3. oled.begin(0x3C)           → OLED 준비 → oReady = true
4. oled.setRotation(2)        → 180° 회전 (ROTATE_DISPLAYS=true일 때)
5. segA.begin()               → 14-Seg A I2C 감지 → aReady = true/false
6. segB.begin()               → 14-Seg B I2C 감지 → bReady = true/false
7. OLED에 "50 ANIMATIONS STARTING..." 표시
8. delay(1000) → loop() 시작
```

### 새 애니메이션 추가하기

1. `anim_yourName()` 함수를 정의합니다.
2. `setAll8Raw()` 또는 `displayText8()` 등 헬퍼를 사용합니다.
3. `oledShow("YOUR NAME", step, total, "Detail")` 로 OLED 대시보드를 갱신합니다.
4. `loop()`에 `anim_yourName(); delay(300);`을 추가합니다.

---

## ⚠️ 문제 해결 (Troubleshooting)

| 증상 | 원인 및 해결 |
|:---|:---|
| OLED 화면 안 나옴 | D5(SDA)/D6(SCL) 핀이 바뀌지 않았는지 확인 |
| 14-Seg에 아무것도 안 나옴 | D1/D2 (모듈A) 또는 D7/D0 (모듈B) 배선 확인. VCC + **Vi2c** 모두 3V3 연결 필수 |
| 한쪽 14-Seg만 동작 | 해당 모듈의 SCL/SDA 핀 배선 확인. `aReady`/`bReady` 시리얼 로그 체크 |
| 화면 깜빡거림 | USB 전원 부족. 더 안정적인 USB 포트 또는 외부 전원 사용 |
| 글자 순서가 뒤집힘 | `ROTATE_DISPLAYS`가 물리 장착 방향과 일치하는지 확인 |

---

## 📜 라이선스

MIT License
