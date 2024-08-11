/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventFact.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/factsDB.h"

IMPLEMENT_ENGINE_CLASS( CIdEventFact )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventFact::Activate( CIDTopicInstance* topicInstance )
{
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() )
	{
		if( m_enable )
		{
			GCommonGame->GetSystem< CFactsDB >()->AddFact( m_fact, 1, CFactsDB::EXP_NEVER );
		}
		else
		{
			GCommonGame->GetSystem< CFactsDB >()->RemoveFact( m_fact );
		}
	}
}
