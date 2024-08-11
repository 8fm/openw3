/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_OVERRUN_DETECTOR_H_
#define _RED_MEMORY_HOOK_OVERRUN_DETECTOR_H_

namespace red
{
namespace memory
{
	struct HookPreParameter;
	struct HookPostParameter;

	void PreAllocateOverrunDetectorCallback( HookPreParameter & param, void* );
	void PostAllocateOverrunDetectorCallback( HookPostParameter & param, void* );

	// DO NOT ACTIVATE AT RUNTIME. UNIT TEST ONLY
	RED_MEMORY_API void EnableOverrunDetector(); 
	RED_MEMORY_API void DisableOverrunDetector();

}
}

#endif
