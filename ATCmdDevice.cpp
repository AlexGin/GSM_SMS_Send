// GSMPhone.cpp : implementation file
//

#include "stdafx.h"
#include "PhoneBook.h"
#include "PBRecord.h" 
#include "ATCmdDevice.h"
#include "Serial.h"	// RS232 support
#include "PDUEncoder.h"		// Added 19.03.2015
#include "GSMModemClassic.h" // Added 19.03.2015
#include <algorithm> // Added 08.10.2014 (std::sort support)

extern CEvent g_evntReady;
extern CEvent g_evntStop;

extern CLogFilePBk g_log; // Added 15.10.2014

bool SMSContentGreater(CSMSContent* pSMS1, CSMSContent* pSMS2) // Global function for std::sort
{
	CTime time1, time2;
	SYSTEMTIME st1, st2;
	if (pSMS1->GetSMSTime().GetAsSystemTime(st1))
		time1 = CTime(st1);
	if (pSMS2->GetSMSTime().GetAsSystemTime(st2))
		time2 = CTime(st2);

	time_t t1 = time1.GetTime();
	time_t t2 = time2.GetTime();

	if (t1 > t2)
		return true;
	else
		return false;
}

// CATCmdDevice (former CGSMPhone)

CATCmdDevice::CATCmdDevice()
: m_pSerial(NULL), // Added 17.03.2015
  m_bIsGSMDeviceConnected(false) // Added 21.03.2015
{
	m_pTXBuff = NULL;  // Transmitter buffer
	m_pRXBuff = NULL;  // Reciver buffer

	m_bUnicodeMode = false;

	m_pPBRecord = NULL;

	m_bReceivedSMSListFull = false;
}

CATCmdDevice::~CATCmdDevice()
{
	ClearSMSArr(); // Restored 01.10.2014

	if (m_pTXBuff)
	{
		delete[] m_pTXBuff;  
		m_pTXBuff = NULL;
	} 

	if (m_pRXBuff)
	{
		delete[] m_pRXBuff;  
		m_pRXBuff = NULL;
	}
}

void CATCmdDevice::InitTX()
{
	if (m_pTXBuff)
	{
		delete[] m_pTXBuff; 
		m_pTXBuff = NULL;
	}

	if (!m_pTXBuff)
	{
		m_pTXBuff = new TCHAR[SIZE_TX_BUFF];   // 
		::ZeroMemory((void*)m_pTXBuff, SIZE_TX_BUFF);
	}	
}

void CATCmdDevice::InitRX()
{
	if (m_pRXBuff)
	{
		delete[] m_pRXBuff; 
		m_pRXBuff = NULL;
	}

	if (!m_pRXBuff)
	{
		m_pRXBuff = new char[SIZE_RX_BUFF]; 
		::ZeroMemory((void*)m_pRXBuff, SIZE_RX_BUFF);
	}	
}

void CATCmdDevice::Waiting()
{
	MSG msg; 
	while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{	/* Loop of message-processing: */	 
		::DispatchMessage(&msg);
	}
}

long CATCmdDevice::OpenSerial(int iComChannel, int iComBaudRate)
{
	if (m_pSerial)
		delete m_pSerial;

	m_pSerial = new CSerial();
	if (m_pSerial != NULL)
	{   // m_pSerial->Open(1, 9600);
		BOOL bOpen = m_pSerial->Open(iComChannel, iComBaudRate); 
		if(!bOpen)
		{
			g_log.SaveLogFile(PBK_ERROR, "Error during opening port: COM%u (baud rate = %u)",iComChannel,iComBaudRate);
			return 1L; // Error occur
		}
		else
			g_log.SaveLogFile(PBK_DEBUG, "Port: COM%u (baud rate = %u) - opening success!",iComChannel,iComBaudRate);
	}
	return 0L; // OK!
}

long CATCmdDevice::ReOpenComPort()
{
	CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();
	if(m_pSerial != NULL)
	{	
		m_pSerial->Close();
		m_bIsGSMDeviceConnected = false;
		
		// Serial port initializing:
		/* m_pISerial->Open(1, 9600, &iOpen); */
		BOOL bOpen = m_pSerial->Open(pApp->m_iComChannel, pApp->m_iComBaudRate);
		if(!bOpen)
		{
			g_log.SaveLogFile(PBK_ERROR, "Error during reopening port: COM%u (baud rate = %u)", 
				pApp->m_iComChannel, pApp->m_iComBaudRate);

			pApp->m_iErrorFlag = ERROR_UNABLE_OPEN_SELECTED_PORT;
			// PrepareCOMErrorInfo(true, pApp->m_iComChannel);	
			return 1L;
		}
		else
		{
			g_log.SaveLogFile(PBK_DEBUG, "Port: COM%u (baud rate = %u) - reopening success!", 
				pApp->m_iComChannel, pApp->m_iComBaudRate);

			pApp->m_iErrorFlag = 0; // Success
			// PrepareCOMErrorInfo(true);
		}
	} 
	else
		return 1L;

	return 0L;
}

void CATCmdDevice::ConnectionDelete()
{
	CloseSerial();
}

void CATCmdDevice::CloseSerial()
{
	if (m_pSerial)
	{
		m_pSerial->Close();
		m_bIsGSMDeviceConnected = false;
	}
}

long CATCmdDevice::StartInit(CWnd* pWnd, bool bSielent)
{
	CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();
	try
	{
		long nLenInfo = 0L; 
		long nCBC = 0L;     // Current batary charge (%) 
		long lResult = 1L;  // If error occur
		
		CString s1 = _T("ATE0V0"); 
		SendCommand(s1, 1); 
		s1 = _T("");
		GetAnswer(s1); 

		CString s2 = _T("");
		s1 = _T("AT+CGMI");
		SendCommand(s1, 0); 
		GetAnswer(s2); 
		m_strManufacturer = ClearString(s2, true);

		s1 = _T("AT+CGMM"); 
		s2 = _T("");
		SendCommand(s1, 0); 
		GetAnswer(s2); 
		m_strModel = ClearString(s2, true); 

		s1 = _T("AT+CGMR"); 
		s2 = _T("");
		SendCommand(s1, 0); 
		GetAnswer(s2);
		m_strRevision = ClearString(s2, true);
	
		s1 = _T("AT+CGSN"); 
		s2 = _T("");
		SendCommand(s1, 0); 
		GetAnswer(s2);
		m_strIMEI = ClearIMEI(s2); // ClearString(s2, true); 
		if (2 == pApp->m_i3GModemUse) // Automatic using
		{
			long nNewMode = pApp->Retrieve3GModeViaIMEI(m_strIMEI);
			if ((0L == nNewMode) || (1L == nNewMode))
				pApp->m_i3GModemUse = (int)(nNewMode);
		}

		CGSMModemClassic* pMdm = dynamic_cast<CGSMModemClassic*>(this); 
		if (pMdm) // Huawei 3G modem classic (old school)
		{			
			CString s11;
			s11.Format("AT+CPIN=\"%u\"", pApp->m_iPINCodeVal);
			CString s22 = _T("");
			SendCommand(s11, 0); 
			GetAnswer(s22);
		}

		s1 = _T("AT+CSCS?"); 
		s2 = _T("");
		SendCommand(s1, 0); 
		GetAnswer(s2);
		CString strCurCS = ClearString(s2);     

		if (!pMdm) // GSM Phone
			GetCBCState(&nCBC);
		
		UnicodeOn(true); // Added 10.06.2015 
		// if ( (GetCBCState(&nCBC) || (!nCBC) ) && (!bSielent) )
		//	AfxMessageBox(_T("Невозможно определить процент заряда аккумулятора"));
				
		nLenInfo += m_strManufacturer.GetLength(); 
		nLenInfo += m_strModel.GetLength(); 
		nLenInfo += m_strRevision.GetLength(); 
		nLenInfo += m_strIMEI.GetLength(); 
		
		if(nLenInfo <= 10)
		{
			m_bIsGSMDeviceConnected = false;
			return lResult; // Devise is not connected!
		}

		lResult--; // return without error
		m_bIsGSMDeviceConnected = true;
		return lResult; 
	}
	catch(...)
	{
		TRACE("CATCmdDevice::StartInit: - error occur !\r\n");
		m_bIsGSMDeviceConnected = false;
		return 1L;  // Error occur
	}
}

CString CATCmdDevice::ClearIMEI(CString& strInp)
{
	CString strResult;
	long lCurChar = 0L; 
	char* szInput = new char[strInp.GetLength() + 1]; 

	::ZeroMemory((void*)szInput, (strInp.GetLength() + 1));
	lstrcpy(szInput, LPCTSTR(strInp));

	CString strTempResult;
	long lIMEILength  = 15L;
	for( long lIndex = 0; lIndex < strInp.GetLength(); lIndex++)
	{
		TCHAR tChar = szInput[lIndex];   // Current char

		if ((tChar >= 0x30) && (tChar <= 0x39))		// ONLY DIGIT (0...9)
		{
			strTempResult.Insert(lCurChar, tChar); 
			lCurChar++;
		}
		else // If no digit - search again:
		{
			strTempResult = "";
			lCurChar = 0L;
		}

		if (lCurChar == lIMEILength)
		{
			strResult = strTempResult;
			break;
		}
	}
	strResult.TrimLeft(0x20);
	strResult.TrimRight(0x20);

	delete[] szInput;
	return strResult;
}

