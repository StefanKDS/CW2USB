/*
  CW2USBKB - funktionale Neuimplementierung fuer Pro Micro
  mit ATmega32U4.

  Inspiriert durch:
  barryshaffer/cw2usbkb, Autor KK7JXG

  Anschluss:
    Paddle-Kontakt 1 -> Pin 2
    Paddle-Kontakt 2 -> Pin 3
    Gemeinsamer Anschluss -> GND

  Standard:
    Pin 2 -> Plus
    Pin 3 -> u-Umlaut

  Fuer diese Zeichen muss das empfangende Geraet
  die externe Tastatur als Deutsch / QWERTZ behandeln.
*/

#include <Keyboard.h>

#if !defined(__AVR_ATmega32U4__)
#error "Dieser Sketch ist fuer einen Pro Micro mit ATmega32U4 vorgesehen."
#endif

// ======================================================
// Einstellungen
// ======================================================

constexpr uint8_t PADDLE1_PIN = 2;
constexpr uint8_t PADDLE2_PIN = 3;

// false: sofort den unten eingestellten Standardmodus nutzen.
// true: Modus nach dem Einschalten per Paddle auswaehlen.
constexpr bool MODUSAUSWAHL_BEIM_START = true;

// RX-LED beim klassischen Pro Micro:
// Pin 17, aktiv bei LOW.
// Bei einem abweichenden Nachbau bei Bedarf deaktivieren.
constexpr bool STATUS_LED_NUTZEN = true;
constexpr uint8_t STATUS_LED_PIN = 17;

// Entprellzeit pro Kontakt.
constexpr unsigned long ENTPRELLZEIT_MS = 5;

// Grenze zwischen kurzer und langer Modusauswahl.
constexpr unsigned long LANGER_DRUCK_MS = 250;

// Arduino Keyboard verwendet fuer direkte HID-Tastencodes
// einen Offset von 136.
//
// HID 0x30: rechte eckige Klammer auf US-Tastatur,
//           Plus auf deutscher Tastatur.
//
// HID 0x2F: linke eckige Klammer auf US-Tastatur,
//           u-Umlaut auf deutscher Tastatur.
constexpr uint8_t MANIA_TASTE_1 = 136 + 0x30;
constexpr uint8_t MANIA_TASTE_2 = 136 + 0x2F;

// Fuer die Belegung aus dem tatsaechlichen Originalcode
// stattdessen die beiden Zeilen oben ersetzen durch:
//
// constexpr uint8_t MANIA_TASTE_1 = 'a';
// constexpr uint8_t MANIA_TASTE_2 = 'b';

enum Betriebsmodus : uint8_t {
  HANDTASTE_MANIA = 1,
  PADDLE_MANIA    = 2,
  HANDTASTE_VBAND = 3,
  PADDLE_VBAND    = 4
};

// Wird verwendet, wenn die Startauswahl deaktiviert ist.
constexpr uint8_t STANDARDMODUS = PADDLE_MANIA;

// ======================================================
// Entprellter Eingang
// ======================================================

class Kontakt {
public:
  explicit Kontakt(uint8_t pin)
    : pin_(pin),
      rohGedrueckt_(false),
      stabilGedrueckt_(false),
      letzteAenderung_(0) {
  }

  void begin() {
    pinMode(pin_, INPUT_PULLUP);

    rohGedrueckt_ = (digitalRead(pin_) == LOW);
    stabilGedrueckt_ = rohGedrueckt_;
    letzteAenderung_ = millis();
  }

  void update() {
    const unsigned long jetzt = millis();
    const bool neuerRohzustand = (digitalRead(pin_) == LOW);

    if (neuerRohzustand != rohGedrueckt_) {
      rohGedrueckt_ = neuerRohzustand;
      letzteAenderung_ = jetzt;
    }

    if (jetzt - letzteAenderung_ >= ENTPRELLZEIT_MS) {
      stabilGedrueckt_ = rohGedrueckt_;
    }
  }

  bool gedrueckt() const {
    return stabilGedrueckt_;
  }

private:
  const uint8_t pin_;
  bool rohGedrueckt_;
  bool stabilGedrueckt_;
  unsigned long letzteAenderung_;
};

Kontakt kontakt1(PADDLE1_PIN);
Kontakt kontakt2(PADDLE2_PIN);

uint8_t aktuellerModus = STANDARDMODUS;

uint8_t aktiveTaste1 = 0;
uint8_t aktiveTaste2 = 0;

bool taste1Gesendet = false;
bool taste2Gesendet = false;

// ======================================================
// Hilfsfunktionen
// ======================================================

void statusLed(bool an) {
  if (STATUS_LED_NUTZEN) {
    digitalWrite(STATUS_LED_PIN, an ? LOW : HIGH);
  }
}

