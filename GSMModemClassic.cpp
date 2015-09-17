#include "StdAfx.h"
#include "PhoneBook.h"
#include "GSMModemClassic.h"
#include "PDUEncoder.h"

extern CLogFilePBk g_log; 

CGSMModemClassic::CGSMModemClassic(void)
{
}

CGSMModemClassic::~CGSMModemClassic(void)
{
}

void CGSMModemClassic::USSDSend(CString& strUSSDText, bool bPhoneOnLine)
{
	CWnd* pWnd = AfxGetApp()->GetMainWnd();
	if(!bPhoneOnLine)
	{
		::MessageBox(pWnd->m_hWnd, "GSM модем не подключен!", "PhoneBook", MB_OK | MB_ICONINFORMATION);
		return;
	}

	AfxGetApp()->DoWaitCursor(1); // Start of waiting cursor // BeginWaitCursor();

	CString strUSSDToSend = strUSSDText;

	CPhoneBookApp::ShowUSSDOutput(&strUSSDToSend, 0L); // Instead of: ShowUSSDText(strUSSDToSend, false); // 0L-Request

	Waiting();	
	AfxGetApp()->DoWaitCursor(0);

	CString s1 = _T("ATE0V0"); 
	SendCommand(s1, 1); 
	s1 = _T("");
	GetAnswer(s1); 

	CPhoneBookApp* pApp = (CPhoneBookApp*)::AfxGetApp();

	CString s111;
	s111.Format("AT+CPIN=\"%u\"", pApp->m_iPINCodeVal);
	CString s222 = _T("");
	SendCommand(s111, 0); 
	GetAnswer(s222);  
  
	g_log.SaveLogFile(PBK_DEBUG, "3G Modem answer +CPIN: %s", s222);

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

	g_log.SaveLogFile(PBK_DEBUG, "3G Modem answer +CSCS?: %s", s21); 

	/* s11 = _T("AT+CUSD?");
	s21 = _T("");
	m_phone.SendCommand(s11, 0); 
	m_phone.GetAnswer(s21); 
	g_log.SaveLogFile(PBK_DEBUG, "3G Modem answer +CUSD?: %s", s21); */ 
	
	CString strPDU;
	CPDUEncoder* pPDUEncoder = new CPDUEncoder();
	if (pPDUEncoder)
	{
		g_log.SaveLogFile(PBK_DEBUG, "USSD 3G Modem sending request: %s", strUSSDToSend);
		pPDUEncoder->PrepareString_7Bits(strUSSDToSend, &strPDU, FALSE);
		s1.Format("AT+CUSD=1,\"%s\",15",strPDU);
	}
	// s1.Format("AT+CUSD=1,\"AA180C3602\",15"); // It's get balance: *100# 
	SendCommand(s1, 0);
	GetSpecialAnswer(m_strCUSDRawResult);
	
	CString s22 = _T("");

	if (pPDUEncoder && (m_strCUSDRawResult.GetLength() > 0))
	{	
		CString strAT; strAT.Format("AT");
		SendCommand(strAT, 0); // Handshake answer from 3G modem

		s22 = m_strCUSDRawResult;
		m_strCUSDRawResult = _T("");
		PrepareCUSD3GPCUIAnswer(s22);

		if (1 == pApp->m_iUSSDUnicode) // Added 25.08.2014
			ConvertString_UCS2(s22); // Added 25.08.2014
		else // Added 25.08.2014
			ConvertString_7Bits(s22);

		PrepareCUSDAnswer(s22);	
	}

	AfxGetApp()->DoWaitCursor(-1); // End of waiting cursor //EndWaitCursor();

	CString strLog0; strLog0.Format("USSD 3G Modem answer: %s", s22);
	g_log.SaveLogFile(PBK_DEBUG, (CString&)strLog0); // Do not change this call!

	CPhoneBookApp::ShowUSSDOutput(&s22, 1L); // Instead of: ShowUSSDText(s22, true); // 1L-Answer

	if (pPDUEncoder)
		delete pPDUEncoder;
}

long CGSMModemClassic::PrepareCUSD3GPCUIAnswer(CString& strCUSDAnswer)
{
	CString strInput(strCUSDAnswer);
	int iCUSDIndex = strInput.Find("CUSD:");
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
			if (ch != 0x22) // Symbol '"'
				strOut.Insert(iIndexOut++, ch);
			else
				break;
		}
	}
	strCUSDAnswer.Format("%s", strOut);
	return 0L;
}

