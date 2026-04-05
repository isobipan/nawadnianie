# PRD: System Automatycznego Nawadniania Doniczek

**Wersja:** 1.1
**Data:** 2026-04-05
**Autor:** Irek
**Status:** ✅ Gotowy do implementacji
**Priorytet:** 🟡 Medium
**Breaking Changes:** ❌ Nie dotyczy (nowy projekt)

---

## 📋 Executive Summary

System automatycznego nawadniania dwóch doniczek oparty na ESP32 z czujnikami wilgotności gleby, zaworami solenoidowymi (przepływ grawitacyjny) i wyświetlaczem LCD I2C. Urządzenie zasilane akumulatorem 18650 z głębokim snem ESP32 dla maksymalnej oszczędności energii.

**Kluczowe funkcje:**
- 💧 **Automatyczne podlewanie** - dwie niezależne doniczki z zaworami solenoidowymi
- 🌱 **Pomiar wilgotności** - czujniki pojemnościowe co 4 godziny
- 📺 **Wyświetlacz LCD** - status systemu i poziom wilgotności (faza prototypu)
- 🔋 **Oszczędność energii** - deep sleep ESP32 (~10μA podczas snu)
- 📊 **Monitoring baterii** - pomiar napięcia przez dzielnik na ADC

**Fazy rozwoju:**
- 🔧 **v1.0 (MVP)** - ESP32 + czujniki + pompki + relay + OLED + bateria
- 🌐 **v2.0** - WiFi + web dashboard + zdalne sterowanie + logi

---

## 🎯 Business Requirements

### BR-1: Automatyczne nawadnianie dwóch doniczek
System musi niezależnie monitorować i nawadniać dwie doniczki na podstawie pomiaru wilgotności gleby.

**Kryteria akceptacji:**
- Dwa niezależne kanały (czujnik + zawór) dla każdej doniczki
- Niezależna logika podlewania dla każdej doniczki
- Brak wzajemnych interferencji między kanałami

### BR-2: Oszczędność energii (praca na baterii)
System musi działać na akumulatorze przez minimum 30 dni bez ładowania.

**Kryteria akceptacji:**
- ESP32 w trybie deep sleep między pomiarami
- Aktywacja czujników i zaworów tylko gdy potrzebne
- Monitoring poziomu baterii z ostrzeżeniem na LCD

### BR-3: Praca autonomiczna
System musi działać bez nadzoru użytkownika przez długi czas.

**Kryteria akceptacji:**
- Automatyczne podlewanie według progu wilgotności
- Brak konieczności interwencji przy normalnej pracy
- Wyświetlanie statusu na LCD po każdym cyklu pomiarowym

### BR-4: Konfigurowalność progów
Próg wilgotności inicjujący podlewanie musi być łatwo modyfikowalny.

**Kryteria akceptacji:**
- Próg zdefiniowany jako stała w kodzie
- Możliwość ustawienia różnych progów dla każdej doniczki
- Dokumentacja kalibracji w komentarzach

---

## 👤 User Stories

### US-1: Użytkownik - Automatyczne podlewanie

**Jako** właściciel doniczek
**Chcę** żeby system automatycznie podlewał rośliny gdy gleba jest sucha
**Aby** nie martwić się o regularne podlewanie

**Scenariusz:**
```cpp
// ESP32 budzi się z deep sleep po 4 godzinach
// Odczytuje wilgotność doniczka 1: 22% (poniżej progu 35%)
// Odczytuje wilgotność doniczka 2: 58% (powyżej progu 35%)
// Otwiera zawór 1 na 20 sekund
// Zawór 2 pozostaje zamknięty
// LCD wyświetla: "D1: 22%->OK  D2: 58% OK"
// ESP32 wraca do deep sleep na 4 godziny
```

**Kryteria akceptacji:**
- Zawór otwiera się tylko gdy wilgotność < próg
- Czas otwarcia zaworu konfigurowalny (domyślnie 20s)
- LCD potwierdza akcję po podlaniu

### US-2: Użytkownik - Monitoring statusu