CString CATCmdDevice::ClearSCAString(CString& strInp)
{
	CString strResult;
	long lCurChar = 0L; 
	char* szInput = new char[strInp.GetLength() + 1]; 

	::ZeroMemory((void*)szInput, (strInp.GetLength() + 1));
	lstrcpy(szInput, LPCTSTR(strInp));

	for( long lIndex = 0; lIndex < strInp.GetLength(); lIndex++)
	{
		TCHAR tPrevChar = 0;    // Previous char
		TCHAR tNextChar = 0;    // Next char
		TCHAR tChar = szInput[lIndex];   // Current char

		if (tChar != 0x22)							// Do not insert symbol: "
		{	
			if (tChar != 0x3F) // Do not insert symbol: '?'
			{
				strResult.Insert(lCurChar, tChar); 
				lCurChar++;
			}
		}
	}

	strResult.TrimLeft(0x20);
	strResult.TrimRight(0x20);

	delete[] szInput;
	return strResult;
}

CString CATCmdDevice::ClearString(CString& strInp, bool bSpecMode)
{	
	CString strResult;
	long lCurChar = 0L; 
	char* szInput = new char[strInp.GetLength() + 1]; 

	::ZeroMemory((void*)szInput, (strInp.GetLength() + 1));
	lstrcpy(szInput, LPCTSTR(strInp));

	for( long lIndex = 0; lIndex < strInp.GetLength(); lIndex++)
	{
		TCHAR tPrevChar = 0;    // Previous char
		TCHAR tNextChar = 0;    // Next char
		TCHAR tChar = szInput[lIndex];   // Current char

		if (tChar != 0x22)							// Do not insert symbol: "
		{	if (lIndex > 0L) 
				tPrevChar = szInput[lIndex - 1L]; 

			if (lIndex < (strInp.GetLength() - 1L))
				tNextChar = szInput[lIndex + 1L]; 

			if (tChar < 0x20) 
					tChar = 0x20; 

			if (tChar == 0x30)
			{   if((tPrevChar == 0x0d) || (tPrevChar == 0x0a))  // The "0" digit, after CR
					tChar = 0x20; 

				if (!bSpecMode)
				{
					if((tNextChar == 0x0d) || (tNextChar == 0x0a))  // The "0" digit, before CR 
						tChar = 0x20; 
				}
			} 

			strResult.Insert(lCurChar, tChar); 
			lCurChar++;
		} 
	}

	strResult.TrimLeft(0x20);
	strResult.TrimRight(0x20);

	delete[] szInput;
	return strResult;
}

long CATCmdDevice::SendCommand(CString& strATCmd, int iMd)
{
	CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp();

	// iMd == 0 - Do not waiting echo signal.
	// iMd == 1 - Waiting echo signal.
	InitTX();
	
	unsigned int counter2=0;
	unsigned int nBytesWrit, nBytesRead;

	lstrcpy(m_pTXBuff, LPCTSTR(strATCmd));
	int iLen = (int)(lstrlen(m_pTXBuff)); 
	m_pTXBuff[iLen] = 0x0d; // CR  
	// m_pTXBuff[iLen+1] = 0x0a; // LF 

	nBytesWrit = m_pSerial->SendData((const char*)m_pTXBuff, (iLen+1));

	Sleep(pApp->GetWaitTime());  //(100); //(300); //(1000);

	if (!iMd)   // Do not waiting echo signal
		return 0L;

	do   // Waiting echo signal:
	{
     counter2++;

	 nBytesRead = m_pSerial->ReadDataWaiting();

	 if(nBytesRead == nBytesWrit) break;	// If Burst in buffer
	 Sleep(100);							// else, wait 0.1 sec
	}while(counter2 < 30);
	
	if(nBytesRead == nBytesWrit)
	{
		InitRX();
		nBytesRead = m_pSerial->ReadData((void*)m_pRXBuff, iLen); 
	}
	return 0L;
}

void CATCmdDevice::CorrectRxBuff(TCHAR* szRxBuff)
{
	if (szRxBuff)
	{
		char* pSzTarget = strstr(szRxBuff, "OK");
		if (pSzTarget)
		{
			pSzTarget[0] = 0x30; // "0" symbol
			pSzTarget[1] = 0x0d; // CR special symbol
		}
	}
}

CString CATCmdDevice::CorrectAnswer(TCHAR* pSzBuff)
{
	CString strResult = "";
	bool bFlagIgnore = false;
	int iOutIndex = 0;
	for (int iIndex = 0; iIndex < SIZE_RX_BUFF; iIndex++)
	{
		TCHAR ch = pSzBuff[iIndex];
		BYTE bt = (BYTE)ch;
		if (!bt)
			break;

		BYTE btNext = 0;
		if (iIndex < (SIZE_RX_BUFF - 1))
			btNext = (BYTE)(pSzBuff[iIndex + 1]);
	
		if (bt == 0x5E) // Symbol "^"
		{
			bFlagIgnore = true;
			continue;
		}

		if ((bt == 0x0D) || (bt == 0x0A))
		{
			if ((btNext == 0x0D) || (btNext == 0x0A)) 
				bFlagIgnore = false;
		}

		if (!bFlagIgnore)
			strResult.Insert(iOutIndex++, ch);
	}
	return strResult;
}

long CATCmdDevice::GetAnswer(CString& strATCmd)
{
	InitTX();

	unsigned int counter2=0; 
	unsigned int nBytesRead=0, nLen=0;
	do
	{
	 Sleep(100);
     counter2++;

	 nLen = m_pSerial->ReadDataWaiting();
	 
	 if(nLen) break;	// If Burst in buffer
	 						// else, wait 0.1 sec
	}while(counter2 < 30);

	if(nLen) 
	{
		InitRX();
		nBytesRead = m_pSerial->ReadData((void*)m_pRXBuff, nLen);

		CorrectRxBuff(m_pRXBuff);
		
		CGSMModemClassic* pMdm = dynamic_cast<CGSMModemClassic*>(this);
		if (pMdm) // If HUAWEI 3G modem use:
		{
			CString strAnswer = CorrectAnswer(m_pRXBuff);
			strATCmd.Format(_T("%s"), strAnswer); // Added 01.08.2014
		}
		else
			strATCmd.Format(_T("%s"), m_pRXBuff); 		
	}
	return 0L;
}

long CATCmdDevice::GetSpecialAnswer(CString& strATCmd)
{	// HUAWEI 3G modem support - search "CUSD:" string:
	InitRX();

	unsigned int counter2=0; 
	unsigned int nBytesRead=0, nLen=0;
	do
	{
	 Sleep(100);
     counter2++;
	 nLen = m_pSerial->ReadDataWaiting();
	 // g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::GetSpecialAnswer: m_pSerial->ReadDataWaiting nLen=%u", nLen);
	 
	 if(nLen > 0)
	 {
		nBytesRead = m_pSerial->ReadData((void*)m_pRXBuff, nLen);
		// g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::GetSpecialAnswer: m_pSerial->ReadData nBytesRead=%u", nBytesRead);
		char* pCUSD = strstr(m_pRXBuff, "CUSD:");
		if (pCUSD)
		{
			// CorrectRxBuff(m_pRXBuff);
			strATCmd.Format(_T("%s"), m_pRXBuff);
			break;
		}
	 }
	}while(counter2 < 70);
	
	return 0L;
}

bool CATCmdDevice::PurgeComPort(DWORD dwFlags)
{
	bool bResult = (BOOL)m_pSerial->PurgeComPort(dwFlags);
	return bResult;
}

