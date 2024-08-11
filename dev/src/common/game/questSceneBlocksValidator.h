/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questGraphBlock.h"
#include "storyScene.h"

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CQuestSceneBlocksValidator
{
public:
	Bool IsOutdated( CStoryScene* scene, const CQuestGraphBlock* block ) const;

private:
	template< typename T >
	Bool FindMatchingScenePart( TDynArray< T* >& sceneParts, const String& refName ) const
	{
		Uint32 count = sceneParts.Size();
		for ( Uint32 j = 0; j < count; ++j )
		{
			if ( sceneParts[ j ]->GetName() == refName )
			{
				return true;
			}
		}

		return false;
	}

	template <typename T >
	Bool CheckSockets( CStoryScene* scene, const CQuestGraphBlock* block, ELinkedSocketDirection dir ) const
	{
		TDynArray< T* > sceneSockets;
		scene->CollectControlParts< T >( sceneSockets );


		Uint32 blockSocketsCount = 0;
		const TDynArray< CGraphSocket* >& sockets = block->GetSockets();
		Uint32 count = sockets.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( sockets[ i ]->GetDirection() == dir )
			{
				if ( sockets[ i ]->GetName() == CNAME( Cut ) )
				{
					continue;
				}

				++blockSocketsCount;

				Bool found = FindMatchingScenePart< T >( sceneSockets, sockets[ i ]->GetName().AsString() );
				if ( !found )
				{
					return false;
				}
			}
		}

		if ( blockSocketsCount != sceneSockets.Size() )
		{
			return false;
		}

		return true;
	}
};

#endif