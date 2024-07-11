#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <ESP8266WiFi.h>
#include <DS3231.h>
#include <MD_MAX72xx.h>
#include <TickTwo.h>

#include "clock-fonts.h"

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D8  // or CS

#define MAX_POS_SCROLL_X  101

void timer50ms();

MD_MAX72XX led_matrix = MD_MAX72XX(HARDWARE_TYPE, D7, D5, D8, MAX_DEVICES);

DS3231 rtc;
RTClib rtc_lib;
DateTime date_time;

TickTwo ticker(timer50ms, 50, 0, MILLIS);

unsigned short maxPosX = MAX_DEVICES * 8 - 1;            
unsigned short LEDarr[MAX_DEVICES][8];                   
unsigned short helpArrMAX[MAX_DEVICES * 8];              
unsigned short helpArrPos[MAX_DEVICES * 8];  

/* These counters keep track of the previous and current seconds. They are needed for the vertical scrolling effect */
byte secs_units = 0, secs_tens = 0;
byte sec_units_prev = 0, sec_units_curr = 0, sec_tens_prev = 0, sec_tens_curr = 0;
bool scroll_sec_units = false, scroll_sec_tens = false;

/* These counters keep track of the previous and current minutes. They are needed for the vertical scrolling effect */
byte min_units = 0, min_tens = 0;
byte min_units_prev = 0, min_units_curr = 0, min_tens_prev = 0, min_tens_curr = 0;
bool scroll_min_units = false, scroll_min_tens = false;

/* These counters keep track of the previous and current hour. They are needed for the vertical scrolling effect */
byte hour_units = 0, hour_tens = 0;
byte hour_units_prev = 0, hour_units_curr = 0, hour_tens_prev = 0, hour_tens_curr = 0;
bool scroll_hour_units = false, scroll_hour_tens = false;

uint8_t day_week, day, month, year;

unsigned int z_PosX = 0;
signed int d_PosX = 0;

bool f_tckr1s = false;
bool f_tckr50ms = false;

/* Two numbers + Null character */
char str_day[3] = {'0', '0', 0};

const char *str_days_week[] = {
  "MON",
  "TUE",
  "WEN",
  "THU",
  "FRI",
  "SAT",
  "SUN"
};

const char *str_months[] = {
  NULL, // This is left null since the range of the values for the month are 1 - 12.
  "JAN",
  "FEB",
  "MAR",
  "APR",
  "MAY",
  "JUN",
  "JUL",
  "AUG",
  "SEP",
  "OCT",
  "NOV",
  "DEC"
};

/* Two numbers + Null character */
char str_year[3] = {'0', '0', 0};

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