void CATCmdDevice::USSDSend(CString& strUSSDText, bool bPhoneOnLine)
{ 
	CWnd* pWnd = AfxGetApp()->GetMainWnd();
	if(!bPhoneOnLine)
	{
		::MessageBox(pWnd->m_hWnd, "GSM телефон не подключен!", "PhoneBook", MB_OK | MB_ICONINFORMATION);
		return;
	}

	AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor // BeginWaitCursor(); 

	CString strUSSDToSend = strUSSDText;

	CPhoneBookApp::ShowUSSDOutput(&strUSSDToSend, 0L); // 0L-Request

	Waiting();	
	AfxGetApp()->DoWaitCursor(0);

	CString s1 = _T("ATE0V0"); 
	SendCommand(s1, 1); 
	s1 = _T("");
	GetAnswer(s1); 

	CString strLog0;

	Waiting();	
	AfxGetApp()->DoWaitCursor(0);

	s1.Format("AT+CSCS=\"GSM\"");
	SendCommand(s1, 0); 
	s1 = _T("");
	GetAnswer(s1);

	CString s11 = _T("AT+CSCS?");
	CString s21 = _T("");
	SendCommand(s11, 0); 
	GetAnswer(s21); 

	g_log.SaveLogFile(PBK_DEBUG, "Answer +CSCS?: %s", s21); 

	if((-1) != s21.Find("GSM")) // Added 04.08.2014
		SetUnicodeMode(false);	

	g_log.SaveLogFile(PBK_DEBUG, "USSD GSM Phone sending request: %s", strUSSDToSend);

	s1.Format("AT+CUSD=1,\"%s\",15",strUSSDToSend); 
	SendCommand(s1, 0); 
	
	CString s22 = _T("");
	for (int i = 0; i < 3; i++)
	{	
		Waiting();	
		AfxGetApp()->DoWaitCursor(0);
		GetAnswer(s22);
		if (s22.GetLength() > 3) 
			break;
		::Sleep(1000);	 
	}
  
	strLog0.Format("Answer +CUSD_2: %s", s22);  
	g_log.SaveLogFile(PBK_DEBUG, (CString&)strLog0); // Do not change this call!

	AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor //EndWaitCursor();

	PrepareCUSDAnswer(s22);

	// Next three lines were added 25.08.2014:
	CPhoneBookApp* pApp = (CPhoneBookApp*)::AfxGetApp();
	if (1 == pApp->m_iUSSDUnicode)
		ConvertString_UCS2(s22);

	strLog0.Format("USSD GSM Phone answer: %s", s22);
	g_log.SaveLogFile(PBK_DEBUG, (CString&)strLog0); // Do not change this call!

	CPhoneBookApp::ShowUSSDOutput(&s22, 1L); // 1L-Answer
	UnicodeOn(true); // Added 19.06.2015
}

long CATCmdDevice::PrepareCUSDAnswer(CString& strCUSDAnswer)
{
	CString strInput(strCUSDAnswer);
	int iCUSDIndex = strInput.Find("+CUSD:");
	if (-1 == iCUSDIndex)
		return 1L; // Error occur 

	bool bTextStart = false;
	CString strOut = _T("");
	int iIndexOut = 0;
	int iLen = strInput.GetLength();
	
	for (int i = iCUSDIndex; i < iLen; i++)
	{
		TCHAR ch = strInput[i];
		if (!bTextStart && (ch == 0x22)) // Symbol '"'
		{
			bTextStart = true;
			continue;
		}

		if (bTextStart)
		{
			if (ch != 0x22) // It is NOT symbol '"'
			{
				if ((ch == 0x0D) || (ch == 0x0A))
				{
					TCHAR chNext = strInput[i + 1];
					if ((chNext != 0x0D) && (chNext != 0x0A)) 
					{
						strOut.Insert(iIndexOut++, (TCHAR)0x0D);
						strOut.Insert(iIndexOut++, (TCHAR)0x0A);
					}
					else
						continue;
				}
				else
					strOut.Insert(iIndexOut++, ch);
			}
			else
				break;
		}
	}
	strCUSDAnswer.Format("%s", strOut);
	return 0L;
}

void CATCmdDevice::UnicodeOn(bool bSilent)
{
	AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor

	CString s0, s1, s2;
	
	CPhoneBookApp* pApp = (CPhoneBookApp*)::AfxGetApp();
	CGSMModemClassic* pMdm = dynamic_cast<CGSMModemClassic*>(this); 
	if (pMdm) // If HUAWEI 3G modem use: 
	{
		s1.Format("AT+CPIN=%u", pApp->m_iPINCodeVal);
		s2 = _T("");
		SendCommand(s1, 0); 
		GetAnswer(s2); 
		g_log.SaveLogFile(PBK_DEBUG, "Answer +CPIN=%u: %s", pApp->m_iPINCodeVal, s2);
	}

	s0.Format("AT+CSCS=?"); 
	SendCommand(s0, 0); 

	s0 = _T("");
	GetAnswer(s0);

	g_log.SaveLogFile(PBK_WORK, "Answer +CSCS=?: %s", s0);

	s0.MakeUpper();
	if (s0.Find("UCS2") == (-1)) // Is not support Unicode in this model
	{
		AfxMessageBox(_T("Данная модель не поддерживает режим Unicode"));
		AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor
		return;
	}
	
	s1.Format("AT+CSCS=\"UCS2\""); 
	// s1.Format("AT+CSCS=\"HEX\"");
	SendCommand(s1, 0); 

	s1 = _T("");
	GetAnswer(s1);

	s1.Format("AT+CSMP=17,167,0,8"); // Russian symbols support
	SendCommand(s1, 0);

	s1 = _T("");
	GetAnswer(s1);

	CString strOut = ClearString(s1);  

	if(!IsWithoutError(strOut)) 
	{
		CString strInfo; strInfo.Format(_T("Ошибка включения режима Unicode"));
		AfxMessageBox(strInfo);  // Error occur
		g_log.SaveLogFile(PBK_DEBUG, (CString&)strInfo);
	}
	else
	{
		CWnd* pWnd = AfxGetApp()->GetMainWnd();
		::SendMessage(pWnd->m_hWnd, MYWN_SET_UNICODE, (WPARAM)1L, (LPARAM)bSilent);
		// In the old version: SetUnicode(true, bSilent); 
	}

	AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor
}

