/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once


#include "renderObject.h"


// Given a render scene and frame, this will analyze the frame and allow any render resources used by the frame to begin
// prefetching. This could include textures being streamed, meshes being force-loaded, etc.
class IRenderFramePrefetch : public IRenderObject
{
public:
	// Is the prefetch complete? This only accounts for the work done by the prefetch itself, and does not include any
	// potential asynchronous work that may result for it (e.g. textures actually streaming).
	virtual Bool IsFinished() const = 0;
};
