/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "singleton.h"
#include "objectGC.h"

namespace Legacy
{
	class CLegacyGCWrapper
	{
	public:
		void Collect() { return GObjectGC->Collect(); }
		void CollectNow() { return GObjectGC->Collect(); }
	};
} // Legacy

typedef TSingleton< Legacy::CLegacyGCWrapper > SGarbageCollector;
