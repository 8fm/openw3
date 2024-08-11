/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkDefault3DParams.cpp
//
// Windows specific implementation of default 3D parameters.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkDefault3DParams.h"
#include "AkMath.h"

AkSoundPosition g_DefaultSoundPosition =
{
	AK_DEFAULT_SOUND_POSITION_X,					// Position.X
	AK_DEFAULT_SOUND_POSITION_Y,					// Position.Y
	AK_DEFAULT_SOUND_POSITION_Z,					// Position.Z
	AK_DEFAULT_SOUND_ORIENTATION_X,					// Orientation.X
	AK_DEFAULT_SOUND_ORIENTATION_Y,					// Orientation.Y
	AK_DEFAULT_SOUND_ORIENTATION_Z					// Orientation.Z
};

ConeParams g_OmniConeParams =
{
	AK_OMNI_INSIDE_CONE_ANGLE,		// fInsideAngle
	AK_OMNI_OUTSIDE_CONE_ANGLE,		// fOutsideAngle
	AK_DEFAULT_LEVEL_DB,			// fOutsideVolume
	0								// LoPass
};

AkListenerPosition g_DefaultListenerPosition =
{
	AK_DEFAULT_LISTENER_FRONT_X,		// OrientationFront.X
	AK_DEFAULT_LISTENER_FRONT_Y,		// OrientationFront.Y
	AK_DEFAULT_LISTENER_FRONT_Z,		// OrientationFront.Z
	AK_DEFAULT_LISTENER_TOP_X,			// OrientationTop.X
	AK_DEFAULT_LISTENER_TOP_Y,			// OrientationTop.Y
	AK_DEFAULT_LISTENER_TOP_Z,			// OrientationTop.Z
	AK_DEFAULT_LISTENER_POSITION_X,		// Position.X
	AK_DEFAULT_LISTENER_POSITION_Y,		// Position.Y
	AK_DEFAULT_LISTENER_POSITION_Z,		// Position.Z
};
