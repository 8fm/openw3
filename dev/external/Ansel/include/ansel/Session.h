#pragma once
#include <ansel/Defines.h>

namespace ansel
{
	struct SessionConfiguration
	{
		// User can move the camera during session
		bool isTranslationAllowed;
		// Camera can be rotated during session
		bool isRotationAllowed;
		// FoV can be modified during session
		bool isFovChangeAllowed;
		// Game is paused during session
		bool isPauseAllowed;
		// Game allows highres capture during session
		bool isHighresAllowed;
		// Game allows 360 capture during session
		bool is360MonoAllowed;
		// Game allows 360 stereo capture during session
		bool is360StereoAllowed;

		SessionConfiguration()
		{
			isTranslationAllowed = true;
			isRotationAllowed = true;
			isFovChangeAllowed = true;
			isPauseAllowed = true;
			isHighresAllowed = true;
			is360MonoAllowed = true;
			is360StereoAllowed = true;
		}
	};

	enum StartSessionStatus
	{
		kDisallowed = 0,
		kAllowed
	};

	typedef StartSessionStatus(*StartSessionCallback)(SessionConfiguration& settings, void* userPointer);
	typedef void(*StopSessionCallback)(void* userPointer);
	typedef void(*StartCaptureCallback)(void* userPointer);
	typedef void(*StopCaptureCallback)(void* userPointer);

	// Stops current session if there is one active
	ANSEL_SDK_API void stopSession();
}