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
// AkSrcPhysModel.cpp
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRC_PHYSICAL_MODEL_H_
#define _AK_SRC_PHYSICAL_MODEL_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>

#include "AkSrcBase.h"
#include "AkFXContext.h"

class CAkSrcPhysModel : public CAkVPLSrcNode
{
public:

	//Constructor and destructor
	CAkSrcPhysModel( CAkPBI * in_pCtx );
	virtual ~CAkSrcPhysModel();

	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
#ifdef AK_PS3
	virtual void	 ProcessDone( AkVPLState & io_state );
#endif
	virtual AKRESULT TimeSkip( AkUInt32 & io_uFrames );
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );

	virtual AkReal32 GetDuration() const;
	virtual AKRESULT StopLooping();
	virtual AK::IAkPluginParam * GetPluginParam() { return m_pParam; }

	virtual AKRESULT ChangeSourcePosition();

	// Returns estimate of relative loudness at current position, compared to the loudest point of the sound, in dBs (always negative).
	virtual AkReal32 GetAnalyzedEnvelope( AkUInt32 in_uBufferedFrames );

private:
	AkPluginID				m_FXID;				// Effect unique type ID. 
	AK::IAkPluginParam*		m_pParam;			// Parameters.
	IAkSourcePlugin* 		m_pEffect;			// Pointer to a Physical Modelling effect.
	AkPipelineBuffer		m_pluginBuffer;		// Output plugin buffer.	
	CAkSourceFXContext *	m_pSourceFXContext;	// Source FX context (for physical model)
	AkAudioFormat			m_AudioFormat;		// Audio format output by source.
};

#endif // _AK_SRC_PHYSICAL_MODEL_H_
