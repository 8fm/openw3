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

#include "AkSrcACPBase.h"

IAkSoftwareCodec* CreateATRAC9BankPlugin( void* in_pCtx );

class CAkSrcBankAt9 : public CAkSrcACPBase<CAkSrcBaseEx>
{
public:
	//Constructor and destructor
										CAkSrcBankAt9( CAkPBI * in_pCtx );
	virtual								~CAkSrcBankAt9();

	// Data access.
	virtual AKRESULT 					StartStream();
	virtual AKRESULT					RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia );

	// ACP job funntions
	/*virtual AKRESULT 					CreateHardwareInstance( const SceAjmContextId in_AjmContextId );
	AKRESULT 							InitialiseDecoderJob( SceAjmBatchInfo* in_Batch );
	virtual void 						CreateDecodingJob(SceAjmBatchInfo* in_Batch);*/
	AkUInt32							DecodingEnded(); // Return number of frames decoded
	void 								FixLoopPoints();

	virtual bool						Register();
	virtual void						Unregister();
	virtual void						VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT					VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset );
	
	CAkSrcBankAt9* 						pNextItem;	// List bare light sibling

private:

	virtual int							SetDecodingInputBuffers(
											AkInt32 in_uSampleStart, 
											AkInt32 in_uSampleEnd, 
											AkUInt16 in_uBufferIndex, 
											AkUInt32& out_remainingSize);

	//uint8_t 							m_At9ConfigData[SCE_AJM_DEC_AT9_CONFIG_DATA_SIZE];
	AkUInt8*							m_pSourceFileStartPosition;
	
};