void kontakteAktualisieren() {
  kontakt1.update();
  kontakt2.update();
}

void warteAufFreigabe() {
  // Erst fortfahren, wenn beide Kontakte sowohl
  // elektrisch als auch entprellt geoeffnet sind.
  while (true) {
    kontakteAktualisieren();

    if (digitalRead(PADDLE1_PIN) == HIGH &&
        digitalRead(PADDLE2_PIN) == HIGH &&
        !kontakt1.gedrueckt() &&
        !kontakt2.gedrueckt()) {
      return;
    }

    delay(1);
  }
}

void modusAuswaehlen() {
  warteAufFreigabe();

  uint8_t ausgewaehlterKontakt = 0;
  unsigned long druckBeginn = 0;

  while (true) {
    kontakteAktualisieren();

    // Blinkende LED: Auswahl ist aktiv.
    statusLed(((millis() / 250UL) & 1UL) != 0);

    if (ausgewaehlterKontakt == 0) {
      if (kontakt1.gedrueckt()) {
        ausgewaehlterKontakt = 1;
        druckBeginn = millis();
      } else if (kontakt2.gedrueckt()) {
        ausgewaehlterKontakt = 2;
        druckBeginn = millis();
      }
    } else {
      const bool nochGedrueckt =
        (ausgewaehlterKontakt == 1)
          ? kontakt1.gedrueckt()
          : kontakt2.gedrueckt();

      if (!nochGedrueckt) {
        const bool langerDruck =
          (millis() - druckBeginn >= LANGER_DRUCK_MS);

        if (ausgewaehlterKontakt == 1) {
          aktuellerModus =
            langerDruck ? HANDTASTE_VBAND : HANDTASTE_MANIA;
        } else {
          aktuellerModus =
            langerDruck ? PADDLE_VBAND : PADDLE_MANIA;
        }

        break;
      }
    }

    delay(1);
  }

  statusLed(false);
  warteAufFreigabe();
}

void tastenFestlegen() {
  aktiveTaste1 = 0;
  aktiveTaste2 = 0;

  switch (aktuellerModus) {
    case HANDTASTE_MANIA:
      aktiveTaste1 = ' ';
      break;

    case PADDLE_MANIA:
      aktiveTaste1 = MANIA_TASTE_1;
      aktiveTaste2 = MANIA_TASTE_2;
      break;

    case HANDTASTE_VBAND:
      aktiveTaste1 = KEY_LEFT_CTRL;
      break;

    case PADDLE_VBAND:
      aktiveTaste1 = KEY_LEFT_CTRL;
      aktiveTaste2 = KEY_RIGHT_CTRL;
      break;
  }
}

void modusBestaetigen() {
  // Anzahl kurzer LED-Blinksignale:
  // 1 = Handtaste Mania
  // 2 = Paddle Mania
  // 3 = Handtaste VBand
  // 4 = Paddle VBand
  if (!STATUS_LED_NUTZEN) {
    return;
  }

  delay(200);

  for (uint8_t i = 0; i < aktuellerModus; ++i) {
    statusLed(true);
    delay(100);
    statusLed(false);
    delay(150);
  }
}

void tasteAktualisieren(
  bool kontaktGedrueckt,
  uint8_t taste,
  bool &bereitsGesendet
) {
  // Taste 0 bedeutet: Eingang in diesem Modus unbenutzt.
  if (taste == 0) {
    return;
  }

  if (kontaktGedrueckt == bereitsGesendet) {
    return;
  }

  if (kontaktGedrueckt) {
    Keyboard.press(taste);
  } else {
    Keyboard.release(taste);
  }

  bereitsGesendet = kontaktGedrueckt;
}

// ======================================================
// Arduino
// ======================================================

void setup() {
  if (STATUS_LED_NUTZEN) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    pinMode(STATUS_LED_PIN, OUTPUT);
  }

  kontakt1.begin();
  kontakt2.begin();

  Keyboard.begin();

  // Kurze Startpause, ohne auf eine serielle Verbindung
  // zu warten. Dadurch auch fuer den Handy-Betrieb geeignet.
  delay(1000);

  Keyboard.releaseAll();

  if (MODUSAUSWAHL_BEIM_START) {
    modusAuswaehlen();
  }

  tastenFestlegen();
  modusBestaetigen();

  // Ein beim Start gehaltener Kontakt soll keine
  // unbeabsichtigte Tastatureingabe ausloesen.
  warteAufFreigabe();

  statusLed(false);
}

void loop() {
  kontakteAktualisieren();

  tasteAktualisieren(
    kontakt1.gedrueckt(),
    aktiveTaste1,
    taste1Gesendet
  );

  tasteAktualisieren(
    kontakt2.gedrueckt(),
    aktiveTaste2,
    taste2Gesendet
  );

  statusLed(taste1Gesendet || taste2Gesendet);
}
