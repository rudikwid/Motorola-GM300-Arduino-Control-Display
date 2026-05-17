/*
 * Sketch: GM300_MAXDROID_V1.0
 * Fungsi: Konversi radio Motorola GM300 menjadi transceiver dengan VFO & memori
 * Author: MaxDroid (berdasarkan kode sumber asli)
 * Modifikasi: Perbaikan pin LCD, aktifkan EEPROM, perbaikan logika menu, dll.
 * Tanggal: 2026-05-17
 * 
 * Pin mapping:
 * - Encoder A  -> pin 4
 * - Encoder B  -> pin 2
 * - Encoder push -> pin 3
 * - LCD: RS=14(A0), E=15(A1), D4=16(A2), D5=17(A3), D6=18(A4), D7=19(A5)
 * - PLL DATA -> pin 5
 * - PLL CLK  -> pin 6
 * - PLL LE   -> pin 7
 * - PTT input (dari radio) -> pin 8
 * - RX indicator (dari radio) -> pin 10
 * - PL/COR output -> pin 9
 * - Power control (H/L) -> pin 11
 * 
 * Catatan: Pin A0-A5 digunakan untuk LCD, jangan di-set sebagai OUTPUT di setup().
 *          EEPROM menyimpan 64 memori (masing-masing 4 byte).
 */

#include <avr/pgmspace.h>
#include <Bounce2.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>   // Untuk penyimpanan memori

// ==================== DEFINE PIN & KONSTANTA ====================
// Encoder
#define ENC_A   4
#define ENC_B   2
#define ENC_BTN 3

// PLL
#define PLL_DATA  5
#define PLL_CLK   6
#define PLL_LE    7

// Radio interface
#define PTT_PIN    8
#define RX_PIN     10
#define PL_OUT_PIN 9
#define POWER_PIN  11

// LCD (RS, E, D4, D5, D6, D7) menggunakan pin analog sebagai digital
LiquidCrystal lcd(14, 15, 16, 17, 18, 19);

// Debouncer untuk encoder dan tombol
Bounce encDebouncer = Bounce();
Bounce btnDebouncer = Bounce();

// Ukuran memori (64 channel)
#define MAX_MEM  64

// ==================== VARIABEL GLOBAL ====================
unsigned long int FMem[MAX_MEM];      // Array penyimpanan memori (frekuensi + shift + pl)
unsigned long int Frq = 145000;       // Frekuensi VFO saat ini (dalam kHz * 10? Lihat: Frq = 145000 -> 145.000 MHz)
int EncDo = 1;                        // Arah putaran encoder (+1 / -1)
long Step = 5;                        // Step VFO (kHz)
int Menue = 0;                        // Menu aktif (0..8)
boolean Mode = 1;                     // 0=VFO, 1=Memory
int Mem = 0;                          // Index memori aktif (0..63)
boolean Curspress = 0;                // Flag tombol encoder ditekan
int Shift = 0;                        // Repeater shift: -1 (minus), 0 (simplex), 1 (plus)
boolean PlSq = 0;                     // 0=CTCSS, 1=COR
boolean Tx = 0;                       // Status PTT (0=RX, 1=TX)
boolean TxOld = 0;
boolean Rx = 0;
boolean Power = 0;                    // 0=10W, 1=30W
boolean ScanActive = 0;               // Flag scan aktif
int MemBeforeScan = 0;                // Memori sebelum scan dimulai
unsigned long int NewMemToSave = 0;   // Nilai memori baru untuk disimpan
boolean SaveVfo = 0;                  // Flag konfirmasi simpan VFO

// Untuk timing manual (diganti dengan delayMicroseconds nanti)
// Variabel time tidak digunakan, kita gunakan delayMicroseconds

// ==================== PROTOTYPE FUNGSI ====================
void set_pll(unsigned long int hz);
void drive_bus(byte enable);
void emit_byte(byte c);
void pulse_le(void);
void cursorpress(void);
void WriteFrq(unsigned long int Frq1);
void WriteMenue(void);
void addShiftToNewMem(void);
void scan(void);
void eeprom_write_mem(void);
void eeprom_read_mem(void);

