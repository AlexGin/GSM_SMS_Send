#pragma once
#include "PhoneBook.h"

////////////////////////////////////////////////////////////////
// This file support HiLing GSM modems (contemporary 3G modems)
////////////////////////////////////////////////////////////////

class CGSMModemHiLink :	public IGSMDeviceHTTP
{
private:
	STRING_VECTOR	m_vectValidPhonePrefix;
	void PrepareForTen(LPCTSTR szPhoneCode, TCHAR chExclude = 0x00);
	bool IsPhonePrefixValid(std::string sPhoneCode);
private:
	// String m_strManufacturer - already is: HUAWEI
	CString m_strModel;    // For example: E303
	CString m_strRevision;
	CString m_strIMEI;
	CString m_strSpecInfo; // MAC address
	bool m_bIsGSMDeviceConnected; // Added 21.03.2015
protected:
	short CalculateTotalForHiLink(ISMSSender* pSender); // Added 20.02.2015 (corrected 19.03.2015)
	void  PrepareDeviceTechInfo(); // Added 21.03.2015
	bool  PhoneNumValidation(CString* pStrPhoneNum); // Added 21.03.2015
public:
	CGSMModemHiLink(void);
	virtual ~CGSMModemHiLink(void);
public:
	virtual bool GSMDeviceConnect();
	virtual long StartInit(CWnd* pWnd, bool bSielent);
	virtual void USSDSend(CString& strUSSDText, bool bPhoneOnLine);
	virtual bool IsPhoneBookPresent(){ return false; }
	virtual bool IsGSMDeviceConnected() const { return m_bIsGSMDeviceConnected; } // Added 21.03.2015
	virtual void ConnectionDelete(); // Added 21.03.2015
public:
	virtual long SendSmsMass(ISMSSender* pSender);
public:	
	virtual CString GetManufacturer(){return "HUAWEI";} // 3G HiLink HUAWEI modem
	virtual CString GetModel(){return m_strModel;} 
	virtual CString GetRevision(){return m_strRevision;} 
	virtual CString GetIMEI(){return m_strIMEI;} 
	virtual CString GetSpecialInfo(){return m_strSpecInfo;}
public:
	virtual long GETProcessing(LPCTSTR szBuffer, bool bDeviceOnLine);
	virtual long POSTProcessing(LPCTSTR szBuffer);
	virtual void POSTSentOK(bool bSetMode);
public:
	virtual UINT GetUssdRepeatCount() const;
	virtual void SetUssdRepeatCount(UINT nUssdRepeatCount);
public:
	virtual long GetGetReqMode() const;
	virtual long GetPostReqMode() const;
	virtual void SetGetReqMode(long lGetReqMode);
	virtual void SetPostReqMode(long lPostReqMode);
public:
	virtual bool Connect(bool bDeviceInfoMode=false);
	virtual bool ReConnect(bool bWaitCursor=false);
	virtual bool IsNeedReConnect();
	virtual void ShowHiLinkSMSList(); // Added 03.02.2015
	virtual CString GetToken() const;
protected:
	long m_nGetReqMode; // HiLink support (29.01.2014)
	long m_nPostReqMode; // HiLink support (30.01.2014)
};
