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
// AkLEngineCmds.h
//
// Cross-platform lower engine commands management.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_LENGINE_CMDS_H_
#define _AK_LENGINE_CMDS_H_

#include "AkLEngineStructs.h"
#include "AkList2.h"
#include <AK/Tools/Common/AkListBare.h>
// Warning: all platforms must implement a CAkVPLSrcCbxNode and a static CAkLEngine
// with the methods used in this class. TODO Move to a separate, cross-platform interface.
#include "AkVPLSrcCbxNode.h"

//-----------------------------------------------------------------------------
// Enums.
//-----------------------------------------------------------------------------
enum LEState
{
	LEStatePlay			= 0,
	LEStatePlayPause	= 1,
	LEStateStop			= 2,
	LEStatePause		= 3,
	LEStateResume		= 4,
	LEStateStopLooping	= 5,
	LEStateSeek			= 6
};

//-----------------------------------------------------------------------------
// Structs.
//-----------------------------------------------------------------------------
struct AkLECmd
{
	CAkPBI *	m_pCtx;				// Pointer to a sound context.
	LEState		m_eState;			// Running state of the source.	
    AkUInt32	m_ulSequenceNumber;
	bool		m_bSourceConnected;
};

struct AkListVPLSrcsNextItem
{
	static AkForceInline CAkVPLSrcCbxNode *& Get( CAkVPLSrcCbxNode * in_pItem ) 
	{
		return *((CAkVPLSrcCbxNode **) &in_pItem->pNextItem);
	}
};

typedef AkListBare<AKPBI_CBXCLASS> AkListVPLSrcsBase;
typedef AkListBare<CAkVPLSrcCbxNode, AkListVPLSrcsNextItem, AkCountPolicyWithCount> AkListVPLSrcs;

//-----------------------------------------------------------------------------
// CAkLEngineCmds static class.
//-----------------------------------------------------------------------------

class CAkLEngineCmds
{
public:
	static AKRESULT Init();
	static void Term();

	// Behavioral engine interface
	static inline bool ProcessPlayCmdsNeeded() { return m_bProcessPlayCmdsNeeded; }
	static void ProcessPlayCommands();
	static void IncrementSyncCount(){ ++m_ulPlayEventID; };

	// Interface for CAkLEngine.
	static void ProcessAllCommands();
	static void DeleteAllCommandsForSource( AKPBI_CBXCLASS * in_pCbx );
	static void ProcessDisconnectedSources( AkUInt32 in_uFrames );
	static void DestroyDisconnectedSources();
	static inline void AddDisconnectedSource( AKPBI_CBXCLASS * in_pCbx ) { m_listSrcsNotConnected.AddLast( in_pCbx ); }
	static inline void RemoveDisconnectedSource( AKPBI_CBXCLASS * in_pCbx ) { AKVERIFY( m_listSrcsNotConnected.Remove( in_pCbx ) == AK_Success ); }

	// Interface for PBI/URenderer.
	static void DequeuePBI( CAkPBI* in_pPBI );
	static AKRESULT	EnqueueAction( LEState in_eState, CAkPBI * in_pContext );
	static AKRESULT	EnqueueActionStop( CAkPBI * in_pContext );

private:
    static void ProcessPendingCommands();
	
private: 
	typedef CAkList2<AkLECmd, const AkLECmd&, AkAllocAndFree, ArrayPoolLEngineDefault> AkListCmd;
	static AkListCmd			m_listCmd;					// List of command posted by the upper engine.
    static AkListVPLSrcsBase	m_listSrcsNotConnected;		// List of sounds not yet connected to a bus.	
	static AkUInt32				m_ulPlayEventID;			// Play event id.
	static bool					m_bProcessPlayCmdsNeeded;
};
#endif	//_AK_LENGINE_CMDS_H_