// ==================== SETUP ====================
void setup() {
  // Inisialisasi LCD
  lcd.begin(8, 2);
  lcd.print("MaxDroid");
  delay(1000);
  lcd.clear();

  // Konfigurasi pin I/O (jangan sentuh pin A0-A5 karena sudah dipakai LCD)
  pinMode(PL_OUT_PIN, OUTPUT);
  pinMode(POWER_PIN, OUTPUT);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  pinMode(PTT_PIN, INPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(PLL_LE, OUTPUT);
  
  // Debouncer
  encDebouncer.attach(ENC_A);
  encDebouncer.interval(2);
  btnDebouncer.attach(ENC_BTN);
  btnDebouncer.interval(5);
  
  // Inisialisasi PLL
  drive_bus(0);
  digitalWrite(PLL_LE, LOW);
  digitalWrite(PLL_CLK, LOW);
  
  // Matikan timer1 interrupt (tidak dipakai)
  TIMSK1 = 0;
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = 888;   // tidak dipakai untuk interrupt, hanya setting
  
  // Baca memori dari EEPROM
  eeprom_read_mem();
  
  // Set frekuensi awal dari memori pertama
  Frq = FMem[0] / 10;
  PlSq = FMem[0] % 2;
  int tmp = FMem[0] % 10;
  if(tmp > 4) Shift = 1;
  else if(tmp > 2) Shift = -1;
  else Shift = 0;
  
  set_pll(Frq);
  WriteFrq(Frq);
}

// ==================== LOOP UTAMA ====================
void loop() {
  // Update debouncer
  encDebouncer.update();
  btnDebouncer.update();
  
  // Deteksi putaran encoder
  if(encDebouncer.fell()) {
    if(digitalRead(ENC_A) == digitalRead(ENC_B))
      EncDo = -1;
    else
      EncDo = 1;
  } else {
    EncDo = 0;
  }
  
  // Deteksi tekan encoder
  if(btnDebouncer.fell()) {
    cursorpress();
  }
  
  // Tampilkan menu jika tombol ditekan
  if(Curspress) {
    WriteMenue();
  }
  
  // Baca status RX
  boolean newRx = !digitalRead(RX_PIN); // Sesuaikan logika: asumsikan LOW saat RX aktif
  if(Rx != newRx) {
    Rx = newRx;
    lcd.setCursor(6, 1);
    lcd.print(Rx ? "Rx" : "  ");
  }
  
  // Baca status PTT
  boolean newTx = !digitalRead(PTT_PIN); // LOW saat ditekan (PTT aktif)
  if(newTx != TxOld) {
    Tx = newTx;
    set_pll(Frq);
    lcd.setCursor(6, 1);
    if(Tx) {
      lcd.print("Tx");
      Menue = 0;
    } else {
      lcd.print("  ");
    }
    WriteFrq(Frq + (Tx ? 0 : Shift * 600)); // tampilkan frekuensi dengan shift saat RX
    TxOld = Tx;
  }
  
  // Proses perubahan dari encoder
  if(EncDo != 0) {
    if(Menue == 0) {
      if(Mode == 0) {   // Mode VFO
        if(EncDo > 0) Frq += Step;
        else Frq -= Step;
        set_pll(Frq);
        WriteFrq(Frq);
        addShiftToNewMem();
      } else {          // Mode Memory
        if(EncDo > 0) {
          if(Mem < MAX_MEM - 1) Mem++;
          else Mem = 0;
        } else {
          if(Mem > 0) Mem--;
          else Mem = MAX_MEM - 1;
        }
        // Baca data dari memori
        Frq = FMem[Mem] / 10;
        PlSq = FMem[Mem] % 2;
        int tmp = FMem[Mem] % 10;
        if(tmp > 4) Shift = 1;
        else if(tmp > 2) Shift = -1;
        else Shift = 0;
        WriteFrq(Frq);
        set_pll(Frq);
      }
    }
    else if(Menue == 1) { // Toggle Mode
      Mode = !Mode;
      WriteMenue();
      if(Mode) {
        // Saat beralih ke mode memory, load memori aktif
        Frq = FMem[Mem] / 10;
        PlSq = FMem[Mem] % 2;
        int tmp = FMem[Mem] % 10;
        if(tmp > 4) Shift = 1;
        else if(tmp > 2) Shift = -1;
        else Shift = 0;
        set_pll(Frq);
      }
    }
    else if(Menue == 2) { // Shift repeater
      if(EncDo > 0) {
        if(Shift < 1) Shift++;
        else Shift = -1;
      } else {
        if(Shift > -1) Shift--;
        else Shift = 1;
      }
      WriteMenue();
      addShiftToNewMem();
    }
    else if(Menue == 3) { // PL/COR
      PlSq = !PlSq;
      digitalWrite(PL_OUT_PIN, PlSq);
      WriteMenue();
    }
    else if(Menue == 4) { // Step VFO
      if(EncDo > 0) {
        if(Step == 2) Step = 5;
        else if(Step == 5) Step = 10;
        else if(Step == 10) Step = 12;
        else if(Step == 12) Step = 20;
        else if(Step == 20) Step = 25;
        else if(Step == 25) Step = 50;
        else if(Step == 50) Step = 1000;
        else Step = 2;
      } else {
        if(Step == 1000) Step = 50;
        else if(Step == 50) Step = 25;
        else if(Step == 25) Step = 20;
        else if(Step == 20) Step = 12;
        else if(Step == 12) Step = 10;
        else if(Step == 10) Step = 5;
        else if(Step == 5) Step = 2;
        else Step = 1000;
      }
      WriteMenue();
    }
    else if(Menue == 5 && Mode == 0) { // Save VFO
      SaveVfo = !SaveVfo;
      WriteMenue();
    }
    else if(Menue == 6) { // Power
      Power = !Power;
      digitalWrite(POWER_PIN, Power);
      WriteMenue();
    }
    else if(Menue == 7 && Mode == 1) { // Scan
      ScanActive = 1;
      scan();
    }
  }
}

// ==================== FUNGSI PLL ====================
void set_pll(unsigned long int hz) {
  unsigned int n;
  byte a;
  unsigned long int target = hz * 1000;
  // Tambah offset mixer dan shift
  if(Tx == 0) target += 45100000;                // Receive: +45.1 MHz
  else target += 45100000 - (Shift * 600000);    // Transmit: +45.1 MHz - offset
  
  target /= 5000;   // Resolusi 5 kHz
  n = target / 64;
  a = target % 64;
  
  drive_bus(1);
  // Kirim N (10 bit) dan A (7 bit) -> total 24 bit, LSB 0
  emit_byte((n >> 8) & 0xFF);
  emit_byte(n & 0xFF);
  emit_byte(a << 1);
  pulse_le();
  
  // Kirim konfigurasi R divider (5kHz step)
  emit_byte(0x16);
  emit_byte(0x81);
  pulse_le();
  
  drive_bus(0);
}

void drive_bus(byte enable) {
  if(enable) {
    pinMode(PLL_DATA, OUTPUT);
    pinMode(PLL_CLK, OUTPUT);
    digitalWrite(PLL_DATA, HIGH);
  } else {
    pinMode(PLL_DATA, INPUT);
    pinMode(PLL_CLK, INPUT);
    digitalWrite(PLL_DATA, LOW);
  }
}

void emit_byte(byte c) {
  for(byte bit = 0; bit < 8; bit++) {
    digitalWrite(PLL_DATA, (c & 0x80) ? HIGH : LOW);
    delayMicroseconds(2);
    digitalWrite(PLL_CLK, HIGH);
    c <<= 1;
    delayMicroseconds(2);
    digitalWrite(PLL_CLK, LOW);
    delayMicroseconds(1);
  }
  digitalWrite(PLL_DATA, HIGH);
}

void pulse_le(void) {
  digitalWrite(PLL_LE, HIGH);
  delayMicroseconds(2);
  digitalWrite(PLL_LE, LOW);
}

// ==================== FUNGSI MENU & TAMPILAN ====================
void cursorpress() {
  Curspress = 1;
  if(Mode == 1 && Menue == 1) {
    Menue = 6;
    return;
  }
  if(Mode == 0 && Menue == 0 && NewMemToSave != 0) {
    SaveVfo = 0;
    Menue = 5;
    return;
  }
  if(Menue == 5 && Mode == 0) {
    Menue = 0;
    Mode = 1;
    if(SaveVfo) {
      FMem[Mem] = NewMemToSave;
      eeprom_write_mem();
    }
    Frq = FMem[Mem] / 10;
    SaveVfo = 0;
    NewMemToSave = 0;
    set_pll(Frq);
    return;
  }
  if(Menue == 8) {
    Menue = 0;
    return;
  }
  if(Menue < 8) {
    Menue++;
    if(Menue == 5) Menue = 6;
    if(Menue == 7 && Mode == 0) Menue = 0;
  } else {
    Menue = 0;
  }
}

void WriteFrq(unsigned long int Frq1) {
  lcd.setCursor(0, 0);
  unsigned long int mhz = Frq1 / 1000;
  unsigned int khz = Frq1 % 1000;
  lcd.print(mhz);
  lcd.print(".");
  if(khz < 100) lcd.print("0");
  if(khz < 10) lcd.print("0");
  lcd.print(khz);
  
  lcd.setCursor(7, 0);
  if(Shift < 0) lcd.print("-");
  else if(Shift > 0) lcd.print("+");
  else lcd.print(" ");
  
  lcd.setCursor(3, 1);
  lcd.print(PlSq ? " C" : " T");
  digitalWrite(PL_OUT_PIN, PlSq);
  
  lcd.setCursor(0, 1);
  if(!Mode) lcd.print("    ");
  else {
    lcd.print("M");
    lcd.print(Mem);
    if(Mem < 10) lcd.print(" ");
  }
  
  lcd.setCursor(5, 1);
  lcd.print(Power ? "H" : "L");
}

void WriteMenue() {
  if(Menue == 0) {
    lcd.clear();
    WriteFrq(Frq);
    Curspress = 0;
  }
  else if(Menue == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODE  ");
    lcd.setCursor(0, 1);
    lcd.print(Mode ? "Memory" : "VFO");
    Curspress = 0;
  }
  else if(Menue == 2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SHIFT  ");
    lcd.setCursor(0, 1);
    if(Shift == 0) lcd.print("Simplex");
    else if(Shift == -1) lcd.print("Rpt -");
    else lcd.print("Rpt +");
    Curspress = 0;
  }
  else if(Menue == 3) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Squelch  ");
    lcd.setCursor(0, 1);
    lcd.print(PlSq ? "Cor" : "Pl");
    Curspress = 0;
  }
  else if(Menue == 4) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("VFO Step  ");
    lcd.setCursor(0, 1);
    lcd.print(Step);
    lcd.print("Khz");
    Curspress = 0;
  }
  else if(Menue == 5 && Mode == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Save?  ");
    lcd.setCursor(0, 1);
    lcd.print(SaveVfo ? " Yes  " : " No  ");
    Curspress = 0;
  }
  else if(Menue == 6) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Power  ");
    lcd.setCursor(0, 1);
    lcd.print(Power ? " 30w  " : " 10w  ");
    Curspress = 0;
  }
  else if(Menue == 7 && Mode == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan  ");
    lcd.setCursor(0, 1);
    lcd.print("           ");
    Curspress = 0;
  }
}

