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
// AkPathManager.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkPathManager.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkAudioMgr.h"
#include "AkGen3DParams.h"

//====================================================================================================
//====================================================================================================
CAkPathManager::CAkPathManager()
{
	m_uMaxPathNumber = 0;
}
//====================================================================================================
//====================================================================================================
CAkPathManager::~CAkPathManager()
{
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkPathManager::Init( AkUInt32 in_uMaxNumPath )
{
	m_uMaxPathNumber = in_uMaxNumPath ? in_uMaxNumPath : DEFAULT_MAX_NUM_PATHS;

	return m_ActivePathsList.Reserve( m_uMaxPathNumber );
}
//====================================================================================================
//====================================================================================================
void CAkPathManager::Term()
{
	AkPathList::Iterator iter = m_ActivePathsList.Begin();
	while( iter != m_ActivePathsList.End() )
	{
		CAkPath*	pPath = *iter;
		pPath->Term();
		AkDelete( g_DefaultPoolId, pPath );
		++iter;
	}
	m_ActivePathsList.Term();
}
//====================================================================================================
// add a new one to the list of those to be processed
//====================================================================================================
CAkPath* CAkPathManager::AddPathToList()
{
	CAkPath*	pThisPath;

	pThisPath = NULL;

	if( m_ActivePathsList.Length() < m_uMaxPathNumber )
	{
		// get a new one
		pThisPath = AkNew( g_DefaultPoolId, CAkPath() );

		// have we got one ?
		if(pThisPath != NULL)
		{
			// add it to the active list
			if(!m_ActivePathsList.AddLast(pThisPath))
			{
				AkDelete( g_DefaultPoolId, pThisPath );
				pThisPath = NULL;
			}
		}
	}

	return pThisPath;
}
//====================================================================================================
// removes a path from the list
//====================================================================================================
AKRESULT CAkPathManager::RemovePathFromList(CAkPath* in_pPath)
{
	AKRESULT	eResult;

	AKASSERT(in_pPath != NULL);

	// assume no Path
	eResult = AK_PathNotFound;

	// if we've got that one
	AkPathList::Iterator it = m_ActivePathsList.FindEx( in_pPath );
	if( it != m_ActivePathsList.End() )
	{
		CAkPath * pPath = *it;

		m_ActivePathsList.EraseSwap( it );

		// stop it
		pPath->Term();
		AkDelete( g_DefaultPoolId, pPath );

		eResult = AK_Success;
	}

	return eResult;
}
//====================================================================================================
// adds a given sound to a given multi user Path
//====================================================================================================
AKRESULT CAkPathManager::AddPathUser(CAkPath* in_pPath, CAkPBI* in_pPBI)
{
#ifndef AK_OPTIMIZED
	// WG-6668 alessard
	if( !m_ActivePathsList.Exists(in_pPath) )
	{
		return AK_Fail;
	}
#endif

		// assume we're full
	AKRESULT eResult = AK_Fail;

	if(in_pPath->m_PBIsList.Length() < AK_PATH_USERS_LIST_SIZE)
	{
		// assume already in list
		eResult = AK_PathNodeAlreadyInList;

		if( !in_pPath->m_PBIsList.Exists(in_pPBI) )
		{
			// add it to our list
			if( in_pPath->m_PBIsList.AddLast(in_pPBI) )
			{
				
				eResult = AK_Success;
				// we have one more
				++in_pPath->m_iNumUsers;
			}
		}
	}

	return eResult;
}
//====================================================================================================
// removes a given sound from a given multi user Path
//====================================================================================================
AKRESULT CAkPathManager::RemovePathUser(CAkPath* in_pPath, CAkPBI* in_pPBI)
{
#ifndef AK_OPTIMIZED
	// WG-6668 alessard
	if( !m_ActivePathsList.Exists(in_pPath) )
	{
		return AK_Success;
	}
#endif

	// assume we don't have this user
	AKRESULT eResult = AK_PathNodeNotInList;

	// look for our user in the list
	if(in_pPath->m_PBIsList.RemoveSwap(in_pPBI) == AK_Success)
	{
		// we have one less user
		--in_pPath->m_iNumUsers;

		// anybody using this one any more ?
		if((in_pPath->m_iPotentialUsers == 0) && (in_pPath->m_iNumUsers == 0))
		{
			eResult = RemovePathFromList(in_pPath);
		}
		else
		{
			eResult = AK_Success;
		}
	}

	return eResult;
}
//====================================================================================================
// 
//====================================================================================================
void CAkPathManager::AddPotentialUser(CAkPath* in_pPath)
{
#ifndef AK_OPTIMIZED
	// WG-6668 alessard
	if( !m_ActivePathsList.Exists(in_pPath) )
	{
		return;
	}
#endif

	// we have one more
	++in_pPath->m_iPotentialUsers;
}

//====================================================================================================
// 
//====================================================================================================
AKRESULT CAkPathManager::RemovePotentialUser(CAkPath* in_pPath)
{
#ifndef AK_OPTIMIZED
	// WG-6668 alessard
	if( !m_ActivePathsList.Exists(in_pPath) )
	{
		return AK_Success;
	}
#endif
	AKASSERT(in_pPath->m_iPotentialUsers > 0);

	// have we got any of these ?
	if(in_pPath->m_iPotentialUsers)
	{
		// one less
		--in_pPath->m_iPotentialUsers;
	}

	// have we still got anybody interested in this path ?
	if((in_pPath->m_iPotentialUsers == 0) && (in_pPath->m_iNumUsers == 0))
	{
		// nope, get rid of it
		return RemovePathFromList(in_pPath);
	}
	else
	{
		return AK_Success;
	}
}
//====================================================================================================
// sets the playlist, default mode is sequence, step
//====================================================================================================
AKRESULT CAkPathManager::SetPathsList(CAkPath*			in_pPath,
										AkPathListItem*	in_pPathList,
										AkUInt32		in_ulListSize,
										AkPathMode		in_PathMode,
										bool			in_bIsLooping,
										AkPathState*	in_pState)
{
	AKASSERT( m_ActivePathsList.Exists(in_pPath) );

	// assume something's wrong
	AKRESULT eResult = AK_InvalidParameter;
	if((in_pPathList != NULL) && (in_ulListSize > 0))
	{
		eResult = in_pPath->SetPathsList(in_pPathList,in_ulListSize,in_PathMode,in_bIsLooping,in_pState);
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkPathManager::Start(CAkPath* in_pPath,AkPathState* in_pState)
{
	AKASSERT( m_ActivePathsList.Exists(in_pPath) );

	AKRESULT eResult = AK_Fail;
	if( in_pPath->m_eState == CAkPath::Idle )
	{
		if( !in_pPath->m_bWasStarted )
		{
			// same as what's done in SetPathsLists()
			if(in_pState->pbPlayed != NULL)
			{
				// use these
				in_pPath->m_pbPlayed = in_pState->pbPlayed;
				in_pPath->m_ulCurrentListIndex = (AkUInt16) in_pState->ulCurrentListIndex;
				// set the current one
				in_pPath->m_pCurrentList = in_pPath->m_pPathsList + in_pPath->m_ulCurrentListIndex;
			}

			eResult = in_pPath->Start( g_pAudioMgr->GetBufferTick() );
			// if continuous do not save the state
			// next sound instance should start at first one
			if(!(in_pPath->m_PathMode & AkPathContinuous))
			{
				// save the ones we have
				in_pPath->GetNextPathList();
				in_pState->ulCurrentListIndex = in_pPath->m_ulCurrentListIndex;
				in_pState->pbPlayed = in_pPath->m_pbPlayed;
			}
		}
		else
		{
			in_pPath->UpdateStartPosition();
		}
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkPathManager::Stop(CAkPath* in_pPath)
{
	AKASSERT( m_ActivePathsList.Exists(in_pPath) );

	AKRESULT eResult = AK_PathNotRunning;
	if(in_pPath->m_eState == CAkPath::Running)
	{
		eResult = AK_Success;
		in_pPath->Stop();
	}

	return eResult;
}
//====================================================================================================
// pauses a path
//====================================================================================================
AKRESULT CAkPathManager::Pause(CAkPath* in_pPath)
{
	AKASSERT( m_ActivePathsList.Exists(in_pPath) );

	// assume not running
	AKRESULT eResult = AK_PathNotRunning;

	// is it pause-able ?
	if(in_pPath->m_eState == CAkPath::Running)
	{
		eResult = AK_Success;
		// pause it
		in_pPath->Pause( g_pAudioMgr->GetBufferTick() );
	}

	return eResult;
}
//====================================================================================================
// resumes a path
//====================================================================================================
AKRESULT CAkPathManager::Resume(CAkPath* in_pPath)
{
	AKASSERT( m_ActivePathsList.Exists(in_pPath) );

	// assume not paused
	AKRESULT eResult = AK_PathNotPaused;

	// is it resume-able
	if(in_pPath->m_eState == CAkPath::Paused)
	{
		eResult = AK_Success;
		// resume it
		in_pPath->Resume( g_pAudioMgr->GetBufferTick() );
	}

	return eResult;
}
//====================================================================================================
// does what it takes to get things moving
//====================================================================================================
void CAkPathManager::ProcessPathsList( AkUInt32 in_uCurrentBufferTick )
{
	AkPathList::Iterator iter = m_ActivePathsList.Begin();
	while( iter != m_ActivePathsList.End() )
	{
		CAkPath* pThisPath = *iter;
		if(pThisPath->m_eState == CAkPath::Running)
		{
			pThisPath->UpdatePosition( in_uCurrentBufferTick );
		}
		++iter;
	}
}
