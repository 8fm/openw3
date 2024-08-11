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

#ifndef _AK_VPL_FILTER_NODE_EX_H_
#define _AK_VPL_FILTER_NODE_EX_H_

#include "AkVPLFilterNodeBase.h"

class CAkVPLFilterNodeOutOfPlace : public CAkVPLFilterNodeBase
{
public:
	virtual void		VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual void		GetBuffer( AkVPLState & io_state );
	virtual void		ConsumeBuffer( AkVPLState & io_state );

	virtual void		ReleaseBuffer();
	virtual bool		ReleaseInputBuffer();

	virtual void		ProcessDone( AkVPLState & io_state );

	virtual AKRESULT	Seek();

	AKRESULT			Init(
		IAkPlugin * in_pPlugin,
		const AkFXDesc & in_fxDesc,
		AkUInt32 in_uFXIndex,
		CAkPBI * in_pCtx,
		AkAudioFormat &	io_format );
	virtual void		Term();
	virtual void		ReleaseMemory();

	virtual AKRESULT	TimeSkip( AkUInt32 & io_uFrames );

	virtual AK::IAkPlugin* GetPlugin(){ return m_pEffect; }
	virtual AkChannelMask GetOutputChannelMask();

private:

	void InitInputBuffer(AkPipelineBuffer &in_buffer);

	AK::IAkOutOfPlaceEffectPlugin *	m_pEffect;		// Pointer to Fx.
	AkUInt16						m_usRequestedFrames;
	AkPipelineBuffer				m_BufferIn;			// Input buffer.
	AkPipelineBuffer				m_BufferOut;		// Output buffer.
	AkUInt32                		m_uInOffset;
	AkUInt32						m_InputFramesBeforeExec;
	AkUInt32						m_uConsumedInputFrames;
	AkUInt32						m_uRequestedInputFrames;
	AkReal32						m_fAveragedInput;	//For rate estimation
	AkReal32						m_fAveragedOutput;	//For rate estimation
	AkUInt32						m_uConsumedSinceLastOutput;
};

#endif //_AK_VPL_FILTER_NODE_EX_H_
