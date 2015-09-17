#include "StdAfx.h"
#include "PhoneBookProcessor.h"

extern CEvent g_evntReady;
extern CEvent g_evntStop;

extern CLogFilePBk g_log;

CPhoneBookProcessor::CPhoneBookProcessor(void)
: CATCmdDevice()
{
}

CPhoneBookProcessor::~CPhoneBookProcessor(void)
{
}

UINT CPhoneBookProcessor::ClearPBk(IPBkManager* pPBkMan)
{
	HWND hWndManager = pPBkMan->GetManagerHWND(); 
	try
	{		
		CString s1;
		long nMaxIndex;
		pPBkMan->ClearErrorsCounter(); // pView->m_iErrorsCounter = 0;
	
		s1.Format("AT+CPBS=\"%s\" ", GetPBType());
		SendCommand(s1, 0);
		GetMaxIndex(&nMaxIndex);

		::SendMessage(hWndManager, MYWN_STEP, SET_MAX_INDEX, nMaxIndex);

		int iCode;
		long lIndex = 1L;
		for(long i = 0; i < nMaxIndex; i++, lIndex++) 
		{		
			ProcSetPBRecord(lIndex, nMaxIndex, pPBkMan->GetPtrErrorsCounter(), true); 
		
			::SendMessage(hWndManager, MYWN_STEP, DISP_STEP, LONG(i));
			// Checking stop request:
			iCode = ::WaitForSingleObject(::g_evntStop.m_hObject, 0);
			if(iCode == WAIT_OBJECT_0)  // if stop request !  
			{	// Reset stop-flag:
				::g_evntStop.ResetEvent();
				break;
			} 
		} 
		::SendMessage(hWndManager, MYWN_STEP, DISP_END, LONG(0));
		::g_evntReady.SetEvent(); 
		return 0; 
	} 
	catch(...)
	{
		AfxMessageBox(_T("Ошибка удаления массива телефонной книги.")); 
		::SendMessage(hWndManager, MYWN_STEP, DISP_END, LONG(0));
		::g_evntReady.SetEvent(); 
		return 1; 
	}
}

UINT CPhoneBookProcessor::WritePBk(IPBkManager* pPBkMan)
{
	HWND hWndManager = pPBkMan->GetManagerHWND(); 
	try
	{		
		CString s1;
		long nMaxIndex;

		pPBkMan->ClearErrorsCounter(); // pView->m_iErrorsCounter = 0;

		g_log.SaveLogFile(PBK_WORK, "CPhoneBookView::WritePBk: NewSAMSUNG=%d.", (int)pPBkMan->IsNewSAMSUNGOn());
	
		s1.Format("AT+CPBS=\"%s\" ", GetPBType());
		SendCommand(s1, 0);
		GetMaxIndex(&nMaxIndex);
		
		CPBRecordsArray& recordsArr = pPBkMan->GetRecordsArray();
		long nSize = recordsArr.GetSize();
		if (nSize > nMaxIndex)
		{
			AfxMessageBox(
				_T("Массив превышает объем выбранной телефонной книги (сохранение отменено).")); 
			::SendMessage(hWndManager, MYWN_STEP, DISP_END, LONG(0));
			::g_evntReady.SetEvent(); 
			 return 1;
		} 

		::SendMessage(hWndManager, MYWN_STEP, SET_MAX_INDEX, nMaxIndex);

		int iCode;
		long lIndex = 1L;
		for(long i = 0; i < nSize; i++, lIndex++) 
		{
			CPBRecord* pRec = recordsArr.GetAt(i); // The 0 - start index in this array
			if (pRec == NULL)
				break;

			SetPBRecord(pRec);
			if (pPBkMan->IsUnicodeOn())
				ConvertPBStringToWChar();

			if (ProcSetPBRecord(lIndex, nMaxIndex, pPBkMan->GetPtrErrorsCounter()) != 0L) 
				break;

			::SendMessage(hWndManager, MYWN_STEP, DISP_STEP, LONG(i));
			// Checking stop request:
			iCode = ::WaitForSingleObject(::g_evntStop.m_hObject, 0);
			if(iCode == WAIT_OBJECT_0)  // if stop request !  
			{	// Reset stop-flag:
				::g_evntStop.ResetEvent();
				break;
			} 
		} 
		::SendMessage(hWndManager, MYWN_STEP, DISP_END, LONG(0));
		::g_evntReady.SetEvent(); 
		return 0; 
	}
	catch(...)
	{
		AfxMessageBox(_T("Ошибка сохранения массива телефонной книги .")); 
		::SendMessage(hWndManager, MYWN_STEP, DISP_END, LONG(0));
		::g_evntReady.SetEvent(); 
		return 1; 
	}
}

UINT CPhoneBookProcessor::ReadPBk(IPBkManager* pPBkMan)
{	
	HWND hWndManager = pPBkMan->GetManagerHWND();
	try
	{
		g_log.SaveLogFile(PBK_WORK, "CPhoneBookProcessor::ReadPBk: NewSAMSUNG=%d.", (int)pPBkMan->IsNewSAMSUNGOn());

		CString s1;
		long nMaxIndex;
		pPBkMan->ClearErrorsCounter(); // pView->m_iErrorsCounter = 0;

		s1.Format("AT+CPBS=\"%s\" ", GetPBType());
		SendCommand(s1, 0);
		GetMaxIndex(&nMaxIndex);  

		::SendMessage(hWndManager, MYWN_STEP, SET_MAX_INDEX, nMaxIndex);

		CPBRecordsArray& recordsArr = pPBkMan->GetRecordsArray();

		int iCode;
		for(long i = 1; i <= nMaxIndex; i++) 
		{
			CPBRecord* pRec = new CPBRecord; 
			int iErrorCounter = 0;
			SetPBRecord(pRec);
			long lRes = ProcGetPBRecord(i, nMaxIndex, &iErrorCounter); // &pView->m_iErrorsCounter);
			if (lRes != 0L) // && (pView->m_bProcAllRec != TRUE)) 
				break;

			if (iErrorCounter) // Added 18.10.2014
				g_log.SaveLogFile("CPhoneBookProcessor::ReadPBk: Do not exist valid record on position with ID=%d.", i);

			if (!lRes)
			{	
				CString strPN = GetPBRecord()->m_strPhone;
				CString strName = GetPBRecord()->m_strName;
				g_log.SaveLogFile(PBK_WORK, "CPhoneBookProcessor::ReadPBk: PN=%s, Name=%s, ID=%d.", strPN, strName, i); 

				pRec->m_strAddInfo.Format(_T("%s -  <index> = %u"), GetPBType(), i); 
	
				recordsArr.Add(pRec);   
			}

			::SendMessage(hWndManager, MYWN_STEP, DISP_STEP, LONG(i));
			
			// Checking stop request:
			iCode = ::WaitForSingleObject(::g_evntStop.m_hObject, 0);
			if(iCode == WAIT_OBJECT_0)  // if stop request !
			{	// Reset stop-flag:
				::g_evntStop.ResetEvent();
				break;
			} 
		} 
		::SendMessage(hWndManager, MYWN_STEP, DISP_END, LONG(0));
		::g_evntReady.SetEvent(); 
		return 0; 
	}
	catch(...)
	{
		AfxMessageBox(_T("Ошибка получения массива телефонной книги .")); 
		::SendMessage(hWndManager, MYWN_STEP, DISP_END, LONG(0));
		::g_evntReady.SetEvent(); 
		return 1; 
	}
}
