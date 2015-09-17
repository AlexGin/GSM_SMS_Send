#pragma once

#include "stdafx.h"

// Interfaces for object-model of GSM-device

typedef map<int, PTR_SMS_MASS_RECORD> SMS_MASS_MAP;
typedef CTypedPtrArray<CObArray, CPBRecord*> CPBRecordsArray;

class ISMSSender;
class IPBkManager;

class IGSMDevice // It is general GSM-device
{
public:
	virtual bool GSMDeviceConnect()=0;
	virtual void ConnectionDelete()=0;
	virtual bool IsGSMDeviceConnected() const=0;
	virtual long StartInit(CWnd* pWnd, bool bSielent)=0;
	virtual void USSDSend(CString& strUSSDText, bool bPhoneOnLine)=0;
	virtual long SendSmsMass(ISMSSender* pSender)=0;
	virtual bool IsPhoneBookPresent()=0;
	
	virtual CString GetManufacturer()=0;
	virtual CString GetModel()=0;
	virtual CString GetRevision()=0;
	virtual CString GetIMEI()=0;
	virtual CString GetSpecialInfo()=0;
};

class IGSMDeviceHTTP : public IGSMDevice // It is GSM-device, connected with PC via HTTP protocol
{
public:
	virtual bool Connect(bool bDeviceInfoMode=false)=0;
	virtual bool ReConnect(bool bWaitCursor=false)=0;
	virtual bool IsNeedReConnect()=0;
	virtual void ShowHiLinkSMSList()=0;
public:
	virtual long GETProcessing(LPCTSTR szBuffer, bool bDeviceOnLine)=0;
	virtual long POSTProcessing(LPCTSTR szBuffer)=0;
	virtual void POSTSentOK(bool bSetMode)=0;
public:
	virtual long GetGetReqMode() const=0;
	virtual long GetPostReqMode() const=0;
	virtual void SetGetReqMode(long lGetReqMode)=0;
	virtual void SetPostReqMode(long lPostReqMode)=0;
	virtual UINT GetUssdRepeatCount() const=0;
	virtual void SetUssdRepeatCount(UINT nUssdRepeatCount)=0;
	virtual CString GetToken() const=0;
};

class ISMSSender
{
public:
	virtual HWND GetSenderHWND() const=0; // HWND of sender (usually the CView descendant) class
	virtual HWND GetSenderHistoryHWND() const=0; // HWND of sender history (usually the CView descendant) class
	virtual long GetTotalSections() const=0; // Total sections (for SendSmsMass) - only for CATCmdDevice and descendant
	virtual bool IsBlockedRecord(LPCTSTR szName, LPCTSTR szPhone=NULL)=0;
	virtual UINT GetRecordID() const=0; // It is RecordID of text-record in data-base (table "ID_Sent_SMS")
	virtual CString PrepareCurrPart(CString strSmsText, int iCurPart)=0;
	virtual CString GetSmsText() const=0;
	virtual SMS_MASS_MAP& GetSmsMassMap() const=0;
};

class ISMSReceiver
{
public:
	virtual void ResetSMSListFull()=0; 
	virtual long SMSArchiveRawPopulate(CStringArray* pStrArr)=0;
	virtual long SMSArchiveLoopPopulate(CStringArray* pStrArr)=0;
	virtual bool IsReceivedSMSListFull() const=0;
	virtual long SMSDelete(long lIndex, long lAddIndex)=0;
};

class IGSMDeviceATCmd : public IGSMDevice, public ISMSReceiver // It is GSM-device, connected with PC via COM/USB and use AT-Commands
{
public:
	virtual long OpenSerial(int iComChannel, int iComBaudRate)=0;
	virtual long ReOpenComPort()=0;
	virtual bool PurgeComPort(DWORD dwFlags)=0; // Added 17.06.2015
	virtual void CloseSerial()=0;
	virtual bool GetUnicodeMode()=0;
	virtual void SetUnicodeMode(bool bUnicodeMode)=0;
	virtual void UnicodeOn( bool bSilent=false)=0;
	virtual void UnicodeOff(bool bSilent=false)=0;
// PhoneBook support:
public:
	virtual UINT ClearPBk(IPBkManager* pPBkMan)=0;
	virtual UINT WritePBk(IPBkManager* pPBkMan)=0;
	virtual UINT ReadPBk(IPBkManager* pPBkMan)=0;
};

class IPBkManager
{
public:
	virtual HWND GetManagerHWND() const=0; // HWND of PhoneBook manager (usually the CView descendant) class
	virtual bool IsUnicodeOn()=0;
	virtual bool IsNewSAMSUNGOn()=0;
	virtual void ClearErrorsCounter()=0;
	virtual int* GetPtrErrorsCounter()=0;
	virtual CPBRecordsArray& GetRecordsArray()=0;
};
