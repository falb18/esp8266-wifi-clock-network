# ESP8266 WiFi Clock Network

This repository contains the firmware for the LED Matrix clock you can buy on Aliexpress.

<img src="./imgs/aliexpress-wifi-clock.png" width="480" height="">

The original author of the project is: https://www.youtube.com/@hacklabs. Although it is unlikely that he sells those
devices on Aliexpress.

Here is a video of the project description: https://www.youtube.com/watch?v=wF05PX6ary8

You can find the original Arduino code for the ESP8266 inside the zip file ***hackclock_original_files.zip***

The device that you buy from Aliexpress comes with an updated version of the firmware. You can find the new version
inside the directory **MatrixClock_update**. The only problem is that I only found on the internet the binary so the
source code is not available.

## New version of the firmware

Since for the second version of the firmware the source code is not available, I working on a new firmware that
implements the same features of both versions. You can find the new version of the Arduino code inside the directory
***MatrixClock_v3***.

The project uses the following Arduino libraries:
- [DS3231 v1.1.2](https://github.com/NorthernWidget/DS3231)
- [MD_MAX72XX v3.5.1](https://github.com/MajicDesigns/MD_MAX72XX)
- [NTP v1.7.0](https://github.com/sstaub/NTP)
- [TickTwo v4.4.0](https://github.com/sstaub/TickTwo)

## References
- This is a different project that uses the same device: https://github.com/trip5/EspHome-Led-PixelClock