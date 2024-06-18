// this little Project is for my bandmates
// i have to measure the length of our songs,
// so i simply design this. A nano, a footbutton and a TM1637 display is all i need.
// short tap starts/stop/restart the timer
// long tap reset the timer, northing more
#include <arduino.h>
#include <TM1637Display.h>
#include "avdweb_Switch.h"
#include <eeprom.h>
#include <Adafruit_NeoPixel.h>

// #define debug

// Version
#define VS_MAJ 0
#define VS_MIN 7
// constants
#define ON 1
#define OFF 0
// Display
#define CLK 2
#define DIO 3
// Footswitch
#define FS 8

const uint8_t TTD[] = {SEG_F | SEG_G | SEG_E | SEG_D};
const uint8_t STD[] = {SEG_A | SEG_F | SEG_G | SEG_C | SEG_D, SEG_F | SEG_G | SEG_E | SEG_D};
const uint8_t CDD[] = {SEG_G | SEG_E | SEG_D, SEG_B | SEG_G | SEG_E | SEG_D | SEG_C};
const uint8_t CPD[] = {SEG_G | SEG_E | SEG_D};
const uint8_t MND[] = {SEG_G};
const uint8_t MED[] = {SEG_A | SEG_F | SEG_E | SEG_B | SEG_C, SEG_A | SEG_F | SEG_G | SEG_E | SEG_D, SEG_G | SEG_E | SEG_C, SEG_C | SEG_E | SEG_D, };

// show the given time
void showTime(int act);
// lit the LED
void led(bool on);
// toggle the LED output
void toggled();
// checking if the LED is on
bool isLED();
// read the timer mode
void readCdm();
// do the timer setup
void doSetup();
// showing the given count down mode
void showCdm(bool mode);
// showing the count down time for setup
void setupCdmTimeShow(int time);
// showing the count down time
void showCdmTime(int time);
// write out the config via serial
void outputConfig();

Adafruit_NeoPixel pixel(1, 5, NEO_RGB + NEO_KHZ800);
TM1637Display display = TM1637Display(CLK, DIO);
Switch fs = Switch(FS);

const uint32_t RED = pixel.Color(0xff, 0, 0);
const uint32_t BLUE = pixel.Color(0, 0, 0xff);
const uint32_t GREEN = pixel.Color(0, 0xff, 0);

long start = 0L;
bool started = false;
bool ledOn = false;
// count down mode, if true the timer operate in count down mode.
// Taking the actual time from the eeprom and on first click starts the count down.
// Default mode is taken from the eeprom, too
bool cdm = true;
byte cdmtime = 45;
byte bright = 4;
// time to start the warning, blinking LED,
// in CD mode: time in minutes, when cd time is lower than this, the alarm starts
// in ST mode: time in minutes, when time is greater than this, the alarm starts
byte warningtime = 4;
bool alarm = false;
unsigned long alarmblk = 0;

// actual color of led
uint32_t color = GREEN;
bool endsetup = false;

void setupPixel() 
{
  pixel.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixel.setBrightness(7*8);
  pixel.show();

  color = BLUE;
}

void showVersion()
{
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
}

void afterSetup() {
  outputConfig();
  color = GREEN;
  display.clear();
  display.setBrightness(bright);
  pixel.setBrightness(bright * 8);
  cdm ? showCdmTime(0) :showTime(0);
  led(OFF);
}

void setup()
{
  Serial.begin(115200);

  setupPixel();

  showVersion();

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

  afterSetup();
}

long actualSec, oldSec = 0;
bool colon = false;

