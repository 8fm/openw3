/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "gameApplicationOrbis.h"

#include "states.h"

#include <system_service.h>

#include "../../orbis/platformOrbis/orbisApiCall.h"
#include "../../common/engine/baseEngine.h"
#include "../../common/engine/userProfile.h"
#include "../../common/core/messagePump.h"
#include "../../orbis/platformOrbis/dlcInstallerOrbis.h"

#pragma comment( lib, "libSceSystemService_stub_weak.a" )		// Used to get the application state

class CMessagePumpOrbis : public IMessagePump
{
public:
	CMessagePumpOrbis();
	virtual ~CMessagePumpOrbis() {}

	virtual void PumpMessages() override final;

private:
	Red::System::StopClock m_timer;
	Red::System::Bool m_executingInBackground;
};

CMessagePumpOrbis::CMessagePumpOrbis()
:	m_executingInBackground( false )
{
}

namespace Orbis
{
	extern CDlcInstallerOrbis* GDlcInstaller;
}

#ifdef RED_LOGGING_ENABLED
const Char* GetServiceEventTxtForLog( SceSystemServiceEventType type )
{
	const Char* txt = TXT("<Unknown>");

	switch (type)
	{
	case SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE:
		txt = TXT("SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE");
		break;
	case SCE_SYSTEM_SERVICE_EVENT_ADDCONTENT_INSTALL:
		txt = TXT("SCE_SYSTEM_SERVICE_EVENT_ADDCONTENT_INSTALL");
		break;
	default:
		break;
	}

	return txt;
}
#endif

void CMessagePumpOrbis::PumpMessages()
{
	if ( m_timer.GetDelta() > 0.1 )
	{
		SceSystemServiceStatus status;
		ORBIS_SYS_CALL( sceSystemServiceGetStatus( &status ) );

		// isInBackgroundExecution Is the Orbis equivalent of Durango constrained mode
		// Caused when the user taps the PS4 button on the controller

		// isSystemUiOverlaid Indicates that the user has brought up the system menu that
		// allows them to adjust their controllers/log out etc, but it doesn't cause the system
		// to go into background execution.
		// At the moment the UI is full screen, so just treat it as constrained mode even if it isn't
		// If a later SDK changes it to allow gameplay to continue in parallel we can revisit this then
		Bool constrained = status.isInBackgroundExecution || status.isSystemUiOverlaid;

		if( constrained && !m_executingInBackground )
		{
			m_executingInBackground = true;
			//RequestState( GameConstrained );

			if ( GEngine )
			{
				GEngine->OnEnterConstrainedRunningMode();
			}
		}
		else if( !constrained && m_executingInBackground )
		{
			m_executingInBackground = false;
			//RequestState( GameRunning );

			if ( GEngine )
			{
				GEngine->OnExitConstrainedRunningMode();
				GEngine->OnEnterNormalRunningMode();
			}
		}

		for( Int32 i = 0; i < status.eventNum; ++i )
		{
			SceSystemServiceEvent event;
			ORBIS_SYS_CALL( sceSystemServiceReceiveEvent( &event ) );

			RED_LOG_SPAM( OrbisMessagePump, TXT( "Message received: %x" ), static_cast< Int32 >( event.eventType ) );
			switch (event.eventType)
			{
			case SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE: /* fall through */
			case SCE_SYSTEM_SERVICE_EVENT_ADDCONTENT_INSTALL:
				{
					LOG_ENGINE(TXT("DLC related sceSystemServiceReceiveEvent: %ls"), GetServiceEventTxtForLog(event.eventType));
					if ( Orbis::GDlcInstaller )
					{
						Orbis::GDlcInstaller->OnEntitlementUpdated();
					}
				}
				break;
			default:
				break;

			}
		}

		m_timer.Reset();
	}

	if( GEngine && GUserProfileManager )
		GUserProfileManager->Update();
}

CGameApplicationOrbis::CGameApplicationOrbis()
{
	RED_FATAL_ASSERT( GMessagePump == nullptr, "Message pump already initialised" );
	GMessagePump = new CMessagePumpOrbis();
}

CGameApplicationOrbis::~CGameApplicationOrbis()
{

}

void CGameApplicationOrbis::PumpEvents()
{
	RED_FATAL_ASSERT( GMessagePump, "Message pump not initialised yet" );

	GMessagePump->PumpMessages();
}
