/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifdef USE_NVIDIA_FUR

#include "../../../external/NvidiaHair/include/GFSDK_HairWorks.h"


namespace HairWorksHelpers
{

	class DefaultLogHandler : public GFSDK_HAIR_LogHandler
	{
	public:
		virtual void Log(GFSDK_HAIR_LOG_TYPES logType, const char* message, const char* file, int line) override;
	};


	// Initialize hair sdk with given log handler. logHandler can be null.
	GFSDK_HairSDK* InitSDK( GFSDK_HAIR_LogHandler* logHandler );

	// Release sdk, nulling it out. Fine to pass in a null sdk.
	void ShutdownSDK( GFSDK_HairSDK*& sdk );


	Bool SaveShaderCache( GFSDK_HairSDK* sdk, const String& absolutePath );
	Bool LoadShaderCache( GFSDK_HairSDK* sdk, const String& absolutePath, Bool append );

}

#endif
