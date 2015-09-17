#include "StdAfx.h"
#include "NewSamsung.h"
#include "Serial.h"	// RS232 support
#include "PDUEncoder.h"

extern CEvent g_evntReady;
extern CEvent g_evntStop;

extern CLogFilePBk g_log;

CNewSamsung::CNewSamsung(void)
{
}

CNewSamsung::~CNewSamsung(void)
{
}

int CNewSamsung::ReadMemoryIndex(CString& strInput)
{
	int iResult = -1;
	CString strInp(strInput);
	int iIndex = strInp.Find("+CMGW:");
	if (-1 != iIndex)
	{
		int iLen = strInp.GetLength();
		int iOutIndex = 0;
		CString strOut;
		for (int i = iIndex; i < iLen; i++)
		{
			TCHAR ch = strInp[i];
			if ((ch >= 0x30) && (ch <= 0x39))
				strOut.Insert(iOutIndex++, ch); 
		}
		iResult = atoi((LPCTSTR)strOut);
	}
	return iResult;
}

long CNewSamsung::SendSmsMass(ISMSSender* pSender)
{	
	HWND hWndSender = pSender->GetSenderHWND();
	try
	{
		long lTotalSec = pSender->GetTotalSections();
		g_log.SaveLogFile(SMS_MASS, "CNewSamsung: start SMS Mass sending (total %u sections)", lTotalSec);

		CString strLog0, s1;
		CString s222 = _T("");
		
		s1 = _T("AT+CMGF=0");
		
		SendCommand(s1, 0); 
		s1 = _T("");
		GetAnswer(s1);
  
		g_log.SaveLogFile(SMS_MASS, "Answer +CMGF: %s", s1);

		CString strSCA = _T("AT+CSCA?");
		SendCommand(strSCA, 0);  
		strSCA = _T("");
		GetAnswer(strSCA); 
		
		ConvertString_UCS2(strSCA); 
		CString strTemp = ClearSCAString(strSCA); 
		strSCA.Format("%s", strTemp); 

		g_log.SaveLogFile(SMS_MASS, "Answer +CSCA: %s", strSCA);
		
		::SendMessage(hWndSender, MYWN_STEP_SMS_PHONE, (WPARAM)1L, (LPARAM)0L); // Added 04.08.2014

		CString strSmsText = pSender->GetSmsText();
		CString strTextInfo; strTextInfo.Format("SMS text: %s", strSmsText);
		g_log.SaveLogFile(SMS_MASS, (CString&)strTextInfo);

		std::vector<int> vectMemoryIndex;

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

			if (bFirstPass)
			{
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
					long nNeedRepeat = 3L; // Repeat counter (for SAMSUNG)
					bool bSuccess = false; 
					int iMemoryIndex = -1;
					do
					{					
						s2.Format("AT+CMGW=%d,\r", nLen);// s2.Format("AT+CMGS=%d,\r", nLen); 
						SendCommand(s2, 0);
						s2 = _T("");
						for (int i = 0; i < 6; i++)
						{	
							Waiting();					
							::Sleep(1000);
							GetAnswer(s2); 
							if (IsPrompt(s2)) // It is "> " string
							{								
								g_log.SaveLogFile(SMS_MASS, "Answer +CMGW (prompt is ready): %s", s2);
								break;
							}
						}		
						
						PurgeComPort(PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

						s2.Format("%s\x1a",  LPCTSTR(strPDUString)); 
						SendCommand(s2, 0);
						s2 = _T("");			
						GetAnswer(s2);
						
						if (-1 == s2.Find("+CMS ERROR: 304")) // If sending without errors
						{
							nNeedRepeat = 0L; // The repeat counter set in zero
							bSuccess = true;
							CString strAns = ClearString(s2, true);
							iMemoryIndex = ReadMemoryIndex(strAns);
							vectMemoryIndex.push_back(iMemoryIndex);
						}
						else // If error occur - set the bNeedRepeat flag
							nNeedRepeat--; // Decrement the repeat counter
					
						g_log.SaveLogFile(SMS_MASS, "CNewSamsung: answer +CMGW: %s", s2);
					}while (nNeedRepeat);
							
					if (pPDUEncoder)
						delete pPDUEncoder;

					if (!bSuccess) 
					{
						g_log.SaveLogFile(PBK_ERROR, "Fatal failure (+CMS ERROR: 304) for SAMSUNG!");
						// PostMessage to the main thread - insteand of call AfxMessageBox(...));
						::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L);
						::g_evntReady.SetEvent();
						::PostMessage(hWndSender, MYWN_SHOW_ERROR_FAILURE, (WPARAM)0L, (LPARAM)4L);
						return 1L;
					}
				}
			}
			int iSendPart = 0;
			for (std::vector<int>::const_iterator cit=vectMemoryIndex.begin();cit!=vectMemoryIndex.end();cit++)
			{
				CString strPhoneNum = pSmsMassRecord->m_strPhone;
				ConvertStringToWChar(strPhoneNum);
				int iMemIndex = *cit;
				CString sOut;				
				sOut.Format("AT+CMSS=%d,\"%s\",145", iMemIndex, strPhoneNum);
				SendCommand(sOut, 0);
				g_log.SaveLogFile(PBK_DEBUG, (CString&)sOut);
	
				CString s27 = _T("");
				for (int i = 0; i < 3; i++)
				{	
					Waiting();	
					GetAnswer(s27);
					if (s27.GetLength() > 3) 
						break;
					::Sleep(1000);	 
				}

				CString strAnswer = ClearString(s27, true);
				CString strInfo; strInfo.Format("Шаг %d/%d, Номер: %s (Ответ устройства \"%s\")\r\n",
					iIndex, (iSendPart+1), sPhoneNum.c_str(), strAnswer);
				
				char* szStr = new char[1024];
				memset(szStr, 0, sizeof(char)*1024);
				strcpy(szStr, strInfo);

				lCurrSection++;
				::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)(szStr), (LPARAM)lCurrSection);
				g_log.SaveLogFile(SMS_MASS, (CString&)strInfo);	
				iSendPart++;
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

		for (std::vector<int>::const_iterator it=vectMemoryIndex.begin();it!=vectMemoryIndex.end();it++)
		{
			int iMemDeleteIndex = *it;
			CString sMemDelete;
			sMemDelete.Format("AT+CMGD=%d", iMemDeleteIndex); 
			SendCommand(sMemDelete, 0);
			g_log.SaveLogFile(PBK_DEBUG, (CString&)sMemDelete);

			sMemDelete = _T("");
			GetAnswer(sMemDelete);
			CString strAnswerDelete = ClearString(sMemDelete, true);
			g_log.SaveLogFile(PBK_DEBUG, "Answer +CMGD=: %s", strAnswerDelete); 
		}
		vectMemoryIndex.clear();
					
		::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L); // End of SMSMass sending
		::g_evntReady.SetEvent(); 
		return 0L;
	}
	catch(...)
	{
		g_log.SaveLogFile(PBK_ERROR, "Catch exception - during SMS sending.");
		// AfxMessageBox(_T("SAMSUNG: ошибка рассылки SMS."));
		::SendMessage(hWndSender, MYWN_STEP_SMS_MASS, (WPARAM)0L, (LPARAM)0L);
		::g_evntReady.SetEvent(); 
		::PostMessage(hWndSender, MYWN_SHOW_ERROR_FAILURE, (WPARAM)0L, (LPARAM)3L);
		return 1L;
	}
}

/////////////////////////////////////////////////
//     SMS Support (MultipartSMS) for NewSAMSUNG
////////////////////////////////////////////////
bool CNewSamsung::IsPrompt(CString& strInput)
{
	return (bool)(-1 != strInput.Find("> "));
}
