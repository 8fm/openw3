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

#pragma once

#include "AkSrcBase.h"
#include "AkADPCMCodec.h"

class CAkSrcBankADPCM : public CAkSrcBaseEx
{
public:

	//Constructor and destructor
	CAkSrcBankADPCM( CAkPBI * in_pCtx );
	virtual ~CAkSrcBankADPCM();

	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual AKRESULT VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset );

	virtual AKRESULT ChangeSourcePosition();

	virtual bool SupportMediaRelocation() const;
	virtual AKRESULT RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia );

protected:
	// Override OnLoopComplete() handler: place input pointer to loop start.
	virtual AKRESULT OnLoopComplete(
		bool in_bEndOfFile		// True if this was the end of file, false otherwise.
		);

private:
	AKRESULT		 SeekToSourceOffset();
	inline AkUInt8 * ConvertToInputDataAddress( AkUInt32 in_uSampleOffset )
	{
		AKASSERT( ( in_uSampleOffset % ADPCM_SAMPLES_PER_BLOCK ) == 0 );
		AkUInt8 * pBuffer;
	    AkUInt32 ulBufferSize;
		m_pCtx->GetDataPtr( pBuffer, ulBufferSize );
		AKASSERT( pBuffer );
		return pBuffer + m_uDataOffset + ( ( in_uSampleOffset / ADPCM_SAMPLES_PER_BLOCK ) * m_uInputBlockSize );
	}

    // File data.
	AkUInt8	*		m_pucData;			// Current input.

	// ADPCM
	AkUInt16		m_uInputBlockSize;
	AkUInt8 *       m_pOutBuffer;
};