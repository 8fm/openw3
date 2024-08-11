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

#ifndef _AK_SRC_FILEPCM_H_
#define _AK_SRC_FILEPCM_H_

#include "AkSrcFileBase.h"

class CAkSrcFilePCM : public CAkSrcFileBase
{
public:

	//Constructor and destructor
    CAkSrcFilePCM( CAkPBI * in_pCtx );
	virtual ~CAkSrcFilePCM();

	// Data access.
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual void	 VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual void	 StopStream();

	virtual AKRESULT ChangeSourcePosition();	// Overrides CAkSrcFileBase in order to clear out PCM-specific status

	virtual bool MustRelocatePitchInputBufferOnMediaRelocation();

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

	// Returns block align. This function is virtual, in order for derived sources to manage their own 
	// block align independently of that of the pipeline/PBI (read 24 bits).
	// NEVER access PBI's block align directly inside this class.
	virtual AkUInt16 GetBlockAlign() const;

	// Incomplete sample frames handling.
	AkUInt8	*		 m_pStitchBuffer;
	AkUInt32         m_uNumBytesBuffered;	// Number of bytes temporarily stored in stitched sample frame buffer.
	
	AkUInt32		 m_uSizeToRelease;		// Size of returned streamed buffer (in bytes).
	
};

#endif // _AK_SRC_FILEPCM_H_
