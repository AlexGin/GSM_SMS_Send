// PBRecord.cpp : implementation file
//

#include "stdafx.h"
#include "phonebook.h"
#include "PBRecord.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPBRecord::CPBRecord()
{
}

CPBRecord::CPBRecord(CString strPhone, CString strName, CString strAddInfo)
{
	m_strPhone  = strPhone;
	m_strName   = strName;
	m_strAddInfo= strAddInfo;
}

void CPBRecord::SetAt(long nIndex, CString& str)
{
	switch(nIndex)
	{
		case 0:
			m_strName = str;
			break;

		case 1:
			m_strPhone = str;
			break; 

		case 2:
			m_strAddInfo = str;
			break;
	}
} 

CString CPBRecord::GetAt(long nIndex) const
{
	switch(nIndex)
	{
		case 0:
			return m_strName;

		case 1:
			return m_strPhone;

		case 2:
			return m_strAddInfo;
	}
	return _T("");
} 

////////////////////////////////////////////////////
CSMSMassPBRecord::CSMSMassPBRecord()
: CPBRecord((CString)"", (CString)"", (CString)""), m_iFlag(0), m_iLevel(0), m_iPosInList(1)
{
}

// CSMSMassPBRecord::CSMSMassPBRecord(CString strPhone, CString strName, int iFlag, int iLevel, int iPosInList)
// : CPBRecord(strPhone, strName, (CString)""), m_iFlag(iFlag), m_iLevel(iLevel), m_iBB(0), m_iLoyal(0), m_iPosInList(iPosInList)
// {
// }

CSMSMassPBRecord::CSMSMassPBRecord(CString strPhone, CString strName, int iFlag, int iLevel, int iBB, int iLoyal, std::string sHK, std::string sStatus, int iPosInList)
: CPBRecord(strPhone, strName, (CString)""), m_iFlag(iFlag), m_iLevel(iLevel), m_iBB(iBB), m_iLoyal(iLoyal), m_sHK(sHK), m_sStatus(sStatus),  m_iPosInList(iPosInList) 
{
}

/////////////////////////////////////////////////////
CSMSBlockPBRecord::CSMSBlockPBRecord()
: CPBRecord((CString)"", (CString)"", (CString)""), m_iBlockFlag(1)
{
}

CSMSBlockPBRecord::CSMSBlockPBRecord(CString strPhone, CString strName, int iBlockFlag, int iPosInList)
: CPBRecord(strPhone, strName, (CString)""), m_iBlockFlag(iBlockFlag), m_iPosInList(iPosInList) 
{
}
