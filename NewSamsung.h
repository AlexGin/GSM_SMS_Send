#pragma once
#include "atcmddevice.h"

class CNewSamsung :	public CATCmdDevice // Smartphones SAMSUNG support
{
public:
	CNewSamsung(void);
	virtual ~CNewSamsung(void);

public:
	virtual long SendSmsMass(ISMSSender* pSender);

	bool IsPrompt(CString& strInput);	
	int  ReadMemoryIndex(CString& strInput);
};
