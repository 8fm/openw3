/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "platform.h"

/////////////////////////////////////////////////////////////////////
// Statics
CPlatform::VersionControlFn CPlatform::m_versionControlFn = nullptr;
Red::System::Error::Handler::AssertHook CPlatform::m_assertHandlerFn = nullptr;

/////////////////////////////////////////////////////////////////////
// SetVersionControlEnabled
//	Call this before Initialise!
void CPlatform::SetVersionControlInitialiser( VersionControlFn versionControlFn )
{
	m_versionControlFn = versionControlFn;
}

/////////////////////////////////////////////////////////////////////
// SetAssertHandler
//	Pass a handler function ptr to red system on initialise
void CPlatform::SetAssertHandler( Red::System::Error::Handler::AssertHook assertFn )
{
	m_assertHandlerFn = assertFn;
}

/////////////////////////////////////////////////////////////////////
// GetFeature
// Return platform specific features subsystems like Kinect etc.
CPlatformFeature* CPlatform::GetFeature( EPlatforFeature feature )
{
	return SPlatformFeatureManager::GetInstance().GetFeature( feature );
}