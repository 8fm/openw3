/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventQuest.h"
#include "../../common/game/questStoryPhaseProperty.h"


IMPLEMENT_ENGINE_CLASS( CIdEventQuest )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventQuest::Activate( CIDTopicInstance* topicInstance )
{
	m_data->Perform();
}

