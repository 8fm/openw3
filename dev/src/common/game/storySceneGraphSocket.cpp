#include "build.h"

#include "storySceneSectionBlock.h"
#include "storySceneGraphSocket.h"
#include "storySceneSection.h"
#include "../engine/graphConnection.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CStorySceneGraphSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// We can connect to any scene graph socket that isn't in the same block
	return otherSocket->IsA< CStorySceneGraphSocket >() && otherSocket->GetBlock() != GetBlock();
}

void CStorySceneGraphSocket::OnSpawned( class CGraphBlock* block, const class GraphSocketSpawnInfo& info )
{
	// Pass to base class	
	CGraphSocket::OnSpawned( block, info );

	// Grab the link
	const StorySceneGraphSocketSpawnInfo& sinfo = ( const StorySceneGraphSocketSpawnInfo& ) info;
	m_linkElement = sinfo.m_linkElement;
}

void CStorySceneGraphSocket::OnConnectionCreated( CGraphConnection* connection )
{
	ASSERT( connection );
	ASSERT( connection->GetSource() == this );

	// Disconnect previous connection
	if ( connection->GetSource() == this && IsMultiLink() == false )
	{
		if ( GetConnections().Size() > 1 )
		{
			DisconnectFrom( GetConnections()[ 0 ]->GetDestination( true ) );
		}
	}

#ifdef W2_PLATFORM_WIN32
	// Update the link, but only for output sockets
	if ( m_direction == LSD_Output )
	{
		// Get the target socket
		CStorySceneGraphSocket* destinationSocket = SafeCast<CStorySceneGraphSocket>( connection->GetDestination( true ) );
		if ( destinationSocket )
		{
			// Get the link element we are connecting to
			CStorySceneLinkElement* destinationElement = destinationSocket->GetLinkElement();
			ASSERT( destinationElement );
			if ( destinationElement )
			{
				// Connect them
				ASSERT( m_linkElement );
				if ( m_linkElement )
				{
					m_linkElement->ConnectToElement( destinationElement );
				}
			}
		}
	}

	if ( m_direction == LSD_Variable )
	{
		// Get the target socket
		CStorySceneGraphSocket* destinationSocket = SafeCast<CStorySceneGraphSocket>( connection->GetDestination( true ) );
		if ( destinationSocket )
		{
			CStorySceneSection* interceptSection = GetSectionFromSocket( destinationSocket );
			CStorySceneSection* parentSection = GetSectionFromSocket( this );
			if ( interceptSection && parentSection )
			{
				parentSection->AddInterceptSection( interceptSection );
			}
		}
	}
#endif
}

void CStorySceneGraphSocket::OnConnectionBroken( CGraphConnection* connection )
{
	ASSERT( connection );
	ASSERT( connection->GetSource( true ) == this );

	// Update the link, but only for output sockets
#ifdef W2_PLATFORM_WIN32
	if ( m_direction == LSD_Output )
	{
		// Get the target socket
		CStorySceneGraphSocket* destinationSocket = SafeCast<CStorySceneGraphSocket>( connection->GetDestination( true ) );
		if ( destinationSocket )
		{
			// Get the link element we are connecting to
			CStorySceneLinkElement* destinationElement = destinationSocket->GetLinkElement();
			ASSERT( destinationElement );
			if ( destinationElement )
			{
				// Check graph links consistency
				ASSERT( m_linkElement );
				if ( m_linkElement )
				{
					ASSERT( m_linkElement->GetNextElement() == destinationElement );
				}
			}
		}

		// Disconnect
		ASSERT( m_linkElement );
		if ( m_linkElement )
		{
			m_linkElement->ConnectToElement( NULL );
		}
	}

	if ( m_direction == LSD_Variable )
	{
		// Get the target socket
		CStorySceneGraphSocket* destinationSocket = SafeCast<CStorySceneGraphSocket>( connection->GetDestination( true ) );
		if ( destinationSocket )
		{
			CStorySceneSection* interceptSection = GetSectionFromSocket( destinationSocket );
			CStorySceneSection* parentSection = GetSectionFromSocket( this );
			if ( interceptSection && parentSection )
			{
				parentSection->RemoveInterceptSection( interceptSection );
			}
		}
	}
#endif
}

#endif

CStorySceneSection* CStorySceneGraphSocket::GetSectionFromSocket( CStorySceneGraphSocket* sceneSocket )
{
	if ( sceneSocket->GetBlock() )
	{
		CStorySceneSectionBlock* sectionBlock = sceneSocket->GetBlock()->FindParent< CStorySceneSectionBlock >();
		if ( sectionBlock != NULL )
		{
			return sectionBlock->GetSection();
		}
	}

	return NULL;
}
