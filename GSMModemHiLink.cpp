#include "StdAfx.h"
#include "PhoneBook.h"
#include "GSMModemHiLink.h"
#include "ClientWinInet.h"

extern CLogFilePBk g_log;

extern CEvent g_evntReady; 
extern CEvent g_evntStop;

// Next ten lines were added 29.01.2015 and 30.01.2015 (3G modem HiLink support):
extern CString g_strServerName;  
extern CString g_strFile;  
extern CString g_strToken;
extern CXMLDecoder g_xmlDecode;  
extern CXMLEncoder g_xmlEncode;  
extern CHttpConnection* g_pConnection;
extern CEvent g_evntPOSTResOK;
extern CEvent g_evntWinInetStartReq; 
extern SendResponse g_sendResponse;
extern std::string g_sDataPost;

// Next four lines - USSD via HiLink support (added 03.02.2015):
extern CEvent g_evntPOSTRequest; 
extern CEvent g_evntPOSTSentOK;
extern CEvent g_evntGETRequest;
extern UINT g_nUssdRepeatCount;

extern CTime g_timeLastRequest; // HiLink support (added 09.02.2015):

extern CInternetSession* g_pSession; // HiLink support (added 05.02.2015) 

CGSMModemHiLink::CGSMModemHiLink(void)
 : m_nGetReqMode(WIC_GET_GENERAL), m_nPostReqMode(0L), m_bIsGSMDeviceConnected(false)
{
	// See: http://blackcat.deal.by/a9084-mobilnye-kody-belarusi.html
	// and: http://soligorsk.me/info/show/4/telefonnye-kody-mobilnyh-operatorov-belarusi
	// for valid phone-codes for GSM mobile operators (BY)
	std::string s1 = "+37529";
	PrepareForTen(s1.c_str(), 0x34); // Digit "4" for exclude "+375294"

	std::string s2 = "+37544";
	PrepareForTen(s2.c_str());

	std::string s3 = "+37533";
	PrepareForTen(s3.c_str());

	std::string s4 = "+37525";
	PrepareForTen(s4.c_str());
}

CGSMModemHiLink::~CGSMModemHiLink(void)
{
	m_vectValidPhonePrefix.clear();
}

UINT CGSMModemHiLink::GetUssdRepeatCount() const
{
	return ::g_nUssdRepeatCount;
}

void CGSMModemHiLink::PrepareForTen(LPCTSTR szPhoneCode, TCHAR chExclude)
{
	TCHAR szLastDigit[2];
	CString strFullPhoneCode;
	for (int i = 0; i < 10; i++)
	{
		TCHAR chLast = 0x30 + i;
		if (chExclude == chLast)
			continue;

		memset((void*)szLastDigit, 0, 2);
		szLastDigit[0] = chLast;
		strFullPhoneCode.Format("%s%s", szPhoneCode, szLastDigit);

		std::string sFullPhoneCode = (CStringA)strFullPhoneCode;
		m_vectValidPhonePrefix.push_back(sFullPhoneCode);
	}
}

bool CGSMModemHiLink::PhoneNumValidation(CString* pStrPhoneNum)
{
	if (!pStrPhoneNum)
		return false;

	CString strInputPhoneNum = *pStrPhoneNum;

	TCHAR szPhonePrefix[8];
	memset((void*)szPhonePrefix, 0, 8);
	for (int i = 0; i < 7; i++)
	{
		TCHAR chCurr = strInputPhoneNum[i];
		szPhonePrefix[i] = chCurr;
	}
	szPhonePrefix[7] = '\0';
	std::string sPhonePrefix(szPhonePrefix);
	return IsPhonePrefixValid(sPhonePrefix);
}

bool  CGSMModemHiLink::IsPhonePrefixValid(std::string sPhoneCode)
{
	for(STRING_VECTOR::const_iterator cit=m_vectValidPhonePrefix.begin();cit!=m_vectValidPhonePrefix.end();cit++)
	{
		std::string s = *cit;
		if (s == sPhoneCode)
			return true;
	}
	return false;
}

void CGSMModemHiLink::SetUssdRepeatCount(UINT nUssdRepeatCount)
{
	::g_nUssdRepeatCount = nUssdRepeatCount;
}

CString CGSMModemHiLink::GetToken() const
{
	return ::g_strToken;
}

long CGSMModemHiLink::GetGetReqMode() const
{
	return m_nGetReqMode;
}

long CGSMModemHiLink::GetPostReqMode() const
{
	return m_nPostReqMode;
}

