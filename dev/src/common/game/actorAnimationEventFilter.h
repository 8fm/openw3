/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/hashmap.h"
#include "../core/staticarray.h"

class CAnimationEventFilter
{
public:
	Bool CanTriggerEvent( const StringAnsi& eventName, Int32 boneIndex, Float cooldown );

private:
	static const Int8 c_numBonesMax = 4;
	typedef TPair< Int32, Double > BoneTimestampPair;
	typedef TStaticArray< BoneTimestampPair, c_numBonesMax > EventTimestampList;
	typedef THashMap< StringAnsi, EventTimestampList > AnimEventLastPlayedMap;

	AnimEventLastPlayedMap m_eventMap;
};