void CATCmdDevice::UnicodeOff(bool bSilent)
{
	AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor

	CString s1;
	s1.Format("AT+CSCS=\"GSM\""); 
	SendCommand(s1, 0); 

	s1 = _T("");
	GetAnswer(s1);
	CString strOut = ClearString(s1); 

	if(!IsWithoutError(strOut)) 
	{
		CString strInfo; strInfo.Format(_T("Ошибка отключения режима Unicode"));
		g_log.SaveLogFile(SMS_MASS, (CString&)strInfo);
		AfxMessageBox(strInfo);  // Error occur
	}
	else
	{
		CWnd* pWnd = AfxGetApp()->GetMainWnd();
		::SendMessage(pWnd->m_hWnd, MYWN_SET_UNICODE, (WPARAM)0L, (LPARAM)bSilent);
		// In the old version: SetUnicode(false, bSilent);
	}
	
	AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
long CATCmdDevice::ProcSetPBRecord(long nIndOfRecord, long lMaxIndex, int* pErrCnt, bool bDelRec) 
{
	try
	{
		if (nIndOfRecord == 0L)
			return 1L; // CRITICAL ERROR occur
		
		if (nIndOfRecord > lMaxIndex)
			return 1L; // CRITICAL ERROR occur

		CPhoneBookApp* pApp = (CPhoneBookApp*)AfxGetApp(); // Added 17.10.2014
		int iErrCnt = *pErrCnt;

		int iLenPh = 0; // Length of "Phone" field
		int iLenNm = 0; // Length of "Name" field
		TCHAR tChar = 0;
		CString s1, strIndOfRecord, strOut; 
		strIndOfRecord.Format(_T("%u"), nIndOfRecord); 
		ClearString(strIndOfRecord); 

		if ((m_pPBRecord != NULL) && (m_pPBRecord->m_strPhone) && (m_pPBRecord->m_strName)) // Corrected 17.10.2014
		{
			iLenPh = m_pPBRecord->m_strPhone.GetLength();
			iLenNm = m_pPBRecord->m_strName.GetLength();  

			if(iLenPh)
				tChar = m_pPBRecord->m_strPhone.GetAt(0);
		}

		bool bDelete = false; 

		if ((iLenPh == 0) && (iLenNm == 0)) 
			bDelete = true; 
		else
			bDelete = bDelRec;

		if (nIndOfRecord > 0L)
		{
			if (!bDelete)   // Write record, without delete
			{	
				CString strName;
				strName.Format("%s", (LPCTSTR)m_pPBRecord->m_strName);
				
				if (tChar != 0x2b)   // If not used "international" number ("+" - 0x2b) 
				{	s1.Format(_T("AT+CPBW=%s, \"%s\", 129, \"%s\" "),  
						strIndOfRecord, m_pPBRecord->m_strPhone, strName); // Corrected 17.10.2014   
				}
				else   // If used "international" number ("+" - 0x2b)
				{	s1.Format(_T("AT+CPBW=%s, \"%s\", 145, \"%s\" "),  
						strIndOfRecord, m_pPBRecord->m_strPhone, strName); // Corrected 17.10.2014 
				}			
			}
			else
				s1.Format(_T("AT+CPBW=%s"), strIndOfRecord);   // Delete record  
		}
		else  
			return 1L;  // CRITICAL ERROR occur

		g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::ProcSetPBRecord: %s", s1);
		
		SendCommand(s1, 0);

		CString s11 = _T("");
		GetAnswer(s11);
	
		g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::ProcSetPBRecord answer: %s", s11);

		strOut = ClearString(s11); 

		if(strOut.GetLength() != 0) // Added 17.10.2014
		{
			iErrCnt++;
			if (!bDelete)   // Write record, without delete
				g_log.SaveLogFile(PBK_ERROR, "Error during Writing record: '%s'", (LPCTSTR)s1);
			else
				g_log.SaveLogFile(PBK_ERROR, "Error during Deleting record: '%s'", (LPCTSTR)s1);
		}
		else
		{
			if (!bDelete)   // Write record, without delete
				g_log.SaveLogFile(PBK_WORK, "Writing record ID=%s passed OK!", (LPCTSTR)strIndOfRecord);
			else
				g_log.SaveLogFile(PBK_WORK, "Deleting record ID=%s passed OK!", (LPCTSTR)strIndOfRecord);
		}
		*pErrCnt = iErrCnt;
		return 0L;  // All correct (if error occur, it's NOT CRITICAL error).
	} 
	catch(...) 
	{
		g_log.SaveLogFile(PBK_ERROR, "CATCmdDevice::ProcSetPBRecord: - Critical error occur !");
		return 1L;  // CRITICAL ERROR occur
	}
}

long CATCmdDevice::ProcGetPBRecord(long nIndOfRecord,  long lMaxIndex,  int* pErrCnt)
{
	try
	{
		if (nIndOfRecord == 0L)
			return 1L; 

		if (nIndOfRecord > lMaxIndex)
			return 1L;

		int iErrCnt = *pErrCnt;

		long lIndex = 0L; 
		CString s1, strIndOfRecord;
		
		strIndOfRecord.Format(_T("%u"), nIndOfRecord); 
		ClearString(strIndOfRecord);

		s1.Format(_T("AT+CPBR=%s"), strIndOfRecord); 
		SendCommand(s1, 0); 

		CStringArray strArr;

		s1 = _T("");
		GetAnswer(s1); 
		CString strPBk = ClearString(s1); 
		if(strPBk.GetLength() <= 1)
		{
			iErrCnt++;
			*pErrCnt = iErrCnt;
			return 1L;  // Return, if empty PhoneBook string...
		}

		char* szInput = new char[strPBk.GetLength() + 1]; 
		::ZeroMemory((void*)szInput, (strPBk.GetLength() + 1));
		lstrcpy(szInput, LPCTSTR(strPBk));
	
		long lParamCount = 4L;  // Parameters-counter (count-down) 
		long lCurChar = 0L;	    // Number of current char in the current string 
		CString strCur = _T("");	// This is current string (current parameter)
		for(lIndex = 0L; lIndex < strPBk.GetLength(); lIndex++, lCurChar++)
		{
			if (szInput[lIndex] == 0x2c)  // ","
			{
				lCurChar = 0;
				strCur.TrimLeft(0x20);
				strCur.TrimRight(0x20);
				if (strCur.GetLength() == 0)
					strCur = _T(" ");

				// Next two linew were added 10.06.2015:
				if (1L == lParamCount) // DEBUG !!! 
					ConvertString_UCS2(strCur);
				
				strArr.Add(strCur);

				strCur = _T(""); 
				lParamCount--;
				continue;
			}

			TCHAR tChar = szInput[lIndex];
			if (tChar != 0x22)							// Do not insert symbol: "
			{	if (tChar < 0x20)
					 tChar = 0x20;
				strCur.Insert(lCurChar, tChar);
			}

			if(lIndex == (strPBk.GetLength() - 1))  // End of file occur !
			{	
				strCur.TrimLeft(0x20);
				strCur.TrimRight(0x20); 
				if (strCur.GetLength() == 0)
					strCur = _T(" ");
				else	// Added 10.06.2015
					ConvertString_UCS2(strCur); // Added 10.06.2015

				strArr.Add(strCur); 
				lParamCount--;
			} 
		} 

		for(long i = 0; i < strArr.GetSize(); i++) 
		{
			CString str = strArr[i];

			if (i == 1L)
				m_pPBRecord->m_strPhone = str;

			if (i == 3L)
				m_pPBRecord->m_strName = str;
		}

		delete[] szInput;
		return 0L;  // All correct
	}
	catch(...) 
	{
		TRACE("CATCmdDevice::ProcGetPBRecord: - error occur !\r\n");
		return 1L;  // Error occur
	}
}

long CATCmdDevice::GetCBCState(long* ptrCBCState)
{
	try
	{   
		CString s1;
		s1.Format(_T("AT+CBC"));
		SendCommand(s1, 0);   

		s1 = _T("");
		GetAnswer(s1); 
		
		CString strCBC;
	
		bool bFindStartFlag = false; 

		long lTargetIndex = 0L;
		for(long lIndex = 0L; lIndex < s1.GetLength(); lIndex++)
		{
			TCHAR chCurChar = s1.GetAt(lIndex);
			if(bFindStartFlag) 
			{				
				if ((chCurChar & 0xf0) == 0x30)
					strCBC.Insert(lTargetIndex++, chCurChar);
				else 
					bFindStartFlag = false;
			}

			if(chCurChar == (TCHAR)(0x2c))  // Search the symbol ","
				 bFindStartFlag = true;
		}

		long lResult = atol(LPCTSTR(strCBC));

		*ptrCBCState = lResult; 
		m_strCBC.Format(_T("Заряд аккумулятора %u %%"), lResult);

		return 0L;
	}  
	catch(...)
	{
		TRACE("CATCmdDevice::GetCBCState: - error occur !\r\n");
		return 1L;  // Error occur
	} 
}

long CATCmdDevice::GetMaxIndex(long* ptrMaxIndex)
{
	try
	{	CString s1;
		s1.Format(_T("AT+CPBR=?"));
		SendCommand(s1, 0); 

		s1 = _T("");
		GetAnswer(s1); 
		CString strPBk = ClearString(s1);

		g_log.SaveLogFile(PBK_DEBUG, "Answer +CPBR=?: %s", s1); 

		int koeff[3];
		koeff[0] = 1;
		koeff[1] = 10;
		koeff[2] = 100;

		char* szInput = new char[strPBk.GetLength() + 1]; 
		::ZeroMemory((void*)szInput, (strPBk.GetLength() + 1));
		lstrcpy(szInput, LPCTSTR(strPBk));
		
		long lResult = 0L;
		long lIndex = 0L;
		bool bFindStartFlag = false;
		for(lIndex = 0L; lIndex < strPBk.GetLength(); lIndex++)
		{
			if(szInput[lIndex] == 0x2c)  // Search the symbol ","
				break;
		}

		if (lIndex < 5)
		{
			delete[] szInput;
			return 1L;
		}

		lIndex--;
		lIndex--;
		for(int i = 0; i < 3; i++)
		{
			TCHAR ch = szInput[lIndex];
			lIndex--;
		    if((ch & 0xf0) == 0x30)
                 lResult += (ch & 0x0f) * koeff[i]; 
		}

		delete[] szInput;
		*ptrMaxIndex = lResult;
		return 0L;
	}
	catch(...)
	{
		TRACE("CATCmdDevice::GetMaxIndex: - error occur !\r\n");
		return 1L;  // Error occur
	} 
}

TCHAR CATCmdDevice::ToHex(TCHAR chInp)
{
	TCHAR tcResult = 0x30;

	if((chInp & 0x0f) < 0x0a)
		tcResult += chInp;
	else
		tcResult += (chInp + 0x7);
	
	return tcResult;
}

TCHAR CATCmdDevice::ToNibble(TCHAR chInp)
{
	TCHAR tcResult;

	if (chInp <= 0x39)
		tcResult = chInp - 0x30;
	else
		tcResult = chInp - 0x37;
	
	return tcResult;
}

long CATCmdDevice::ConvertPBStringToUCS2()
{
	CString strInput = m_pPBRecord->m_strName;
	int iLen = strInput.GetLength();
	CString str;
	int iOutIndex = 0;
	for (int iIndex = 0; iIndex < iLen; iIndex++)
	{
		TCHAR ch = strInput[iIndex];
		if (iIndex > 1)	// Do not using firs and second symbols! 
			str.Insert(iOutIndex++, ch);
	}
	long lRes = ConvertString_UCS2(str);
	m_pPBRecord->m_strName = str;
	return lRes;
}

long CATCmdDevice::ConvertPBStringToWChar()
{
	return ConvertStringToWChar(m_pPBRecord->m_strName);
}

long CATCmdDevice::ConvertPBStringToMBCS()
{
	return ConvertStringToMBCS(m_pPBRecord->m_strName);
}

long CATCmdDevice::ConvertStringToMBCS(CString& strString)
{
	CString str = strString;
	wchar_t wbuf[MAX_OUT_STR];  

	int n;
	for (n = 0; n < MAX_OUT_STR; n++)
			wbuf[n] = 0; 
	
	int iSize = str.GetLength();
	int iWChars = (int)(iSize / 4);

	TCHAR tCh;  
	long lCurPos = 0L; 

	for (int i = 0; i < iWChars; i++)
	{		
		tCh = str.GetAt(lCurPos++);
		wbuf[i] = (ToNibble(tCh) << 12) & 0xf000; 
	
		tCh = str.GetAt(lCurPos++);
		wbuf[i] += (ToNibble(tCh) << 8) & 0x0f00;  

		tCh = str.GetAt(lCurPos++);
		wbuf[i] += (ToNibble(tCh) << 4) & 0x00f0;  

		tCh = str.GetAt(lCurPos++);
		wbuf[i] += ToNibble(tCh) & 0x000f;  
	}

	char szOutString[MAX_OUT_STR];
	memset(szOutString, 0, MAX_OUT_STR);  

	::WideCharToMultiByte(CP_ACP, 0, wbuf, -1, szOutString, MAX_OUT_STR, NULL, NULL);

	str = _T("");  
	str.Format(_T("%s"), szOutString);

	strString = str;
	return 0L;
}

long CATCmdDevice::ConvertStringToWChar(CString& strString)
{
	CString str = strString;
	wchar_t wbuf[MAX_OUT_STR]; 

	char szOutString[MAX_OUT_STR];
	memset(szOutString, 0, MAX_OUT_STR);  

	::MultiByteToWideChar(CP_ACP, 0, LPCTSTR(str), -1, wbuf, MAX_OUT_STR);

	TCHAR tChar1, tChar1L, tChar1H;  // Fist char 
	TCHAR tChar2, tChar2L, tChar2H;  // Second char

	long lCounter = 0L;
	long lCurWbuf = 0L;

	while(wbuf[lCurWbuf])
	{
		tChar1 =  wbuf[lCurWbuf] & 0x00ff;
		tChar1L = tChar1 & 0x0f;
		tChar1H = (tChar1 >> 4) & 0x0f;  

		tChar2 =  (wbuf[lCurWbuf] >> 8) & 0x00ff; 
		tChar2L = tChar2 & 0x0f;
		tChar2H = (tChar2 >> 4) & 0x0f;  

		szOutString[lCounter++] = ToHex(tChar2H); 
		szOutString[lCounter++] = ToHex(tChar2L);  

		szOutString[lCounter++] = ToHex(tChar1H); 
		szOutString[lCounter++] = ToHex(tChar1L); 

		lCurWbuf++;
	};

	str = _T("");  

	for(long l = 0; l < lCounter; l++)
	{
		TCHAR tCh = (TCHAR)szOutString[l];
		str.Insert(lCounter, tCh); 
	}

	strString = str;
	return 0L;
}

bool CATCmdDevice::IsWithoutError(CString strStr)
{
	bool bResult = false;
	if (strStr.IsEmpty())
		bResult = true;

	int iLen = strStr.GetLength();
	if (1 == iLen)
	{
		TCHAR ch1 = strStr[0];
		if (ch1 == 0x30) // Symbol "0"
			bResult = true;
	}
	if (2 == iLen)
	{
		TCHAR ch1 = strStr[0];
		TCHAR ch2 = strStr[1];
		if ( (ch1 == 0x30) && ((ch2 == 0x0A) || (ch2 == 0x0D)) )  // Symbol "0"
			bResult = true;		
	}
	return bResult;
}

///////////////////////////////////////
//     HUAWEI support (old SMS Support)
///////////////////////////////////////
long CATCmdDevice::ConvertString_7Bits(CString& strString)
{
	try
	{
		int iLen = strString.GetLength();
		long lLen = iLen + 1L; // for ending null char 
		char* szString = new char[lLen];

		::ZeroMemory((void*)szString, lLen);
		strcpy(szString, strString.GetBuffer(iLen));
		strString.ReleaseBuffer(); 

		CString strOut;
		long lCurChar = 0L;

		TCHAR ch_l = (TCHAR)0;
		TCHAR ch_h = (TCHAR)0;

		unsigned char chCur = 0;
		unsigned char chOut = 0;

		UINT uTmp = 0;

		int iByteCounter = 1;
		unsigned char nMask = 0xff;
		bool bWhole = false;   // After 8 bytes, we have ONE whole addition byte
		for (int i = 0; i < iLen; i++) 
		{
			if (i & 0x01)  // odd index
			{
				ch_l = szString[i];

				chCur = 16 * ToNibble(ch_h) + ToNibble(ch_l);
				nMask = 0xff;

				unsigned char uchAdd = (unsigned char)((uTmp >> 8) & 0x7f);
				uTmp = 0;

				if (bWhole)
				{
					iByteCounter = 1;
					uchAdd = 0;
					bWhole = false;
				}
		
				int n = iByteCounter % 8;

				nMask = nMask >> n;
				unsigned char tMask = ~nMask;
				unsigned char uchTmp = tMask & chCur;
				uTmp += uchTmp; 
				uTmp = uTmp << n;

				chOut = chCur & nMask;
				chOut = chOut << (n - 1);
				chOut += uchAdd;
		
				if (chOut >= 0x20)
					strOut.Insert(lCurChar++, (TCHAR)(chOut)); 

				if ((chOut == 0x0d) || (chOut == 0x0a))
					strOut.Insert(lCurChar++, (TCHAR)0x20);

				iByteCounter++; 
			}
			else       // even index
			{
				ch_h = szString[i];
				if (iByteCounter == 8)
				{
					unsigned char ucr = (unsigned char)((uTmp >> 8) & 0x7f);

					if (ucr >= 0x20)
						strOut.Insert(lCurChar++, (TCHAR)(ucr)); 

					if ((ucr == 0x0d) || (ucr == 0x0a))
						strOut.Insert(lCurChar++, (TCHAR)0x20); 

					bWhole = true;
				}
			}
		}

		strString = strOut; 

		delete[] szString;
		return 0L; 
	}
	catch(...)
	{
		TRACE("CATCmdDevice::ConvertString_7Bits: - error occur !\r\n");
		return 1L;  // Error occur
	} 
}

long CATCmdDevice::ConvertString_UCS2(CString& strString)
{
	CString strMBCS = strString;
	ConvertStringToMBCS(strMBCS);

	CString strOut;
	long lCurChar = 0L;

	for(int i = 0; i < strMBCS.GetLength(); i++)
	{
		unsigned char uch = (unsigned char)(strMBCS.GetAt(i));

		if (uch >= 0x20)
			strOut.Insert(lCurChar++, (TCHAR)(uch)); 

		if ((uch == 0x0d) || (uch == 0x0a))
			strOut.Insert(lCurChar++, (TCHAR)0x20);
	}

	strString = strOut;
	return 0L;
}

/////////////////////////
//     SMS Support
/////////////////////////
long CATCmdDevice::SMSArchiveRawPopulate(CStringArray* pStrArr)
{	
	m_mapSMSValidStrings.clear();

	CString s1 = _T("AT+CMGF=0"); // Default value: 0
	SendCommand(s1, 0);   
	s1 = _T("");
	GetAnswer(s1);

	// AT+CPMS see:
	// http://www.developershome.com/sms/cpmsCommand.asp
 	CString s11 = _T("AT+CPMS?");
	SendCommand(s11, 0); 
	CString s22 = _T("");
	GetAnswer(s22);

	g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::SMSArchiveLoopPopulate AT+CPMS? answer: %s", s22);

	char szStorage[3];
	memset((void*)szStorage, 0, 3);
	FillStorageType(s22, szStorage); // Added 13.10.2014

	int iCurCount = 0;
	int iMaxCount = GetMaxSMSCount(s22, szStorage, &iCurCount);

	g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::SMSArchiveLoopPopulate results: ('%s') заполнено(iCurCount) %u; максимум(iMaxCount) %u",
		szStorage, iCurCount, iMaxCount);

	if (0 == iCurCount) // Success, but is not present any SMS in the GSM device
		return 0L;

	if (iCurCount == iMaxCount)
		m_bReceivedSMSListFull = true;

	CString s31;
	s31.Format(_T("AT+CPMS=\"%s\",\"%s\",\"%s\""), szStorage, szStorage, szStorage);
	CString s21 = _T("");
	SendCommand(s31, 0); 
	GetAnswer(s21);

	g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::SMSArchiveLoopPopulate AT+CPMS='%s','%s','%s' answer: %s",
		szStorage, szStorage, szStorage, s21);

	// AT+CMGR see:
	// http://www.developershome.com/sms/cmgrCommand.asp
	int iValidStringKey = 0; // Added 08.10.2014
	for (int iCurSMSIndex = 0; iCurSMSIndex < iMaxCount; iCurSMSIndex++) // Start index 0 
	{
		CString strCommand;
		strCommand.Format(_T("AT+CMGR=%d"), iCurSMSIndex);
		SendCommand(strCommand, 0); 

		CString strAnswer;
		GetAnswer(strAnswer);

		// if (IsErrSMS(strAnswer)) // Commented out 09.10.2014
		//	continue;
		
		if (!FillRawSMS(strAnswer, pStrArr))
		{	// Find valid string:
			m_mapSMSValidStrings[iValidStringKey++] = iCurSMSIndex;
		} 
	}
	return 0L; 
}

