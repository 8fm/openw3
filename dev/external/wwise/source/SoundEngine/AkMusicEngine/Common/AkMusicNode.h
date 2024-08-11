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
// AkMusicNode.h
//
// The Music node is meant to be a parent of all playable music objects (excludes tracks).
// Has the definition of the music specific Play method.
// Defines the method for grid query (music objects use a grid, either their own, or that of their parent).
//
//////////////////////////////////////////////////////////////////////

#ifndef _MUSIC_NODE_H_
#define _MUSIC_NODE_H_

#include "AkActiveParent.h"
#include "AkParameterNode.h"
#include "AkMusicStructs.h"
#include <AK/Tools/Common/AkArray.h>

class CAkMatrixAwareCtx;
class CAkSegmentBucket;

class CAkMusicNode : public CAkActiveParent<CAkParameterNode>
{
public:

	AKRESULT SetMusicNodeParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );

    virtual CAkMatrixAwareCtx * CreateContext( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams & in_rUserparams
        ) = 0;

    // Music implementation of game triggered actions handling ExecuteAction(): 
    // For Stop/Pause/Resume, call the music renderer, which searches among its
    // contexts (music renderer's contexts are the "top-level" contexts).
    // Other actions (actions on properties) are propagated through node hierarchy.
    virtual AKRESULT ExecuteAction( 
        ActionParams& in_rAction 
        );
	virtual AKRESULT ExecuteActionExcept( 
		ActionParamsExcept& in_rAction 
		);

	// Block some parameters notifications (pitch)
	virtual void ParamNotification( NotifParams& in_rParams );

	virtual AkObjectCategory Category();

	typedef AkArray<CAkStinger, const CAkStinger&, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof( CAkStinger )> StingerArray;

	class CAkStingers
	{
	public:
		const StingerArray&	GetStingerArray() const { return m_StingerArray; }
        StingerArray&	GetStingerArray() { return m_StingerArray; }
		void			Term(){ m_StingerArray.Term(); }

		void RemoveAllStingers() { m_StingerArray.RemoveAll(); }

	private:
		StingerArray m_StingerArray;
	};

	AKRESULT SetStingers( CAkStinger* in_pStingers, AkUInt32 in_NumStingers );

	void GetStingers( CAkStingers* io_pStingers );

    // Wwise access.
    // -----------------------------------------------
    void MeterInfo(
        const AkMeterInfo * in_pMeterInfo   // Music grid info. NULL if inherits that of parent.
        );

	const AkMusicGrid & GetMusicGrid();

protected:
    CAkMusicNode( 
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicNode();

	void FlushStingers();

    AKRESULT Init() { return CAkActiveParent<CAkParameterNode>::Init(); }

	virtual AKRESULT	PrepareData();
	virtual void		UnPrepareData();
	virtual AKRESULT	PrepareMusicalDependencies();
	virtual void		UnPrepareMusicalDependencies();

private:
    // Music grid.
    AkMusicGrid     m_grid;
    AkUInt8         m_bOverrideParentGrid :1;

	CAkStingers* 	m_pStingers;
};

#endif //_MUSIC_NODE_H_
