/*
   Human Washing Machine
   setup:
   - 1x electrical water valve
   - 1x power relay for heater
   - 1x fluid switch
   - 1x push button

   scenerio:
   1. app checks the water level first, starts to fill if tank is empty
   2. while filling the water, turns on the hot water valve for 3 minutes
   3. if water starts to drain after 5 minutes, wait 30 minutes and fill the tank and stop
*/

#include <avr/wdt.h>
#include <RCSwitch.h>

#define togglrate 3000
RCSwitch rc = RCSwitch();

#define rcRate 1000               // one sec

#define in_water 0           // water level sensor
#define in_power 1           // power switch
#define in_start 2           // start switch 
#define in_temp A0           // temperature i/o 

#define out_selenoid 5   // water valve
#define out_therm  4     // recycle pump
#define out_heater 8     // 3kw heater
#define out_pump 6       // speed adjustable water pump

#define temp_upper 350
#define temp_lower 5
#define water_treshold 1250
#define btn_treshold 100

const int numReadings = 50;

// ###############################################

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

// ###############################################

int state = 0;            // 0:idle, 1:filling, 2:heating, 3: pumping
bool active = false;

unsigned long lastrc = 0;
unsigned long fillstart = 0;
unsigned long lastcycle = 0;
unsigned long laststart = 0;
unsigned long lasttoggl = 0;
unsigned long lastpump = 0;
unsigned long lastheat = 0;

void setup() {
  watchdogOn();
  //  disableADC();
  prepare();
  reset();
}

void loop() {
  if (active == false) {
    cycle();
    return;
  }

  switch (readPin(in_water, water_treshold)) {
    case 0:
      digitalWrite(out_heater, HIGH);
      digitalWrite(out_therm, HIGH);
      digitalWrite(out_pump, HIGH);
      digitalWrite(out_selenoid, LOW);
      state = 1;
      break;
    case 1:
      digitalWrite(out_selenoid, HIGH);
      digitalWrite(out_therm, LOW);
      state = 2;
      break;
  }

  if (state == 2) {
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(in_temp);
    total = total + readings[readIndex];
    readIndex = readIndex + 1;
    if (readIndex >= numReadings) {
      readIndex = 0;
    }
    average = total / numReadings;
    if (average >= temp_upper) {
      if (((millis() - lastheat) > togglrate) & digitalRead(out_heater) == HIGH) {
        lastheat = millis();
        digitalWrite(out_heater, LOW);
      }
    }

    if (average <= temp_lower) {
      if (((millis() - lastheat) > togglrate) & digitalRead(out_heater) == LOW) {
        lastheat = millis();
        digitalWrite(out_heater, HIGH);
      }
    }

  }
  cycle();
}

// #############################################################

void cycle() {
  collect();
  ping();
  wdt_reset();
}

void toggle(bool e) {
  if ((millis() - lasttoggl) < togglrate && lasttoggl > 0) {
    return ;
  }
  lasttoggl = millis();
  reset();
  lastrc = 0;
  fillstart = 0;
  state = 0;
  delay(25);
  active = e;
}

void parseRC(long data, int bitlen, int proto) {
  if (bitlen != 24 || proto != 1 || data < 1000 || data > 9999) {
    return ;
  }
  if ((millis() - lastrc) < rcRate) {
    return ;
  }
  lastrc = millis();
  switch (data) {
    case 2001:
      if (active == true) {
        return ;
      }
      toggle(true);
      break;
    case 2002:
      if (active == false) {
        return ;
      }
      toggle(false);
      break;
    case 2003:
      toggle(!active);
      break;
    case 2004:
      if ((millis() - lastpump) > togglrate) {
        lastpump = millis();
        toggle(false);
        digitalWrite(out_pump, LOW);
      }
      break;
    case 2005:
      if ((millis() - lastpump) > togglrate) {
        lastpump = millis();
        toggle(false);
        digitalWrite(out_pump, HIGH);
      }
      break;
    case 2006:
      toggle(false);
      reset();
      break;
  }
}

void collect() {

  if (readPin(in_power, btn_treshold) == 0) {
    if (((millis() - laststart) > togglrate)) {
      laststart = millis();
      toggle(!active);
    }
  }

  if (readPin(in_start, 100) == 0) {
    if (((millis() - lastpump) > togglrate)) {

      lastpump = millis();
      if (digitalRead(out_pump) == LOW) {
        digitalWrite(out_pump, HIGH);
        return ;
      }
      if (digitalRead(out_pump) == HIGH) {
        if (active == true) {
          toggle(false);
        }
        active = false;
        state = 3;
        digitalWrite(out_pump, LOW);
        return ;
      }
    }
  }

  if (rc.available()) {
    long val = rc.getReceivedValue();
    int bitlen = rc.getReceivedBitlength();
    int proto = rc.getReceivedProtocol();
    rc.resetAvailable();
    parseRC(val, bitlen, proto);
  }
}

void prepare() {
  digitalWrite(LED_BUILTIN, HIGH);
  rc.enableReceive(1);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(out_selenoid, OUTPUT);
  pinMode(out_therm, OUTPUT);
  pinMode(out_heater, OUTPUT);
  pinMode(out_pump, OUTPUT);

  pinMode(in_water, INPUT_PULLUP);
  pinMode(in_power, INPUT_PULLUP);
  pinMode(in_start, INPUT_PULLUP);

  reset();
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  laststart = millis();
  lasttoggl = millis();
  lastpump = millis();
  lastheat = millis();
}

void reset() {
  active = false;
  digitalWrite(out_selenoid, HIGH);
  digitalWrite(out_therm, HIGH);
  digitalWrite(out_heater, HIGH);
  digitalWrite(out_pump, HIGH);
  state = 0;
}

// #############################################################

unsigned long blink = 0;

void watchdogOn() {
  MCUSR = MCUSR & B11110111;
  WDTCSR = WDTCSR | B00011000;
  WDTCSR = B00100001;
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}
/*
  void disableADC() {
  ADCSRA = ADCSRA & B01111111;
  ACSR = B10000000;
  }
*/
void ping() {
  if ((millis() - blink) > 500) {
    blink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

unsigned long cps[13];
int cfs[13];

int readPin(int i, int tmr) {
  int t = (digitalRead(i) == HIGH ? 1 : 0);
  if (tmr == 0) {
    return t;
  }
  unsigned long n = millis();
  if ((n - cps[i] >= tmr) && t == cfs[i]) {
    return t;
  }
  if (t != cfs[i]) {
    cps[i] = n;
    cfs[i] = t;
  }
  return -1;
}
