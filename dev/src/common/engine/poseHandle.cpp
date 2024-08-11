/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poseHandle.h"
#include "poseBlock.h"

void CPoseHandle::SignalPoseAvailable()
{
	m_owner->SignalPoseAvailable();
}