**Jako** właściciel doniczek
**Chcę** móc sprawdzić aktualny status systemu na LCD
**Aby** wiedzieć czy rośliny są odpowiednio nawodnione

**Scenariusz:**
```cpp
// Po każdym cyklu pomiarowym LCD pokazuje:
// Linia 1: "D1:45% D2:62%"
// Linia 2: "Bat:87% OK"
// Lub gdy podlewa:
// Linia 1: "D1:22% PODLEWA"
// Linia 2: "D2:58% OK"
```

**Kryteria akceptacji:**
- LCD wyświetla wilgotność obu doniczek
- LCD wyświetla poziom naładowania baterii
- Informacja widoczna przez 10 sekund po pomiarze

### US-3: Użytkownik - Ostrzeżenie o niskiej baterii

**Jako** właściciel doniczek
**Chcę** otrzymać ostrzeżenie gdy bateria jest bliska rozładowania
**Aby** naładować akumulator zanim system przestanie działać

**Scenariusz:**
```cpp
// Napięcie baterii spada poniżej progu (np. 3.4V na celę = 6.8V łącznie)
// LCD wyświetla przez cały czas: "!! NISKI POZIOM !!"
// Linia 2: "BATERII: 12%"
// System nadal działa, ale ostrzega użytkownika
```

**Kryteria akceptacji:**
- Ostrzeżenie przy baterii < 20%
- LCD miga lub wyświetla alert
- System nie wyłącza się automatycznie

---

## 🔧 Technical Requirements

### TR-1: Mikrokontroler ESP32

**Specyfikacja:**
```
ESP32 DevKit V1 (lub podobny)
- CPU: 240MHz dual-core (nieużywany w pełni - oszczędność energii)
- ADC: 12-bit (piny 32-39 dla analogowych odczytów)
- Deep sleep: ~10μA
- Aktywny: ~80-240mA
- Napięcie logiki: 3.3V
```

**Uwaga:** Piny ADC ESP32 (ADC2) nie działają gdy WiFi jest aktywne.
Używać wyłącznie ADC1 (piny: 32, 33, 34, 35, 36, 39).

### TR-2: Czujniki wilgotności gleby

**Specyfikacja:**
```
Typ: Pojemnościowy (zalecany, nie koroduje)
Model: np. Capacitive Soil Moisture Sensor v1.2
Napięcie: 3.3V lub 5V
Wyjście: Analogowe (0-3.3V)
Piny: Czujnik 1 → GPIO34, Czujnik 2 → GPIO35
```

**Kalibracja:**
```cpp
// Wartości do wyznaczenia doświadczalnie:
#define MOISTURE_DRY   2800  // ADC przy suchej glebie
#define MOISTURE_WET   1200  // ADC przy mokrej glebie

// Przeliczenie na procenty:
int moisturePercent(int raw) {
  return map(raw, MOISTURE_DRY, MOISTURE_WET, 0, 100);
}
```

**Uwaga:** Wyłączać czujniki między pomiarami (przez tranzystor lub pin GPIO) dla oszczędności energii.

### TR-3: Pompki wodne i moduł relay

**Specyfikacja pompek:**
```
Typ: Mini pompka DC brushless
Napięcie: DC 3-5V
Prąd: ~100-200mA podczas pracy
Wyjście: wąż 5mm (pasuje do dołączonego węża PVC)
Ilość: 2 (z zestawu AliExpress, zapas 2 sztuki)
```

**Specyfikacja modułu relay:**
```
Typ: 4-kanałowy moduł relay 5V (z zestawu AliExpress)
Napięcie sterowania: 5V (VCC) / sygnał IN: 3.3V-5V
Sterowanie: aktywne LOW (LOW = relay włączony)
Piny: Relay 1 → GPIO26, Relay 2 → GPIO27
```

**Schemat sterowania:**
```
ESP32 GPIO26 → IN1 (relay moduł) → NO/COM → Pompka 1 → 5V
ESP32 GPIO27 → IN2 (relay moduł) → NO/COM → Pompka 2 → 5V
Relay VCC → 5V
Relay GND → GND
```

