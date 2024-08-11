/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventAI.h"
#include "idTopic.h"
#include "idInstance.h"
#include "idEventSenderDataStructs.h"
#include "idInterlocutor.h"


IMPLEMENT_ENGINE_CLASS( CIdEventAI )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventAI::Activate( CIDTopicInstance* topicInstance )
{
	CIDInterlocutorComponent*	interlocutor	= topicInstance->GetDialogInstance()->GetInterlocutor( m_data.m_InterlocutorName );
	interlocutor->RaiseAIEvent( &m_data );
}