void loop()
{
  fs.poll();
  if (fs.doubleClick()) {
    if (!started) {
       doSetup();
       afterSetup();
    }
  }
  if (fs.longPress())
  {
    started = false;
    actualSec = 0;
    colon = false;
    display.setColon(colon);
    led(OFF);
  }
  if (fs.singleClick())
  {
    if (!started)
    {
      started = true;
      start = millis() - (actualSec * 1000L);
      oldSec = -1;
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
    actualSec = (millis() - start) / 1000L;
    if (warningtime > 0)
    {
      if (cdm)
      {
        alarm = (int(cdmtime) * 60) - int(actualSec) < (int(warningtime) * 60);
      }
      else
      {
        alarm = actualSec > (warningtime * 60L);
      }
    }
    if (alarm)
    {
      color = RED;
      if (alarmblk < millis())
      {
        alarmblk = millis() + 500;
        toggled();
        display.setBrightness(7);
      }
    } else {
      color = GREEN;
    }
    led(started);
  }
  if (actualSec != oldSec)
  {
    display.setColon(colon);
    if (cdm)
    {
      showCdmTime(actualSec);
    }
    else
    {
      showTime(actualSec);
    }
    colon = !colon;
    oldSec = actualSec;
  }
}

void showTime(int act)
{
  bool neg = act < 0;
  int t = abs(act);
  byte sec = t % 60;
  byte min = (t - sec) / 60;
  if (neg)
  {
    display.setSegments(MND, 1, 0);
    display.showNumberDec(min, false, 1, 1);
  }
  else
  {
    display.showNumberDec(min, false, 2, 0);
  }
  display.showNumberDec(sec, true, 2, 2);
}

void showCdmTime(int ct)
{
  int time = (cdmtime * 60) - int(ct);
  Serial.println(time);
  if ((time <= 90 * 60) && (time > -9 * 60))
  {
    showTime(long(time));
  }
  else
  {
    int value = time / 60;
    // display.clear();
    display.showNumberDec(value, false, 4, 0);
  }
}

void led(bool on)
{
  if (!on)
  {
    pixel.clear();
    pixel.show();
  }
  else
  {
    pixel.setPixelColor(0, color);
    pixel.show();
  }
  ledOn = on;
}

void toggled()
{
  led(!isLED());
}

bool isLED()
{
  return ledOn;
}

void readCdm()
{
#ifdef debug
  cdm = true;
#else
  byte value = EEPROM.read(0x00);
  cdm = (value > 0) && (value < 0xFF);
  value = EEPROM.read(0x01);
  if (value <= 240)
  {
    cdmtime = value;
  }
  bright = EEPROM.read(0x02);
  if (bright == 0xff)
  {
    bright = 4;
  }
  bright = bright % 7;
  warningtime = EEPROM.read(0x03);
  if (warningtime == 0xff)
  {
    warningtime = 9;
  }
  warningtime = warningtime % 16;
#endif
}

void setupMode()
{
  led(ON);
  display.clear();
  display.setColon(ON);
  display.setSegments(TTD, 1, 0);
  showCdm(cdm);
  bool end = false;
  bool value = cdm;
  do
  {
    fs.poll();
    if (fs.doubleClick()) {
      endsetup = true;
      return;
    }
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
}

void setupCdmTimeShow(int time)
{
  display.setSegments(CPD, 1, 0);
  display.showNumberDec(time, true, 3, 1);
}

void setupCDTime()
{
  led(ON);
  display.setColon(OFF);
  display.clear();
  setupCdmTimeShow(cdmtime);
  bool end = false;
  byte value = cdmtime;
  do
  {
    if ((value > 240) || (value < 5))
    {
      value = 5;
      setupCdmTimeShow(value);
    }
    fs.poll();
    if (fs.doubleClick()) {
      endsetup = true;
      return;
    }
    if (fs.longPress())
    {
      EEPROM.write(0x01, value);
      cdmtime = value;
      end = true;
    }
    if (fs.singleClick())
    {
      value += 5;
      setupCdmTimeShow(value);
    }
  } while (!end);
  led(OFF);
  display.clear();
  setupCdmTimeShow(cdmtime);
  delay(1000);
}

void setupBrigthnessShow(byte value)
{
  display.showNumberHexEx(0xb, 0, false, 1, 0);
  display.showNumberDec(value, false, 1, 3);
}

void setupBrightness()
{
  led(ON);
  display.setColon(ON);
  display.clear();
  setupBrigthnessShow(bright);
  bool end = false;
  byte value = bright;
  do
  {
    if (value > 7)
    {
      value = 0;
      setupBrigthnessShow(value);
    }
    fs.poll();
    if (fs.doubleClick()) {
      endsetup = true;
      return;
    }
    if (fs.longPress())
    {
      bright = value;
      EEPROM.write(0x02, value);
      end = true;
    }
    if (fs.singleClick())
    {
      value++;
      setupBrigthnessShow(value);
    }
  } while (!end);
  led(OFF);
  display.setColon(OFF);
  display.clear();
  setupBrigthnessShow(bright);
  delay(1000);
}

void setupWarningTimeShow(byte value)
{
  display.showNumberHexEx(0xa, 0, false, 1, 0);
  display.showNumberDec(value, true, 2, 2);
}

void setupWarningTime()
{
  led(ON);
  display.setColon(ON);
  display.clear();
  setupWarningTimeShow(warningtime);
  bool end = false;
  byte value = warningtime;
  do
  {
    if (value > 15)
    {
      value = 0;
      setupWarningTimeShow(value);
    }
    fs.poll();
    if (fs.doubleClick()) {
      endsetup = true;
      return;
    }
    if (fs.longPress())
    {
      warningtime = value;
      EEPROM.write(0x03, value);
      end = true;
    }
    if (fs.singleClick())
    {
      value++;
      setupWarningTimeShow(value);
    }
  } while (!end);
  led(OFF);
  display.setColon(OFF);
  display.clear();
  setupWarningTimeShow(warningtime);
  delay(1000);
}

void doSetup()
{
  color = BLUE;
  endsetup = false;

  // show menu text
  display.clear();
  display.setColon(false);
  display.setSegments(MED, 4, 0);
  delay(2000);
  fs.poll();
  // setup display brightness
  setupBrightness();
  if (endsetup) return;
  // Set counter mode
  setupMode();
  if (endsetup) return;
  // now if cdm is selected, let's adjust the count down time
  setupCDTime();
  if (endsetup) return;
  // setup the warning time
  setupWarningTime();
  if (endsetup) return;
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

void outputConfig()
{
  Serial.print("Bright: ");
  Serial.println(bright);
  Serial.print("Mode: ");
  Serial.println(cdm ? "countdown" : "stoptime");
  Serial.print("Warning time: ");
  Serial.println(warningtime);
  Serial.print("Countdown time: ");
  Serial.println(cdmtime);
}
