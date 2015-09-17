#pragma once

#include "PhoneBook.h"
#include "SMSContent.h"

//////////////////////////////////////////////////////////////////////////////////////
// This file support AT-Command GSM devices (GSM mobile phones and GSM classic modems)
//////////////////////////////////////////////////////////////////////////////////////

// CATCmdDevice (the CATCmdDevice - it is former CGSMPhone)
const int SIZE_TX_BUFF = 16384; //1024;
const int SIZE_RX_BUFF = 16384; //1024;

class CATCmdDevice : public IGSMDeviceATCmd
{
	class CSerial* m_pSerial; 
	class CPBRecord* m_pPBRecord;  // Pointer to the current PhoneBook record

	TCHAR* m_pTXBuff;  // Transmitter buffer
	TCHAR* m_pRXBuff;  // Reciver buffer

	void CorrectRxBuff(TCHAR* szRxBuff); // Added 06.04.2014

	CString m_strManufacturer;  // For example: SIEMENS
	CString m_strModel;         // For example: ME45
	CString m_strRevision;
	CString m_strIMEI;
	CString m_strCBC;
	CString m_strSpecInfo; // Added 17.03.2015

	CString m_strPBType;	// Mode (TYPE) of PhoneBook selection: "SM", "ME", "MT"...
	
	bool m_bUnicodeMode;
	bool m_bIsGSMDeviceConnected; // Added 21.03.2015
public:
	CATCmdDevice();
	virtual ~CATCmdDevice(); 

public:
	virtual long StartInit(CWnd* pWnd, bool bSielent); // Corrected 21.07.2014
	virtual bool GSMDeviceConnect(){ return true; } // DEBUG !!!
	virtual void USSDSend(CString& strUSSDText, bool bPhoneOnLine); //  Added 17.03.2015
	virtual bool IsPhoneBookPresent(){return true;} // Added 17.03.2015
	virtual bool IsGSMDeviceConnected() const { return m_bIsGSMDeviceConnected; } // Added 21.03.2015
public:
	virtual long SendSmsMass(ISMSSender* pSender);

public:
	virtual CString GetManufacturer(){return m_strManufacturer;} 
	virtual CString GetModel(){return m_strModel;} 
	virtual CString GetRevision(){return m_strRevision;} 
	virtual CString GetIMEI(){return m_strIMEI;} 
	virtual CString GetSpecialInfo(){return m_strSpecInfo;}
public:
	virtual long OpenSerial(int iComChannel, int iComBaudRate);
	virtual long ReOpenComPort();
	virtual bool PurgeComPort(DWORD dwFlags);
	virtual void ConnectionDelete();
	virtual void CloseSerial();
	virtual bool GetUnicodeMode(){return m_bUnicodeMode;}
	virtual void SetUnicodeMode(bool bUnicodeMode){m_bUnicodeMode = bUnicodeMode;}

// PhoneBook support (NOT implemented in this class):
public: // Intend for eliminate error C2259 (cannot instantinate abstract class for CGSMModemClassic)
	virtual UINT ClearPBk(IPBkManager* pPBkMan) { return 0; }
	virtual UINT WritePBk(IPBkManager* pPBkMan) { return 0; }
	virtual UINT ReadPBk(IPBkManager* pPBkMan) { return 0; }

protected:
	INT_MAP m_mapSMSValidStrings; // Support received SMS list (added 08.10.2014)
	bool	m_bReceivedSMSListFull; // Support received SMS list (added 08.10.2014) 

public: // SMS-list (received SMS) support (restored 01.10.2014):
	virtual long SMSArchiveRawPopulate(CStringArray* pStrArr); // Added 06.10.2014
	virtual long SMSArchiveLoopPopulate(CStringArray* pStrArr); // Corrected 06.10.2014
	virtual bool IsReceivedSMSListFull() const; // Added 08.10.2014
	virtual void ResetSMSListFull(); // Added 08.10.2014
	virtual long SMSDelete(long lIndex, long lAddIndex); // Added 18.03.2015
	virtual void UnicodeOn(bool bSilent = false);	// Added 21.07.2014
	virtual void UnicodeOff(bool bSilent = false);	// Added 21.07.2014
public:
	long FillRawSMS(CString& strRawSMS, CStringArray* pStrArr); 
	long DecodePDUString(CString& strPDU, CSMSContent* pSMSContent, STRING_MAP& mapOut, BOOL* pTextSMSReady); // Corrected 06.10.2014
	void DecodeSMSDateTime(LPCTSTR szPDU, CSMSContent* pSMSContent, int iDTStartIndex);
	CSMSContent* RetrieveEqualSMSContent(CSMSContent* pSMSContent); // Added 06.10.2014
	int  RetrieveValidIndex(long lIndex); // Added 08.10.2014
	long GetReceivedSMSArrSize();
	void ClearSMSArr();
	
