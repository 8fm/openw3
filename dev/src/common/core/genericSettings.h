#pragma once 

RED_DISABLE_WARNING_MSC(4324) // structure was padded due to __declspec(align())
RED_DISABLE_WARNING_MSC(4702)

// Enable or disable memory alloc/delloc forward to our memory manager
//#define PHYSICS_USE_OUR_MEMORY_MANAGER

#define PROFILE_BEAMTREE 0
#define PROFILE_COLLECTVISIBILITY 0

//// Hacks for Microsoft demo 2014. 10. 08.
//#define MSDEMO2014_HACKS 1
