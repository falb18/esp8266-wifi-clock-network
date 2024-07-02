#include <stdio.h>

#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <TickTwo.h>

#include "clock-fonts.h"

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D8  // or CS

void timer50ms();

MD_MAX72XX led_matrix = MD_MAX72XX(HARDWARE_TYPE, D7, D5, D8, MAX_DEVICES);

TickTwo ticker(timer50ms, 50, 0, MILLIS);

unsigned short maxPosX = MAX_DEVICES * 8 - 1;            
unsigned short LEDarr[MAX_DEVICES][8];                   
unsigned short helpArrMAX[MAX_DEVICES * 8];              
unsigned short helpArrPos[MAX_DEVICES * 8];  

/* These counters keep track of the previous and current seconds. They are needed for the vertical scrolling effect */
unsigned int secs_units = 0, secs_tens = 0;
unsigned short sec_units_prev = 0, sec_units_curr = 0, sec_tens_prev = 0, sec_tens_curr = 0;
bool scroll_sec_units = false, scroll_sec_tens = false;

unsigned int z_PosX = 0;

bool f_tckr1s = false;
bool f_tckr50ms = false;

void helpArr_init(void)
{
  unsigned short i, j = 0, k = 0;

  for (i = 0; i < (MAX_DEVICES * 8); i++) {
    helpArrPos[i] = (1 << j);   //bitmask
    helpArrMAX[i] = k;
    j++;
    if (j > 7) {
      j = 0;
      k++;
    }
  }
}

void char22Arr(unsigned short ch, int PosX, short PosY)
{
  int i, j, k, l, m, o1, o2, o3, o4;  //in LEDarr
  PosX++;
  k = ch - 32;                        //ASCII position in font
  if ((k >= 0) && (k < 96))           //character found in font?
  {
    o4 = font2[k][0];                 //character width
    o3 = 1 << (o4 - 2);
    for (i = 0; i < o4; i++) {
      if (((PosX - i <= maxPosX) && (PosX - i >= 0))
              && ((PosY > -8) && (PosY < 8))) //within matrix?
      {
        o1 = helpArrPos[PosX - i];
        o2 = helpArrMAX[PosX - i];
        for (j = 0; j < 8; j++) {
          if (((PosY >= 0) && (PosY <= j)) || ((PosY < 0) && (j < PosY + 8))) //scroll vertical
          {
            l = font2[k][j + 1];
            m = (l & (o3 >> i));  //e.g. o4=7  0zzzzz0, o4=4  0zz0
            if (m > 0)
              LEDarr[o2][j - PosY] = LEDarr[o2][j - PosY] | (o1);  //set point
            else
              LEDarr[o2][j - PosY] = LEDarr[o2][j - PosY] & (~o1); //clear point
          }
        }
      }
    }
  }
}

void timer50ms()
{
    static unsigned int cnt50ms = 0;

    f_tckr50ms = true;
    cnt50ms++;
    if (cnt50ms == 20) {
        f_tckr1s = true; // 1 sec
        cnt50ms = 0;
    }
}

void setup()
{
  Serial.begin(115200);
  led_matrix.begin();
  helpArr_init();
  ticker.start();
}

void loop()
{  
  signed int x = 0;
  signed int y = 0, y1 = 0, y2 = 0, y3=0;
  bool updown = true;
  bool f_scrollend_y = false;
  unsigned int f_scroll_x = false;

  z_PosX = maxPosX;

  if (updown == false) {
      y2 = -9;
      y1 = 8;
  }

  // Scroll up to down
  if (updown == true) {
      y2 = 8;
      y1 = -8;
  }
  
  while (true) {
    /* In ESP8266, if it is unavoidable to run in a loop for a long time (such as solving floating point), you should call
     * yield from time to time to ensure the normal operation of the 8266 background, with no restart and no disconnection
     * from the network as the standard. 
     */
    yield();

    ticker.update();

    if (f_tckr1s == true) {

      // Scroll updown
      y = y2;
      scroll_sec_units = true;
      secs_units++;

      update_time_values();

      // Store the previous seconds
      sec_units_prev = sec_units_curr;
      sec_units_curr = secs_units;
      sec_tens_prev = sec_tens_curr;
      sec_tens_curr = secs_tens;

      f_tckr1s = false;
    }

    /* The following part is executed every 50ms to control the scrolling speed */
    if (f_tckr50ms == true) {
        
      f_tckr50ms = false;

      // Scroll the first digit in the seconds value
      if (scroll_sec_units == true) {
        if (updown == 1) {
          y--;
        } else {
          y++;
        }
        
        y3 = y;
        if (y3 > 0) {
          y3 = 0;
        }     
        
        // Scroll current second in and scroll previous second out
        char22Arr(48 + sec_units_curr, z_PosX - 27, y3);
        char22Arr(48 + sec_units_prev, z_PosX - 27, y + y1);
        
        if (y == 0) {
          scroll_sec_units = false;
          f_scrollend_y = true;
        }
      }
      else {
        char22Arr(48 + secs_units, z_PosX - 27, 0);
      }

      // Scroll the second digit in the seconds value
      if (scroll_sec_tens == true) {
        // Scroll current second in and scroll previous second out
        char22Arr(48 + sec_tens_curr, z_PosX - 23, y3);
        char22Arr(48 + sec_tens_prev, z_PosX - 23, y + y1);
        
        if (y == 0)
          scroll_sec_tens = false;
      }
      else {
        char22Arr(48 + secs_tens, z_PosX - 23, 0);
      }

      refresh_display();
      if (f_scrollend_y == true) {
          f_scrollend_y = false;
      }
        
    }
    
  }
}

void refresh_display()
{
  unsigned short dev, row;
  
  led_matrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  led_matrix.clear();

  for (row = 0; row < 8; row++) {
    for (dev = 0; dev < MAX_DEVICES; dev++) {
      led_matrix.setRow(dev, row, LEDarr[dev][row]);
    }
  }

  led_matrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void update_time_values()
{
  if (secs_units == 10) {
    scroll_sec_tens = true;
    secs_tens++;
    secs_units = 0;
  }

  if (secs_tens == 6) {
    secs_tens = 0;
  }
}