void CGSMModemHiLink::SetGetReqMode(long lGetReqMode)
{
	m_nGetReqMode = lGetReqMode;
}

void CGSMModemHiLink::SetPostReqMode(long lPostReqMode)
{
	m_nPostReqMode = lPostReqMode;
}

void CGSMModemHiLink::POSTSentOK(bool bSetMode)
{
	if (bSetMode)
		::g_evntPOSTSentOK.SetEvent();
	else
		::g_evntPOSTSentOK.ResetEvent();
}

short CGSMModemHiLink::CalculateTotalForHiLink(ISMSSender* pSender)
{
	short nTotalCounter = 0;
	SMS_MASS_MAP& mapSmsMass = pSender->GetSmsMassMap();
	for (SMS_MASS_MAP::const_iterator cit=mapSmsMass.begin();cit!=mapSmsMass.end();cit++)
	{
		SMS_MASS_PAIR pair = *cit;
		PTR_SMS_MASS_RECORD pSmsMassRecord = pair.second;
		int iFlag = pSmsMassRecord->m_iFlag;
		if (iFlag != 1)
			continue;	

		bool bBlocked = pSender->IsBlockedRecord((LPCTSTR)pSmsMassRecord->m_strName); // Added 06.11.2014
		if (bBlocked) 
			continue;

		nTotalCounter++;
	}
	return nTotalCounter;
}

long CGSMModemHiLink::StartInit(CWnd* pWnd, bool bSielent)
{
	CWinThread* pWinThread = ::AfxBeginThread(CClientWinInet::WinInetThreadBegin, LPVOID(pWnd));
	DWORD dwThreadID = pWinThread->m_nThreadID;
	::g_log.SaveLogFile(PBK_DEBUG, "CClientWinInet::WinInetThreadBegin dwThreadID = %u",dwThreadID);

	::g_evntWinInetStartReq.SetEvent();
	return 0L;
}

void CGSMModemHiLink::PrepareDeviceTechInfo()
{
	std::string sDeviceName = ::g_xmlDecode.GetTagValue("DeviceName");
	std::string sHVersion   = ::g_xmlDecode.GetTagValue("HardwareVersion");
	std::string sImei		= ::g_xmlDecode.GetTagValue("Imei");
	std::string sMacAddress = ::g_xmlDecode.GetTagValue("MacAddress1");

	m_strModel.Format("%s", sDeviceName.c_str());
	m_strRevision.Format("%s", sHVersion.c_str());
	m_strIMEI.Format("%s", sImei.c_str());
	m_strSpecInfo.Format("%s", sMacAddress.c_str());
}

bool CGSMModemHiLink::GSMDeviceConnect()
{
	bool bIsConnectOK = Connect(true);
	if (bIsConnectOK)
		PrepareDeviceTechInfo();

	return bIsConnectOK;
}

bool CGSMModemHiLink::Connect(bool bDeviceInfoMode)
{
	CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();
	CWnd* pWnd = pApp->GetMainWnd();

	m_nGetReqMode = WIC_GET_GENERAL; // General GET or connection status
	::g_strServerName = pApp->GetServerIPAddr();

	if (!CClientWinInet::SetConnection())
	{
		::g_strFile = "/api/monitoring/status";
		CClientWinInet::ClientWinInetGETProc(pWnd->m_hWnd, (int)m_nGetReqMode); // General GET or connection status
		std::string sConnStat = ::g_xmlDecode.GetTagValue("ConnectionStatus");
		if (sConnStat.length() > 0)
			g_log.SaveLogFile(PBK_DEBUG, "Server (IP=%s) ConnectionStatus: %s", ::g_strServerName, sConnStat.c_str());
		else
			g_log.SaveLogFile(PBK_ERROR, "Server (IP=%s) Connection ERROR!", ::g_strServerName);

		if (::g_pConnection && (sConnStat.length() > 0))
		{	// Connection - SUCCESS!!!
			m_bIsGSMDeviceConnected = true;
			m_nGetReqMode = WIC_GET_TOKEN; // Get Token
			CClientWinInet::ClientWinInetGETProc(pWnd->m_hWnd, (int)m_nGetReqMode); // Get Token
			
			// Next five lines were added 17.03.2015:
			if (bDeviceInfoMode)
			{
				m_nGetReqMode =  WIC_GET_DEVICE_INFO; // Get device information
				CClientWinInet::ClientWinInetGETProc(pWnd->m_hWnd, (int)m_nGetReqMode); // Get device information
			}

			return true; // OK!
		}
	} 
	return false; // Error
}

