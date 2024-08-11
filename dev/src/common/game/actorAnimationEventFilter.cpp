/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorAnimationEventFilter.h"

Bool CAnimationEventFilter::CanTriggerEvent( const StringAnsi& eventName, Int32 boneIndex, Float cooldown )
{
	Double currentTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	auto& timestampList = m_eventMap[ eventName ];
	for( auto iter = timestampList.Begin(); iter != timestampList.End(); ++iter )
	{
		BoneTimestampPair& info = *iter;
		if( boneIndex == info.m_first )
		{
			Double delta = currentTime - info.m_second;
			if( delta >= cooldown )
			{
				info.m_second = currentTime;
				return true;
			}
			return false;
		}
	}

	if( timestampList.Size() < timestampList.Capacity() )
	{
		timestampList.PushBack( BoneTimestampPair( boneIndex, currentTime ) );
	}
	else
	{
		// We've run out of slots, overwrite the oldest
		Sort( timestampList.Begin(), timestampList.End(), []( BoneTimestampPair& lhs, BoneTimestampPair& rhs ) { return lhs.m_second < rhs.m_second; } );
		( *timestampList.Begin() ) = BoneTimestampPair( boneIndex, currentTime );
	}
	return true;
}