**Uwaga:** Relay jest aktywne LOW — `digitalWrite(RELAY_PIN, LOW)` włącza pompkę.

**Zmiana względem v1.0 PRD:**
- ~~Zawory solenoidowe grawitacyjne~~ → Pompki DC z rezerwuaru
- ~~Tranzystory BC337 + diody flyback~~ → Moduł relay (w zestawie)
- Zbiornik wody może być na dowolnej wysokości

### TR-4: Wyświetlacz OLED (wbudowany w płytkę ESP32)

**Specyfikacja:**
```
Typ: OLED 0.96" SSD1306 (wbudowany w ideaspark ESP32)
Rozdzielczość: 128x64 px
Interfejs: I2C (wbudowany, piny wewnętrzne płytki)
Biblioteka: Adafruit SSD1306 + Adafruit GFX
Napięcie: 3.3V (wewnętrzne)
```

**Zmiana względem v1.0 PRD:**
- ~~LCD 16x2 + moduł I2C~~ → OLED 0.96" wbudowany w płytkę ESP32
- ~~LiquidCrystal_I2C~~ → Adafruit SSD1306
- Więcej miejsca na ekranie (128x64 px vs 16x2 znaków)

### TR-5: Zasilanie z akumulatora

**Specyfikacja:**
```
Akumulator: 2x 18650 Li-Ion (szeregowo = 7.4V)
Pojemność: 2x 3000mAh = 6000mAh
Przetwornica: Buck DC-DC do 5V (np. LM2596 lub MP1584)
Ładowanie: Moduł TP4056 (osobno dla każdej celi) lub BMS 2S

Monitoring napięcia:
  Bateria → R1 (100kΩ) → GPIO32 (ADC) → R2 (47kΩ) → GND
  Zakres pomiaru: 0-8.4V → 0-3.3V na ADC
```

**Obliczenie dzielnika:**
```cpp
#define R1 100000.0  // 100kΩ
#define R2 47000.0   // 47kΩ

float readBatteryVoltage() {
  int raw = analogRead(32);
  float adcVoltage = (raw / 4095.0) * 3.3;
  return adcVoltage * ((R1 + R2) / R2);
}

int batteryPercent(float voltage) {
  // Li-Ion 2S: 8.4V = 100%, 6.4V = 0%
  return constrain((int)((voltage - 6.4) / (8.4 - 6.4) * 100), 0, 100);
}
```

### TR-6: Deep Sleep i harmonogram pomiarów

**Specyfikacja:**
```cpp
#define SLEEP_HOURS 4
#define uS_TO_S_FACTOR 1000000ULL

void goToSleep() {
  esp_sleep_enable_timer_wakeup(SLEEP_HOURS * 3600 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}
```

**Cykl pracy:**
```
Budzenie → Inicjalizacja (0.5s)
         → Odczyt czujników (0.5s)
         → Logika podlewania
         → Otwarcie zaworów jeśli potrzeba (max 30s)
         → Aktualizacja LCD (10s)
         → Deep sleep (4h)
─────────────────────────────────
Czas aktywny: ~12s na cykl
```

### TR-7: Pinout ESP32

```
GPIO21  - SDA (LCD I2C)
GPIO22  - SCL (LCD I2C)
GPIO26  - Zawór 1 (przez tranzystor)
GPIO27  - Zawór 2 (przez tranzystor)
GPIO32  - ADC bateria (ADC1_CH4)
GPIO34  - ADC czujnik wilgotności 1 (ADC1_CH6, input only)
GPIO35  - ADC czujnik wilgotności 2 (ADC1_CH7, input only)
GPIO25  - Zasilanie czujników (włącz przed pomiarem, wyłącz po)
```

---

## 📐 Implementation Plan

### Phase 1: Setup sprzętowy (30 min)

**Krok 1.1: Test czujników wilgotności**
```cpp
// Test odczytu surowych wartości ADC
void setup() {
  Serial.begin(115200);
  pinMode(25, OUTPUT);
}

void loop() {
  digitalWrite(25, HIGH);  // Włącz czujniki
  delay(100);
  int s1 = analogRead(34);
  int s2 = analogRead(35);
  digitalWrite(25, LOW);   // Wyłącz czujniki

  Serial.printf("Czujnik 1: %d | Czujnik 2: %d\n", s1, s2);
  delay(2000);
}
```

