/*
 * ESP8266 + Dual 14-Segment LED (8 Digits Total)
 * 50 Creative Animations
 *
 * - OLED: D5(SDA), D6(SCL) [Rotated 180]
 * - 14-Seg A: D1(SCL), D2(SDA) [Physically Right]
 * - 14-Seg B: D7(SCL), D0(SDA) [Physically Left]
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ROTATE_DISPLAYS true

#define OLED_SDA        D5
#define OLED_SCL        D6
#define OLED_ADDR       0x3C

#define SEGA_SCL        D1
#define SEGA_SDA        D2
#define SEGB_SCL        D7
#define SEGB_SDA        D0
#define HT16K33_ADDR    0x70

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ============================================================
// 14-Segment 폰트 데이터 (수정됨: '1'=0x0006, '7'=0x0007)
// ============================================================
const uint16_t alphafonttable[] PROGMEM = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0006, 0x0220, 0x12CE, 0x12ED, 0x0000, 0x235D, 0x0200, 0x2400, 0x0900, 0x3FC0, 0x12C0, 0x0800, 0x00C0, 0x4000, 0x0C00,
    0x0C3F, 0x0006, 0x00DB, 0x008F, 0x00E6, 0x00ED, 0x00FD, 0x0007, 0x00FF, 0x00EF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x00F7, 0x128F, 0x0039, 0x120F, 0x0079, 0x0071, 0x00BD, 0x00F6, 0x1200, 0x001E, 0x2470, 0x0038, 0x0536, 0x2136, 0x003F,
    0x00F3, 0x083F, 0x20F3, 0x00ED, 0x1201, 0x003E, 0x0C30, 0x2836, 0x2D00, 0x1500, 0x0C09, 0x0039, 0x2100, 0x000F, 0x0000, 0x0000,
    0x0000, 0x00F7, 0x128F, 0x0039, 0x120F, 0x0079, 0x0071, 0x00BD, 0x00F6, 0x1200, 0x001E, 0x2470, 0x0038, 0x0536, 0x2136, 0x003F,
    0x00F3, 0x083F, 0x20F3, 0x00ED, 0x1201, 0x003E, 0x0C30, 0x2836, 0x2D00, 0x1500, 0x0C09, 0x0039, 0x2100, 0x000F, 0x0000, 0x0000
};

// ============================================================
// 14-Segment 비트 상수 (Adafruit 표준 매핑)
// ============================================================
#define SEG_A   0x0001  // top
#define SEG_B   0x0002  // upper right
#define SEG_C   0x0004  // lower right
#define SEG_D   0x0008  // bottom
#define SEG_E   0x0010  // lower left
#define SEG_F   0x0020  // upper left
#define SEG_G1  0x0040  // middle left
#define SEG_G2  0x0080  // middle right
#define SEG_H   0x0100  // diagonal TL
#define SEG_I   0x0200  // vertical top center
#define SEG_J   0x0400  // diagonal TR
#define SEG_K   0x0800  // diagonal BR
#define SEG_L   0x1000  // vertical bottom center
#define SEG_M   0x2000  // diagonal BL
#define SEG_DP  0x4000  // decimal point

// ============================================================
// 180도 회전 변환
// ============================================================
uint16_t rotateCharacter180(uint16_t v) {
  uint16_t r = 0;
  if (v & SEG_A)  r |= SEG_D;  if (v & SEG_D)  r |= SEG_A;
  if (v & SEG_B)  r |= SEG_E;  if (v & SEG_E)  r |= SEG_B;
  if (v & SEG_C)  r |= SEG_F;  if (v & SEG_F)  r |= SEG_C;
  if (v & SEG_G1) r |= SEG_G2; if (v & SEG_G2) r |= SEG_G1;
  if (v & SEG_H)  r |= SEG_K;  if (v & SEG_K)  r |= SEG_H;
  if (v & SEG_I)  r |= SEG_L;  if (v & SEG_L)  r |= SEG_I;
  if (v & SEG_J)  r |= SEG_M;  if (v & SEG_M)  r |= SEG_J;
  if (v & SEG_DP) r |= SEG_DP;
  return r;
}

// ============================================================
// HT16K33 소프트웨어 I2C 클래스
// ============================================================
class HT16K33_SoftBus {
private:
  uint8_t _addr, _sda, _scl;
  uint16_t displayBuffer[4];
  void i2c_start() {
    digitalWrite(_sda, HIGH); digitalWrite(_scl, HIGH); delayMicroseconds(5);
    digitalWrite(_sda, LOW); delayMicroseconds(5);
    digitalWrite(_scl, LOW); delayMicroseconds(5);
  }
  void i2c_stop() {
    digitalWrite(_sda, LOW); digitalWrite(_scl, HIGH); delayMicroseconds(5);
    digitalWrite(_sda, HIGH); delayMicroseconds(5);
  }
  bool i2c_write(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
      digitalWrite(_sda, (data & 0x80) ? HIGH : LOW);
      data <<= 1; delayMicroseconds(5);
      digitalWrite(_scl, HIGH); delayMicroseconds(5);
      digitalWrite(_scl, LOW); delayMicroseconds(5);
    }
    pinMode(_sda, INPUT); digitalWrite(_scl, HIGH); delayMicroseconds(5);
    bool ack = (digitalRead(_sda) == LOW);
    digitalWrite(_scl, LOW); pinMode(_sda, OUTPUT);
    return ack;
  }
  void sendCmd(uint8_t cmd) { i2c_start(); i2c_write(_addr << 1); i2c_write(cmd); i2c_stop(); }

public:
  HT16K33_SoftBus(uint8_t addr, uint8_t sda, uint8_t scl) : _addr(addr), _sda(sda), _scl(scl) { clear(); }
  bool begin() {
    pinMode(_scl, OUTPUT); pinMode(_sda, OUTPUT);
    i2c_start(); bool f = i2c_write(_addr << 1); i2c_stop();
    if (!f) return false;
    sendCmd(0x21); sendCmd(0x81); sendCmd(0xEF);
    return true;
  }
  void setBrightness(uint8_t b) { sendCmd(0xE0 | (b & 0x0F)); }
  void clear() { for (int i = 0; i < 4; i++) displayBuffer[i] = 0; }
  void setRaw(uint8_t n, uint16_t d) { if (n < 4) displayBuffer[n] = d; }
  uint16_t getRaw(uint8_t n) { return (n < 4) ? displayBuffer[n] : 0; }
  void update() {
    i2c_start(); i2c_write(_addr << 1); i2c_write(0x00);
    for (int i = 0; i < 4; i++) { i2c_write(displayBuffer[i] & 0xFF); i2c_write(displayBuffer[i] >> 8); }
    i2c_stop();
  }
};

HT16K33_SoftBus segA(HT16K33_ADDR, SEGA_SDA, SEGA_SCL);
HT16K33_SoftBus segB(HT16K33_ADDR, SEGB_SDA, SEGB_SCL);
bool oReady = false, aReady = false, bReady = false;

// ============================================================
// 헬퍼 함수들
// ============================================================

// 8자리 raw 배열을 양쪽 모듈에 출력 (인덱스 0=왼쪽 끝, 7=오른쪽 끝)
void setAll8Raw(uint16_t d[8]) {
  if (ROTATE_DISPLAYS) {
    for (int i = 0; i < 4; i++) {
      segB.setRaw(3 - i, rotateCharacter180(d[i]));
      segA.setRaw(3 - i, rotateCharacter180(d[i + 4]));
    }
  } else {
    for (int i = 0; i < 4; i++) {
      segB.setRaw(i, d[i]);
      segA.setRaw(i, d[i + 4]);
    }
  }
  if (aReady) segA.update();
  if (bReady) segB.update();
}

void clearAll() {
  uint16_t d[8] = {0};
  setAll8Raw(d);
}

void setDigit(int pos, uint16_t val) {
  // pos 0=left, 7=right
  if (pos < 4) {
    if (ROTATE_DISPLAYS) segB.setRaw(3 - pos, rotateCharacter180(val));
    else segB.setRaw(pos, val);
    if (bReady) segB.update();
  } else {
    int p = pos - 4;
    if (ROTATE_DISPLAYS) segA.setRaw(3 - p, rotateCharacter180(val));
    else segA.setRaw(p, val);
    if (aReady) segA.update();
  }
}

void displayText8(const char* txt) {
  char fullTxt[9] = "        ";
  int len = strlen(txt);
  for (int i = 0; i < 8 && i < len; i++) fullTxt[i] = txt[i];
  uint16_t d[8];
  for (int i = 0; i < 8; i++) d[i] = pgm_read_word(&alphafonttable[(uint8_t)fullTxt[i]]);
  setAll8Raw(d);
}

void oledShow(const char* name, int step, int total, const char* detail = "") {
  if (!oReady) return;
  int pct = (total > 0) ? map(step, 0, total, 0, 100) : 0;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(1);
  oled.setCursor(0, 0); oled.print("ANIM: ");
  oled.setTextSize(1); oled.println(name);
  oled.drawFastHLine(0, 10, 128, 1);
  oled.setCursor(0, 14);
  if (strlen(detail) > 0) oled.println(detail);
  // progress bar
  oled.setCursor(0, 48); oled.print("Step "); oled.print(step); oled.print("/"); oled.println(total);
  oled.drawRect(0, 56, 128, 7, 1);
  oled.fillRect(1, 57, map(pct, 0, 100, 0, 126), 5, 1);
  oled.display();
}

void setBrightAll(uint8_t b) {
  if (aReady) segA.setBrightness(b);
  if (bReady) segB.setBrightness(b);
}

// ============================================================
// 50가지 애니메이션
// ============================================================

// 1. OUTER RING SWEEP - 외곽 프레임 회전
void anim_outerRingSweep() {
  const uint16_t outer[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
  int total = 6 * 3;
  for (int rep = 0; rep < 3; rep++) {
    for (int s = 0; s < 6; s++) {
      oledShow("OUTER RING", rep * 6 + s, total, "Frame sweep");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = outer[s];
      setAll8Raw(d);
      delay(80);
    }
  }
}

// 2. SHOOTING STAR - 한 점이 왼→오로 흐름
void anim_shootingStar() {
  int total = 24;
  for (int rep = 0; rep < 3; rep++) {
    for (int pos = 0; pos < 8; pos++) {
      oledShow("SHOOT STAR", rep * 8 + pos, total, "L -> R");
      uint16_t d[8] = {0};
      d[pos] = SEG_G1 | SEG_G2;
      setAll8Raw(d);
      delay(80);
    }
  }
}

// 3. BINARY RAIN - 이진수 비처럼 떨어짐
void anim_binaryRain() {
  for (int step = 0; step < 32; step++) {
    oledShow("BINARY RAIN", step, 32, "01 cascade");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
      d[i] = (random(2)) ? (SEG_B | SEG_C) : (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F);
    }
    setAll8Raw(d);
    delay(120);
  }
}

// 4. HEARTBEAT - 박동처럼 밝기 맥동
void anim_heartbeat() {
  uint8_t bright[] = {0, 2, 5, 10, 15, 10, 5, 2, 0, 0, 0};
  uint16_t d[8];
  for (int i = 0; i < 8; i++) d[i] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G1 | SEG_G2;
  for (int rep = 0; rep < 3; rep++) {
    for (int b = 0; b < 11; b++) {
      oledShow("HEARTBEAT", rep * 11 + b, 33, "Pulse");
      setBrightAll(bright[b]);
      setAll8Raw(d);
      delay(60);
    }
  }
  setBrightAll(15);
}

// 5. TYPEWRITER - 글자가 하나씩 타이핑
void anim_typewriter() {
  const char* msg = "HELLO   ";
  for (int i = 0; i < 8; i++) {
    oledShow("TYPEWRITER", i, 8, msg);
    char buf[9] = "        ";
    for (int j = 0; j <= i; j++) buf[j] = msg[j];
    displayText8(buf);
    delay(200);
  }
  delay(500);
}

// 6. WIPE RIGHT - 왼쪽에서 오른쪽으로 밀어지는 선
void anim_wipeRight() {
  for (int pos = 0; pos < 8; pos++) {
    oledShow("WIPE RIGHT", pos, 8, "Fill L->R");
    uint16_t d[8] = {0};
    for (int i = 0; i <= pos; i++) d[i] = SEG_A | SEG_D;
    setAll8Raw(d);
    delay(100);
  }
  delay(300);
  for (int pos = 7; pos >= 0; pos--) {
    uint16_t d[8] = {0};
    for (int i = 0; i <= pos; i++) d[i] = SEG_A | SEG_D;
    setAll8Raw(d);
    delay(100);
  }
}

// 7. VERTICAL FILL - 위→아래로 채워지는 선분
void anim_verticalFill() {
  uint16_t stages[] = {SEG_A, SEG_A|SEG_G1|SEG_G2, SEG_A|SEG_G1|SEG_G2|SEG_D};
  for (int s = 0; s < 3; s++) {
    oledShow("VERT FILL", s, 3, "Top->Bottom");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = stages[s];
    setAll8Raw(d);
    delay(300);
  }
  delay(300);
}

// 8. DIAGONAL CROSS - 대각선 X 패턴
void anim_diagonalCross() {
  uint16_t frames[] = {SEG_H | SEG_K, SEG_J | SEG_M, SEG_H | SEG_J | SEG_K | SEG_M, 0};
  for (int rep = 0; rep < 4; rep++) {
    for (int f = 0; f < 4; f++) {
      oledShow("DIAG CROSS", rep * 4 + f, 16, "X pattern");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = frames[f];
      setAll8Raw(d);
      delay(150);
    }
  }
}

// 9. SNAKE - 뱀처럼 구불구불 이동
void anim_snake() {
  uint16_t seg_seq[] = {SEG_A, SEG_J, SEG_B, SEG_G2, SEG_G1, SEG_K, SEG_C, SEG_D, SEG_E, SEG_M, SEG_F, SEG_H, SEG_I, SEG_L};
  int len = 14;
  for (int step = 0; step < len * 2; step++) {
    oledShow("SNAKE", step, len * 2, "Slither");
    uint16_t d[8] = {0};
    for (int i = 0; i < 8; i++) d[i] = seg_seq[(step + i) % len];
    setAll8Raw(d);
    delay(80);
  }
}

// 10. EXPLODE - 가운데서 바깥으로 폭발
void anim_explode() {
  uint16_t frames[] = {
    SEG_G1 | SEG_G2,
    SEG_G1 | SEG_G2 | SEG_I | SEG_L,
    SEG_A | SEG_D | SEG_G1 | SEG_G2 | SEG_I | SEG_L | SEG_H | SEG_J | SEG_K | SEG_M,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G1 | SEG_G2 | SEG_H | SEG_I | SEG_J | SEG_K | SEG_L | SEG_M,
    0
  };
  for (int rep = 0; rep < 3; rep++) {
    for (int f = 0; f < 5; f++) {
      oledShow("EXPLODE", rep * 5 + f, 15, "Burst!");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = frames[f];
      setAll8Raw(d);
      delay(120);
    }
  }
}

// 11. COUNTDOWN - 8→0 카운트다운
void anim_countdown() {
  for (int n = 8; n >= 0; n--) {
    oledShow("COUNTDOWN", 8 - n, 9, "T-minus...");
    char buf[9];
    memset(buf, ' ', 8); buf[8] = 0;
    buf[7] = '0' + n;
    displayText8(buf);
    delay(400);
  }
}

// 12. SCANLINE - 수평선이 위→아래 스캔
void anim_scanline() {
  uint16_t lines[] = {SEG_A, SEG_G1 | SEG_G2, SEG_D};
  for (int rep = 0; rep < 4; rep++) {
    for (int l = 0; l < 3; l++) {
      oledShow("SCANLINE", rep * 3 + l, 12, "H-scan");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = lines[l];
      setAll8Raw(d);
      delay(150);
    }
  }
}

// 13. STARFIELD - 무작위 별빛
void anim_starfield() {
  uint16_t stars[] = {SEG_DP, SEG_I, SEG_H | SEG_J, SEG_H | SEG_J | SEG_K | SEG_M};
  for (int step = 0; step < 30; step++) {
    oledShow("STARFIELD", step, 30, "Twinkle");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = stars[random(4)];
    setAll8Raw(d);
    delay(100);
  }
}

// 14. MORSE - SOS 모스 코드 (... --- ...)
void anim_morse() {
  uint16_t dot = SEG_DP;
  uint16_t dash = SEG_G1 | SEG_G2;
  uint16_t pattern[] = {1,1,1,0,0,0,2,2,2,0,0,0,1,1,1}; // 1=dot, 2=dash, 0=space
  for (int i = 0; i < 15; i++) {
    oledShow("MORSE SOS", i, 15, "... --- ...");
    uint16_t d[8] = {0};
    uint16_t sym = (pattern[i] == 1) ? dot : (pattern[i] == 2) ? dash : 0;
    for (int j = 0; j < 8; j++) d[j] = sym;
    setAll8Raw(d);
    delay(pattern[i] == 0 ? 200 : 150);
  }
}

// 15. RAIN DROP - 한 컬럼에 빗방울 낙하
void anim_raindrop() {
  for (int rep = 0; rep < 3; rep++) {
    int col = random(8);
    for (int s = 0; s < 3; s++) {
      oledShow("RAIN DROP", rep * 3 + s, 9, "Drip");
      uint16_t d[8] = {0};
      uint16_t stages[] = {SEG_A, SEG_G1 | SEG_G2, SEG_D};
      d[col] = stages[s];
      setAll8Raw(d);
      delay(120);
    }
  }
}

// 16. MIRROR - 좌우 대칭으로 펼쳐지는 패턴
void anim_mirror() {
  for (int step = 0; step < 4; step++) {
    oledShow("MIRROR", step, 4, "Symmetry");
    uint16_t d[8] = {0};
    for (int i = 0; i <= step; i++) {
      d[3 - i] = SEG_A | SEG_D;
      d[4 + i] = SEG_A | SEG_D;
    }
    setAll8Raw(d);
    delay(200);
  }
  delay(300);
  for (int step = 3; step >= 0; step--) {
    uint16_t d[8] = {0};
    for (int i = 0; i <= step; i++) {
      d[3 - i] = SEG_A | SEG_D;
      d[4 + i] = SEG_A | SEG_D;
    }
    setAll8Raw(d);
    delay(200);
  }
}

// 17. BREATHING - 전체 밝기 천천히 호흡
void anim_breathing() {
  uint16_t d[8];
  for (int i = 0; i < 8; i++) d[i] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
  setAll8Raw(d);
  for (int rep = 0; rep < 2; rep++) {
    for (int b = 0; b <= 15; b++) {
      oledShow("BREATHING", rep * 32 + b, 64, "Inhale...");
      setBrightAll(b); delay(60);
    }
    for (int b = 15; b >= 0; b--) {
      oledShow("BREATHING", rep * 32 + (31 - b), 64, "Exhale...");
      setBrightAll(b); delay(60);
    }
  }
  setBrightAll(15);
}

// 18. DOMINO - 하나씩 순서대로 켜짐
void anim_domino() {
  for (int pos = 0; pos < 8; pos++) {
    oledShow("DOMINO", pos, 8, "One by one");
    setDigit(pos, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F);
    delay(150);
  }
  delay(300);
  for (int pos = 7; pos >= 0; pos--) {
    setDigit(pos, 0);
    delay(150);
  }
}

// 19. WAVE - 사인파처럼 위아래 교대
void anim_wave() {
  uint16_t hi[] = {SEG_A | SEG_B | SEG_F, SEG_A};
  uint16_t lo[] = {SEG_D | SEG_C | SEG_E, SEG_D};
  for (int step = 0; step < 24; step++) {
    oledShow("WAVE", step, 24, "Sine wave");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
      d[i] = ((i + step) % 2 == 0) ? hi[0] : lo[0];
    }
    setAll8Raw(d);
    delay(100);
  }
}

// 20. CLOCK TICK - 시계 초침처럼 한 칸씩
void anim_clockTick() {
  for (int pos = 0; pos < 8; pos++) {
    oledShow("CLOCK TICK", pos, 8, "Tick tock");
    uint16_t d[8] = {0};
    d[pos] = SEG_I | SEG_L;
    setAll8Raw(d);
    delay(200);
  }
}

// 21. PINWHEEL - 중앙 수직선이 회전하듯
void anim_pinwheel() {
  uint16_t frames[] = {SEG_A, SEG_J, SEG_B|SEG_C, SEG_K, SEG_D, SEG_M, SEG_E|SEG_F, SEG_H};
  for (int rep = 0; rep < 3; rep++) {
    for (int f = 0; f < 8; f++) {
      oledShow("PINWHEEL", rep * 8 + f, 24, "Spin");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = frames[(f + i) % 8];
      setAll8Raw(d);
      delay(80);
    }
  }
}

// 22. TELEGRAPH - 전신 타전처럼 점멸
void anim_telegraph() {
  uint16_t on = SEG_G1 | SEG_G2 | SEG_I | SEG_L;
  for (int step = 0; step < 20; step++) {
    oledShow("TELEGRAPH", step, 20, "Tap tap tap");
    uint16_t d[8] = {0};
    if (random(3) != 0) for (int i = 0; i < 8; i++) d[i] = on;
    setAll8Raw(d);
    delay(random(50, 200));
  }
}

// 23. FIRE - 불꽃처럼 아래서 위로 타오름
void anim_fire() {
  uint16_t sparks[] = {SEG_D, SEG_D|SEG_E|SEG_C, SEG_D|SEG_G1|SEG_G2|SEG_E|SEG_C,
                       SEG_D|SEG_G1|SEG_G2|SEG_A|SEG_E|SEG_C, SEG_G1|SEG_G2|SEG_A, SEG_A};
  for (int step = 0; step < 30; step++) {
    oledShow("FIRE", step, 30, "Crackle!");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = sparks[random(6)];
    setAll8Raw(d);
    delay(80);
  }
}

// 24. MATRIX FALL - 세로로 흘러내리는 매트릭스
void anim_matrixFall() {
  uint16_t col[8] = {0};
  for (int step = 0; step < 32; step++) {
    oledShow("MATRIX", step, 32, "Follow rabbit");
    col[random(8)] = (random(2)) ? SEG_I | SEG_L : SEG_B | SEG_C;
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = col[i];
    setAll8Raw(d);
    col[random(8)] = 0;
    delay(100);
  }
}

// 25. ZEN CIRCLE - 빈 원에서 꽉찬 원으로
void anim_zenCircle() {
  uint16_t circle[] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G1 | SEG_G2,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G1 | SEG_G2 | SEG_I | SEG_L,
    0xFFFF & ~SEG_DP
  };
  for (int rep = 0; rep < 3; rep++) {
    for (int f = 0; f < 4; f++) {
      oledShow("ZEN CIRCLE", rep * 4 + f, 12, "Fill up");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = circle[f];
      setAll8Raw(d);
      delay(200);
    }
    uint16_t d[8] = {0};
    setAll8Raw(d);
    delay(200);
  }
}

// 26. TICKER - 텍스트 스크롤
void anim_ticker() {
  const char* msg = "  HELLO WORLD   ESP8266  14SEG  ";
  int msgLen = strlen(msg);
  for (int start = 0; start < msgLen; start++) {
    oledShow("TICKER", start, msgLen, "Scroll");
    char buf[9] = "        ";
    for (int i = 0; i < 8 && (start + i) < msgLen; i++) buf[i] = msg[start + i];
    displayText8(buf);
    delay(150);
  }
}

// 27. PULSE CENTER - 가운데서 양쪽으로 맥동
void anim_pulseCenter() {
  for (int rep = 0; rep < 4; rep++) {
    for (int r = 0; r < 4; r++) {
      oledShow("PULSE CTR", rep * 4 + r, 16, "Heart");
      uint16_t d[8] = {0};
      for (int i = 0; i <= r; i++) {
        d[3 - i] = SEG_G1 | SEG_G2;
        d[4 + i] = SEG_G1 | SEG_G2;
      }
      setAll8Raw(d);
      delay(80);
    }
    uint16_t d[8] = {0}; setAll8Raw(d); delay(80);
  }
}

// 28. SHUFFLE - 세그먼트 무작위 셔플
void anim_shuffle() {
  uint16_t allSegs[] = {SEG_A,SEG_B,SEG_C,SEG_D,SEG_E,SEG_F,SEG_G1,SEG_G2,SEG_H,SEG_I,SEG_J,SEG_K,SEG_L,SEG_M};
  for (int step = 0; step < 28; step++) {
    oledShow("SHUFFLE", step, 28, "Random bits");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = allSegs[random(14)];
    setAll8Raw(d);
    delay(100);
  }
}

// 29. CROSSFADE - 두 패턴이 교차 페이드 (밝기 교대)
void anim_crossfade() {
  uint16_t patA[8], patB[8];
  for (int i = 0; i < 8; i++) { patA[i] = SEG_A | SEG_D; patB[i] = SEG_G1 | SEG_G2; }
  for (int b = 0; b < 16; b++) {
    oledShow("CROSSFADE", b, 32, "A->B");
    setBrightAll(15 - b);
    setAll8Raw(patA);
    delay(40);
  }
  for (int b = 0; b < 16; b++) {
    oledShow("CROSSFADE", 16 + b, 32, "B->A");
    setBrightAll(b);
    setAll8Raw(patB);
    delay(40);
  }
  setBrightAll(15);
}

// 30. WATERFALL - 오른쪽에서 왼쪽으로 흐르는 물
void anim_waterfall() {
  uint16_t buf[8] = {0};
  for (int step = 0; step < 24; step++) {
    oledShow("WATERFALL", step, 24, "Flow");
    for (int i = 7; i > 0; i--) buf[i] = buf[i - 1];
    buf[0] = (random(3) == 0) ? (SEG_A | SEG_D | SEG_G1 | SEG_G2) : 0;
    setAll8Raw(buf);
    delay(120);
  }
}

// 31. MORSE LOVE - LOVE를 모스로
void anim_morseLove() {
  // L=.-.. O=--- V=...- E=.
  const char* morse = ".-.. --- ...- .";
  int len = strlen(morse);
  for (int i = 0; i < len; i++) {
    oledShow("MORSE LOVE", i, len, "L-O-V-E");
    uint16_t d[8] = {0};
    uint16_t sym = (morse[i] == '.') ? SEG_DP : (morse[i] == '-') ? (SEG_G1|SEG_G2) : 0;
    for (int j = 0; j < 8; j++) d[j] = sym;
    setAll8Raw(d);
    delay(morse[i] == ' ' ? 250 : 200);
  }
}

// 32. NEON FLICKER - 형광등 깜빡임
void anim_neonFlicker() {
  uint16_t d[8];
  for (int i = 0; i < 8; i++) d[i] = SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G1|SEG_G2;
  for (int step = 0; step < 20; step++) {
    oledShow("NEON FLICK", step, 20, "Zzt...");
    setAll8Raw(d); delay(random(50, 300));
    uint16_t off[8] = {0}; setAll8Raw(off); delay(random(20, 80));
  }
}

// 33. RACE - 하나의 점이 가속하며 질주
void anim_race() {
  int delays[] = {200,160,130,100,80,60,50,40,35,30,30,30,40,60,100};
  for (int step = 0; step < 15; step++) {
    oledShow("RACE", step, 15, "Full speed!");
    for (int pos = 0; pos < 8; pos++) {
      uint16_t d[8] = {0};
      d[pos] = SEG_G1 | SEG_G2 | SEG_A | SEG_D;
      setAll8Raw(d);
      delay(delays[step]);
    }
  }
}

// 34. BUILD UP - 세그먼트를 하나씩 켜나가며 완성
void anim_buildUp() {
  uint16_t order[] = {SEG_A,SEG_B,SEG_C,SEG_D,SEG_E,SEG_F,SEG_G1,SEG_G2,SEG_H,SEG_I,SEG_J,SEG_K,SEG_L,SEG_M};
  uint16_t current = 0;
  for (int s = 0; s < 14; s++) {
    oledShow("BUILD UP", s, 14, "Add segment");
    current |= order[s];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = current;
    setAll8Raw(d);
    delay(150);
  }
  delay(500);
}

// 35. TEAR DOWN - 반대로 하나씩 끔
void anim_tearDown() {
  uint16_t order[] = {SEG_M,SEG_L,SEG_K,SEG_J,SEG_I,SEG_H,SEG_G2,SEG_G1,SEG_F,SEG_E,SEG_D,SEG_C,SEG_B,SEG_A};
  uint16_t current = 0x3FFF;
  for (int s = 0; s < 14; s++) {
    oledShow("TEAR DOWN", s, 14, "Remove seg");
    current &= ~order[s];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = current;
    setAll8Raw(d);
    delay(150);
  }
}

// 36. BOUNCE - 공이 양 끝에서 튕김
void anim_bounce() {
  int pos = 0, dir = 1;
  for (int step = 0; step < 24; step++) {
    oledShow("BOUNCE", step, 24, "Boing!");
    uint16_t d[8] = {0};
    d[pos] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
    setAll8Raw(d);
    pos += dir;
    if (pos == 7 || pos == 0) dir = -dir;
    delay(100);
  }
}

// 37. LOADING - 로딩 바 점진적 채우기
void anim_loading() {
  for (int pct = 0; pct <= 8; pct++) {
    oledShow("LOADING", pct, 8, "Please wait");
    uint16_t d[8] = {0};
    for (int i = 0; i < pct; i++) d[i] = SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G1|SEG_G2;
    setAll8Raw(d);
    delay(300);
  }
  delay(300);
}

// 38. MUSIC NOTE - 음표처럼 리드미컬하게 점멸
void anim_musicNote() {
  // 4/4 박자 패턴
  uint16_t beat[8][8];
  for (int t = 0; t < 8; t++) {
    for (int i = 0; i < 8; i++) beat[t][i] = 0;
    if (t % 2 == 0) { beat[t][0]=SEG_A|SEG_F|SEG_B; beat[t][7]=SEG_A|SEG_F|SEG_B; }
    if (t % 4 == 1) { beat[t][2]=SEG_G1|SEG_G2; beat[t][5]=SEG_G1|SEG_G2; }
    if (t % 8 == 3) { for(int j=0;j<8;j++) beat[t][j]=SEG_D|SEG_E|SEG_C; }
  }
  for (int rep = 0; rep < 4; rep++) {
    for (int t = 0; t < 8; t++) {
      oledShow("MUSIC", rep * 8 + t, 32, "4/4 beat");
      setAll8Raw(beat[t]);
      delay(120);
    }
  }
}

// 39. DNA HELIX - 나선 패턴
void anim_dnaHelix() {
  for (int step = 0; step < 32; step++) {
    oledShow("DNA HELIX", step, 32, "Life code");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
      int phase = (i + step) % 4;
      if (phase == 0)      d[i] = SEG_A | SEG_B;
      else if (phase == 1) d[i] = SEG_G1 | SEG_G2;
      else if (phase == 2) d[i] = SEG_D | SEG_E;
      else                 d[i] = SEG_G1 | SEG_G2;
    }
    setAll8Raw(d);
    delay(90);
  }
}

// 40. RIPPLE - 가운데서 물결처럼 퍼짐
void anim_ripple() {
  for (int rep = 0; rep < 4; rep++) {
    for (int r = 0; r < 4; r++) {
      oledShow("RIPPLE", rep * 4 + r, 16, "Drop");
      uint16_t d[8] = {0};
      if (r == 0) { d[3] = SEG_DP; d[4] = SEG_DP; }
      else {
        d[3 - r + 1] = SEG_G1 | SEG_G2;
        d[4 + r - 1] = SEG_G1 | SEG_G2;
      }
      setAll8Raw(d);
      delay(120);
    }
    uint16_t d[8] = {0}; setAll8Raw(d); delay(60);
  }
}

// 41. CLOCK FACE - 외곽을 시계처럼 도는 세그먼트
void anim_clockFace() {
  uint16_t clock_segs[] = {SEG_A, SEG_J, SEG_B, SEG_K, SEG_C, SEG_D, SEG_M, SEG_E, SEG_H, SEG_F};
  for (int rep = 0; rep < 3; rep++) {
    for (int s = 0; s < 10; s++) {
      oledShow("CLOCK FACE", rep * 10 + s, 30, "Tick");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = clock_segs[s];
      setAll8Raw(d);
      delay(100);
    }
  }
}

// 42. ZOOM IN - 바깥에서 가운데로 줌인
void anim_zoomIn() {
  for (int step = 0; step < 5; step++) {
    oledShow("ZOOM IN", step, 5, "Focus");
    uint16_t d[8] = {0};
    for (int i = step; i < 8 - step; i++) {
      d[i] = SEG_A | SEG_D | SEG_B | SEG_E | SEG_C | SEG_F;
    }
    setAll8Raw(d);
    delay(200);
  }
  delay(300);
}

// 43. ZOOM OUT - 가운데서 바깥으로
void anim_zoomOut() {
  for (int step = 4; step >= 0; step--) {
    oledShow("ZOOM OUT", 4 - step, 5, "Wide");
    uint16_t d[8] = {0};
    for (int i = step; i < 8 - step; i++) {
      d[i] = SEG_A | SEG_D | SEG_B | SEG_E | SEG_C | SEG_F;
    }
    setAll8Raw(d);
    delay(200);
  }
  delay(300);
}

// 44. EKG - 심전도 파형
void anim_ekg() {
  uint16_t wave[] = {0, 0, SEG_G1|SEG_G2, SEG_A|SEG_I, SEG_D|SEG_L, SEG_G1|SEG_G2, 0, 0};
  uint16_t buf[8] = {0};
  for (int step = 0; step < 32; step++) {
    oledShow("EKG", step, 32, "Pulse");
    for (int i = 7; i > 0; i--) buf[i] = buf[i - 1];
    buf[0] = wave[step % 8];
    setAll8Raw(buf);
    delay(100);
  }
}

// 45. PIXEL FADE - 각 자리 독립 페이드
void anim_pixelFade() {
  for (int step = 0; step < 16; step++) {
    oledShow("PIXEL FADE", step, 16, "Indie glow");
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
      uint8_t phase = (step + i * 2) % 16;
      d[i] = (phase < 8) ? (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F) : 0;
    }
    setAll8Raw(d);
    delay(120);
  }
}

// 46. STROBE - 스트로보 효과
void anim_strobe() {
  uint16_t d[8];
  for (int i = 0; i < 8; i++) d[i] = SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G1|SEG_G2|SEG_H|SEG_I|SEG_J|SEG_K|SEG_L|SEG_M;
  uint16_t off[8] = {0};
  for (int step = 0; step < 20; step++) {
    oledShow("STROBE", step, 20, "Flash!");
    setAll8Raw(d); delay(30);
    setAll8Raw(off); delay(30);
  }
}

// 47. GRAVITY FALL - 중력처럼 아래로 쌓임
void anim_gravityFall() {
  uint16_t stack[8] = {0};
  for (int ball = 0; ball < 5; ball++) {
    for (int pos = 0; pos < 8; pos++) {
      oledShow("GRAVITY", ball * 8 + pos, 40, "Stack up");
      uint16_t d[8];
      for (int i = 0; i < 8; i++) d[i] = stack[i];
      d[pos] = SEG_A;
      setAll8Raw(d);
      delay(80);
    }
    stack[7 - ball] = SEG_D;
  }
}

// 48. KNIGHT RIDER - 기사의 눈처럼 양쪽 왕복
void anim_knightRider() {
  int pos = 0, dir = 1;
  for (int step = 0; step < 32; step++) {
    oledShow("KNIGHT", step, 32, "KITT scan");
    uint16_t d[8] = {0};
    d[pos] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G1 | SEG_G2;
    if (pos > 0) d[pos - 1] = SEG_G1 | SEG_G2;
    if (pos < 7) d[pos + 1] = SEG_G1 | SEG_G2;
    setAll8Raw(d);
    pos += dir;
    if (pos == 7 || pos == 0) dir = -dir;
    delay(80);
  }
}

// 49. AURORA - 천천히 변하는 극광 패턴
void anim_aurora() {
  uint16_t patterns[][8] = {
    {SEG_A,SEG_A|SEG_B,SEG_B,SEG_B|SEG_G2,SEG_G2|SEG_G1,SEG_G1,SEG_G1|SEG_F,SEG_F},
    {SEG_B,SEG_B|SEG_G2,SEG_G2,SEG_G2|SEG_C,SEG_C,SEG_C|SEG_D,SEG_D,SEG_D|SEG_E},
    {SEG_G1|SEG_G2,SEG_A|SEG_D,SEG_B|SEG_E,SEG_C|SEG_F,SEG_H|SEG_K,SEG_J|SEG_M,SEG_I|SEG_L,SEG_G1|SEG_G2},
  };
  for (int rep = 0; rep < 3; rep++) {
    for (int f = 0; f < 3; f++) {
      oledShow("AURORA", rep * 3 + f, 9, "Northern light");
      setAll8Raw(patterns[f]);
      delay(400);
    }
  }
}

// 50. GRAND FINALE - 모든 세그먼트 총출동 클라이맥스
void anim_grandFinale() {
  oledShow("FINALE", 0, 1, "All in!");
  for (int rep = 0; rep < 5; rep++) {
    uint16_t d[8];
    for (int i = 0; i < 8; i++) d[i] = (rep % 2 == 0) ? 0x3FFF : 0;
    setAll8Raw(d);
    delay(80);
  }
  // 가운데서 폭발
  for (int r = 0; r < 4; r++) {
    uint16_t d[8] = {0};
    d[3 - r] = 0x3FFF; d[4 + r] = 0x3FFF;
    setAll8Raw(d);
    delay(60);
  }
  // 전체 켜고 천천히 페이드
  uint16_t d[8];
  for (int i = 0; i < 8; i++) d[i] = 0x3FFF;
  setAll8Raw(d);
  for (int b = 15; b >= 0; b--) {
    setBrightAll(b); delay(80);
  }
  setBrightAll(15);
  clearAll();
}

// ============================================================
// Setup & Loop
// ============================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oReady = true;
    if (ROTATE_DISPLAYS) oled.setRotation(2);
  }
  if (segA.begin()) aReady = true;
  if (segB.begin()) bReady = true;

  if (oReady) {
    oled.clearDisplay(); oled.setTextSize(1); oled.setTextColor(1);
    oled.setCursor(20, 20); oled.println("50 ANIMATIONS");
    oled.setCursor(30, 35); oled.println("STARTING...");
    oled.display();
  }
  delay(1000);
}

void loop() {
  anim_outerRingSweep();   delay(300);
  anim_shootingStar();     delay(300);
  anim_binaryRain();       delay(300);
  anim_heartbeat();        delay(300);
  anim_typewriter();       delay(300);
  anim_wipeRight();        delay(300);
  anim_verticalFill();     delay(300);
  anim_diagonalCross();    delay(300);
  anim_snake();            delay(300);
  anim_explode();          delay(300);
  anim_countdown();        delay(300);
  anim_scanline();         delay(300);
  anim_starfield();        delay(300);
  anim_morse();            delay(300);
  anim_raindrop();         delay(300);
  anim_mirror();           delay(300);
  anim_breathing();        delay(300);
  anim_domino();           delay(300);
  anim_wave();             delay(300);
  anim_clockTick();        delay(300);
  anim_pinwheel();         delay(300);
  anim_telegraph();        delay(300);
  anim_fire();             delay(300);
  anim_matrixFall();       delay(300);
  anim_zenCircle();        delay(300);
  anim_ticker();           delay(300);
  anim_pulseCenter();      delay(300);
  anim_shuffle();          delay(300);
  anim_crossfade();        delay(300);
  anim_waterfall();        delay(300);
  anim_morseLove();        delay(300);
  anim_neonFlicker();      delay(300);
  anim_race();             delay(300);
  anim_buildUp();          delay(300);
  anim_tearDown();         delay(300);
  anim_bounce();           delay(300);
  anim_loading();          delay(300);
  anim_musicNote();        delay(300);
  anim_dnaHelix();         delay(300);
  anim_ripple();           delay(300);
  anim_clockFace();        delay(300);
  anim_zoomIn();           delay(300);
  anim_zoomOut();          delay(300);
  anim_ekg();              delay(300);
  anim_pixelFade();        delay(300);
  anim_strobe();           delay(300);
  anim_gravityFall();      delay(300);
  anim_knightRider();      delay(300);
  anim_aurora();           delay(300);
  anim_grandFinale();      delay(1000);
}
