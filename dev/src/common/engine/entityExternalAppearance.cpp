/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityExternalAppearance.h"

IMPLEMENT_ENGINE_CLASS( CEntityExternalAppearance );

CEntityExternalAppearance::CEntityExternalAppearance()
{

}

void CEntityExternalAppearance::PrintLogInfo() const
{
	RED_LOG(RED_LOG_CHANNEL(ExternalAppearance), TXT("Load: %ls"), GetDepotPath().AsChar() );
	RED_LOG(RED_LOG_CHANNEL(ExternalAppearance), TXT("Appearance  name: %ls"), m_appearance.GetName().AsChar() );
}
