#pragma once

#include "ticketSourceProvider.h"

class CGlabalTicketSourceProvider : public CObject, public ITicketSourceProvider
{
	DECLARE_ENGINE_CLASS( CGlabalTicketSourceProvider, CObject, 0 );
public:
	void Initialize();
};


BEGIN_CLASS_RTTI( CGlabalTicketSourceProvider  )
	PARENT_CLASS( CObject )	
END_CLASS_RTTI();