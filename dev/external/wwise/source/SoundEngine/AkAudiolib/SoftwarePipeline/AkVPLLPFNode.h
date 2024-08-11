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
// AkVPLLPFNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_LPF_NODE_H_
#define _AK_VPL_LPF_NODE_H_

#include "AkSrcLpFilter.h"
#include "AkVPLNode.h"

class CAkVPLLPFNode : public CAkVPLNode
{
public:
	/*virtual*/ void	ConsumeBuffer( AkVPLState & io_state );
	/*virtual*/ void	ProcessDone( AkVPLState & io_state );
	virtual void		ReleaseBuffer();

	AKRESULT			TimeSkip( AkUInt32 & io_uFrames );
	AkForceInline AKRESULT Init( AkChannelMask in_uChannelMask ) { return m_LPF.Init( in_uChannelMask ); }
	AkForceInline void	Term() { m_LPF.Term(); }

	AkForceInline void SetLPF( AkReal32 in_fLPF ) { m_LPF.SetLPFPar( in_fLPF ); }
	AkForceInline AkReal32 GetLPF() const { return m_LPF.GetLPFPar(); }

	AkForceInline bool 	IsInitialized() { return m_LPF.IsInitialized(); }
	virtual void		VirtualOn( AkVirtualQueueBehavior eBehavior );

private:
	CAkSrcLpFilter		m_LPF;			// Pointer to lpf object.
};

#endif //_AK_VPL_LPF_NODE_H_
