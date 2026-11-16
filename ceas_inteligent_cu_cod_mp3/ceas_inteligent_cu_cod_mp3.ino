#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
ThreeWire myWire(12, 11, 13);
RtcDS1302<ThreeWire> Rtc(myWire);

SoftwareSerial mySoftwareSerial(7, 8); // TX pe pin 7, RX pe pin 8
DFRobotDFPlayerMini myDFPlayer;

uint8_t oraAlarma = 16;
uint8_t minutAlarma = 45;
const bool DOAR_LA_ALARMA = false;
bool muzicaPermisa = !DOAR_LA_ALARMA;

const uint16_t LUM_PRAG_PORNIRE = 900;
const uint16_t LUM_PRAG_OPRIRE  = 850;
const uint16_t LUM_MAXIM        = 1000;

const uint8_t VOL_MIN = 1;
const uint8_t VOL_MAX = 15; // maxim pentru dfplayer este 30
const uint8_t PAS_VOLUM = 1;

uint16_t lumina = 0;
uint8_t volumCurent = 0;
bool muzicaPornita = false;
unsigned long tLumina = 0, tVolum = 0, momentUltimaActualizare = 0;

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

uint8_t calculeazaTinta(uint16_t l) {
  if (!muzicaPermisa || l <= LUM_PRAG_OPRIRE) return 0;
  if (l >= LUM_MAXIM) return VOL_MAX;
  return VOL_MIN + (uint32_t)(l - LUM_PRAG_OPRIRE) * (VOL_MAX - VOL_MIN) / (LUM_MAXIM - LUM_PRAG_OPRIRE);
}

void print2(uint8_t n) {
  if (n < 10) lcd.print('0');
  lcd.print(n);
}

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);

  adcInit();
  lumina = citesteLumina();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Pornit...");

  Rtc.Begin();
  if (Rtc.GetIsWriteProtected()) Rtc.SetIsWriteProtected(false);
  if (!Rtc.GetIsRunning()) Rtc.SetIsRunning(true);

  if (myDFPlayer.begin(mySoftwareSerial)) {
    myDFPlayer.volume(0);
  }

  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long acum = millis();

  if (acum - tLumina >= 10) {
    tLumina = acum;
    lumina = (lumina * 7 + citesteLumina()) / 8;

    if (!muzicaPornita && muzicaPermisa && lumina >= LUM_PRAG_PORNIRE) {
      muzicaPornita = true;
      volumCurent = VOL_MIN;
      myDFPlayer.volume(volumCurent);
      myDFPlayer.play(1);
    }
  }

  // ajustare lina volum din 150 in 150 ms
  if (muzicaPornita && acum - tVolum >= 150) {
    tVolum = acum;
    uint8_t tinta = calculeazaTinta(lumina);

    if (volumCurent < tinta) {
      volumCurent = (tinta - volumCurent > PAS_VOLUM) ? volumCurent + PAS_VOLUM : tinta;
    } else if (volumCurent > tinta) {
      volumCurent = (volumCurent - tinta > PAS_VOLUM) ? volumCurent - PAS_VOLUM : tinta;
    }

    myDFPlayer.volume(volumCurent);

    if (volumCurent == 0 && tinta == 0) {
      myDFPlayer.pause();
      muzicaPornita = false;
    }
  }

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
    print2((uint32_t)volumCurent * 99 / VOL_MAX); 

    Serial.println(lumina);
  }
}