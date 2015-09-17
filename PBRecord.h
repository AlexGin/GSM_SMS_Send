#if !defined(_PBRECORD_H)
#define _PBRECORD_H

#include "stdafx.h"

class CPBRecord : public CObject
{
	public: 
		CPBRecord();
		CPBRecord(CString strPhone, CString strName, CString strAddInfo);

		CString m_strName;
		CString m_strPhone;
		CString m_strAddInfo;

		void SetAt(long nIndex, CString& str);
		CString GetAt(long nIndex) const;
};

typedef CTypedPtrArray<CObArray, CPBRecord*> CPBRecordsArray;  
typedef CPBRecord* PTR_RECORD;

class CSMSMassPBRecord : public CPBRecord
{
	public: 
		CSMSMassPBRecord();
		// CSMSMassPBRecord(CString strPhone, CString strName, int iFlag, int iLevel, int iPosInList);
		CSMSMassPBRecord(CString strPhone, CString strName, int iFlag, int iLevel, int iBB, int iLoyal, std::string sHK, std::string sStatus, int iPosInList);

		int m_iFlag;
		int m_iLevel;
		int m_iPosInList;

		std::string m_sStatus; // Added 31.07.2014
		std::string m_sHK; // Added 31.07.2014
		int m_iBB; // Added 31.07.2014
		int m_iLoyal; // Added 31.07.2014
};
typedef CSMSMassPBRecord* PTR_SMS_MASS_RECORD;

typedef map<int, PTR_SMS_MASS_RECORD> SMS_MASS_MAP;
typedef pair<int, PTR_SMS_MASS_RECORD> SMS_MASS_PAIR;

class CSMSBlockPBRecord : public CPBRecord // Added 01.11.2014
{
	public: 
		CSMSBlockPBRecord();
		CSMSBlockPBRecord(CString strPhone, CString strName, int iBlockFlag, int iPosInList=-1);

		int m_iBlockFlag; // 0-active, 1-unactive
		int m_iPosInList; // Index in table ID_Black_List of data-base 
};
typedef CSMSBlockPBRecord* PTR_SMS_BLOCK_RECORD;

typedef map<int, PTR_SMS_BLOCK_RECORD> SMS_BLOCK_MAP;
typedef pair<int, PTR_SMS_BLOCK_RECORD> SMS_BLOCK_PAIR;
typedef vector<PTR_SMS_BLOCK_RECORD> SMS_BLOCK_VECTOR;

#endif  /* _PBRECORD_H */
