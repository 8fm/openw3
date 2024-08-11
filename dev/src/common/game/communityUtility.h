/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "communityFindApResult.h"
#include "communityAgentState.h"


///////////////////////////////////////////////////////////////////////////////

class CLayer;

///////////////////////////////////////////////////////////////////////////////

class CCommunityUtility
{
public:
	static void FindLayersByNames( const TDynArray< CName > &layersNames, TDynArray< CLayer* > &foundLayers /* out */ );
	static CLayerInfo *GetCachedLayerInfo( const CName &layerName );
	static String GetFriendlyAgentStateName( ECommunityAgentState agentState );
	static String GetFriendlyFindAPStateDescription( EFindAPResult apResult );
	template< class T >
	static void AppendStringToMap( THashMap< const T*, String > &map, T *key, const String &value )
	{
		if ( String *str = map.FindPtr(key) )
		{
			*str += value;
		}
		else
		{
			map.Insert( key, value );
		}
	}

	template< class T >
	static const T *GetTimeActiveEntry( const TDynArray< T > &timetable, const GameTime& currGameTime )
	{
		GameTime currentGameDayTime = currGameTime % GameTime::DAY;

		if ( timetable.Empty() == true ) 
		{
			return NULL;
		}

		// table with one entry is active all the time
		if ( timetable.Size() == 1 ) 
		{
			return &(timetable[0]);
		}

		// find the appropriate interval
		for ( typename TDynArray< T >::const_iterator entry = timetable.Begin();; )
		{
			typename TDynArray< T >::const_iterator entryNext = entry + 1;
			if ( entryNext == timetable.End() )
			{
				return &*entry;
			}
			if ( entry->m_time <= currentGameDayTime && currentGameDayTime < entryNext->m_time )
			{
				return &*entry;
			}
			entry = entryNext;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
