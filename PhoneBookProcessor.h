#pragma once
#include "PhoneBook.h"
#include "ATCmdDevice.h"

class CPhoneBookProcessor : public CATCmdDevice
{
public:
	CPhoneBookProcessor(void);
	virtual ~CPhoneBookProcessor(void);
public:
	virtual UINT ClearPBk(IPBkManager* pPBkMan);
	virtual UINT WritePBk(IPBkManager* pPBkMan);
	virtual UINT ReadPBk(IPBkManager* pPBkMan);
};