	CSMSContent* GetReceivedSMS(long lSMSIndex);  // Indexes started from 1... 
	CString DecodeSMSPNum(LPCTSTR szPDU, int iPNumStartIndex, int iPNumLength);
	SMS_CONTENT_VECTOR m_SMSContentVector;  // Archive of received SMS (corrected 08.10.2014)
	
	long PrepareCUSDAnswer(CString& strCUSDAnswer);

public: // SMS-list (received SMS) support:
	bool ProcessTextSMSMap(STRING_MAP& mapTextSMS, CString& strTextSMS); // Added 04.10.2014 
	bool IsMultipartSMS(int iStartIndex, char* szPDU); // Added 04.10.2014
	long GetTotalParts(int iStartIndex, char* szPDU, long* pCurrentPart, int* pSMSMPID); // Added 04.10.2014
	int GetMaxSMSCount(CString strInp, LPCTSTR szStorage, int* ptrCurCount); // Added 04.10.2014 (corrected 13.10.2014)
	void FillStorageType(CString strInp, LPSTR szStorage); // Added 13.10.2014
public: // 3G modem HUAWEI support (05.07.2014):
	long GetSpecialAnswer(CString& strATCmd); // Added 05.07.2014

public:
	long ConvertStringToMBCS(CString& strString);  
	long ConvertStringToWChar(CString& strString);

	long ConvertPBStringToMBCS();  
	long ConvertPBStringToWChar();
	long ConvertPBStringToUCS2(); // Added 14.10.2014 (for support SAMSUNG smartphone)
	
	long GetMaxIndex(long* ptrMaxIndex);  
	long GetCBCState(long* ptrCBCState);
	long SendCommand(CString& strATCmd, int iMd); 
	long GetAnswer(CString& strATCmd); 
	CString CorrectAnswer(TCHAR* pSzBuff); // Added 01.08.2014
	CString ClearIMEI(CString& strInp); // Added 02.08.2014
	CString ClearString(CString& strInp, bool bSpecMode = false); 
	CString ClearSCAString(CString& strInp); // Added 10.06.2015
	CString GetCBCString(){return m_strCBC;} 

	void InitTX();
	void InitRX();
	void Waiting();

	long ConvertString_7Bits(CString& strString);
	long ConvertString_UCS2(CString& strString);

	TCHAR ToHex(TCHAR chInp); 
	TCHAR ToNibble(TCHAR chInp);

	bool IsWithoutError(CString strStr); // Added 10.04.2014

	CString GetPBType(){return m_strPBType;} 
	void SetPBType(CString strPBType){m_strPBType = strPBType;}  
	
	long ProcGetPBRecord(long nIndOfRecord,  long lMaxIndex, int* pErrCnt); // Corrected 17.10.2014  
	long ProcSetPBRecord(long nIndOfRecord, long lMaxIndex, int* pErrCnt, bool bDelRec = false); // Corrected 17.10.2014
	class CPBRecord* GetPBRecord(){return m_pPBRecord;} 
	void SetPBRecord(class CPBRecord* pPBRecord){m_pPBRecord = pPBRecord;}
};