bool CGSMModemHiLink::ReConnect(bool bWaitCursor)
{
	if (bWaitCursor)
		AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor

	g_log.SaveLogFile(PBK_DEBUG, "CMainFrame::ReConnectHiLink");
	
	CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();
	
	ConnectionDelete();		
	Connect();
	bool bPhoneOnLine = IsGSMDeviceConnected();
	if (bPhoneOnLine)
		pApp->m_iErrorFlag = 0; // Success
		
	if (bWaitCursor)
		AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor

	return bPhoneOnLine;
}

bool CGSMModemHiLink::IsNeedReConnect()
{
	CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();
	int iMins = pApp->m_iHTTPRecMinutes;
	if (!iMins)
		return FALSE;

	CTimeSpan tsMinutes(0, 0, iMins, 0);

	CTime time = CTime::GetCurrentTime();
	CTimeSpan tsFromLast = time - ::g_timeLastRequest;

	return (bool)(tsFromLast > tsMinutes);
}

void CGSMModemHiLink::ConnectionDelete()
{
	int iCnt = 0;
	if (::g_pConnection) 
	{
		::g_pConnection->Close();
		delete ::g_pConnection;
		::g_pConnection = NULL;
		iCnt++;
	}

	if (::g_pSession)
	{
		delete ::g_pSession;
		::g_pSession = NULL;
		iCnt++;
	}
	if (iCnt)
		g_log.SaveLogFile(PBK_DEBUG, "HiLink Connection - delete!");

	m_bIsGSMDeviceConnected = false;
} 

void CGSMModemHiLink::USSDSend(CString& strUSSDText,  bool bPhoneOnLine)
{
	CWnd* pWnd = AfxGetApp()->GetMainWnd();
	if(!bPhoneOnLine)
	{
		::MessageBox(pWnd->m_hWnd, "GSM 3G HiLink модем не подключен!", "PhoneBook", MB_OK | MB_ICONINFORMATION);
		return;
	}

	AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor

	if (IsNeedReConnect()) // Because flag INTERNET_FLAG_KEEP_CONNECTION is not support on 3G HiLink modems
	{
		g_log.SaveLogFile(PBK_DEBUG, "CGSMModemHiLink::USSDSend: HTTPRecMinutes expired - starting ReConnectHiLink");
		ReConnect();
	}

	::g_sendResponse.sUssdText = ""; // Empty string

	CString strUSSDToSend = strUSSDText;
	CPhoneBookApp::ShowUSSDOutput(&strUSSDToSend, 0L); // 0L-Request

	::g_xmlEncode.PrepareUSSDText(strUSSDToSend);
 
	m_nPostReqMode = WIC_POST_USSD;
	::g_sDataPost = ::g_xmlEncode.GetEncodedText();

	::g_evntPOSTRequest.SetEvent();
	::g_evntPOSTResOK.ResetEvent();
	::g_evntPOSTSentOK.ResetEvent();	

	MSG msg; 
	int iCode; 
	int iWaitingSecCount = 5; // Waiting - 5 sec
	do
	{  while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{	// Loop of message-processing:	 
			::DispatchMessage(&msg);
		}
		AfxGetApp()->DoWaitCursor(0);
		iCode = ::WaitForSingleObject(::g_evntPOSTResOK.m_hObject, 1000); // 1000 ms 
		iWaitingSecCount--;		
	}while ((iCode != WAIT_OBJECT_0) && (iWaitingSecCount > 0));

	iWaitingSecCount = 5; // Waiting - 5 sec
	if (iCode == WAIT_OBJECT_0)
	{
		m_nGetReqMode = WIC_GET_RESPONSE_USSD; // Get response after USSD was sent
		::g_evntGETRequest.SetEvent();
		do
		{  while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{	// Loop of message-processing:	 
				::DispatchMessage(&msg);
			}
			AfxGetApp()->DoWaitCursor(0);
			iCode = ::WaitForSingleObject(::g_evntPOSTSentOK.m_hObject, 1000); // 1000 ms 
			iWaitingSecCount--;			
		}while ((iCode != WAIT_OBJECT_0) && (iWaitingSecCount > 0));
	}

	CString s22; s22.Format("%s", (LPCTSTR)::g_sendResponse.sUssdText.c_str()); 
	CString strLog0; strLog0.Format("USSD 3G HiLink Modem answer: %s", (LPCTSTR)::g_sendResponse.sUssdText.c_str());
	g_log.SaveLogFile(PBK_DEBUG, (CString&)strLog0); 

	AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor //EndWaitCursor();
	CPhoneBookApp::ShowUSSDOutput(&s22, 1L); // 1L-Answer
}

