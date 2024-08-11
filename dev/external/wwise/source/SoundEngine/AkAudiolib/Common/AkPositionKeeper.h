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
// AkPositionKeeper.h
//
//////////////////////////////////////////////////////////////////////


#ifndef _AKPOSITIONKEEPER_H_
#define _AKPOSITIONKEEPER_H_

#include <AK/Tools/Common/AkObject.h>
#include "AkDefault3DParams.h"

extern AkSoundPosition g_DefaultSoundPosition;

static AkForceInline void _SetSoundPosToListener( const AkListenerPosition & in_listPos, AkSoundPosition & io_sndPos )
{
	// Take the listener orientation, as is
	AkReal32 fScale = AK_LOWER_MIN_DISTANCE;

	// Position the sound AK_LOWER_MIN_DISTANCE Distance unit in front of the Listener
	io_sndPos.Position.X = in_listPos.Position.X + (in_listPos.OrientationFront.X * fScale);
	io_sndPos.Position.Y = in_listPos.Position.Y + (in_listPos.OrientationFront.Y * fScale);
	io_sndPos.Position.Z = in_listPos.Position.Z + (in_listPos.OrientationFront.Z * fScale);

	// Orientation should be the opposite of the listener one
	io_sndPos.Orientation.X = -(in_listPos.OrientationFront.X);
	io_sndPos.Orientation.Y = -(in_listPos.OrientationFront.Y);
	io_sndPos.Orientation.Z = -(in_listPos.OrientationFront.Z);
}

class AkSoundPositionRef
{
	friend class AkPositionKeeper;
public:

	AkForceInline AkSoundPositionRef()
		: m_aPos( NULL )
		, m_uNumPos( 0 )
		, m_uListenerMask( AK_DEFAULT_LISTENER_MASK )
		, m_eMultiPositionType( AK::SoundEngine::MultiPositionType_SingleSource )
#if defined AK_WII_FAMILY
		, m_uControllerActiveMask( 0 )
#endif
	{}

	// GetFirstPositionFixme is temporary only
	// Everyone using this function should change its implementation, it is currently only there for backward compatibility and
	// for development time only. Using GetFirstPositionFixme() will result in the old behavior, which will suppose there may be only one
	// position per object.
	AkForceInline const AkSoundPosition& GetFirstPositionFixme() const
	{
		return *GetPositions();
	}
	
	AkForceInline const AkSoundPosition * GetPositions() const
	{
		if( m_aPos )
			return m_aPos;

		return GetDefaultPosition();
	}

	AkForceInline const AkSoundPosition* GetDefaultPosition() const
	{
		return &g_DefaultSoundPosition;
	}

	AkForceInline AkUInt16 GetNumPosition() const
	{
		// IMPORTANT: "Single-source" mode supersedes number of positions stored in game object.
		return ( m_eMultiPositionType == AK::SoundEngine::MultiPositionType_SingleSource ) ? 1 : m_uNumPos;
	}

	AkForceInline AK::SoundEngine::MultiPositionType GetMultiPositionType() const
	{
		return (AK::SoundEngine::MultiPositionType)m_eMultiPositionType;
	}

	AkForceInline AkUInt32 GetListenerMask() const
	{
		return m_uListenerMask;
	}

#if defined AK_WII_FAMILY
	AkForceInline AkUInt32 GetControllerActiveMask() const
	{
		return m_uControllerActiveMask;
	}
#endif

protected:

	AkSoundPosition*	m_aPos;
	AkUInt16			m_uNumPos;
	
	AkUInt8				m_uListenerMask;	// bitmask of active listeners
	AkUInt8				m_eMultiPositionType :3; // AK::SoundEngine::MultiPositionType, on 8 bits to save some memory.

#if defined AK_WII_FAMILY
	AkUInt16			m_uControllerActiveMask;	// Wiimote activation mask
#endif

};


class AkPositionKeeper : public AkSoundPositionRef
{
public:
	~AkPositionKeeper()
	{
		FreeCurrentPosition();
	}

	AKRESULT SetPosition( const AkSoundPosition* in_Positions, AkUInt16 in_NumPos )
	{
		AKRESULT eResult = Allocate( in_NumPos );

		if( eResult != AK_Success )
			return eResult;

		//now that we are sure we have the right size allocated, copy it.
		memcpy( m_aPos, in_Positions, m_uNumPos * sizeof( AkSoundPosition ) );

		return AK_Success;
	}

	AkForceInline AKRESULT Copy( const AkSoundPositionRef& in_rPosKeeper )
	{
		// Simili-Copy constructor.

		m_uListenerMask = in_rPosKeeper.m_uListenerMask;
		m_eMultiPositionType = in_rPosKeeper.m_eMultiPositionType;
		return SetPosition( in_rPosKeeper.m_aPos, in_rPosKeeper.m_uNumPos );
	}

	AkForceInline AKRESULT SetPositionWithDefaultOrientation( const AkVector& in_Position )
	{
		SetListenerMask( AK_DEFAULT_LISTENER_MASK );
		// allocate a single entry
		AKRESULT eResult = Allocate( 1 );
		if( eResult != AK_Success )
			return eResult;

		m_aPos->Orientation.X = AK_DEFAULT_SOUND_ORIENTATION_X;
		m_aPos->Orientation.Y = AK_DEFAULT_SOUND_ORIENTATION_Y;
		m_aPos->Orientation.Z = AK_DEFAULT_SOUND_ORIENTATION_Z;
		m_aPos->Position = in_Position;
		return AK_Success;
	}

	AkForceInline AKRESULT SetPositionToListener( const AkListenerPosition& in_listenerPosition )
	{
		AKRESULT eResult = Allocate( 1 );
		if( eResult != AK_Success )
			return eResult;

		_SetSoundPosToListener( in_listenerPosition, (*m_aPos) );

		return AK_Success;
	}

	AkForceInline void SetListenerMask( AkUInt32 in_uListenerMask)
	{
		m_uListenerMask = (AkInt8)in_uListenerMask;
	}

	AkForceInline void  SetMultiPositionType( AK::SoundEngine::MultiPositionType in_eMultiPositionType )
	{
		m_eMultiPositionType = in_eMultiPositionType;
	}

#if defined AK_WII_FAMILY
	AkForceInline void SetControllerActiveMask( AkUInt32 in_uControllerActiveMask )
	{
		m_uControllerActiveMask = (AkUInt16)in_uControllerActiveMask;
	}
#endif

private:

	AkForceInline AKRESULT Allocate( AkUInt16 in_NumItems )
	{
		if( in_NumItems != m_uNumPos )
		{
			FreeCurrentPosition();
			
			if( in_NumItems == 0 )
				return AK_Success;

			// We have to re-allocate, the actual size doesn't fit.
			m_aPos = (AkSoundPosition*)AkAlloc( g_DefaultPoolId, in_NumItems * sizeof( AkSoundPosition ) );

			if( !m_aPos )
				return AK_InsufficientMemory;

			// set the size only if we successfully allocated memory.
			m_uNumPos = in_NumItems;
		}
		return AK_Success;
	}

	void FreeCurrentPosition()
	{
		if( m_aPos )
		{
			AkFree( g_DefaultPoolId, m_aPos );
			m_aPos = NULL;
			m_uNumPos = 0;
		}
	}
};

#endif //_AKPOSITIONKEEPER_H_
