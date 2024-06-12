// this little Project is for my bandmates
// i have to measure the length of our songs,
// so i simply design this. A nano, a footbutton and a TM1637 display is all i need.
// short tap starts/stop/restart the timer
// long tap reset the timer, northing more
#include <arduino.h>
#include <TM1637Display.h>
#include "avdweb_Switch.h"

// Version
#define VS_MAJ 0
#define VS_MIN 4
// Display
#define CLK 2
#define DIO 3
// Footswitch
#define FS 8

void showTime(long act);

TM1637Display display = TM1637Display(CLK, DIO);
Switch fs = Switch(FS); 

long start = 0L;
bool started = false; 

void setup() {
  display.clear();
  display.setBrightness(7); 
  display.showNumberDec(VS_MAJ, false,1,1);
  display.showNumberDec(VS_MIN, true,2,2);
  display.showNumberHexEx(0xF, 0xF, false, 1, 0);
  delay(5 * 1000);
  showTime(0);
}

long actual, old = 0;

void loop() {
  fs.poll();
  if (fs.longPress()) {
      started = false;
      actual = 0;
  }
  if (fs.pushed()) {
    if (!started) {
      started = true;
      start = millis() - (actual * 1000L);
      old = -1;        
    } else {
      started = false;
    }
  }
  if (started) {
    actual = (millis() - start) / 1000L;
  }
  if (actual != old) {
    showTime(actual);
    old = actual;
  }
}

void showTime(long act) {
    byte sec = act % 60;
    byte min = (act - sec) / 60;
    //display.clear();
    display.showNumberDec(min, false, 2, 0);
    display.showNumberDec(sec, true, 2, 2);
}
