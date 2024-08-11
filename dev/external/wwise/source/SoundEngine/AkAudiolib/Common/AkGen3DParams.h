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
// AkGen3DParams.h
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#ifndef _GEN_3D_PARAMS_H_
#define _GEN_3D_PARAMS_H_

#include "Ak3DParams.h"
#include "AkRTPC.h"
#include "AkBitArray.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkConversionTable.h"
#include "AkAudioLibIndex.h"

struct AkPathState;
struct AkPathVertex;
struct AkPathListItem;
struct AkPathListItemOffset;
class CAkPath;
class CAkAttenuation;

// IDs of the AudioLib known RTPC capable parameters
enum AkPositioning_ParameterID
{
	POSID_Positioning_Divergence_Center_PCT			= RTPC_Positioning_Divergence_Center_PCT,
	POSID_Positioning_Cone_Attenuation_ON_OFF		= RTPC_Positioning_Cone_Attenuation_ON_OFF,
	POSID_Positioning_Cone_Attenuation				= RTPC_Positioning_Cone_Attenuation,
	POSID_Positioning_Cone_LPF						= RTPC_Positioning_Cone_LPF,
	POSID_Position_PAN_X_2D							= RTPC_Position_PAN_X_2D,
	POSID_Position_PAN_Y_2D							= RTPC_Position_PAN_Y_2D,
	POSID_PositioningType							= RTPC_PositioningType,
	POSID_Position_PAN_X_3D							= RTPC_Position_PAN_X_3D,
	POSID_Position_PAN_Y_3D							= RTPC_Position_PAN_Y_3D,

//ASSERT that  RTPC_MaxNumRTPC == 32 == sizeof(AkUInt64)*8

	POSID_2DPannerEnabled							= sizeof(AkUInt64)*8,
	POSID_IsPositionDynamic,
	POSID_IsLooping,
	POSID_Transition,
	POSID_PathMode
};

// pan normally range from 0 to 100, 101 would be invalid.
#define INVALID_PAN_VALUE (101)

struct BaseGenParams
{
	AkReal32 m_fPAN_X_2D;
	AkReal32 m_fPAN_Y_2D;
	AkReal32 m_fCenterPCT;
	bool	 bIsPannerEnabled;

	bool operator ==(const BaseGenParams& in_Op)
	{
		return ( m_fPAN_X_2D		== in_Op.m_fPAN_X_2D ) 
			&& ( m_fPAN_Y_2D		== in_Op.m_fPAN_Y_2D ) 
			&& ( m_fCenterPCT	== in_Op.m_fCenterPCT ) 
			&& ( bIsPannerEnabled	== in_Op.bIsPannerEnabled );
	}

	void Invalidate()
	{
		m_fPAN_X_2D = INVALID_PAN_VALUE;
	}

	void Init()
	{
		m_fPAN_X_2D = 0.5f;
		m_fPAN_Y_2D = 1.f;
		m_fCenterPCT = 100.f;
		bIsPannerEnabled = false;
	}

	bool operator !=(const BaseGenParams& in_Op)
	{
		return ( m_fPAN_X_2D		!= in_Op.m_fPAN_X_2D ) 
			|| ( m_fPAN_Y_2D		!= in_Op.m_fPAN_Y_2D ) 
			|| ( m_fCenterPCT	!= in_Op.m_fCenterPCT ) 
			|| ( bIsPannerEnabled	!= in_Op.bIsPannerEnabled );
	}
};

struct AkPanningConversion
{
	AkReal32 fX;
	AkReal32 fY;
	AkReal32 fCenter;

	AkPanningConversion( const BaseGenParams& in_rBaseParams )
	{
		//transform -100 +100 values to 0..1 float
		fX = ( in_rBaseParams.m_fPAN_X_2D + 100.0f ) * 0.005f;// /200
		fX = AkClamp( fX, 0.f, 1.f );

#if defined( AK_REARCHANNELS )
		fY = ( in_rBaseParams.m_fPAN_Y_2D + 100.0f ) * 0.005f;// /200
		fY = AkClamp( fY, 0.f, 1.f );
#else
		fY = 0.0f;
#endif
# if defined( AK_LFECENTER )
		fCenter = in_rBaseParams.m_fCenterPCT / 100.0f;
		/// No need to clamp fCenter because RTPC is not available for this property. fCenter = AkClamp( fCenter, 0.f, 1.f );
#else
		fCenter = 0.0f;
#endif

	}
};

struct Gen3DParams
{
public:
	Gen3DParams();
	~Gen3DParams();

public:
	AkUniqueID			m_ID;					// Id of the owner

	// Global members
	AkUniqueID			m_uAttenuationID;		// Attenuation short ID

	// Shared members
	AkReal32			m_fConeOutsideVolume;
	AkReal32			m_fConeLoPass;

