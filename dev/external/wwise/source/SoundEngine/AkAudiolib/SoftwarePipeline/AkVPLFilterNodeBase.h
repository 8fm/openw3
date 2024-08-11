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

#ifndef _AK_VPL_FILTER_NODE_BASE_H_
#define _AK_VPL_FILTER_NODE_BASE_H_

#include "AkFXContext.h"
#include "AkVPLNode.h"

class CAkVPLFilterNodeBase : public CAkVPLNode
{
public:
	
	virtual void		VirtualOn( AkVirtualQueueBehavior eBehavior );
    virtual AKRESULT	VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset );

	virtual AKRESULT Init(
		IAkPlugin * in_pPlugin,
		const AkFXDesc & in_fxDesc,
		AkUInt32 in_uFXIndex,
		CAkPBI * in_pCtx,
		AkAudioFormat &	in_format );
	virtual void		Term() = 0;
	virtual void		ReleaseMemory() = 0;
	virtual bool		ReleaseInputBuffer() = 0;

	virtual void		GetBuffer( AkVPLState & io_state ) = 0;
	virtual void		ConsumeBuffer( AkVPLState & io_state ) = 0;

	bool GetBypassed() { return m_bBypassed; }
	void SetBypassed( bool in_bBypassed ) { m_bBypassed = in_bBypassed; }
	AkPluginID GetFXID() { return m_FXID; }

	inline void			SetPBI( CAkPBI * in_pPBI ) { m_pCtx = in_pPBI; m_pInsertFXContext->SetPBI( in_pPBI ); }

	bool IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot )
	{
		return m_pInsertFXContext && m_pInsertFXContext->IsUsingThisSlot( in_pUsageSlot, GetPlugin() );
	}

	bool IsUsingThisSlot( const AkUInt8* in_pData )
	{
		return m_pInsertFXContext && m_pInsertFXContext->IsUsingThisSlot( in_pData );
	}

	AK::IAkPluginParam* GetPluginParam() { return m_pParam; }
	virtual AK::IAkPlugin* GetPlugin() = 0;
	virtual AkChannelMask GetOutputChannelMask() = 0;

protected:
	CAkPBI *				m_pCtx;			// Pointer to context.
	CAkInsertFXContext *	m_pInsertFXContext;	// FX context.
	AK::IAkPluginParam *	m_pParam;			// Parameters.
	AkPluginID				m_FXID;				// Effect unique type ID. 

	bool					m_bLast;		// True=was the last input buffer.
	bool					m_bBypassed;
	bool					m_LastBypassed;	// FX bypassed last buffer (determine whether Reset necessary)
	AkUInt32                m_uFXIndex;
};

#endif //_AK_VPL_FILTER_NODE_BASE_H_
