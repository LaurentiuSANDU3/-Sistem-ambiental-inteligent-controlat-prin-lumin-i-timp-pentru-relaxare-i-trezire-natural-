
//
// - Constante de timp:
//     ORA_START_TREZIRE, MIN_START_TREZIRE   (ex. 06:30)
//     ORA_LIMITA_TREZIRE, MIN_LIMITA_TREZIRE (ex. 08:00)
//     ORA_SEARA (ex. 21:00)
//     DURATA_SUNETE_SOMN_MIN (ex. 30)
//
// - Constante de lumina (de calibrat cu LDR-ul tau, vezi Serial Monitor):
//     PRAG_ZI, PRAG_NOAPTE, PRAG_DELTA_RASARIT
//     Foloseste DOI pragi diferiti (histerezis) ca sa nu "palpaie" decizia
//     cand lumina e fix pe prag.
//
// - Pini: LDR pe A0, buton snooze/dezactivare pe un pin digital,
//   backlight LCD pe un pin PWM (daca il comanzi direct), RX/TX DFPlayer.
//
// - Enum pentru starile sistemului (masina de stari):
//     STARE_IDLE, STARE_ASTEPTARE_RASARIT, STARE_TREZIRE_ACTIVA,
//     STARE_SNOOZE, STARE_ASISTENT_SOMN, STARE_NOAPTE
//
// - Variabile globale: stareCurenta, ultimaCitireLumina, momentStartSomn,
//   alarmaDeclansataAzi (flag ca sa nu sune de doua ori in aceeasi zi),
//   volumCurent, momentUltimaActualizareLCD.




// ------------------------------------------------------------
// 1 RTC DS3231 I2C
// ------------------------------------------------------------
// void setupRTC()
//   - Init I2C; verifica daca RTC raspunde (daca nu, afiseaza eroare pe LCD/Serial).
//
// uint8_t bcdLaDecimal(uint8_t bcd) si uint8_t decimalLaBcd(uint8_t dec)
//   - DS3231 stocheaza valorile in BCD.
//
// void citesteOra(uint8_t &ora, uint8_t &minut, uint8_t &secunda)
//   - Citeste registrele 0x00 (secunde), 0x01 (minute), 0x02 (ore).
//  
// void setareOra(uint8_t ora, uint8_t minut, uint8_t secunda)
//   - Folosita o singura data (sau prin butoane/Serial) ca sa setezi ceasul.
//
// uint16_t minuteDinZi(uint8_t ora, uint8_t minut)
//   - Converteste in minute de la miezul noptii (ora*60+minut) ca sa compari
//     usor ferestre de timp, inclusiv ferestre care trec peste miezul noptii.
//
// bool inFereastraTrezire(uint16_t minuteAcum)
//   - true daca suntem intre START_TREZIRE si LIMITA_TREZIRE.
//
// bool esteSeara(uint16_t minuteAcum)
//   - true daca ora >= ORA_SEARA (sau intr-un interval seara-noapte).
//
// (Optional) citire temperatura din DS3231 (registrele 0x11-0x12) si afisare pe LCD.


// ------------------------------------------------------------
//  UART + DFPlayer MINI
// ------------------------------------------------------------
// void setupUART()
//   - Varianta registre: UBRR0 pentru 9600 baud, UCSR0B (TXEN0, RXEN0),
//     UCSR0C (8 biti date, 1 stop).
/

// ------------------------------------------------------------
// LCD I2C (de obicei adresa 0x27 sau 0x3F, expander PCF8574)
// ------------------------------------------------------------
// void setupLCD()
//   - Init, curatare, backlight conform starii initiale.
//
// void lcdAfiseazaStatus(ora, minut, stare, nivelLumina)
//   - Linia 1: ora curenta (HH:MM) si numele starii (ex. "ASTEPT. RASARIT").
//   - Linia 2: nivel lumina sau timp ramas pana la fallback / pana la oprirea sunetelor.
//   - Actualizeaza o data pe secunda (millis), nu la fiecare iteratie de loop,
//     si sterge doar ce se schimba ca sa eviti palpairea.
//
// void lcdBacklight(bool pornit)
//   - Comanda biti de backlight de pe PCF8574 sau pin dedicat.
//
// void autoDimming(uint16_t nivelLumina, bool esteNoapte)
//   - Noaptea (lumina < PRAG_NOAPTE): backlight oprit sau minim.
//   - Ziua: backlight normal.
//   - Daca backlight-ul e pe pin PWM: map() intre lumina ambientala si luminozitate,
//     cu histerezis ca sa nu clipeasca.
//   - Optional: butonul reaprinde ecranul 5-10 s ("wake on button").


// ------------------------------------------------------------
//  BUTON (SNOOZE / OPRIRE )
// ------------------------------------------------------------
// void setupButon()
//   - Pin cu INPUT_PULLUP (sau registre DDRx/PORTx).
//
// uint8_t citesteButon()
//   - Debounce (~30-50 ms cu millis) si diferentiere apasare scurta vs lunga:
//       scurta  = snooze 5-10 min / oprire sunet
//       lunga   = arma/dezarma alarma pentru ziua urmatoare
//
// void gestioneazaSnooze()
//   - Opreste melodia, muta starea in STARE_SNOOZE si reia alarma dupa X minute.


// ------------------------------------------------------------
// 6. LOGICA INTELIGENTA (masina de stari)
// ------------------------------------------------------------
// void logicaDimineata(minuteAcum, nivelLumina, tendinta)
//   - In fereastra [START, LIMITA):
//       daca (lumina > PRAG_ZI SI tendinta pozitiva) -> TREZIRE NATURALA (fade-in).
//   - La LIMITA, daca alarma nu a sunat inca -> FALLBACK: porneste sigur alarma audio.
//   - Seteaza alarmaDeclansataAzi = true ca sa nu se repete.
//   - Reseteaza flag-ul alarmaDeclansataAzi la o ora fixa (ex. 12:00 sau miezul noptii).
//
// void logicaSeara(minuteAcum, nivelLumina)
//   - Daca esteSeara SI lumina < PRAG_NOAPTE de cel putin ~1-2 min (confirmare)
//     -> pornesti sunete relaxare, memorezi momentStartSomn.
//   - Dupa DURATA_SUNETE_SOMN_MIN: fade-out + stop + STARE_NOAPTE.
//   - Daca se aprinde lumina in timpul asistentului (lumina > PRAG_ZI), opresti
//     sunetele (utilizatorul s-a trezit / nu doarme).
//
// void actualizeazaStare()
//   - Apelezi in loop: citesti senzori, aplici logicaDimineata / logicaSeara
//     dupa ora, apoi autoDimming si LCD.
//
// Siguranta: tratezi cazul "RTC nu raspunde" (fallback pe millis sau semnal de eroare),
//   si cazul "cardul SD lipseste" (mesaj pe LCD).


void setup() {
}

void loop() {
}
