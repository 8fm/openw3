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

#ifndef _AK_RECTIFIER_H
#define _AK_RECTIFIER_H

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "SPUInline.h"

// Rectification of signals
// 0 is none, 50 is half wave and 100 is full wave

namespace DSP
{
	class CAkRectifier
	{
	public:

		enum RectifierMode
		{
			RectifierMode_Bypass = 0,
			RectifierMode_HalfWave,
			RectifierMode_FullWave
		};

#ifndef __SPU__
		CAkRectifier();
		void SetRectification( AkReal32 in_fRectification, bool in_bFirstSet = false ); 
#endif
		SPUInline void ProcessBuffer(	AkAudioBuffer * io_pBuffer );

	private:

		SPUInline void ProcessChannel( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames );
		
		AkReal32		m_fHWRThresh;
		AkReal32		m_fPrevHWRThresh;
		AkReal32		m_fFWRGain;
		AkReal32		m_fPrevFWRGain;
		RectifierMode	m_eRectifierMode;
	};

} // namespace DSP
#endif // _AK_RECTIFIER_H