**Weryfikacja:** Odczyty zmieniają się gdy gleba jest sucha/mokra.

**Krok 1.2: Kalibracja czujników**
- Zmierz ADC przy całkowicie suchej glebie → `MOISTURE_DRY`
- Zmierz ADC przy nasyconej glebie → `MOISTURE_WET`
- Zapisz wartości w kodzie

**Krok 1.3: Test pompek przez relay**
```cpp
#define RELAY1_PIN 26
#define RELAY2_PIN 27

void setup() {
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);  // Relay aktywne LOW - wyłącz na start
  digitalWrite(RELAY2_PIN, HIGH);
}

void loop() {
  digitalWrite(RELAY1_PIN, LOW);   // Włącz pompkę 1 (relay aktywne LOW)
  delay(3000);
  digitalWrite(RELAY1_PIN, HIGH);  // Wyłącz pompkę 1
  delay(2000);
  digitalWrite(RELAY2_PIN, LOW);   // Włącz pompkę 2
  delay(3000);
  digitalWrite(RELAY2_PIN, HIGH);  // Wyłącz pompkę 2
  delay(5000);
}
```

### Phase 2: Integracja czujniki + LCD (45 min)

**Krok 2.1: Wyświetlanie wilgotności na OLED**
```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void displayStatus(int m1, int m2, int bat) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.printf("Doniczka 1: %d%%", m1);
  display.setCursor(0, 16);
  display.printf("Doniczka 2: %d%%", m2);
  display.setCursor(0, 32);
  display.printf("Bateria:    %d%%", bat);
  display.display();
}
```

### Phase 3: Logika podlewania + deep sleep (45 min)

**Krok 3.1: Kompletna logika**
```cpp
#define MOISTURE_THRESHOLD_1  35  // % - próg dla doniczki 1 (do kalibracji)
#define MOISTURE_THRESHOLD_2  35  // % - próg dla doniczki 2 (do kalibracji)
#define PUMP_RUN_TIME         5000   // ms - czas pracy pompki (zaczynamy od 5s!)

// Relay aktywne LOW: LOW = pompka włączona, HIGH = wyłączona
void checkAndWater(int relayPin, int moisture, int threshold) {
  if (moisture < threshold) {
    digitalWrite(relayPin, LOW);   // Włącz pompkę
    delay(PUMP_RUN_TIME);
    digitalWrite(relayPin, HIGH);  // Wyłącz pompkę
  }
}
```

### Phase 4: Testy i kalibracja (60 min)

- Kalibracja progu wilgotności dla konkretnych roślin
- Pomiar rzeczywistego czasu pracy na baterii
- Weryfikacja poprawności podlewania po 24h

---

## ✅ Acceptance Criteria

### AC-1: Odczyt wilgotności
**Given** ESP32 budzi się z deep sleep
**When** wykonuje cykl pomiarowy
**Then** odczytuje wilgotność obu czujników w czasie <1s

### AC-2: Podlewanie przy niskiej wilgotności
**Given** wilgotność doniczki < próg
**When** system wykonuje cykl
**Then** otwiera zawór na zdefiniowany czas i zamyka

### AC-3: Brak podlewania przy wystarczającej wilgotności
**Given** wilgotność doniczki >= próg
**When** system wykonuje cykl
**Then** zawór pozostaje zamknięty

### AC-4: Wyświetlanie statusu na LCD
**Given** zakończono cykl pomiarowy
**When** wyniki są dostępne
**Then** LCD wyświetla wilgotność obu doniczek i poziom baterii przez 10 sekund

### AC-5: Ostrzeżenie o baterii
**Given** poziom baterii < 20%
**When** system wykonuje cykl
**Then** LCD wyświetla ostrzeżenie o niskiej baterii

