/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "addFactPerformableAction.h"

#include "factsDB.h"
#include "../engine/gameTimeManager.h"

IMPLEMENT_ENGINE_CLASS( CAddFactPerformableAction )


CAddFactPerformableAction::CAddFactPerformableAction()
	: m_value( 1 )
	, m_validForSeconds( CFactsDB::EXP_NEVER_SEC )
{

}

void CAddFactPerformableAction::Perform( CEntity* parent )
{
	const EngineTime& time = GGame->GetEngineTime();
	GCommonGame->GetSystem< CFactsDB >()->AddFact( m_factID, m_value, time, m_validForSeconds );
}