// SMSContent.cpp : implementation file
//

#include "stdafx.h"
#include "phonebook.h"
#include "SMSContent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSMSContent

CSMSContent::CSMSContent()
: m_lSMSIndex(-1L), // This is not exist message
  m_lSMSMPIndex(-2), // It is initial value (without processing)
  m_nTypeOfSMSPNum(0),
  m_nTypeOfSMSAddr(0),
  m_nDataCoding(0),
  m_lTimeZone(0L) 
{	  
}

CSMSContent::CSMSContent(const CSMSContent& refSMSContent)
{
	m_strSMSText = refSMSContent.m_strSMSText;
	m_strSMSPNum = refSMSContent.m_strSMSPNum;
	m_strSMSAddr = refSMSContent.m_strSMSAddr;
	m_timeSMS	 = refSMSContent.m_timeSMS;
	m_lSMSIndex	 = refSMSContent.m_lSMSIndex;
	m_lSMSMPIndex= refSMSContent.m_lSMSMPIndex;
	m_nTypeOfSMSPNum = refSMSContent.m_nTypeOfSMSPNum;
	m_nTypeOfSMSAddr = refSMSContent.m_nTypeOfSMSAddr;
	m_nDataCoding= refSMSContent.m_nDataCoding;
	m_lTimeZone	 = refSMSContent.m_lTimeZone;
}

CSMSContent::CSMSContent(CString strSMSText,CString strSMSPNum,COleDateTime timeSMS)
: m_strSMSText(strSMSText),
  m_strSMSPNum(strSMSPNum),
  m_timeSMS(timeSMS),
  m_lSMSIndex(0L), // This is not indexed message
  m_lSMSMPIndex(-2), // It is initial value (without processing)
  m_nTypeOfSMSPNum(0),
  m_nTypeOfSMSAddr(0),
  m_nDataCoding(0),
  m_lTimeZone(0L)
{	
}

void CSMSContent::SetSMSTime(COleDateTime timeSMS)
{
	m_timeSMS = timeSMS;
}

COleDateTime& CSMSContent::GetSMSTime()
{
	return m_timeSMS;
}

void CSMSContent::SetSMSTimeZone(long lTimeZone)
{
	m_lTimeZone = lTimeZone;
}

long CSMSContent::GetSMSTimeZone() const
{
	return m_lTimeZone;
}

void CSMSContent::SetSMSIndex(long lIndex)
{
	m_lSMSIndex = lIndex;
}

long CSMSContent::GetSMSIndex() const
{
	return m_lSMSIndex;
}

void CSMSContent::SetSMSMPIndex(long lIndex)
{
	m_lSMSMPIndex = lIndex;
}

long CSMSContent::GetSMSMPIndex() const
{
	return m_lSMSMPIndex;
}

bool CSMSContent::IsContentEqual(CSMSContent* pSMSContent1, CSMSContent* pSMSContent2)
{
	if ((pSMSContent1->m_strSMSPNum == pSMSContent2->m_strSMSPNum) &&
		(pSMSContent1->m_lSMSMPIndex == pSMSContent2->m_lSMSMPIndex))
	{		
		CTime time1, time2;
		SYSTEMTIME st1, st2;
		if (pSMSContent1->GetSMSTime().GetAsSystemTime(st1))
			time1 = CTime(st1);
		if (pSMSContent2->GetSMSTime().GetAsSystemTime(st2))
			time2 = CTime(st2);

		time_t t1 = time1.GetTime();
		time_t t2 = time2.GetTime();

		int iDelta = abs((int)(t1 - t2));

		if (abs(iDelta) <= 60) // One minute (60 seconds)
			return true;
		else
			return false;
	}
	return false;
}

void CSMSContent::SetDataCoding(unsigned char nDataCoding)
{
	m_nDataCoding = nDataCoding;
}

unsigned char CSMSContent::GetDataCoding() const
{
	return m_nDataCoding;
}

void CSMSContent::SetSMSPNum(CString strSMSPNum)
{
	if (m_nTypeOfSMSPNum == 0x91) // International format of Phone Number
		m_strSMSPNum.Format("+%s", strSMSPNum);
	else
		m_strSMSPNum.Format("%s", strSMSPNum);
}

CString CSMSContent::GetSMSPNum() const
{
	return m_strSMSPNum;
}

void CSMSContent::AddRawStringID(int iRawStringID)
{
	m_vectRawStringID.push_back(iRawStringID);
}

int CSMSContent::GetRawStringIDValue(int iIndex) const
{
	int iResult = (-1);
	int iSize = (int)m_vectRawStringID.size();
	if (iIndex < iSize)
		iResult = m_vectRawStringID[iIndex];

	return iResult;
}

int CSMSContent::GetRawStringIDVectSize() const
{
	int iSize = (int)m_vectRawStringID.size();
	return iSize;
}
