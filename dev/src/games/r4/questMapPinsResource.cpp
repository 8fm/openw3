#include "build.h"
#include "questMapPinsResource.h"

IMPLEMENT_ENGINE_CLASS( SQuestMapPinInfo );
IMPLEMENT_ENGINE_CLASS( CQuestMapPinsResource );

CQuestMapPinsResource::CQuestMapPinsResource()
{
}

CQuestMapPinsResource::~CQuestMapPinsResource()
{
}

#ifndef NO_EDITOR

Bool CQuestMapPinsResource::AddEntry( const SQuestMapPinInfo& info )
{
	m_mappinsInfo.PushBack( info );
	return true;
}

#endif //NO_EDITOR