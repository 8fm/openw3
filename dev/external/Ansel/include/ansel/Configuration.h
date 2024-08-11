#pragma once
#include <ansel/Defines.h>
#include <ansel/Session.h>
#include <nv/Vec3.h>
#include <stdint.h>

namespace ansel
{
	enum FovType
	{
		kHorizontalFov,
		kVerticalFov
	};

	struct Configuration
	{
		// Basis vectors used by the game. They specify the handedness and orientation of 
		// the game's coordinate system. Think of them as the default orientation of the game
		// camera.
		nv::Vec3 right, up, forward;
		// The speed at which camera moves in the world
		float translationSpeedInWorldUnitsPerSecond;
		// The speed at which camera rotates 
		float rotationalSpeedInDegreesPerSecond;
		// How many frames it takes for camera update to be reflected in a rendered frame
		uint32_t captureLatency;
		// How many frames we must wait for a new frame to settle - i.e. temporal AA and similar
		// effects to stabilize after the camera has been adjusted
		uint32_t captureSettleLatency;
		// Game scale, the size of a world unit measured in meters
		float metersInWorldUnit;
		// Integration will support Camera::screenOriginXOffset/screenOriginYOffset
		bool isCameraOffcenteredProjectionSupported;
		// Integration will support Camera::position
		bool isCameraTranslationSupported;
		// Integration will support Camera::rotation
		bool isCameraRotationSupported;
		// Integration will support Camera::horizontalFov
		bool isCameraFovSupported;
		// Game name, used to name the resulting image files from capturing
		const char* titleName;
		// Camera structure will contain vertical FOV if this is set to kVerticalFov
		// but horizontal FOV if this is set to kHorizontalFov. To simplify integration set
		// this to the same orientation as the game is using.
		FovType fovType;

		// These callbacks will be called on the same thread Present()/glSwapBuffers is called
		// The thread calling to updateCamera() might be a different thread

		// User defined pointer which is then passed to all the callbacks (nullptr by default)
		void* userPointer;

		// The window handle for the game/application where input messages are processed
		void* gameWindowHandle;

		// Called when user activates Ansel. Return false if the game cannot comply with the
		// request. If the function returns true the following must be done:
		// 1. Change the SessionConfigruation settings, but only where you need to (the object
		//    is already populated with default settings).
		// 2. On the next update loop the game will be in an Ansel session. This requires the game
		//    to 
		//    a) stop drawing any UI or HUD related elements
		//    b) pause the simulation (if possible)
		//    c) call ansel::updateCamera and perform associated processing
		// 3. Step 2 is repeated on every iteration of update loop until Session is stopped.
		StartSessionCallback startSessionCallback;

		// Called when Ansel is deactivated. This call will only be made if the previous call
		// to the startSessionCallback returned true.
		// Normally games will use this callback to restore their camera to the settings it had 
		// when the Ansel session was started.
		StopSessionCallback stopSessionCallback;

		// Called when the capture of a multipart shot (highres, 360, etc) has started.
		// Handy to disable those fullscreen effects that aren't uniform (like vignette)
		// This callback is optional (leave nullptr if not needed)
		StartCaptureCallback startCaptureCallback;
		// Called when the capture of a multipart shot (highres, 360, etc) has stopped.
		// Handy to enable those fullscreen effects that were disabled by startCaptureCallback.
		// This callback is optional (leave nullptr if not needed)
		StopCaptureCallback stopCaptureCallback;
		// Integration allows a filter/effect to remain active when the Ansel session is not active
		bool isFilterOutsideSessionAllowed;

		Configuration()
		{
			right.x = 0.0f;
			right.y = 1.0f;
			right.z = 0.0f;
			up.x = 0.0f;
			up.y = 0.0f;
			up.z = 1.0f;
			forward.x = 1.0f;
			forward.y = 0.0f;
			forward.z = 0.0f;
			translationSpeedInWorldUnitsPerSecond = 1.0f;
			rotationalSpeedInDegreesPerSecond = 45.0f;
			captureLatency = 1;
			captureSettleLatency = 0;
			metersInWorldUnit = 1.0f;
			isCameraOffcenteredProjectionSupported = true;
			isCameraTranslationSupported = true;
			isCameraRotationSupported = true;
			isCameraFovSupported = true;
			titleName = nullptr;
			fovType = kHorizontalFov;
			userPointer = nullptr;
			gameWindowHandle = 0;
			startSessionCallback = nullptr;
			stopSessionCallback = nullptr;
			startCaptureCallback = nullptr;
			stopCaptureCallback = nullptr;
			isFilterOutsideSessionAllowed = true;
		}
	};

	// Called during startup by the game. See 'Configuration' for further documentation.
	ANSEL_SDK_API void setConfiguration(const Configuration& cfg);
}

