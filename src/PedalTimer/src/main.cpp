// this little Project is for my bandmates
// i have to measure the length of our songs,
// so i simply design this. A nano, a footbutton and a TM1637 display is all i need.
// short tap starts/stop/restart the timer
// long tap reset the timer, northing more
#include <arduino.h>
#include <TM1637Display.h>
#include "avdweb_Switch.h"
#include <eeprom.h>

// Version
#define VS_MAJ 0
#define VS_MIN 4
// constants
#define ON 1
#define OFF 0
// Display
#define CLK 2
#define DIO 3
#define LED 13
// Footswitch
#define FS 8

const uint8_t TTD[] = {SEG_F | SEG_G | SEG_E | SEG_D};
const uint8_t STD[] = {SEG_A | SEG_F | SEG_G | SEG_C | SEG_D, SEG_F | SEG_G | SEG_E | SEG_D};
const uint8_t CDD[] = {SEG_G | SEG_E | SEG_D, SEG_B | SEG_G | SEG_E | SEG_D | SEG_C};
const uint8_t CPD[] = {SEG_G | SEG_E | SEG_D};

// show the given time
void showTime(int act);
// lit the LED
void led(bool on);
// read the timer mode
void readCdm();
// do the timer setup
void doSetup();
// showing the given count down mode
void showCdm(bool mode);
// showing the count down time for setup
void showCdmTimeSetup(int time);
// showing the count down time
void showCdmTime(int time);

TM1637Display display = TM1637Display(CLK, DIO);
Switch fs = Switch(FS);

long start = 0L;
bool started = false;
// count down mode, if true the timer operate in count down mode.
// Taking the actual time from the eeprom and on first click starts the count down.
// Default mode is taken from the eeprom, too
bool cdm = false;
byte cdmtime = 0;

void setup()
{
  pinMode(LED, OUTPUT);
  display.clear();
  display.setBrightness(7);
  display.showNumberDec(VS_MAJ, false, 1, 1);
  display.showNumberDec(VS_MIN, true, 2, 2);
  display.showNumberHexEx(0xF, 0xF, false, 1, 0);
  led(ON);
  for (byte i = 0; i < 200; i++)
  {
    fs.poll();
    delay(10);
  }
  readCdm();
  display.clear();
  display.setSegments(TTD, 1, 0);
  showCdm(cdm);
  for (byte i = 0; i < 200; i++)
  {
    fs.poll();
    delay(10);
  }
  if (fs.on())
  {
    doSetup();
  }

  display.clear();
  if (cdm)
  {
    showCdmTimeSetup(cdmtime);
  }
  else
  {
    showTime(0);
  }
  led(OFF);
}

long actual, old = 0;
bool colon = false;

void loop()
{
  fs.poll();
  if (fs.longPress())
  {
    started = false;
    actual = 0;
    colon = false;
    display.setColon(colon);
    led(OFF);
  }
  if (fs.singleClick())
  {
    if (!started)
    {
      started = true;
      start = millis() - (actual * 1000L);
      old = -1;
    }
    else
    {
      started = false;
    }
    led(started);
    colon = started;
    display.setColon(colon);
  }
  if (started)
  {
    actual = (millis() - start) / 1000L;
  }
  if (actual != old)
  {
    display.setColon(colon);
    if (cdm)
    {
      int value = (cdmtime * 60) - int(actual);
      showCdmTime(value);
    }
    else
    {
      showTime(actual);
    }
    colon = !colon;
    old = actual;
  }
}

void showTime(int act)
{
  byte sec = act % 60;
  byte min = (act - sec) / 60;
  // display.clear();
  display.showNumberDec(min, false, 2, 0);
  display.showNumberDec(sec, true, 2, 2);
}

void showCdmTimeSetup(int time)
{
  display.showNumberDec(time, false, 3, 1);
}

void showCdmTime(int time)
{
  if ((time <= 90 * 60) && (time > -9 * 60)) {
    showTime(long(time));
  } else {
    int value = time / 60;
    byte min = value % 60;
    byte h = (value - min) / 60;
    // display.clear();
    display.showNumberDec(h, false, 2, 0);
    display.showNumberDec(min, true, 2, 2);
  }
}

void led(bool on)
{
  digitalWrite(LED, on);
}

void readCdm()
{
  byte value = EEPROM.read(0x00);
  cdm = (value > 0) && (value < 0xFF);
  cdmtime = EEPROM.read(0x01);
}

void doSetup()
{
  led(ON);
  // Set counter mode
  display.clear();
  display.setColon(ON);
  display.setSegments(TTD, 1, 0);
  showCdm(cdm);
  bool end = false;
  bool value = cdm;
  do
  {
    fs.poll();
    if (fs.longPress())
    {
      cdm = value;
      EEPROM.write(0x00, cdm ? 1 : 0);
      end = true;
    }
    if (fs.singleClick())
    {
      value = !value;
      showCdm(value);
    }
  } while (!end);
  led(OFF);
  display.setColon(OFF);
  display.clear();
  display.setSegments(TTD, 1, 0);
  showCdm(cdm);
  delay(1000);
  // now if cdm is selected, let's adjust the count down time
  led(ON);
  display.setColon(ON);
  display.clear();
  display.setSegments(CPD, 1, 0);
  end = false;
  do
  {
    if (cdmtime > 240)
    {
      cdmtime = 0;
      showCdmTimeSetup(cdmtime);
    }
    fs.poll();
    if (fs.longPress())
    {
      EEPROM.write(0x01, cdmtime);
      end = true;
    }
    if (fs.singleClick())
    {
      cdmtime += 5;
      showCdmTimeSetup(cdmtime);
    }
  } while (!end);
  led(OFF);
  display.setColon(OFF);
  display.clear();
  display.setSegments(CPD, 1, 0);
  showCdmTimeSetup(cdmtime);
  delay(1000);
}

void showCdm(bool mode)
{
  if (mode)
  {
    display.setSegments(CDD, 2, 2);
  }
  else
  {
    display.setSegments(STD, 2, 2);
  }
}