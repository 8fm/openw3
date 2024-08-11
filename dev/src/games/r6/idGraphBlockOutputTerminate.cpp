/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraphBlockOutputTerminate.h"
#include "idThread.h"
#include "idTopic.h"
#include "idInstance.h"


IMPLEMENT_ENGINE_CLASS( CIDGraphBlockOutputTerminate )

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockOutputTerminate::GetClientColor() const
{
	return Color::RED;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockOutputTerminate::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	// End thread
	CIDThreadInstance*		thread	= topicInstance->GetCurrentThread();
	RED_ASSERT( thread, TXT("Evaluating a block with no thread, should this be allowed? (Maybe when using connectors)"));
	if( !thread )
	{
		 return NULL;
	}

	thread->OnFinished();

	// End topic
	topicInstance->OnEnd();

	// End dialog
	CInteractiveDialogInstance*		dialog	= topicInstance->GetDialogInstance();
	RED_ASSERT( dialog, TXT("Evaluating a block with no dialog instance, should this be allowed? (Maybe when using connectors)"));
	if( !dialog )
	{
		return NULL;
	}

	dialog->OnFinished();

	//return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( m_sockets[ output ] ) );
	return NULL;
}