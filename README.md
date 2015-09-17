# GSM_SMS_Send
Interfaces and classes for sending SMS via GSM-device (C++)

This is set of classes, to provide sending SMS using usb GSM-device:

1) GSMModemClassic.h/cpp - it's support for old-school mobile use AT-modems command set  (for example: NOKIA 6303) and old 3G modems (HUAWEI E353, E153),

2) NewSamsung.h/cpp - it's support for contemporary SAMSUNG smartphones (also use AT-modems command set),

3) ATCmdDevice.h/cpp - common class for support AT-modems commands GSM devices (see - 1) and 2) paragraph above),

4) ClientWinInet.h/cpp - it's support for contemporary HiLink 3G modems (HTTP connection to HUAWEI E303, E3531 and etc.),

5) Serial.h/cpp - support physical (via RS-232) or virtual (via USB) COM-port channel.