/////////////////////////
//     SMS Support
/////////////////////////
long CATCmdDevice::SMSArchiveLoopPopulate(CStringArray* pStrArr)
{
	long lArrSize = pStrArr->GetSize();
	m_SMSContentVector.clear();

	STRING_MAP mapTextSMS;	// Added 04.10.2014
	BOOL bTextSMSReady = FALSE; // Added 04.10.2014
	for (long n = 0; n < lArrSize; n++)
	{
		CString strCurPDU = pStrArr->GetAt(n);

		CSMSContent* pSMSTemp = new CSMSContent; 
		if (!DecodePDUString(strCurPDU, pSMSTemp, mapTextSMS, &bTextSMSReady)) // Corrected 06.10.2014
		{		
			CString strTextSMS;
			if (ProcessTextSMSMap(mapTextSMS, strTextSMS)) // Added 04.10.2014
			{
				int iValidIndex = RetrieveValidIndex(n); // Added 08.10.2014
				CSMSContent* pSMS = RetrieveEqualSMSContent(pSMSTemp); // Added 06.10.2014
				if (!pSMS)
				{
					pSMS = new CSMSContent(*pSMSTemp);
					long lIndex = (long)(m_SMSContentVector.size() + 1); // Start index 1
					pSMS->SetSMSIndex(lIndex);
					pSMS->m_strSMSText = strTextSMS;
					if ((-1) != iValidIndex) // Added 08.10.2014
						pSMS->AddRawStringID(iValidIndex); 

					m_SMSContentVector.push_back(pSMS);
				}
				else
				{
					pSMS->m_strSMSText = strTextSMS;
					if ((-1) != iValidIndex) // Added 08.10.2014
						pSMS->AddRawStringID(iValidIndex); 
				}
			} 
			if (bTextSMSReady)
				mapTextSMS.clear();

			bTextSMSReady = FALSE;
		}
		delete pSMSTemp;		
	}

	int iResultSize = m_SMSContentVector.size();
	if (iResultSize > 0)
		std::sort(m_SMSContentVector.begin(), m_SMSContentVector.end(), ::SMSContentGreater); 

	return 0L;
}