### AC-6: Deep sleep
**Given** zakończono cykl (pomiar + ewentualne podlewanie + LCD)
**When** upłynie 10 sekund wyświetlania
**Then** ESP32 przechodzi do deep sleep na 4 godziny

### AC-7: Czas pracy na baterii
**Given** 2x 18650 (6000mAh łącznie)
**When** system pracuje normalnie (4h interwał)
**Then** bateria wytrzymuje minimum 30 dni

---

## 📊 Success Metrics

### KPI-1: Czas pracy na baterii
**Cel:** ≥30 dni na jednym ładowaniu
**Pomiar:** Logowanie napięcia baterii w czasie
**Target:** ≥30 dni

### KPI-2: Dokładność pomiaru wilgotności
**Cel:** Powtarzalne odczyty ±5%
**Pomiar:** 10 kolejnych odczytów tej samej gleby
**Target:** Odchylenie standardowe <5%

### KPI-3: Niezawodność zaworów
**Cel:** Zawór otwiera/zamyka się przy każdym poleceniu
**Pomiar:** 50 cykli otwierania/zamykania
**Target:** 100% niezawodność

### KPI-4: Efektywność podlewania
**Cel:** Wilgotność wraca do >50% po podlaniu
**Pomiar:** Odczyt wilgotności przed i 30 minut po podlaniu
**Target:** Wzrost wilgotności o minimum 20%

---

## 🚧 Risks & Mitigations

### Risk-1: Niedokładność ADC ESP32
**Opis:** ADC ESP32 jest nieliniowy i może dawać niedokładne odczyty bez kalibracji.
**Prawdopodobieństwo:** 🔴 Wysokie
**Wpływ:** 🟡 Średni

**Mitigacja:**
- Kalibracja w kodzie przez `esp_adc_cal_characterize()`
- Uśrednianie 10 odczytów
- Wyznaczenie `MOISTURE_DRY` i `MOISTURE_WET` doświadczalnie

### Risk-2: Przepełnienie doniczki
**Opis:** Zbyt długi czas otwarcia zaworu lub błąd czujnika może zalać doniczkę.
**Prawdopodobieństwo:** 🟡 Średnie
**Wpływ:** 🔴 Wysoki

**Mitigacja:**
- Maksymalny czas otwarcia zaworu (hard limit w kodzie)
- Testy z małym czasem (5s) przed pełną automatyzacją
- Wyznaczenie właściwego czasu doświadczalnie

### Risk-3: Korozja czujników rezystancyjnych
**Opis:** Czujniki rezystancyjne korodują w glebie po kilku tygodniach.
**Prawdopodobieństwo:** ✅ Pewne przy użyciu rezystancyjnych
**Wpływ:** 🔴 Wysoki

**Mitigacja:**
- Używać wyłącznie czujników pojemnościowych
- Zalecany model: Capacitive Soil Moisture Sensor v1.2

### Risk-4: Głęboki sen a reset ESP32
**Opis:** Po deep sleep ESP32 wykonuje pełny reset - zmienne RAM są tracone.
**Prawdopodobieństwo:** ✅ Zawsze
**Wpływ:** 🟡 Średni

**Mitigacja:**
- Używać `RTC_DATA_ATTR` dla zmiennych które mają przetrwać deep sleep
- Przechowywać stan w pamięci RTC lub EEPROM

### Risk-5: Relay aktywne LOW — pompka włączona przy resecie ESP32
**Opis:** Moduł relay jest aktywne LOW. Przy starcie ESP32 piny GPIO mogą być chwilowo LOW, co włączy pompki przed inicjalizacją kodu.
**Prawdopodobieństwo:** 🟡 Średnie
**Wpływ:** 🟡 Średni - niezamierzone krótkie uruchomienie pompki

**Mitigacja:**
- W `setup()` natychmiast ustawić piny relay na HIGH (wyłączone)
- Inicjalizacja relay jako pierwsza operacja przed wszystkim innym

---

## 🧪 Test Plan

### Unit Tests

**Test 1: Kalibracja czujnika**
- Włóż czujnik do suchej gleby → zapisz wartość ADC
- Włóż czujnik do mokrej gleby → zapisz wartość ADC
- Weryfikuj mapowanie na % (0-100%)