	AkVector			m_Position;				// position parameters

	// Pre-defined specific params 
	AkPathMode			m_ePathMode;			// sequence/random & continuous/step
	AkTimeMs			m_TransitionTime;		// 

	// for the paths
	AkPathVertex*		m_pArrayVertex;			// the current path being used
	AkUInt32			m_ulNumVertices;		// how many vertices in m_pArrayVertex
	AkPathListItem*		m_pArrayPlaylist;		// the play list
	AkUInt32            m_ulNumPlaylistItem;	// own many paths in the play list

	AkUInt8				m_bIsSpatialized :1;	// Use spatialization
	AkUInt8				m_bIsConeEnabled:1;		// cone checkbox
	AkUInt8				m_bIsPanningFromRTPC:1;
	AkUInt8				m_bIsDynamic:1;			// set position continuously
	AkUInt8				m_bFollowOrientation:1;	// 3D user-defined, follow listener orientation
	AkUInt8				m_bIsLooping:1;			// 

friend class CAkPBI;
friend class CAkListener;

private:
	// Only CAkPBI and CAkListener are allowed to access the attenuation using GetAttenuation() function.
	//Inlined
	inline CAkAttenuation *	GetAttenuation()
	{
		if( !m_pAttenuation )
			m_pAttenuation = g_pIndex->m_idxAttenuations.GetPtrAndAddRef( m_uAttenuationID );

		return m_pAttenuation;
	}

private:
	// private member, refcounted pointer
	CAkAttenuation*		m_pAttenuation;
};


class CAkGen3DParams
{
public:
	//Constructor and destructor
	CAkGen3DParams();
	virtual ~CAkGen3DParams();

	void Term();

	// use the cone attenuation or not
	inline void SetConeUsage(
		bool in_bIsConeEnabled
		)
	{ m_Params.m_bIsConeEnabled = in_bIsConeEnabled; }

	// use the spatialization or not
	inline void SetSpatializationEnabled(
		bool in_bIsSpatializationEnabled
		)
	{ m_Params.m_bIsSpatialized = in_bIsSpatializationEnabled; }

	// Set the attenuation to use
	inline void SetAttenuationID(
		AkUniqueID in_AttenuationID
		)
	{ m_Params.m_uAttenuationID = in_AttenuationID; }

	// position related things
	inline void SetPositionFromPath(const AkVector & in_rPosition) { if ( !m_Params.m_bIsPanningFromRTPC ) m_Params.m_Position = in_rPosition; }

	/*Setting cone params*/
	inline void SetConeOutsideVolume( AkReal32 in_fOutsideVolume ) { m_Params.m_fConeOutsideVolume = in_fOutsideVolume; }
	inline void SetConeLPF( AkLPFType in_LPF ) { m_Params.m_fConeLoPass = in_LPF; }

	inline void SetIsPanningFromRTPC( bool in_bIsPanningFromRTPC ) { m_Params.m_bIsPanningFromRTPC = in_bIsPanningFromRTPC; }
 
	inline void SetIsPositionDynamic( bool in_bIsDynamic ) { m_Params.m_bIsDynamic = in_bIsDynamic; }
	inline void SetFollowOrientation( bool in_bFollow ) { m_Params.m_bFollowOrientation = in_bFollow; }

	inline void SetIsLooping( bool in_bIsLooping ) { m_Params.m_bIsLooping = in_bIsLooping; }
	void SetTransition( AkTimeMs in_TransitionTime );

	inline void SetPathMode( AkPathMode in_ePathMode ) { m_Params.m_ePathMode = in_ePathMode; }
	AKRESULT SetPathPlayList(
		CAkPath*		in_pPath,
		AkPathState*	in_pState
		);
	AKRESULT StartPath();
	AKRESULT StopPath();

	AKRESULT SetPath(
		AkPathVertex*           in_pArrayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		);

	void SetPathOwner(AkUniqueID in_PathOwner) { m_Params.m_ID = in_PathOwner; }

	AkUniqueID GetPathOwner() { return m_Params.m_ID; }

	AKRESULT UpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		);

	inline Gen3DParams * GetParams() { return &m_Params; }

#ifndef AK_OPTIMIZED
	void InvalidatePaths();
#endif

protected:
	Gen3DParams			m_Params;

private:

	void				UpdateTransitionTimeInVertex();
	void				ClearPaths();
};

struct AkPathState
{
	AkForceInline AkPathState()
		:ulCurrentListIndex(0)
		,pbPlayed(NULL)
	{}
	AkUInt32	ulCurrentListIndex;
	bool*		pbPlayed;
};

class CAkGen3DParamsEx : public CAkGen3DParams
{
public:
	virtual ~CAkGen3DParamsEx();

	void FreePathInfo();

	AkPathState	m_PathState;
};

#endif //_GEN_3D_PARAMS_H_
