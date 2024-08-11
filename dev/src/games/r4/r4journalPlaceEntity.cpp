#include "build.h"
#include "r4journalPlaceEntity.h"
#include "../../common/game/journalPath.h"

IMPLEMENT_ENGINE_CLASS( CR4JournalPlaceEntity );

void CR4JournalPlaceEntity::funcGetJournalPlaceEntry( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CJournalPath* path = m_placeEntry.Get();
	if ( path )
	{
		RETURN_OBJECT( path->GetTarget() );
		return;
	}
	RETURN_OBJECT( nullptr );
}