**Test 2: Sterowanie zaworem**
- Wyślij HIGH na pin zaworu → sprawdź czy woda płynie
- Wyślij LOW → sprawdź czy woda przestaje płynąć
- Zmierz prąd podczas otwarcia (powinien być <400mA)

**Test 3: Monitoring baterii**
- Zmierz napięcie multimetrem
- Porównaj z odczytem z ADC
- Weryfikuj dokładność ±0.2V

### Integration Tests

**Test 4: Pełny cykl - sucha gleba**
1. Ustaw wilgotność poniżej progu (sucha gleba)
2. Uruchom system
3. Weryfikuj: zawór otwiera się na zdefiniowany czas
4. Weryfikuj: LCD pokazuje status podlewania
5. Weryfikuj: zawór zamyka się po czasie

**Test 5: Pełny cykl - mokra gleba**
1. Ustaw wilgotność powyżej progu (mokra gleba)
2. Uruchom system
3. Weryfikuj: zawór pozostaje zamknięty
4. Weryfikuj: LCD pokazuje status OK

**Test 6: Deep sleep**
1. Uruchom system
2. Zmierz prąd po przejściu do deep sleep
3. Weryfikuj: prąd < 1mA (docelowo ~10-50μA)
4. Weryfikuj: budzenie po 4h działa poprawnie

**Test 7: Czas pracy na baterii (test skrócony)**
1. Ustaw interwał na 10 minut (zamiast 4h)
2. Monitoruj napięcie baterii przez 24h
3. Ekstrapoluj czas pracy na normalnym interwale

### Performance Tests

**Test 8: Czas cyklu aktywnego**
- Zmierz czas od budzenia do deep sleep (bez podlewania)
- Target: < 15 sekund

---

## ✍️ Changelog

| Wersja | Data       | Autor | Opis zmian                                              |
|--------|------------|-------|---------------------------------------------------------|
| 1.0    | 2026-04-05 | Irek  | Pierwsza wersja - MVP bez WiFi                         |
| 1.1    | 2026-04-05 | Irek  | Zmiana: pompki+relay zamiast zaworów, OLED zamiast LCD |

---

## 📚 Appendix

### A. Lista materiałów (BOM)

| Komponent                        | Ilość | Uwagi                              |
|----------------------------------|-------|------------------------------------|
| ESP32 DevKit V1                  | 1     | Mikrokontroler główny              |
| ESP32 ideaspark z OLED 0.96"     | 1     | Zamówiony ✅                        |
| Czujnik wilgotności pojemnościowy| 4     | Z zestawu AliExpress ✅             |
| Moduł relay 4-kanałowy 5V        | 1     | Z zestawu AliExpress ✅             |
| Mini pompka DC 3-5V              | 4     | Z zestawu AliExpress ✅ (2 używane) |
| Wąż PVC 4m                       | 1     | Z zestawu AliExpress ✅             |
| Akumulator 18650 LG              | 1-3   | Do zamówienia (Allegro)            |
| Koszyk na 1x 18650               | 1     | Do zamówienia                      |
| Moduł TP4056 USB-C z protekcją   | 1     | Do zamówienia                      |
| Przetwornica boost MT3608 5V     | 1     | Do zamówienia                      |
| Rezystor 100kΩ                   | 1     | Do zamówienia (zestaw rezystorów)  |
| Rezystor 47kΩ                    | 1     | Do zamówienia (zestaw rezystorów)  |
| Zbiornik na wodę ~1-2L           | 1     | Dowolny pojemnik                   |
| Breadboard + przewody            | 1     | Do prototypowania                  |

### B. Roadmapa

**v2.0 - WiFi & Monitoring (Q3 2026):**
- 🌐 Połączenie WiFi
- 📊 Web dashboard (historia wilgotności, wykresy)
- 🔔 Powiadomienia (email/Telegram) przy niskiej baterii lub problemie
- ⚙️ Zdalne ustawianie progów podlewania
- 📡 Integracja z TrueNAS lub Home Assistant

---

**Koniec dokumentu**
