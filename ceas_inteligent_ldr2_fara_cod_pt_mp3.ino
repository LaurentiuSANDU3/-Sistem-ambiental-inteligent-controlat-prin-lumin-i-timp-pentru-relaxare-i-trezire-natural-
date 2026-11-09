#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
ThreeWire myWire(12, 11, 13);
RtcDS1302<ThreeWire> Rtc(myWire);

uint8_t oraAlarma = 19;
uint8_t minutAlarma = 10;
const bool DOAR_LA_ALARMA = false;
bool muzicaPermisa = !DOAR_LA_ALARMA;

const uint16_t LUM_PRAG_PORNIRE = 900;
const uint16_t LUM_PRAG_OPRIRE  = 850;
const uint16_t LUM_MAXIM        = 1000;

const uint8_t VOL_MIN = 20;
const uint8_t VOL_MAX = 255;
const uint8_t PAS_VOLUM = 2;

const uint16_t MELODIE[] = {
  330, 330, 349, 392, 392, 349, 330, 294, 262, 262, 294, 330, 330, 294, 294,
  330, 330, 349, 392, 392, 349, 330, 294, 262, 262, 294, 330, 294, 262, 262
};
const uint8_t DURATE[] = {
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1, 4,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1, 4
};
const uint8_t NR_NOTE = sizeof(MELODIE) / sizeof(MELODIE[0]);
const uint16_t UNITATE_MS = 150;
const uint16_t GOL_MS = 40;

uint16_t lumina = 0;
uint8_t volumCurent = 0;
bool muzicaPornita = false;
uint8_t idxNota = 0;
bool inGol = false;
unsigned long tStartNota = 0;
unsigned long tLumina = 0, tVolum = 0, momentUltimaActualizare = 0;

// setari adc pt ldr
void adcInit() {
  ADMUX  = (1 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  DIDR0 |= (1 << ADC0D);
}

uint16_t adcCitire() {
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}

const bool LDR_INVERSAT = true;

uint16_t citesteLumina() {
  uint16_t v = adcCitire();
  return LDR_INVERSAT ? (1023 - v) : v;
}

// setari pwm buzzer
void sunetInit() {
  DDRB  |= (1 << DDB1);
  TCCR1A = (1 << WGM11);
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
  ICR1   = 19999;
  OCR1A  = 0;
}

void sunetVolum(uint8_t v) {
  uint32_t top = ICR1;
  OCR1A = (uint16_t)((top * v * v) / (2UL * 65025UL));
}

void sunetFrecventa(uint16_t f) {
  ICR1 = (uint16_t)(2000000UL / f) - 1;
  TCCR1A |= (1 << COM1A1);
  sunetVolum(volumCurent);
}

void sunetStop() {
  TCCR1A &= ~(1 << COM1A1);
  PORTB  &= ~(1 << PB1);
}

// parcurgere vector note
void melodieReset() {
  idxNota = 0;
  inGol = false;
  tStartNota = millis();
  sunetFrecventa(MELODIE[0]);
}

void melodieUpdate() {
  unsigned long durata = (unsigned long)DURATE[idxNota] * UNITATE_MS;
  unsigned long t = millis() - tStartNota;

  if (!inGol && t >= durata - GOL_MS) {
    sunetStop();
    inGol = true;
  } else if (inGol && t >= durata) {
    idxNota = (idxNota + 1) % NR_NOTE;
    tStartNota = millis();
    inGol = false;
    sunetFrecventa(MELODIE[idxNota]);
  }
}

// mapare lumina - volum
uint8_t calculeazaTinta(uint16_t l) {
  if (!muzicaPermisa || l <= LUM_PRAG_OPRIRE) return 0;
  if (l >= LUM_MAXIM) return VOL_MAX;
  return VOL_MIN + (uint32_t)(l - LUM_PRAG_OPRIRE) * (VOL_MAX - VOL_MIN) / (LUM_MAXIM - LUM_PRAG_OPRIRE);
}

// formatare afisaj
void print2(uint8_t n) {
  if (n < 10) lcd.print('0');
  lcd.print(n);
}

void setup() {
  Serial.begin(9600);

  adcInit();
  sunetInit();
  lumina = citesteLumina();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Pornit...");

  Rtc.Begin();
  if (Rtc.GetIsWriteProtected()) Rtc.SetIsWriteProtected(false);
  if (!Rtc.GetIsRunning()) Rtc.SetIsRunning(true);

  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long acum = millis();

  // filtrare valori ldr
  if (acum - tLumina >= 10) {
    tLumina = acum;
    lumina = (lumina * 7 + citesteLumina()) / 8;

    if (!muzicaPornita && muzicaPermisa && lumina >= LUM_PRAG_PORNIRE) {
      muzicaPornita = true;
      volumCurent = VOL_MIN;
      melodieReset();
    }
  }

  // tranzitie fina volum
  if (muzicaPornita && acum - tVolum >= 20) {
    tVolum = acum;
    uint8_t tinta = calculeazaTinta(lumina);

    if (volumCurent < tinta) {
      volumCurent = (tinta - volumCurent > PAS_VOLUM) ? volumCurent + PAS_VOLUM : tinta;
    } else if (volumCurent > tinta) {
      volumCurent = (volumCurent - tinta > PAS_VOLUM) ? volumCurent - PAS_VOLUM : tinta;
    }

    if (volumCurent == 0 && tinta == 0) {
      sunetStop();
      muzicaPornita = false;
    } else {
      sunetVolum(volumCurent);
    }
  }

  if (muzicaPornita) melodieUpdate();

  // refresh ecran o data pe secunda
  if (acum - momentUltimaActualizare >= 1000) {
    momentUltimaActualizare = acum;

    RtcDateTime now = Rtc.GetDateTime();

    if (DOAR_LA_ALARMA) {
      muzicaPermisa = (now.Hour() == oraAlarma && now.Minute() == minutAlarma);
    }

    lcd.setCursor(0, 0);
    lcd.print("Ora: ");
    print2(now.Hour());   lcd.print(':');
    print2(now.Minute()); lcd.print(':');
    print2(now.Second());
    lcd.setCursor(13, 0);
    lcd.print('L');
    print2((uint32_t)lumina * 99 / 1023);

    lcd.setCursor(0, 1);
    lcd.print("Alarma: ");
    print2(oraAlarma);  lcd.print(':');
    print2(minutAlarma);
    lcd.setCursor(13, 1);
    lcd.print('V');
    print2((uint32_t)volumCurent * 99 / 255);

    Serial.println(lumina);
  }
}