// PDUEncoder.cpp : implementation file
//

#include "stdafx.h"
#include "phonebook.h"
#include "PDUEncoder.h"
#include "ATCmdDevice.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPDUEncoder

CPDUEncoder::CPDUEncoder(CATCmdDevice* pATDev)
: m_pATDev(pATDev) 
{	
	m_nTypeOfSCA = 0;
	m_nTypeOfPNum= 0;

	m_lPDULength = 0L;

	m_nDCS = 0;
	m_btRefNumber = 0x01; // DEBUG !!!
	m_btTotalParts = 1;
}

CPDUEncoder::CPDUEncoder()
: m_pATDev(NULL) 
{
	m_nTypeOfSCA = 0;
	m_nTypeOfPNum= 0;

	m_lPDULength = 0L;

	m_nDCS = 0;
	m_btRefNumber = 0x01; // DEBUG !!!
	m_btTotalParts = 1;
}

CPDUEncoder::~CPDUEncoder()
{
}

void CPDUEncoder::SetSCA(CString strSCA)
{
	m_strSCA = strSCA;
}

void CPDUEncoder::SetSMSText(CString strSMSText)
{
	m_strSMSText = strSMSText;
}

void CPDUEncoder::SetPNum(CString strPNum)
{
	m_strPNum = strPNum;
}

unsigned char CPDUEncoder::PrepareDCS(CString& strSMSText) 
{
	int iSMSLen = strSMSText.GetLength();
	unsigned char ucResult = 0;

	for (int i = 0; i < iSMSLen; i++)
	{
		unsigned char charValue = (unsigned char)(strSMSText.GetAt(i));
		if ( ((charValue >= 32) && (charValue <= 63)) ||
			 ((charValue >= 65) && (charValue <= 90)) ||
			 ((charValue >= 97) && (charValue <= 122)) )
			{
				continue;
			}
			else
			{
				ucResult = 8;
			}
	}

	return ucResult;
}

void CPDUEncoder::IntToHex(int iInp, CString* pStrBaseString, BOOL bAddLenPDU)
{
	int iCurPosition = pStrBaseString->GetLength();

	if (iInp <= 255)
	{
		TCHAR ch_h = (TCHAR)(iInp / 16);
		TCHAR ch_l = (TCHAR)(iInp % 16);

		TCHAR tcRes_h = 0x30;
		TCHAR tcRes_l = 0x30;

		if((ch_h & 0x0f) >= 0x0a)
			tcRes_h += 0x07;
			
		tcRes_h += ch_h;
		pStrBaseString->Insert(iCurPosition++, tcRes_h);
		
		if((ch_l & 0x0f) >= 0x0a)
			tcRes_l += 0x07;

		tcRes_l += ch_l;
		pStrBaseString->Insert(iCurPosition++, tcRes_l);

		if (bAddLenPDU)
			m_lPDULength++;
	}
}

/* void CPDUEncoder::FillRawSCA(CString& strRawSCA)
{
	BOOL bAddToString = FALSE;
	int iCurIndex = 0;

	m_nTypeOfSCA = 0x81;
	m_strSCA = _T("");

	for(int i = 0; i < strRawSCA.GetLength(); i++)
	{
		TCHAR ch = strRawSCA.GetAt(i);
		if (ch == (TCHAR)0x22)   // The " character 
			bAddToString = !bAddToString;

		if (ch == (TCHAR)0x2b)   // The + character 
			m_nTypeOfSCA = 0x91;
		
		if (bAddToString && ((ch >= (TCHAR)0x30) && (ch <= (TCHAR)0x39)) )
			m_strSCA.Insert(iCurIndex++, ch);
	}
} */ 

void CPDUEncoder::FillRawSCANew(CString& strRawSCA) // For NewSAMSUNG support (added 12.06.2015)
{
	BOOL bAddToString = FALSE;
	int iCurIndex = 0;

	m_nTypeOfSCA = 0x91; // 0x81; // Corrected 15.06.2015
	m_strSCA = _T("");

	for(int i = 0; i < strRawSCA.GetLength(); i++)
	{
		TCHAR ch = strRawSCA.GetAt(i);

		if (ch == (TCHAR)0x2b)   // The + character 
			bAddToString = TRUE;
 
		if (bAddToString && ((ch >= (TCHAR)0x30) && (ch <= (TCHAR)0x39)) )
			m_strSCA.Insert(iCurIndex++, ch);

		if (12 == iCurIndex) // Corrected 15.06.2015
			break;
	}
}

