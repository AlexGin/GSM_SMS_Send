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

extern CLogFilePBk g_log;

CPBRecord::CPBRecord()
{
}

CPBRecord::CPBRecord(const CString& strPhone, const CString& strName, const CString& strAddInfo)
 : m_strPhone(strPhone), m_strName(strName), m_strAddInfo(strAddInfo)  
{
}

void CPBRecord::SetAt(long nIndex, const CString& str)
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
	if (nIndex < 0 || nIndex > 2)
		g_log.SaveLogFile(PBK_ERROR, "CPBRecord::SetAt: - invalid Index (%d)", nIndex);
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
	if (nIndex < 0 || nIndex > 2)
		g_log.SaveLogFile(PBK_ERROR, "CPBRecord::GetAt: - invalid Index (%d)", nIndex);

	return _T("");
} 

CString CPBRecord::GetName() const
{
	return m_strName;
}
CString CPBRecord::GetPhone() const
{
	return m_strPhone;
}
CString CPBRecord::GetAddInfo() const
{
	return m_strAddInfo;
}
void CPBRecord::SetName(const CString& strName)
{
	m_strName = strName;
}
void CPBRecord::SetPhone(const CString& strPhone)
{
	m_strPhone = strPhone;
}
void CPBRecord::SetAddInfo(const CString& strAddInfo)
{
	m_strAddInfo = strAddInfo;
}

////////////////////////////////////////////////////
CSMSMassPBRecord::CSMSMassPBRecord()
: CPBRecord(CString(), CString(), CString()), m_iFlag(0), m_iLevel(0), m_iPosInList(1)
{
}

CSMSMassPBRecord::CSMSMassPBRecord(CString strPhone, CString strName, int iFlag, int iLevel, int iBB, int iLoyal, std::string sHK, std::string sStatus, int iPosInList)
: CPBRecord(strPhone, strName, CString()), m_iFlag(iFlag), m_iLevel(iLevel), m_iBB(iBB), m_iLoyal(iLoyal), m_sHK(sHK), m_sStatus(sStatus),  m_iPosInList(iPosInList) 
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
