#if !defined(AFX_PDUENCODER_H__A538D136_1C04_486B_8EF7_1432DA9A4482__INCLUDED_)
#define AFX_PDUENCODER_H__A538D136_1C04_486B_8EF7_1432DA9A4482__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PDUEncoder.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPDUEncoder window

class CPDUEncoder
{
// Construction
public:
	CPDUEncoder(); // Added 29.06.2014
	CPDUEncoder(class CATCmdDevice* pATDev);
	void FillRawSCANew(CString& strRawSCA); // For New SAMSUNG support (added 12.06.2015)
	// void FillRawSCA(CString& strRawSCA); // Do not use in contemporary version
	void FillPNum(CString& strPhoneNumber);
	void IntToHex(int iInp, CString* pStrBaseString, BOOL bAddLenPDU = FALSE);
	void AppendPhoneNumber(CString& strPhoneNumber, CString* pStrBaseString, BOOL bAddLenPDU = FALSE);

	long PopulatePDUString(int iPartIndex = 0);
	long PrepareString_UCS2(CString& strString, CString* pStrBaseString, BOOL bAddLenPDU, BYTE btPartNum=0);
	long PrepareString_7Bits(CString& strString, CString* pStrBaseString, BOOL bAddLenPDU, BYTE btPartNum=0);
	unsigned char PrepareDCS(CString& strSMSText);

	// BOOL IsMessageLarge(CString& strString, unsigned char nDCS);
	BYTE m_btRefNumber;		// Added 12.05.2013
	BYTE m_btTotalParts;	// Added 12.05.2013
// Attributes
public:

// Operations
public:	
	CString GetSCA(){return m_strSCA;}
	CString GetPNum(){return m_strPNum;}
	CString GetSMSText(){return m_strSMSText;}
	CString GetPDUString(){return m_strPDU;}
	long GetPDULength(){return m_lPDULength;}

	void SetSMSText(CString strSMSText);
	void SetSCA(CString strSCA);
	void SetPNum(CString strPNum);

public:
	virtual ~CPDUEncoder();

protected:
	CString m_strPDU;
	CString m_strSMSText;
	long m_lPDULength;

	CString m_strSCA;				// Service Center Address (Phone Number)
	unsigned char m_nTypeOfSCA;		// Type of Service Center Address:
									// 145 (0x91) - international type of phone number
									// 129 (0x81) - local type of phone number
	CString m_strPNum;		// Destination Phone Number
	unsigned char m_nTypeOfPNum;	// 145 (0x91) - international type of phone number
									// 129 (0x81) - local type of phone number
	unsigned char m_nDCS;	// Data-Coding-Scheme

	class CATCmdDevice* m_pATDev;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PDUENCODER_H__A538D136_1C04_486B_8EF7_1432DA9A4482__INCLUDED_)