void CPDUEncoder::FillPNum(CString& strPhoneNumber)
{
	int iCurIndex = 0;
	m_strPNum =_T("");
	m_nTypeOfPNum = 0x81;
	for (int i = 0; i < strPhoneNumber.GetLength(); i++)
	{
		TCHAR ch = strPhoneNumber.GetAt(i);

		if (ch == (TCHAR)0x2b)   // The + character 
			m_nTypeOfPNum = 0x91;

		if ((ch >= (TCHAR)0x30) && (ch <= (TCHAR)0x39))
			m_strPNum.Insert(iCurIndex++, ch);
	}
}

long CPDUEncoder::PopulatePDUString(int iPartIndex)
{
	IntToHex(0x07, &m_strPDU);
	
	IntToHex(m_nTypeOfSCA, &m_strPDU);
	AppendPhoneNumber(m_strSCA, &m_strPDU);

	// Start the main PDU string:
	if (1 == m_btTotalParts)
		IntToHex(0x11, &m_strPDU, TRUE);  // First octet of SMS-SUBMIT: 0x11 - PDU without User-Data-Header
	else
		IntToHex(0x51, &m_strPDU, TRUE);  // First octet of SMS-SUBMIT: 0x51 - PDU with User-Data-Header

	IntToHex(0x00, &m_strPDU, TRUE);

	int iPNumLen = m_strPNum.GetLength();

	IntToHex((iPNumLen & 0x0f), &m_strPDU, TRUE);
	IntToHex(m_nTypeOfPNum, &m_strPDU, TRUE);
	AppendPhoneNumber(m_strPNum, &m_strPDU, TRUE);

	IntToHex(0x00, &m_strPDU, TRUE);

	BYTE nSMSTextLength = (BYTE)(m_strSMSText.GetLength() * 2);
	if (0 == nSMSTextLength)
	{
		int iRes = AfxMessageBox(IDS_SMS_EMPTY, MB_YESNO | MB_ICONQUESTION); 
		if (IDYES != iRes)
			return 1L;
	}

	m_nDCS = 8; // PrepareDCS(m_strSMSText); 

	// if (IsMessageLarge(m_strSMSText, m_nDCS))
	// {
	//	AfxMessageBox(IDS_SMS_LARGE, MB_OK | MB_ICONSTOP); 
	//	return 1L;
	// } 

	IntToHex(m_nDCS, &m_strPDU, TRUE); 
	IntToHex(0xA7, &m_strPDU, TRUE);
	if (1 == m_btTotalParts)
	{
		if (m_nDCS == 8)   // Data Coding Scheme: UNICODE, Russian
			PrepareString_UCS2(m_strSMSText, &m_strPDU, TRUE);
		else					// Standard (Latin) scheme
			PrepareString_7Bits(m_strSMSText, &m_strPDU, TRUE);
	}
	else
	{		
		BYTE btPartNum = (BYTE)(iPartIndex + 1);
		if (m_nDCS == 8)   // Data Coding Scheme: UNICODE, Russian
			PrepareString_UCS2(m_strSMSText, &m_strPDU, TRUE, btPartNum);
		else					// Standard (Latin) scheme
			PrepareString_7Bits(m_strSMSText, &m_strPDU, TRUE, btPartNum);
	}
	return 0L;
}
/*
BOOL CPDUEncoder::IsMessageLarge(CString& strString, unsigned char nDCS)
{
	int iLength = strString.GetLength();

	if (nDCS == 8)  // Data Coding Scheme: UNICODE, Russian
	{
		if (iLength > 70)
			return TRUE;
	}
	else   // Standard (Latin) scheme
	{
		if (iLength > 160)
			return TRUE;
	}

	return FALSE;
} */ 

long CPDUEncoder::PrepareString_UCS2(CString& strString, CString* pStrBaseString, BOOL bAddLenPDU, BYTE btPartNum)
{
	CString strOut;
	long lCurChar = 0L;

	for(int i = 0; i < strString.GetLength(); i++)
	{
		unsigned char uch = (unsigned char)(strString.GetAt(i));

		if (uch >= 0x20)
			strOut.Insert(lCurChar++, (TCHAR)(uch)); 

		if ((uch == 0x0d) || (uch == 0x0a))
		  strOut.Insert(lCurChar++, (TCHAR)0x20);
	}

	CString strWChar = strOut;
	m_pATDev->ConvertStringToWChar(strWChar);

	int iWCharLen = strWChar.GetLength();
	int iMsgLen = (int)(iWCharLen / 2); 
	int iTextLen = iMsgLen;

	if (btPartNum > 0)
		iMsgLen += 6;  

	BYTE btMsgLen_l = (BYTE)(iMsgLen % 256);
	IntToHex(btMsgLen_l, pStrBaseString, bAddLenPDU); // Length of UD (User Data)
	if (btPartNum > 0)
	{
		IntToHex(0x05, pStrBaseString, bAddLenPDU); // Length of UDH (User Data Header)
		IntToHex(0x00, pStrBaseString, bAddLenPDU);  
		IntToHex(0x03, pStrBaseString, bAddLenPDU);
		IntToHex(m_btRefNumber, pStrBaseString, bAddLenPDU);
		IntToHex(m_btTotalParts, pStrBaseString, bAddLenPDU);
		IntToHex(btPartNum, pStrBaseString, bAddLenPDU);
	}
	int iCurPosition = pStrBaseString->GetLength();

	for(int j = 0; j < iWCharLen; j++)
		pStrBaseString->Insert(iCurPosition++, strWChar.GetAt(j));
	
	if (bAddLenPDU)
		m_lPDULength += iTextLen;
		
	return 0L;
}

