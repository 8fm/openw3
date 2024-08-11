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
// AkMusicRanSeqCntr.h
//
// Music Random/Sequence container definition.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_RAN_SEQ_CNTR_H_
#define _MUSIC_RAN_SEQ_CNTR_H_

#include "AkMusicTransAware.h"
#include "AkList2.h"
#include "AkRanSeqBaseInfo.h"
#include "AkRSIterator.h"

class CAkMatrixAwareCtx;
class CAkSequenceCtx;

class CAkMusicRanSeqCntr : public CAkMusicTransAware
{
public:

    // Thread safe version of the constructor.
	static CAkMusicRanSeqCntr * Create(
        AkUniqueID in_ulID = 0
        );

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize );

    // Return the node category.
	virtual AkNodeCategory NodeCategory();

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );

    virtual AKRESULT CanAddChild(
        CAkParameterNodeBase * in_pAudioNode 
        );

    // Context factory. 
    virtual CAkMatrixAwareCtx * CreateContext( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams &  in_rUserparams
        );

    CAkSequenceCtx * CreateSequenceCtx( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams &  in_rUserparams
        );

    // Play the specified node
    //
    // Return - AKRESULT - Ak_Success if succeeded
    virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams );

    // Interface for Wwise
    // ----------------------

    AKRESULT SetPlayListChecked(
		AkMusicRanSeqPlaylistItem*	in_pArrayItems
		);

   

	AKRESULT SetPlayList(
		AkMusicRanSeqPlaylistItem*	in_pArrayItems
		);

	AKRESULT AddPlaylistChildren(	CAkRSSub*					in_pParent,
									AkMusicRanSeqPlaylistItem*&	in_pArrayItems, 
									AkUInt32					in_ulNumItems 
									);

    

    // Interface for Contexts
    // ----------------------

    // Get first level node by index.
    AKRESULT GetNodeAtIndex( 
        AkUInt16        in_index, 
        AkUInt16 &      io_uPlaylistIdx     // TODO Replace with Multiplaylist iterator.
        );

	CAkRSSub* GetPlaylistRoot(){ return &m_playListRoot; }

protected:
    CAkMusicRanSeqCntr( 
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicRanSeqCntr();
    AKRESULT Init();
	void	 Term();

	void	FlushPlaylist();
	
	 bool CheckPlaylistHasChanged( AkMusicRanSeqPlaylistItem* in_pArrayItems );
	 
	 bool CheckPlaylistChildrenHasChanged(   CAkRSSub*					in_pParent,
									        AkMusicRanSeqPlaylistItem*&	in_pArrayItems, 
									        AkUInt32					in_ulNumItems 
									        );


private:

	CAkRSSub	m_playListRoot;
};

#endif //_MUSIC_RAN_SEQ_CNTR_H_