long CGSMModemHiLink::GETProcessing(LPCTSTR szBuffer, bool bDeviceOnLine)
{
	long nResult = 0L;
	if (bDeviceOnLine) // When connection established:
	{
		::g_xmlDecode.ParseInputErrorString(szBuffer);
		std::string sErrCode = ::g_xmlDecode.GetTagValue("error_code");
		if (sErrCode.length() > 0)
		{
			g_log.SaveLogFile(PBK_ERROR, "HTTP Error code: %s", sErrCode.c_str());
			nResult = 1L; // Error occur
		}
	}
	else // On start, connection status prepare:
		::g_xmlDecode.ParseConnectionString(szBuffer);

	return nResult;
}

long CGSMModemHiLink::POSTProcessing(LPCTSTR szBuffer)
{
	long nResult = 0L;
	::g_xmlDecode.ParseConnectionString(szBuffer, true);
	std::string sResponse = ::g_xmlDecode.GetTagValue("response");
	if (sResponse == "OK")
		::g_evntPOSTResOK.SetEvent();
	else
		nResult = 1L; // Error occur

	g_log.SaveLogFile(PBK_DEBUG, "MainFrm - Response: %s", sResponse.c_str());

	return nResult;
}

void CGSMModemHiLink::ShowHiLinkSMSList()
{
	CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();

	CString strWebAddr;
	strWebAddr.Format("http://%s/html/smsinbox.html?smsinbox", (LPCTSTR)pApp->GetServerIPAddr());
	ShellExecute(NULL, "open", (LPCTSTR)strWebAddr, NULL, NULL, SW_SHOWNORMAL);
}