long CPDUEncoder::PrepareString_7Bits(CString& strString, CString* pStrBaseString, BOOL bAddLenPDU, BYTE btPartNum)
{
	int iInpLen = strString.GetLength();

	int iOutLen = iInpLen - (iInpLen / 8);
				
	unsigned char* pOutBytes = new unsigned char[iOutLen];
	::ZeroMemory((void*)pOutBytes, iOutLen);
				
	int i = 0, j = 0;
	int shrCounter = 0, shlCounter = 7;

	while ((i < iInpLen) && (j < iOutLen))
	{
		// Copy the ith byte from the inputBytes array. 
		unsigned char currentByte = (unsigned char)strString.GetAt(i);

		currentByte = (unsigned char)((UINT)currentByte >> shrCounter);

		// Extract low bits from the next byte's bit sequence,
		// and prepend it to the current byte's bit sequence.					
		if ((i + 1) < iInpLen)
		{						
			unsigned char nextByte = (unsigned char)strString.GetAt(i+1);
			unsigned char extraBits = (unsigned char)((int)nextByte << shlCounter);
						
			// Prepend the bits to the left (high bits)
			// of the current byte's bit sequence.
			currentByte = (unsigned char)(currentByte | extraBits);
		}

		pOutBytes[j] = currentByte;

		if ((i + 1) % 8 != 0)
		{	
			// The next character's position is not 
			// a multiple of 8, so continue with 
			// normal processing.
			i += 1;
			j += 1;
			shrCounter += 1;
			shlCounter -= 1;												
		}
		else 
		{	
			// Skip the next character.
			i += 1;

			// Reset shift counters.
			shrCounter = 0;
			shlCounter = 7;										
		}			
	}

	if (btPartNum > 0)
		iInpLen += 6; // 7; // 12; // 6; // 0xC;

	BYTE btMsgLen_l = (BYTE)(iInpLen % 256);
	// Next line was commented out 02.07.2014:
	/*	IntToHex(btMsgLen_l, pStrBaseString, bAddLenPDU); */ // Length of UD (User Data)	
	if (btPartNum > 0)
	{
		IntToHex(0x05, pStrBaseString, bAddLenPDU); // Length of UDH (User Data Header)
		IntToHex(0x00, pStrBaseString, bAddLenPDU);  
		IntToHex(0x03, pStrBaseString, bAddLenPDU);
		IntToHex(m_btRefNumber, pStrBaseString, bAddLenPDU);
		IntToHex(m_btTotalParts, pStrBaseString, bAddLenPDU);
		IntToHex(btPartNum, pStrBaseString, bAddLenPDU);
	}

	int iCurPosition = pStrBaseString->GetLength();

	for (int iIndex = 0; iIndex < iOutLen; iIndex++)
	{
		unsigned char uch = pOutBytes[iIndex];
		IntToHex(uch, pStrBaseString, bAddLenPDU);
	}

	delete[] pOutBytes;

	return 0L;
}

void CPDUEncoder::AppendPhoneNumber(CString& strPhoneNumber, CString* pStrBaseString, BOOL bAddLenPDU)
{
	int iCurPosition = pStrBaseString->GetLength();

	CString strPNum = strPhoneNumber;
	strPNum.TrimLeft();
	strPNum.TrimRight();

	int iLen = strPNum.GetLength();
	if (iLen > 0)
	{
		TCHAR ch;
		for (int i = 0; i < iLen; i += 2)
		{
			if ((i+1) < iLen)
			{
				ch = strPNum.GetAt(i+1);
				pStrBaseString->Insert(iCurPosition++, ch);
			}
			else
				pStrBaseString->Insert(iCurPosition++, (TCHAR)(0x46)); // "F" character

			ch = strPNum.GetAt(i);
			pStrBaseString->Insert(iCurPosition++, ch);

			if (bAddLenPDU)
				m_lPDULength++;
		}
	}
}