/////////////////////////
//     SMS Support
/////////////////////////
int CATCmdDevice::RetrieveValidIndex(long lIndex)
{
	INT_MAP::const_iterator it;
	it = m_mapSMSValidStrings.find((int)lIndex);
	if (it == m_mapSMSValidStrings.end())    // valid index not exist
		return -1;

	int iResult = m_mapSMSValidStrings[(int)lIndex]; 
	return iResult;
}

/////////////////////////
//     SMS Support
/////////////////////////
bool CATCmdDevice::ProcessTextSMSMap(STRING_MAP& mapTextSMS, CString& strTextSMS)
{
	int iSize = mapTextSMS.size();
	if (!iSize)
		return false;

	for (STRING_MAP::const_iterator cit=mapTextSMS.begin();cit!=mapTextSMS.end();cit++)
	{
		STRING_PAIR pair = *cit;
		std::string sText = pair.second;
		CString strText(sText.c_str());
		strTextSMS += strText;
	} 
	return true;
}

/////////////////////////
//     SMS Support
/////////////////////////
long CATCmdDevice::FillRawSMS(CString& strRawSMS, CStringArray* pStrArr)
{
	try
	{
		if (pStrArr == NULL)
			return 1L; // Error occur

		int iStartIndex = strRawSMS.Find("+CMGR:");
		if ((-1) == iStartIndex)
			return 1L; // Not valid string

		int iLen = strRawSMS.GetLength();
		long lLen = iLen + 1L;
		char* szRawSMS = new char[lLen];

		::ZeroMemory((void*)szRawSMS, lLen);
		strcpy(szRawSMS, strRawSMS.GetBuffer(iLen));
		strRawSMS.ReleaseBuffer(); 

		CString strSMSText;
		bool bSMSText  = false;
		long lCurTextChar  = 0L;
		bool bCrLfFlag = false; // Added 09.10.2014
		for (long i = (long)iStartIndex; i < lLen; i++)
		{
			bCrLfFlag = false;

			TCHAR ch = szRawSMS[i];
			TCHAR chNext = (TCHAR)0;

			if (i < (lLen - 1L))
				chNext = szRawSMS[i+1];

			if ( ((ch == (TCHAR)0x0d) && (chNext == (TCHAR)0x0a)) || 
			     ((ch == (TCHAR)0x0a) && (chNext == (TCHAR)0x0d)) )
			{
				bSMSText = !bSMSText;
				bCrLfFlag = true;
			}

			if ( ((ch == (TCHAR)0x2b) && i) || // 0x2b - "+"
				(ch == (TCHAR)0) || 
				(bCrLfFlag && !bSMSText) ) 		// Added 09.10.2014
			{   // At the end of string:
				if (!strSMSText.IsEmpty())
				{
					// AfxMessageBox(strSMSText);
					strSMSText.TrimLeft();
					strSMSText.TrimRight();

					if (strSMSText.GetLength())
						pStrArr->Add(strSMSText); 

					bSMSText = false;
				}

				lCurTextChar = 0L;
				strSMSText = _T("");

				continue;
			}

			if ((ch != (TCHAR)0) && bSMSText)
			{
				strSMSText.Insert(lCurTextChar++, ch); 
			}
		}
		delete[] szRawSMS;
		return 0L; // Success!
	}
	catch(...)
	{
		AfxMessageBox("SMS incorrect read"); 
		TRACE("CATCmdDevice::FillRawSMS: - error occur !\r\n");
		return 1L;  // Error occur
	} 
}

/////////////////////////
//     SMS Support
/////////////////////////
bool CATCmdDevice::IsReceivedSMSListFull() const
{
	return m_bReceivedSMSListFull; 
}

/////////////////////////
//     SMS Support
/////////////////////////
void CATCmdDevice::ResetSMSListFull()
{
	m_bReceivedSMSListFull = false; 
}

/////////////////////////
//     SMS Support
/////////////////////////
long CATCmdDevice::SMSDelete(long lIndex, long lAddIndex)
{
	// AT+CMGD see:
	// http://www.developershome.com/sms/cmgdCommand.asp
	
	CString str;
	str.Format(_T("AT+CMGD=?"));
	SendCommand(str, 0);  
	str = _T("");
	GetAnswer(str);

	g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::OnSMSDelete AT+CMGD=? answer: %s", str);

	if ((lIndex > 0L) && (lAddIndex == 0L))  // Delete selected SMS
	{
		AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor

		CString strCommand;
		strCommand.Format(_T("AT+CMGD=%d,0"), lIndex); // Delete ONLY selected message
		SendCommand(strCommand, 0);  
		strCommand = _T("");
		GetAnswer(strCommand);

		g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::OnSMSDelete AT+CMGD=%d answer: %s", lIndex, strCommand);

		AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor
		return 0L;
	}

	if ((lIndex == -1L) && (lAddIndex == -1L))  // Delete all SMS
	{
		AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor

		CString strDelAll;
		strDelAll.Format(_T("AT+CMGD=1,4"));
		SendCommand(strDelAll, 0);  
		strDelAll = _T("");
		GetAnswer(strDelAll);

		g_log.SaveLogFile(PBK_DEBUG, "CATCmdDevice::OnSMSDelete AT+CMGD=1,4 (delete all SMS) answer: %s", strDelAll);

		AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor
		return 0L;
	} 
	return 1L;
}

