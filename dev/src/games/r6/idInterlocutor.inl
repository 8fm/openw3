/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
template< typename TLineType >
void CIDInterlocutorComponent::TryToPlayLine( const TLineType& line, Uint32 dialogId )
{
	SceneMimicOn();

	StartTicking();
	QueueLine( line, dialogId );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
template< typename TLineType >
void CIDInterlocutorComponent::QueueLine( const TLineType& line, Uint32 dialogId )
{
	delete m_lineQueued;
	m_lineQueued = new SIDLineInstance( this, true, dialogId, line );

	if ( m_linePlaying == nullptr )
	{
		PlayQueuedLine();
	}
}