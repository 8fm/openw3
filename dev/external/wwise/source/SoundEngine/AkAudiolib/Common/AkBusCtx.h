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
// AkBusCtx.h
//
// Definition of the Audio engine bus runtime context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_BUS_CTX_H_
#define _AK_BUS_CTX_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkLEngineDefs.h"

class CAkBus;

enum BusVolumeType
{
	BusVolumeType_ToNextBusWithEffect,
	BusVolumeType_IncludeEntireBusTree
};

namespace AK
{
    //-----------------------------------------------------------------------------
    // CAkBusCtx interface.
    //-----------------------------------------------------------------------------
    class CAkBusCtx
    {
    public:
		CAkBusCtx():m_pBus(NULL){}

		CAkBusCtx GetParentCtx() const;

	    // Sound parameters.
	    AkUniqueID		ID() const;
		AkVolumeValue	GetVolume( BusVolumeType in_VolumeType ) const;
		bool			IsTopBusCtx() const;
		AkUInt32		GetChannelConfig() const;

	    // Effects access.
		void			GetFX( AkUInt32 in_uFXIndex, AkFXDesc& out_rFXInfo ) const;
		void			GetFXDataID( AkUInt32 in_uFXIndex, AkUInt32 in_uDataIndex, AkUniqueID& AkUniqueID) const;
		bool			GetBypassAllFX() const;

		void SetBus( CAkBus* in_pBus ){ m_pBus = in_pBus; }
		CAkBus* GetBus() const { return m_pBus; }
		bool HasBus() const { return ( m_pBus != NULL ); }

		bool			IsAuxBus() const;

	private:
		CAkBus* m_pBus;
    };
}

#endif // _AK_BUS_CTX_H_