long CGSMModemHiLink::SendSmsMass(ISMSSender* pSender)
{
	HWND hWndSender = pSender->GetSenderHWND();	
	try
	{
		CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();
		
		SetPostReqMode(WIC_POST_SMS);	
		SetGetReqMode(WIC_GET_RESPONSE_SMS); // Get response after SMS was sent

		int iReconnectCounter = 0; // Added 08.04.2015
		short nTotal = CalculateTotalForHiLink(pSender); // Total count of SMS (to send via HiLink)
		bool bFirstPass = true;
		bool bErrorFlag = false;
		long lCurrPhone = 0L; 
		CString strSmsText = pSender->GetSmsText();
		CString strTextInfo; strTextInfo.Format("SMS text: %s", strSmsText);
		g_log.SaveLogFile(SMS_MASS, (CString&)strTextInfo);
		SMS_MASS_MAP& mapSmsMass = pSender->GetSmsMassMap();
		for (SMS_MASS_MAP::const_iterator cit=mapSmsMass.begin();cit!=mapSmsMass.end();cit++)
		{
			SMS_MASS_PAIR pair = *cit;
			// int iIndex = pair.first;
			PTR_SMS_MASS_RECORD pSmsMassRecord = pair.second;
			CString strName = pSmsMassRecord->m_strName; 
			CString strPhoneNum = pSmsMassRecord->m_strPhone;

			int iFlag = pSmsMassRecord->m_iFlag;
			if (iFlag != 1)
				continue;	

			bool bBlocked = pSender->IsBlockedRecord((LPCTSTR)pSmsMassRecord->m_strName); // Added 06.11.2014
			if (bBlocked) 
				continue;

			if (pApp->m_iModemHiLinkFilter) // Added 10.04.2015 (ONLY for HiLink!!!)
			{
				bool bValidPhoneNum = PhoneNumValidation(&strPhoneNum); // Added 21.03.2015 (ONLY for HiLink!!!)
				if (!bValidPhoneNum)
				{
					g_log.SaveLogFile(PBK_DEBUG, "CGSMModemHiLink::SendSmsMass: Invalid phone number: %s - sending SMS canceled!", strPhoneNum);
					continue;
				}
			}

			COleDateTime odtNow = COleDateTime::GetCurrentTime();
		
			::g_xmlEncode.PrepareSMSText(strPhoneNum, strSmsText, odtNow); 
			::g_sDataPost = ::g_xmlEncode.GetEncodedText();

			::g_evntPOSTResOK.ResetEvent();
			CClientWinInet::ClientWinInetPOSTProc(hWndSender, (int)GetPostReqMode());

			MSG msg; 
			int iCode;			
			int iWaitingSecCount = 5; // Waiting - MAX 5 sec
			do
			{   while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{	// Loop of message-processing: 	 
					::DispatchMessage(&msg);
				}
				iCode = ::WaitForSingleObject(::g_evntPOSTResOK.m_hObject, 1000); // 1000 ms   
				iWaitingSecCount--;
			}while ((iCode != WAIT_OBJECT_0) && (iWaitingSecCount > 0));
			if (iCode != WAIT_OBJECT_0)
			{
				bErrorFlag = true;
				g_log.SaveLogFile(PBK_ERROR, "SendSMSMassHiLink - Timeout ERROR after POST");
				if (iReconnectCounter >= 5) // Added 08.04.2015
				{
					g_log.SaveLogFile(PBK_ERROR, "SendSMSMassHiLink - Reconnect counter expired(1)");
					break;
				}
				// Next five lines were added 08.04.2015:
				::Sleep(3000); // Timeout 3 sec
				ReConnect();	
				SetGetReqMode(WIC_GET_RESPONSE_SMS); // Get response after SMS was sent
				iReconnectCounter++; 
				g_log.SaveLogFile(PBK_DEBUG, "SendSMSMassHiLink - Reconnect(1) executed");
			}

			g_log.SaveLogFile(PBK_DEBUG, "SendSMSMassHiLink: POSTResOK - ready (PhoneNum: %s)", strPhoneNum);

			::g_evntPOSTSentOK.ResetEvent();
			CClientWinInet::ClientWinInetGETProc(hWndSender, GetGetReqMode());
			
			iWaitingSecCount = 5; // Waiting - MAX 5 sec
			do
			{   while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{	// Loop of message-processing: 	 
					::DispatchMessage(&msg);
				}
				iCode = ::WaitForSingleObject(::g_evntPOSTSentOK.m_hObject, 1000); // 1000 ms   
				iWaitingSecCount--;
			}while ((iCode != WAIT_OBJECT_0) && (iWaitingSecCount > 0));
			if (iCode != WAIT_OBJECT_0)
			{
				bErrorFlag = true;
				g_log.SaveLogFile(PBK_ERROR, "SendSMSMassHiLink - Timeout ERROR during waiting results of POST");
				if (iReconnectCounter >= 5) // Added 08.04.2015
				{
					g_log.SaveLogFile(PBK_ERROR, "SendSMSMassHiLink - Reconnect counter expired(2)");
					break;
				}
				// Next five lines were added 08.04.2015:
				::Sleep(3000); // Timeout 3 sec
				ReConnect();
				SetGetReqMode(WIC_GET_RESPONSE_SMS); // Get response after SMS was sent
				iReconnectCounter++; 
				g_log.SaveLogFile(PBK_DEBUG, "SendSMSMassHiLink - Reconnect(2) executed");
			}

			g_log.SaveLogFile(PBK_DEBUG, "SendSMSMassHiLink - SUCCESS (PhoneNum: %s)", strPhoneNum);
			iReconnectCounter = 0; // Added 08.04.2015 
			lCurrPhone++;
			long lCurrInfo = (long)((nTotal << 16) + (lCurrPhone & 0xffff));
			::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)1L, (LPARAM)lCurrInfo); 
			
			::SendMessage(hWndSender, MYWN_STEP_SMS_PHONE, (WPARAM)1L, (LPARAM)lCurrPhone);
			UINT nRecordID = pSender->GetRecordID();
			if (bFirstPass)
			{
				std::string sTextInfo = (CStringA)strTextInfo;
				g_log.SaveSMSSentLogFile(sTextInfo.c_str(), nRecordID);
			}
			g_log.SaveSMSSentLogFile(strPhoneNum, strName, nRecordID); 
			HWND hWndHistory = pSender->GetSenderHistoryHWND();
			::PostMessage(hWndHistory, MYWN_REFRESH_SMS_SENT_LIST, (WPARAM)0L, (LPARAM)0L);

			// Checking stop request:
			iCode = ::WaitForSingleObject(::g_evntStop.m_hObject, 0);
			if(iCode == WAIT_OBJECT_0)  // if stop request !
			{	// Reset stop-flag:
				::g_evntStop.ResetEvent();
				::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L); // End of SMSMass sending
				::g_evntReady.SetEvent(); 
				return 0;
			}
			bFirstPass = false;
		}
		::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L); // End of SMSMass sending
		::g_evntReady.SetEvent(); 
		return (bErrorFlag) ? 1 : 0;
	}
	catch(...)
	{
		// AfxMessageBox(_T("HiLink: ошибка рассылки SMS ."));
		::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L);
		::g_evntReady.SetEvent(); 
		::PostMessage(hWndSender, MYWN_SHOW_ERROR_FAILURE, (WPARAM)0L, (LPARAM)2L);
		return 1;
	}
	return 0L; // DEBUG !!!
}