/////////////////////////
//     SMS Support
/////////////////////////
long CATCmdDevice::DecodePDUString(CString& strPDU, CSMSContent* pSMSContent, STRING_MAP& mapOut, BOOL* pTextSMSReady)
{
	try
	{
		int iLen = strPDU.GetLength();
		long lLen = iLen + 1L;
		char* szPDU = new char[lLen];

		::ZeroMemory((void*)szPDU, lLen);
		strcpy(szPDU, strPDU.GetBuffer(iLen));
		strPDU.ReleaseBuffer(); 
		
		long nCurIndex = 0L; 

		TCHAR chPos0 = szPDU[nCurIndex++]; 
		TCHAR chPos1 = szPDU[nCurIndex];

		long nStartIndex = nCurIndex - 1L;
		
		int iAddLength = 16 * ToNibble(chPos0) + ToNibble(chPos1); 
		int iStartAddr = iAddLength + 1;

		nStartIndex += 2;
		chPos0 = szPDU[nStartIndex++]; 
		chPos1 = szPDU[nStartIndex++];
		pSMSContent->m_nTypeOfSMSAddr = 16 * ToNibble(chPos0) + ToNibble(chPos1);

		int iAddrLng = iAddLength * 2 - 2;
		pSMSContent->m_strSMSAddr = DecodeSMSPNum(szPDU, nStartIndex, iAddrLng);

		int iOffset1 = 3; // One byte and one nibble
		int iPNumLenStartIndex = iStartAddr * 2 + iOffset1 - 1;

		chPos0 = szPDU[iPNumLenStartIndex++]; 
		chPos1 = szPDU[iPNumLenStartIndex++];
		int iPNumLen = 16 * ToNibble(chPos0) + ToNibble(chPos1);

		chPos0 = szPDU[iPNumLenStartIndex++]; 
		chPos1 = szPDU[iPNumLenStartIndex++];
		pSMSContent->m_nTypeOfSMSPNum = 16 * ToNibble(chPos0) + ToNibble(chPos1);
		
		CString strSMSPNum = DecodeSMSPNum(szPDU, iPNumLenStartIndex, iPNumLen);
		pSMSContent->SetSMSPNum(strSMSPNum);
		
		if (!(iPNumLen & 0x01)) // even Phone Number Length
			iOffset1--;

		int iDataCodingSchemeStartIndex = 
			iPNumLenStartIndex + iOffset1 + iPNumLen;

		chPos0 = szPDU[iDataCodingSchemeStartIndex++]; 
		chPos1 = szPDU[iDataCodingSchemeStartIndex++];
		unsigned char nDCS = 16 * ToNibble(chPos0) + ToNibble(chPos1);

		pSMSContent->SetDataCoding(nDCS);

		int iDateTimeStartIndex = iDataCodingSchemeStartIndex;

		DecodeSMSDateTime(szPDU, pSMSContent, iDateTimeStartIndex);

		int iOffset2 = 14; // 12 for Date-Time and 2 for TimeZone
		int iTextLengthStartIndex = iDateTimeStartIndex + iOffset2;
		chPos0 = szPDU[iTextLengthStartIndex++]; 
		chPos1 = szPDU[iTextLengthStartIndex++];			
		int iTextLengthInChars = 16 * ToNibble(chPos0) + ToNibble(chPos1);
		
		// pSMSContent->SetTextLength(iTextLengthInChars);

		int iSMSMPID = -1; // Multipart SMS-ID (if value ==-1 -> multipart SMS-ID is not present)
		long nCurrentPart = 1L;
		long nTotalParts = 1L;
		bool bMultiPartSMS = IsMultipartSMS(iTextLengthStartIndex, szPDU); // Added 04.10.2014
		if (bMultiPartSMS)
		{			
			nTotalParts = GetTotalParts(iTextLengthStartIndex, szPDU, &nCurrentPart, &iSMSMPID);
			iTextLengthStartIndex += 12; // Caption and length of caption
		}
		pSMSContent->SetSMSMPIndex((long)iSMSMPID); // Added 06.10.2014

		CString strOut;
		long lCurChar = 0L;
		for (int i = iTextLengthStartIndex; i < lLen; i++) 
		{
			TCHAR chr = szPDU[i];
			strOut.Insert(lCurChar++, chr);
		}

		if (nDCS == 8)  // Data Coding Scheme: UNICODE, Russian
			ConvertString_UCS2(strOut);
		else			// Standard (Latin) scheme
			ConvertString_7Bits(strOut);

		std::string sOut = (CStringA)strOut; // Added 04.10.2014
		mapOut[(int)nCurrentPart] = sOut; // Added 04.10.2014
		// pSMSContent->m_strSMSText = strOut; // Commented out 04.10.2014

		// Next seven lines were added 04.10.2014:
		if (pTextSMSReady)
		{
			if (bMultiPartSMS)
				*pTextSMSReady = (BOOL)(nCurrentPart == nTotalParts);
			else 
				*pTextSMSReady = TRUE;
		}
		
		delete[] szPDU;
		return 0L; 
	}
	catch(...)
	{
		TRACE("CATCmdDevice::DecodePDUString: - error occur !\r\n");
		return 1L;  // Error occur
	} 
}

long CATCmdDevice::SendSmsMass(ISMSSender* pSender)
{	
	HWND hWndSender = pSender->GetSenderHWND();
	try
	{
		long lTotalSec = pSender->GetTotalSections();
		g_log.SaveLogFile(SMS_MASS, "Start SMS Mass sending (total %u sections)", lTotalSec);

		CString strLog0, s1;
		CString s222 = _T("");
		CPhoneBookApp* pApp = (CPhoneBookApp*)::AfxGetApp();
		CGSMModemClassic* pMdm = dynamic_cast<CGSMModemClassic*>(this); 
		if (pMdm) // If HUAWEI 3G modem use:  
		{
			s1 = _T("ATE0V0"); 
			SendCommand(s1, 1); 
			s1 = _T("");
			GetAnswer(s1); 

			CString s111;
			s111.Format("AT+CPIN=\"%u\"", pApp->m_iPINCodeVal);
			
			SendCommand(s111, 0); 
			GetAnswer(s222);   
 
			g_log.SaveLogFile(SMS_MASS, "Answer +CPIN: %s", s222);   
		}

		s1 = _T("AT+CMGF=0");

		int iMd = (pMdm) ? 1 : 0;
		SendCommand(s1, iMd); // If 3G modem -> iMd==1
		s1 = _T("");
		GetAnswer(s1);
  
		g_log.SaveLogFile(SMS_MASS, "Answer +CMGF: %s", s1);

		CString strSCA = _T("AT+CSCA?");
		SendCommand(strSCA, 0);  
		strSCA = _T("");
		GetAnswer(strSCA); 

		g_log.SaveLogFile(SMS_MASS, "Answer +CSCA: %s", strSCA);
		
		::SendMessage(hWndSender, MYWN_STEP_SMS_PHONE, (WPARAM)1L, (LPARAM)0L); // Added 04.08.2014

		CString strSmsText = pSender->GetSmsText();
		CString strTextInfo; strTextInfo.Format("SMS text: %s", strSmsText);
		g_log.SaveLogFile(SMS_MASS, (CString&)strTextInfo);
		long lCurrPhone = 0L; // Added 06.10.2013
		long lCurrSection = 0L;
		bool bFirstPass = true; // Added 29.04.2013
		SMS_MASS_MAP& mapSmsMass = pSender->GetSmsMassMap();
		for (SMS_MASS_MAP::const_iterator cit=mapSmsMass.begin();cit!=mapSmsMass.end();cit++)
		{
			SMS_MASS_PAIR pair = *cit;
			int iIndex = pair.first;
			PTR_SMS_MASS_RECORD pSmsMassRecord = pair.second;
			std::string sComment = (CStringA)pSmsMassRecord->m_strName; // Comment as a "NAME"
			std::string sPhoneNum= (CStringA)pSmsMassRecord->m_strPhone;

			int iFlag = pSmsMassRecord->m_iFlag;
			if (iFlag != 1)
				continue;	

			bool bBlocked = pSender->IsBlockedRecord((LPCTSTR)pSmsMassRecord->m_strName); // Added 06.11.2014
			if (bBlocked) 
				continue;
		
			int iLen = strSmsText.GetLength(); 
			int iParts = CPhoneBookApp::CalculateParts(iLen);
		
			for (int iCurPart = 0; iCurPart < iParts; iCurPart++)
			{
				CString strSmsCurrPart = pSender->PrepareCurrPart(strSmsText, iCurPart); // It is 67 chars in each part

				CPDUEncoder* pPDUEncoder = new CPDUEncoder(this);
				if (pPDUEncoder == NULL)
				{
					::g_evntReady.SetEvent(); 
					return 1;
				}

				pPDUEncoder->m_btTotalParts = (BYTE)iParts;
				pPDUEncoder->m_btRefNumber += (BYTE)iIndex;
	
				pPDUEncoder->FillRawSCANew(strSCA); // Corrected 15.06.2015

				pPDUEncoder->FillPNum(pSmsMassRecord->m_strPhone);
				pPDUEncoder->SetSMSText(strSmsCurrPart);

				if (pPDUEncoder->PopulatePDUString(iCurPart))
				{				
					if (pPDUEncoder)
						delete pPDUEncoder;

					::g_evntReady.SetEvent(); 
					return 1;
				} 

				long nLen = pPDUEncoder->GetPDULength();
				CString strPDUString = pPDUEncoder->GetPDUString();

				CString s2, s22;

						s2.Format("AT+CMGS=%d\r%s\x1a", nLen, LPCTSTR(strPDUString)); 	
						SendCommand(s2, 0);
						s2 = _T("");			
						GetAnswer(s2);

					g_log.SaveLogFile(SMS_MASS, "Answer +CMSG: %s", s2);

					for (int i = 0; i < 2; i++)
					{	
						Waiting();					
						::Sleep(1000);
						GetAnswer(s22); 
						if (!s22.IsEmpty())
							break;
					}
				if (pPDUEncoder)
					delete pPDUEncoder;

				CString strAnswer1 = ClearString(s2, true);
				CString strAnswer2 = ClearString(s22, true);
				CString strInfo; strInfo.Format("Шаг %d/%d, Номер: %s (Ответ устройства \"%s\"...\"%s\")\r\n",
					iIndex, (iCurPart+1), sPhoneNum.c_str(), strAnswer1, strAnswer2);
				
				char* szStr = new char[1024];
				memset(szStr, 0, sizeof(char)*1024);
				strcpy(szStr, strInfo);

				lCurrSection++;
				::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)(szStr), (LPARAM)lCurrSection);
				g_log.SaveLogFile(SMS_MASS, (CString&)strInfo);				
			}
			lCurrPhone++;
			::SendMessage(hWndSender, MYWN_STEP_SMS_PHONE, (WPARAM)1L, (LPARAM)lCurrPhone);
			UINT nRecordID = pSender->GetRecordID();
			if (bFirstPass)
			{
				std::string sTextInfo = (CStringA)strTextInfo;
				g_log.SaveSMSSentLogFile(sTextInfo.c_str(), nRecordID);
			}
			g_log.SaveSMSSentLogFile(sPhoneNum.c_str(), sComment.c_str(), nRecordID); 
			HWND hWndHistory = pSender->GetSenderHistoryHWND();
			::PostMessage(hWndHistory, MYWN_REFRESH_SMS_SENT_LIST, (WPARAM)0L, (LPARAM)0L);
			// Checking stop request:
			int iCode = ::WaitForSingleObject(::g_evntStop.m_hObject, 0);
			if(iCode == WAIT_OBJECT_0)  // if stop request !
			{	// Reset stop-flag:
				::g_evntStop.ResetEvent();
				::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L); // End of SMSMass sending
				::g_evntReady.SetEvent(); 
				return 0L;
			}
			bFirstPass = false;
		}
		::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L); // End of SMSMass sending
		::g_evntReady.SetEvent(); 
		return 0L;
	}
	catch(...)
	{
		g_log.SaveLogFile(PBK_ERROR, "Catch exception - during SMS sending.");
		// AfxMessageBox(_T("Ошибка рассылки SMS."));
		::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L);
		::g_evntReady.SetEvent();
		::PostMessage(hWndSender, MYWN_SHOW_ERROR_FAILURE, (WPARAM)0L, (LPARAM)1L);
		return 1L;
	}
}

