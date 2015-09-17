#pragma once
#include "ATCmdDevice.h"

///////////////////////////////////////////////////////////////
// This file support classic GSM modems (old school 3G modems)
///////////////////////////////////////////////////////////////

class CGSMModemClassic : public CATCmdDevice
{
	CString m_strCUSDRawResult; // Result without PDU decoding (added 02.07.2014)
public:
	CGSMModemClassic(void);
	virtual ~CGSMModemClassic(void);
public:
	virtual bool IsPhoneBookPresent(){ return false; }
	virtual void USSDSend(CString& strUSSDText, bool bPhoneOnLine); //  Added 18.03.2015
protected:
	long PrepareCUSD3GPCUIAnswer(CString& strCUSDAnswer); // Huawei 3G classic modem support (added 04.07.2014)
};