void char2Arr(byte ch, int PosX, short PosY) { //characters into arr
  int i, j, k, l, m, o1, o2, o3, o4;  //in LEDarr
  PosX++;
  k = ch - 32;                        //ASCII position in font
  if ((k >= 0) && (k < 96))           //character found in font?
  {
    o4 = font1[k][0];                 //character width
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
            l = font1[k][j + 1];
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

void char22Arr(byte ch, int PosX, short PosY)
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
  
  /* The default pins for I2C are:
   * SDA = GPIO5
   * SCL = GPIO4
   */
  Wire.begin();
  date_time = rtc_lib.now();

  Serial.println();
  Serial.print(date_time.year(), DEC);
  Serial.print('/');
  Serial.print(date_time.month(), DEC);
  Serial.print('/');
  Serial.print(date_time.day(), DEC);
  Serial.print(' ');
  Serial.print(date_time.hour(), DEC);
  Serial.print(':');
  Serial.print(date_time.minute(), DEC);
  Serial.print(':');
  Serial.print(date_time.second(), DEC);
  Serial.println();

  helpArr_init();
  connect_wifi_network();
  ticker.start();

  request_time_date();
}

void loop()
{  
  signed int x = 0;
  signed int y = 0, y1 = 0, y2 = 0, y3=0;
  bool updown = true;
  bool f_scrollend_y = false;
  bool f_scroll_x = false;

  z_PosX = maxPosX;
  d_PosX = -8;

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

      // Store the previous minutes
      min_units_prev = min_units_curr;
      min_units_curr = min_units;
      min_tens_prev = min_tens_curr;
      min_tens_curr = min_tens;

      // Store the previous hour
      hour_units_prev = hour_units_curr;
      hour_units_curr = hour_units;
      hour_tens_prev = hour_tens_curr;
      hour_tens_curr = hour_tens;

      f_tckr1s = false;

      if ((secs_units == 5) && (secs_tens == 4)) {
        f_scroll_x = true;
      }
    }

    /* The following part is executed every 50ms to control the scrolling speed */
    if (f_tckr50ms == true) {
        
      f_tckr50ms = false;

      if (f_scroll_x == true) {
        z_PosX++;
        d_PosX++;
        if (d_PosX == MAX_POS_SCROLL_X)
          z_PosX = 0;
        if (z_PosX == maxPosX) {
          f_scroll_x = false;
          d_PosX = -8;
        }
      }

      // Scroll the first digit in the seconds value
      if (scroll_sec_units == true) {
        if (updown == true) {
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
      } else {
        char22Arr(48 + secs_units, z_PosX - 27, 0);
      }

      // Scroll the second digit in the seconds value
      if (scroll_sec_tens == true) {
        // Scroll current second in and scroll previous second out
        char22Arr(48 + sec_tens_curr, z_PosX - 23, y3);
        char22Arr(48 + sec_tens_prev, z_PosX - 23, y + y1);
        
        if (y == 0)
          scroll_sec_tens = false;
      } else {
        char22Arr(48 + secs_tens, z_PosX - 23, 0);
      }

      if (scroll_min_units == true) {
        char2Arr(48 + min_units_curr, z_PosX - 18, y);
        char2Arr(48 + min_units_prev, z_PosX - 18, y + y1);
        if (y == 0)
          scroll_min_units = false;
      } else {
        char2Arr(48 + min_units, z_PosX - 18, 0);
      }

      if (scroll_min_tens == true) {
        char2Arr(48 + min_tens_curr, z_PosX - 13, y);
        char2Arr(48 + min_tens_prev, z_PosX - 13, y + y1);
        if (y == 0)
          scroll_min_tens = false;
      } else {
        char2Arr(48 + min_tens, z_PosX - 13, 0);
      }

      char2Arr(':', z_PosX - 10 + x, 0);

      if (scroll_hour_units == true) {
        char2Arr(48 + hour_units_curr, z_PosX - 4, y);
        char2Arr(48 + hour_units_prev, z_PosX - 4, y + y1);
        if (y == 0)
          scroll_hour_units = false;
      } else {
        char2Arr(48 + hour_units, z_PosX - 4, 0);
      }

      if (scroll_hour_tens == true) {
        char2Arr(48 + hour_tens_curr, z_PosX + 1, y);
        char2Arr(48 + hour_tens_prev, z_PosX + 1, y + y1);
        if (y == 0)
          scroll_hour_tens = false;
      } else {
        char2Arr(48 + hour_tens, z_PosX + 1, 0);
      }

      char2Arr(' ', (d_PosX + 5), 0);

      /* Day of the week */
      char2Arr(str_days_week[day_week][0], (d_PosX - 1), 0);
      char2Arr(str_days_week[day_week][1], (d_PosX - 7), 0);
      char2Arr(str_days_week[day_week][2], (d_PosX - 13), 0);
      char2Arr(' ', (d_PosX - 19), 0);

      /* Day */
      char2Arr(str_day[0], (d_PosX - 24), 0);
      char2Arr(str_day[1], (d_PosX - 30), 0);
      char2Arr(' ', (d_PosX - 39), 0);

      /* Month */
      char2Arr(str_months[month][0], (d_PosX - 43), 0);
      char2Arr(str_months[month][1], (d_PosX - 49), 0);
      char2Arr(str_months[month][2], (d_PosX - 55), 0);
      char2Arr(' ', (d_PosX - 61), 0);

      /* Year */
      char2Arr('2', d_PosX - 68, 0);
      char2Arr('0', d_PosX - 74, 0);
      char2Arr(str_year[0], d_PosX - 80, 0);
      char2Arr(str_year[1], d_PosX - 86, 0);

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

void clear_led_matrix_buffer()
{
  memset(LEDarr, 0x00, sizeof(LEDarr));
}

void update_time_values()
{
  if (secs_units == 10) {
    scroll_sec_tens = true;
    secs_tens++;
    secs_units = 0;
  }

  if (secs_tens == 6) {
    min_units++;
    secs_tens = 0;
    scroll_min_units = true;
  }

  if (min_units == 10) {
    min_tens++;
    min_units = 0;
    scroll_min_tens = true;
  }

  if (min_tens == 6) {
    hour_units++;
    min_tens = 0;
    scroll_hour_units = true;
  }

  if (hour_units == 10) {
      hour_tens++;
      hour_units = 0;
      scroll_hour_tens = 1;
  }
  if ((hour_tens == 2) && (hour_units == 4)) {
      hour_units = 0;
      hour_tens = 0;
      scroll_hour_tens = 1;
  }
}

void request_time_date(void)
{
  date_time = rtc_lib.now();

  uint8_t second = date_time.second();
  uint8_t minute = date_time.minute();
  uint8_t hour = date_time.hour();
  day_week = rtc.getDoW();
  day = date_time.day();
  month = date_time.month();
  year = date_time.year();
  
  secs_units = (second % 10);
  secs_tens = (second / 10);

  min_units = (minute % 10);
  min_tens = (minute / 10);

  hour_units = (hour % 10);
  hour_tens = (hour / 10);

  sprintf(str_day, "%02u", day);
  sprintf(str_year, "%02u", year);
}

bool connect_wifi_network()
{
  int numAttempts = 0;
  
  if (WiFi.SSID() == "")
  {
    Serial.println("No WiFi network saved.");
    return false;
  }

  /* In case there is a WiFi network saved, try to establish a connection to it */
  Serial.print("Attempting connection to WiFi network: ");
  Serial.println(WiFi.SSID().c_str());
  WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());

  char2Arr('W', 28, 0);
  char2Arr('i', 22, 0);
  char2Arr('-', 18, 0);
  char2Arr('F', 12, 0);
  char2Arr('i', 6, 0);

  refresh_display(); 

  while(numAttempts < WL_MAX_ATTEMPT_CONNECTION)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      numAttempts++;
    }
    else
    {
      break;
    }
  }

  /* Since it is not possible to connect to the WiFi network, then launch
   * smartconfig to allow the user to give new WiFi credentials
   */
  if (numAttempts >= WL_MAX_ATTEMPT_CONNECTION)
  {
    Serial.println("Connection failed.");
    clear_led_matrix_buffer();
    char2Arr('E', 25, 0);
    char2Arr('r', 19, 0);
    char2Arr('r', 12, 0);
    char2Arr('!', 6, 0);
    refresh_display(); 
    delay(2000);
    return false;
  }

  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  clear_led_matrix_buffer();
  char2Arr('O', 25, 0);
  char2Arr('K', 19, 0);
  char2Arr('!', 12, 0);
  char2Arr('!', 6, 0);
  refresh_display();
  delay(2000);

  return true;
}
