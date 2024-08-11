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
// AkActionSeek.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_SEEK_H_
#define _ACTION_SEEK_H_

#include "AkAction.h"
#include "AkPrivateTypes.h"
#include "AkParameterNodeBase.h"	// enum ActionParamType;

class CAkActionSeek : public CAkActionExcept
{
public:
	//Thread safe version of the constructor
	static CAkActionSeek* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	void SetSeekToNearestMarker( bool in_bSeekToNearestMarker );
	void SetSeekPositionTimeAbsolute( AkTimeMs in_position, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax );
	void SetSeekPositionPercent( AkReal32 in_position, AkReal32 in_rangeMin, AkReal32 in_rangeMax );

	//Execute the Action
	//Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

protected:

	CAkActionSeek(AkActionType in_eActionType, AkUniqueID in_ulID);
	virtual ~CAkActionSeek();

protected:
	//Execute object specific execution
	AKRESULT Exec(
		CAkRegisteredObj * in_pGameObj,		//Game object pointer
		AkPlayingID in_TargetPlayingID
		);

	void AllExec(
		CAkRegisteredObj * in_pGameObj,
		AkPlayingID in_TargetPlayingID
		);

	void AllExecExcept(
		CAkRegisteredObj * in_pGameObj,
		AkPlayingID in_TargetPlayingID
		);

	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:
	RANGED_PARAMETER<AkReal32>	m_position;	// In milliseconds or percentage [0,1]
	bool						m_bIsSeekRelativeToDuration;
	bool						m_bSnapToNearestMarker;
};
#endif
