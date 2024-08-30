#pragma once

//#define PROFILE_NON_BLOCKING_QUEUE

#if defined( _WIN32 ) || defined( _DURANGO )
#	define NON_BLOCKING_QUEUE_INT		// Win/Durango
#else
#	define NON_BLOCKING_QUEUE_SUQ		// Orbis
#endif

///////////////////////////////////////////////////////////////////////////////////
//					Implementation lock-less LIFO queue
//////////////////////////////////////////////////////////////////////////////////

#if defined NON_BLOCKING_QUEUE_SUQ
#	include "nonBlockingLIFOQueue_SceUltQueue.h"
#elif defined NON_BLOCKING_QUEUE_INT
#	include "nonBlockingLIFOQueue_InterlockedSList.h"
#endif