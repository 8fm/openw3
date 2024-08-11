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

#ifndef _AK_SRC_BANKPCM_H_
#define _AK_SRC_BANKPCM_H_

#include "AkSrcBase.h"

#ifdef AK_VITA_HW
#include "AkSrcBankNgsHw.h"
#endif

#ifdef AK_VITA_HW
class CAkSrcBankPCM : public CAkSrcBankNgsHw
#else
class CAkSrcBankPCM : public CAkSrcBaseEx
#endif
{
public:
	//Constructor and destructor
	CAkSrcBankPCM( CAkPBI * in_pCtx );
	virtual ~CAkSrcBankPCM();
	
	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual AKRESULT VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset );

	virtual AKRESULT ChangeSourcePosition();

	virtual bool SupportMediaRelocation() const;
	virtual AKRESULT RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia );

	virtual bool MustRelocatePitchInputBufferOnMediaRelocation()
	{
		return true;
	}

private:
	AKRESULT		 SeekToSourceOffset();

    // File data.
#ifndef AK_VITA_HW
	AkUInt8 *		 m_pucDataStart;			// Start of PCM data in bank.
#endif
};

#endif // _AK_SRC_BANKPCM_H_
