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
// AkVPLNode.h
//
// Mananges the execution of the effects applied to a sound or bus.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_VPL_NODE_H_
#define _AK_VPL_NODE_H_

#include <AK/Tools/Common/AkObject.h>
#include "AkCommon.h"
#include "AkLEngineStructs.h"

using namespace AK;

class CAkVPLNode
{
public:
	CAkVPLNode()
		:m_pInput( NULL )
	{}

	virtual ~CAkVPLNode() {}

//	Following two virtual methods have been made non-virtual due to significant impact on performance of RunVPL
//	virtual void		GetBuffer( AkVPLState & io_state ) {}			// request data from node
//	virtual void		ConsumeBuffer( AkVPLState & io_state ) {}		// give data to node
	virtual void		ProcessDone( AkVPLState & io_state ) {}			// called when processing has completed
	virtual void		ReleaseBuffer() = 0;							// called when returned data has been consumed downstream
	virtual AKRESULT	TimeSkip( AkUInt32 & io_uFrames ) = 0;
	virtual void        VirtualOn( AkVirtualQueueBehavior eBehavior ) { if ( m_pInput ) m_pInput->VirtualOn( eBehavior ); }
	virtual AKRESULT    VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset ) { if ( m_pInput ) return m_pInput->VirtualOff( eBehavior, in_bUseSourceOffset ); return AK_Success; }
	virtual AKRESULT	Seek() { return ( m_pInput ) ? m_pInput->Seek() : AK_Fail; }	// If no input, it is too late to seek. 
	virtual AkReal32	GetPitch() { return ( m_pInput ) ? m_pInput->GetPitch() : 1.f; }

	virtual void		Connect( CAkVPLNode * in_pInput );
	void				Disconnect( );

protected:
	static void CopyRelevantMarkers(AkPipelineBuffer* in_pInputBuffer, AkPipelineBuffer* io_pBuffer, AkUInt32 in_ulBufferStartOffset, AkUInt32 in_ulNumFrames);

	CAkVPLNode *		m_pInput;		// Pointer to connected input node.
};

#endif  // _AK_VPL_NODE_H_
