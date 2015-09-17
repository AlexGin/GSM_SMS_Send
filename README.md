# GSM_SMS_Send
Interfaces and classes for sending SMS via GSM-device (C++)

This is set of classes, to provide sending SMS using usb GSM-device:

1) GSMModemClassic.h/cpp - it's support for old-school 3G modems (HUAWEI E353, E153) use AT-modems command set,

2) NewSamsung.h/cpp - it's support for contemporary SAMSUNG smartphones (also use AT-modems command set),

3) PhoneBookProcessor.h/cpp - it's support for old-school mobile phones (for example: NOKIA 6303),

4) ATCmdDevice.h/cpp - common class for support AT-modems commands GSM devices (see - 1, 2, 3 paragraphs above),

5) ClientWinInet.h/cpp - it's support for contemporary HiLink 3G modems (HTTP connection to HUAWEI E303, E3531 and etc.),

6) Serial.h/cpp - support physical (via RS-232) or virtual (via USB) COM-port channel,

7) PDUEncoder.h/cpp - for generate PDU section in commands: AT+CUSD and AT+CMGW (not using for HiLink 3G modems),

8) IGSMDevice - interface (abstract class) for represent general functions, need for SMS-sending, in the GSM-device.

See ITU-T specifications: ETSI, GSM 07.07.
