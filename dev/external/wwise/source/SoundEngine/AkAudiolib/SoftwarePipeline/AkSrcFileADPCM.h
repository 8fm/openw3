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

#ifndef _AK_SRC_FILEADPCM_H_
#define _AK_SRC_FILEADPCM_H_

#include "AkSrcFileBase.h"
#include "AkADPCMCodec.h"

#define ADPCM_MAX_BLOCK_ALIGN (ADPCM_BLOCK_SIZE*AK_VOICE_MAX_NUM_CHANNELS)

class CAkSrcFileADPCM : public CAkSrcFileBase
{
public:

	//Constructor and destructor
    CAkSrcFileADPCM( CAkPBI * in_pCtx );
	virtual ~CAkSrcFileADPCM();

	// Data access.
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );

	virtual void	 StopStream();

	virtual AKRESULT ChangeSourcePosition();

protected:

    virtual AKRESULT ParseHeader(
		AkUInt8 * in_pBuffer	// Buffer to parse
		);
	virtual AkReal32 GetThroughput( const AkAudioFormat & in_rFormat );	// Returns format-specific throughput.
	
	// Finds the closest offset in file that corresponds to the desired sample position.
	// Returns the file offset (in bytes, relative to the beginning of the file) and its associated sample position.
	// Returns AK_Fail if the codec is unable to seek.
	virtual AKRESULT FindClosestFileOffset( 
		AkUInt32 in_uDesiredSample,		// Desired sample position in file.
		AkUInt32 & out_uSeekedSample,	// Returned sample where file position was set.
		AkUInt32 & out_uFileOffset		// Returned file offset where file position was set.
		);

	AkUInt32		m_uInputBlockSize;	

	AkUInt8 *		m_pOutBuffer;

	AkUInt8			m_ExtraBlock[ ADPCM_MAX_BLOCK_ALIGN ];
	AkUInt16        m_wExtraSize;
};

#endif // _AK_SRC_FILEADPCM_H_
