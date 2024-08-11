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

/////////////////////////////////////////////////////////////////////
//
// AkVPLLPFNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLLPFNode.h"
#include "Ak3DParams.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

void CAkVPLLPFNode::ConsumeBuffer( AkVPLState & io_state )
{
	// this could be no more data if we're getting the last buffer
#ifdef AK_PS3
	io_state.resultPrevious = io_state.result;
#endif
	if ( io_state.HasData() )
	{
#ifdef __PPU__
		// this will either start a job or update filter memory
		m_LPF.ExecutePS3(&io_state, io_state.result);
#else
		m_LPF.Execute( &io_state );
#endif
	}
}

void CAkVPLLPFNode::ProcessDone( AkVPLState & io_state )
{
#ifdef AK_PS3
	io_state.result = io_state.resultPrevious;
#endif
}

void CAkVPLLPFNode::ReleaseBuffer()
{
	if ( m_pInput ) // Can be NULL when voice starvation occurs in sample-accurate sequences
		m_pInput->ReleaseBuffer();	 
} // ReleaseBuffer

AKRESULT CAkVPLLPFNode::TimeSkip( AkUInt32 & io_uFrames )
{
	// no need to do anything while skipping time here.
	// this is assuming that the output volume at boundaries of 'skipped time' is equivalent
	// to zero, masking any artefact that might arise from not updating the coefficients.

	return m_pInput->TimeSkip( io_uFrames );
}

void CAkVPLLPFNode::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	m_LPF.ResetRamp();

	if ( m_pInput ) // Could be NULL when voice starvation occurs in sample-accurate sequences
		m_pInput->VirtualOn( eBehavior );
}
