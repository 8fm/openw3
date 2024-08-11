/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace GpuApi
{
	//The different type of gamma ramps available.
	//The format of the enumerator is:
	//GAMMA_inputColorSpace_TO_outputColorSpace_enable/disableTVGamma
	//For example 8BIT_LINEAR_SRGB_NO_TV_GAMMA means that we've got an 8bit front buffer in linear space and we want the hardware
	//gamma ramp to convert everything to sRGB with TV adjustment disabled.
	enum eGammaRampType
	{
		GAMMA_8BIT_LINEAR_TO_SRGB_TV_GAMMA=0,
		GAMMA_8BIT_LINEAR_TO_SRGB_NO_TV_GAMMA,
		GAMMA_8BIT_TV_GAMMA,
		GAMMA_8BIT_NO_TV_GAMMA,
		GAMMA_LINEAR,

		LAST_GAMMA_RAMP_TYPE,
	};

	void SetupDeviceGammaMode(eGammaRampType gammaType, float gamma );
}