//////////////////////////////////
//     SMS Support (MultipartSMS)
//////////////////////////////////
bool CATCmdDevice::IsMultipartSMS(int iStartIndex, char* szPDU)
{
	char szHeader[6] = {0x30, 0x35, 0x30, 0x30, 0x30, 0x33}; // Header string: 050003
	int iMaxIndex = iStartIndex + 6; // Length of header - 6 symbols
	int iCounter = 0;
	int j = 0;
	for (int i = iStartIndex; i < iMaxIndex; i++, j++)
	{
		if (szHeader[j] == szPDU[i])
			iCounter++;
	}
	return (bool)(6 == iCounter);
}

//////////////////////////////////
//     SMS Support (MultipartSMS)
//////////////////////////////////
long CATCmdDevice::GetTotalParts(int iStartIndex, char* szPDU, long* pCurrentPart, int* pSMSMPID)
{
	char szCaption[12];
	memset(szCaption, 0, 12);

	for (int i = 0; i < 12; i++)
		szCaption[i] = szPDU[iStartIndex+i];

	char szDigit[3];
	memset(szDigit, 0, 3);

	int iIndex = 6; // Identifier SMS-ID (in the multipart SMS) 
	szDigit[0] = szCaption[iIndex++];
	szDigit[1] = szCaption[iIndex++];
	int iSMSMPID = atoi(szDigit);	

	// Total count of part (position 8):
	szDigit[0] = szCaption[iIndex++];
	szDigit[1] = szCaption[iIndex++];
	long lTotalParts = atol(szDigit);

	// Current part (position 10):
	szDigit[0] = szCaption[iIndex++];
	szDigit[1] = szCaption[iIndex++];
	long lCurrentPart = atol(szDigit);

	if (pCurrentPart)
		*pCurrentPart = lCurrentPart;

	if (pSMSMPID)
		*pSMSMPID = iSMSMPID;

	return lTotalParts;
}

/////////////////////////
//     SMS Support
/////////////////////////
CString CATCmdDevice::DecodeSMSPNum(LPCTSTR szPDU, int iPNumStartIndex, int iPNumLength)
{
	CString strOut;

	TCHAR ch_l = (TCHAR)0;
	TCHAR ch_h = (TCHAR)0;

	char* pPNum = (char*)(szPDU + iPNumStartIndex);

	long lCurChar = 0L;
	for (int i = 0; i < iPNumLength; i++)
	{
		if (i & 0x01) // odd position 
		{
			ch_h = *(pPNum + i); 
			strOut.Insert(lCurChar++, ch_h);
			strOut.Insert(lCurChar++, ch_l);
		}
		else			// even position 
		{
			ch_l = *(pPNum + i);
			if (i == (iPNumLength - 1))
			{
				ch_h = *(pPNum + iPNumLength); 
				strOut.Insert(lCurChar++, ch_h);
			}
		}
	}

	return strOut;
}

/////////////////////////
//     SMS Support
/////////////////////////
void CATCmdDevice::DecodeSMSDateTime(LPCTSTR szPDU, CSMSContent* pSMSContent, int iDTStartIndex)
{
	TCHAR ch_l = (TCHAR)0;
	TCHAR ch_h = (TCHAR)0;

	char* pDT = (char*)(szPDU + iDTStartIndex);

	unsigned short buffer[6];
	int iBuffIndex = 0;

	for (int i = 0; i < 12; i++) // 12 characters for Date-Time field
	{
		if (i & 0x01) // odd position 
		{
			ch_h = *(pDT + i); 
			if (iBuffIndex < 6)
				buffer[iBuffIndex++] = ToNibble(ch_h) * 10 + ToNibble(ch_l);

		}
		else			// even position 
			ch_l = *(pDT + i);		
	}

	int iYear = buffer[0] + 2000;
	int iMonth= buffer[1];
	int iDay  = buffer[2];

	int iHour = buffer[3];
	int iMinute=buffer[4];
	int iSecond=buffer[5];

	COleDateTime time(iYear, iMonth, iDay, iHour, iMinute, iSecond);
	pSMSContent->SetSMSTime(time);

	ch_l = *(pDT + 12);
	ch_h = *(pDT + 13);
	unsigned short nTimeZone = ToNibble(ch_h) * 10 + ToNibble(ch_l); 
	pSMSContent->SetSMSTimeZone((long)(nTimeZone));			
}

/////////////////////////
//     SMS Support
/////////////////////////
CSMSContent* CATCmdDevice::GetReceivedSMS(long lSMSIndex)  // Indexes started from 1...
{
	if (lSMSIndex > 0L)
	{
		CSMSContent* pSMSCtt = m_SMSContentVector[(int)(lSMSIndex-1)];
		return pSMSCtt;
	}
	else
		return NULL;
}

/////////////////////////
//     SMS Support
/////////////////////////
void CATCmdDevice::FillStorageType(CString strInp, LPSTR szStorage)
{
	if (!szStorage)
		return;

	const char szStorTypes[] = "SM,ME,MT,BM,SR,TA";
	char szStType[3];
	
	for (int i=0; i<6; i++)
	{
		int iCharIndex = i * 3;
		memset((void*)szStType, 0, 3);
		strncpy(szStType, &szStorTypes[iCharIndex], 2);

		if ((-1) != strInp.Find(szStType))
		{
			strcpy(szStorage, szStType);
			break;
		}		
	}
}

/////////////////////////
//     SMS Support
/////////////////////////
int CATCmdDevice::GetMaxSMSCount(CString strInp, LPCTSTR szStorage, int* ptrCurCount)
{
	if (strInp.Find("+CPMS:") == (-1))
		return 0;

	int iSMIndex = strInp.Find(szStorage); 
	int iIndex = iSMIndex + 4;
	int iMaxSMSCount = -1; // Result (if -1 this value is not ready)
	int iCurCount = -1; // This value return via ptrCurCount pointer

	CString strOut;
	int iOutIndex = 0;
	
	int iCommaCnt = 0;
	bool bMaxSMSCountFlag = false; 
	do
	{
		TCHAR ch = strInp[iIndex++];
		if ((ch >= 0x30) && (ch <= 0x39))
			strOut.Insert(iOutIndex++, ch);

		if (ch == 0x2C) // "," comma symbol
		{
			iCommaCnt++;
			if (1 == iCommaCnt)
				iCurCount = atoi((LPCTSTR)strOut);
			if (2 == iCommaCnt)
				iMaxSMSCount = atoi((LPCTSTR)strOut);

			strOut = _T("");
		}
	}
	while(iCommaCnt < 2);

	if (ptrCurCount)
		*ptrCurCount = iCurCount;

	return iMaxSMSCount;
}

/////////////////////////
//     SMS Support
/////////////////////////
long CATCmdDevice::GetReceivedSMSArrSize()
{
	return ((long)m_SMSContentVector.size());
}

void CATCmdDevice::ClearSMSArr()
{
	int iSize = m_SMSContentVector.size();
	for(int i = 0; i < iSize; i++)
	{
		CSMSContent* pSMS = m_SMSContentVector[i];
		if (pSMS)
			delete pSMS;
	}
	m_SMSContentVector.clear();
}

/////////////////////////
//     SMS Support
/////////////////////////
CSMSContent* CATCmdDevice::RetrieveEqualSMSContent(CSMSContent* pSMSContent)
{
	if (!pSMSContent)
		return NULL;

	int iSize = m_SMSContentVector.size();
	for(int i = 0; i < iSize; i++)
	{
		CSMSContent* pSMS = m_SMSContentVector[i];
		if (pSMS)
		{
			if (CSMSContent::IsContentEqual(pSMS, pSMSContent))
				return pSMS;
		}			
	}
	return NULL; 
}
