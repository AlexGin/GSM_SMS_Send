#if !defined(AFX_SMSCONTENT_H__18B0A6AE_3FC6_482E_B71F_E02C7FC6E891__INCLUDED_)
#define AFX_SMSCONTENT_H__18B0A6AE_3FC6_482E_B71F_E02C7FC6E891__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SMSContent.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSMSContent window

class CSMSContent
{
// Construction
public:
	CSMSContent();
	CSMSContent(const CSMSContent& refSMSContent); // Added 06.10.2014
	CSMSContent(CString strSMSText, CString strSMSPNum, COleDateTime timeSMS);

	COleDateTime& GetSMSTime();
	void SetSMSTime(COleDateTime timeSMS);

	void SetSMSTimeZone(long lTimeZone);
	long GetSMSTimeZone() const;

	void SetSMSIndex(long lIndex);
	long GetSMSIndex() const;

	void SetSMSMPIndex(long lIndex); // Added 06.10.2014
	long GetSMSMPIndex() const;		// Added 06.10.2014

	void SetDataCoding(unsigned char nDataCoding);
	unsigned char GetDataCoding() const;

	void SetSMSPNum(CString strSMSPNum);
	CString GetSMSPNum() const;

	void AddRawStringID(int iRawStringID); // Added 07.10.2014
	int  GetRawStringIDVectSize() const; // Added 07.10.2014
	int  GetRawStringIDValue(int iIndex) const; // Added 07.10.2014 
public:
	CString m_strSMSText;   // Text of SMS
public:
	static bool IsContentEqual(CSMSContent* pSMSContent1, CSMSContent* pSMSContent2); // Added 06.10.2014
protected:
	CString m_strSMSPNum;   // Phone number of SMS
	INT_VECTOR m_vectRawStringID; // Vector identifiers of raw-string (added 07.10.2014) 
public:
	unsigned char m_nTypeOfSMSPNum;
	unsigned char m_nTypeOfSMSAddr; 

	CString m_strSMSAddr;   // Address field of SMS 

protected:
	COleDateTime m_timeSMS; // Time of creation the SMS 
	long m_lTimeZone;
	unsigned char m_nDataCoding;  // Data Coding Scheme

	long m_lSMSIndex;  // Index of this SMS message
	long m_lSMSMPIndex; // Index of this SMS message in the multipart SMS (added 06.10.2014)
	// m_lSMSMPIndex: SMS multi-part identifier (Initial value==(-2) - without processing)
};
typedef std::vector<CSMSContent*> SMS_CONTENT_VECTOR; // Corrected (for use STL) 08.10.2014

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SMSCONTENT_H__18B0A6AE_3FC6_482E_B71F_E02C7FC6E891__INCLUDED_)