void addShiftToNewMem() {
  if(Shift < 0) NewMemToSave = Frq * 10 + 3;
  else if(Shift > 0) NewMemToSave = Frq * 10 + 5;
  else NewMemToSave = Frq * 10 + 1;
}

// ==================== SCAN FUNCTION ====================
void scan() {
  int holdDelay = 300;
  MemBeforeScan = Mem;
  while(ScanActive) {
    if(Mem < MAX_MEM - 32) Mem++;
    else Mem = 0;
    
    Frq = FMem[Mem] / 10;
    PlSq = FMem[Mem] % 2;
    int tmp = FMem[Mem] % 10;
    if(tmp > 4) Shift = 1;
    else if(tmp > 2) Shift = -1;
    else Shift = 0;
    WriteFrq(Frq);
    set_pll(Frq);
    
    unsigned long startTime = millis();
    holdDelay = 300;
    while((millis() - startTime) < holdDelay) {
      encDebouncer.update();
      btnDebouncer.update();
      // Deteksi RX aktif
      if(!digitalRead(RX_PIN)) {  // RX aktif (LOW)
        holdDelay = 5000;
        startTime = millis();
      }
      // Deteksi PTT atau encoder putar untuk hentikan scan
      if(!digitalRead(PTT_PIN)) {
        ScanActive = 0;
        Menue = 0;
        return;
      }
      if(encDebouncer.fell()) {
        ScanActive = 0;
        Menue = 0;
        Mem = MemBeforeScan;
        Frq = FMem[Mem] / 10;
        PlSq = FMem[Mem] % 2;
        tmp = FMem[Mem] % 10;
        if(tmp > 4) Shift = 1;
        else if(tmp > 2) Shift = -1;
        else Shift = 0;
        WriteFrq(Frq);
        set_pll(Frq);
        return;
      }
    }
  }
}

// ==================== EEPROM HANDLER ====================
void eeprom_write_mem() {
  EEPROM.put(0, FMem);
}

void eeprom_read_mem() {
  EEPROM.get(0, FMem);
  // Jika EEPROM kosong (semua 0xFF), inisialisasi dengan nilai default
  boolean empty = true;
  for(int i = 0; i < MAX_MEM; i++) {
    if(FMem[i] != 0xFFFFFFFF) {
      empty = false;
      break;
    }
  }
  if(empty) {
    // Isi dengan nilai default (145.0000 MHz, simplex, PL off)
    for(int i = 0; i < MAX_MEM; i++) {
      FMem[i] = 1450001UL; // 145.0000 MHz, simplex, CTCSS
    }
    eeprom_write_mem();
  }
}
//================== eol, rudikwid, 17052026  =========================
