/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class IMessagePump
{
public: 
	virtual void PumpMessages() = 0;
	virtual ~IMessagePump() {}
};

extern IMessagePump* GMessagePump;

// Where we need to pump them for Xbox cert. PS4 as well, but it's more forgiving of timeouts and hasn't caused cert blockers.
#ifndef RED_PLATFORM_DURANGO
RED_INLINE void PUMP_MESSAGES_DURANGO_CERTHACK() {}
#define SCOPED_ENABLE_PUMP_MESSAGES_DURANGO_CERTHACK()
#else
RED_INLINE void PUMP_MESSAGES_DURANGO_CERTHACK()
{
	extern Bool GReentrantMessagePumpCheck;
	extern Bool GEnableMessagePumpDurangoCertHack;

	if ( !GMessagePump || !GEnableMessagePumpDurangoCertHack || !::SIsMainThread() )
		return;

	if ( !GReentrantMessagePumpCheck ) 
	{
		Red::System::ScopedFlag<Bool> flag( GReentrantMessagePumpCheck = true, false );
		GMessagePump->PumpMessages();
	}
};
extern Bool GEnableMessagePumpDurangoCertHack;
#define SCOPED_ENABLE_PUMP_MESSAGES_DURANGO_CERTHACK()\
	const Bool oldPumpMessageCertHack = GEnableMessagePumpDurangoCertHack;\
	Red::System::ScopedFlag<Bool> hackGuard( GEnableMessagePumpDurangoCertHack = true, oldPumpMessageCertHack )
#endif