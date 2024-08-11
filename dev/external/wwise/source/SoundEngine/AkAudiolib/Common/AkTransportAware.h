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
// AkTransportAware.h
//
// Audio context interface for Wwise transport, required by the
// PlayingMgr.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_TRANSPORT_AWARE_H_
#define _AK_TRANSPORT_AWARE_H_

enum AkPBIStopMode
{
	AkPBIStopMode_Normal = 0,
	AkPBIStopMode_StopAndContinue = 1,
	AkPBIStopMode_StopAndContinueSequel = 2
};

class CAkTransportAware
{
public:
	virtual void _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false, bool in_bHasNotStarted = false ) = 0;

#ifndef AK_OPTIMIZED
	virtual void _StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		) = 0;
#endif

	// Returns bitfield of AkCallbackType
	AkForceInline AkUInt32 GetRegisteredNotif()
	{
		return m_uRegisteredNotif;
	}

protected:
	CAkTransportAware()
		: m_uRegisteredNotif( 0 )
	{}
	virtual ~CAkTransportAware() {}


	AkUInt32	m_uRegisteredNotif; // cached copy of registered notifs from playingmgritem
};

#endif // _AK_TRANSPORT_AWARE_H_
