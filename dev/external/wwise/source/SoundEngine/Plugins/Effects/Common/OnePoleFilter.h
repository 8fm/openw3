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

// Generic one pole filter. Coefficients are set explicitely by client.
// y[n] = fB0 * x[n] - fA1 * y[n - 1]
// To be used on mono signals, create as many instances as there are channels if need be
// Also provides processing per sample

#ifndef _AKONEPOLEFILTER_H_
#define _AKONEPOLEFILTER_H_

#include <AK/SoundEngine/Common/AkTypes.h>

namespace DSP
{

	class OnePoleFilter
	{
	public:
		enum FilterType
		{
			FILTERCURVETYPE_NONE			=  0,	
			FILTERCURVETYPE_LOWPASS			=  1,	
			FILTERCURVETYPE_HIGHPASS		=  2,
		};

#ifndef __SPU__
		OnePoleFilter() 
			:	fFFbk1( 0.f ),
			fB0( 0.f ),
			fA1( 0.f ){}

		void Reset( )
		{
			fFFbk1 = 0.f;
		}
		
#endif

		// Note: To avoid repeat costly filter coefficient computation when multi-channel have the same coefs,
		// ComputeCoefs() may be used followed by an explicit SetCoefs per channel
		static void ComputeCoefs( FilterType eType, AkReal32 in_fFc, AkUInt32 in_uSr, AkReal32 & out_fB0, AkReal32 & out_fA1 ); 
		void SetCoefs( AkReal32 in_fB0, AkReal32 in_fA1 )
		{
			fB0 = in_fB0;
			fA1 = in_fA1;
		}

		void SetCoefs( FilterType eType, AkReal32 in_fFc, AkUInt32 in_uSr ); 
		
		void ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames );

		AkForceInline AkReal32 ProcessSample( AkReal32 in_fIn )
		{
			AkReal32 fOut = in_fIn * fB0 - fA1 * fFFbk1;
			fFFbk1 = fOut;
			return fOut;
		}

		AkForceInline AkReal32 ProcessSample( AkReal32 in_fIn, AkReal32 & io_fFFbk1 )
		{
			AkReal32 fOut = in_fIn * fB0 - fA1 * io_fFFbk1;
			io_fFFbk1 = fOut;
			return fOut;
		}

		//	protected:
		AkReal32 fFFbk1;	// first order feedback memory
		AkReal32 fB0;
		AkReal32 fA1;
	};

} // namespace DSP

#endif // _AKONEPOLEFILTER_H_
