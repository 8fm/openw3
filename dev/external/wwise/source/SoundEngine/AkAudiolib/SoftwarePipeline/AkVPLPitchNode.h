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
// AkVPLPitchNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_PITCH_NODE_H_
#define _AK_VPL_PITCH_NODE_H_

#include "AkResampler.h"
#include "AkVPLNode.h"


class CAkVPLPitchNode : public CAkVPLNode
{
public:
	CAkVPLPitchNode( class CAkVPLSrcCbxNode * in_pCbx ) 
		: m_pCbx( in_pCbx )
		, m_pPBI( NULL )
	{ 
		// IMPORTANT: Sensible data must be initialized: this node can be Term()ed even if Init() was not called!
		// FIXME: Review all Constructor/Init/Term/~.
		m_BufferIn.Clear();
		m_BufferOut.Clear();
	}

	/*virtual*/ void	GetBuffer( AkVPLState & io_state );
	/*virtual*/ void	ConsumeBuffer( AkVPLState & io_state );
	virtual void		ReleaseBuffer();

	virtual void		VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT	VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset );
	virtual AKRESULT	TimeSkip( AkUInt32 & io_uFrames );
	
	virtual AKRESULT	Seek();
	
	void				Init( AkAudioFormat *    io_pFormat,		// Format.
							  CAkPBI* in_pPBI, // PBI, to access the initial pbi that created the pipeline.
							  AkUInt32 in_usSampleRate
							  );

	void				Term();	// Memory allocator interface.

	inline AkReal32		GetLastRate(){ return m_Pitch.GetLastRate(); }

	inline bool			IsInitialized() const { return m_pPBI != 0; }

	inline AkUInt32		GetNumBufferedInputSamples() { return m_BufferIn.uValidFrames; }

	void RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia );

#ifdef AK_PS3
	void ProcessDone( AkVPLState & io_state );
#endif

protected:
	AKRESULT			SwitchToNextSrc();
	void				ReleaseInputBuffer( AkPipelineBuffer & io_buffer );	

private:

	CAkResampler			m_Pitch;			// Resampling object.
	AkPipelineBuffer		m_BufferIn;			// Input buffer.
	AkPipelineBuffer		m_BufferOut;		// Output buffer.
	CAkVPLSrcCbxNode *		m_pCbx;
	CAkPBI*					m_pPBI;
	bool					m_bLast;			// True=was the last input buffer.
	bool					m_bStartPosInfoUpdated;
	bool					m_bPadFrameOffset;	// True=Pad buffer with frame offset. Starts true, then indefinitely false.

#ifdef AK_PS3
	AkUInt16				m_uInputFrameOffsetBefore;
#endif
};

#endif //_AK_VPL_PITCH_NODE_H_
