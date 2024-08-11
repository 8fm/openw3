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
// AkAttenuationMgr.h
//
// creator : alessard
//
//////////////////////////////////////////////////////////////////////
#ifndef _ATTENUATION_MGR_H_
#define _ATTENUATION_MGR_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkKeyArray.h"
#include "AkConversionTable.h"
#include "AkIndexable.h"
#include "AkAudioLibIndex.h"
#include "Ak3DParams.h"

class CAkPBI;

#define AK_MAX_NUM_ATTENUATION_CURVE 5 // TODO : Add a shared file between WWise define and SE define, so they don't get duplicated.

enum AttenuationCurveID
{
	AttenuationCurveID_VolumeDry	= 0,
	AttenuationCurveID_VolumeAuxGameDef	= 1,
	AttenuationCurveID_VolumeAuxUserDef	= 2,
	AttenuationCurveID_LowPassFilter= 3,
	AttenuationCurveID_Spread		= 4,
	
	AttenuationCurveID_None			= 255// was -1, but PS3 was complaining
};

struct AkWwiseConeAttenuation
{
	AkUInt8				bIsConeEnabled;		//as bool
	AkReal32			cone_fInsideAngle;
	AkReal32			cone_fOutsideAngle;
	AkReal32			cone_fOutsideVolume;
	AkLPFType			cone_LoPass;
};

struct AkWwiseGraphCurve
{
	AkCurveScaling m_eScaling;
	AkUInt32 m_ulConversionArraySize;
	AkRTPCGraphPoint* m_pArrayConversion;
};

struct AkWwiseRTPCreg : public AkWwiseGraphCurve
{
	AkPluginID m_FXID;
	AkRtpcID m_RTPCID;
	AkRTPC_ParameterID m_paramID;
	AkUniqueID m_RTPCCurveID;
};

struct AkWwiseAttenuation
{
	AkWwiseConeAttenuation	Cone;

	AkUInt8					CurveIndexes[ AK_MAX_NUM_ATTENUATION_CURVE ];
	AkUInt32				uNumCurves;
	AkWwiseGraphCurve*		paCurves;

	AkUInt32				uNumRTPCReg;
	AkWwiseRTPCreg*			paRTPCReg;
};

class CAkAttenuation : public CAkIndexable
{
public:
	struct RTPCSubs
	{
		AkRtpcID			RTPCID;
		AkRTPC_ParameterID	ParamID;		//# of the param that must be notified on change
		AkUniqueID			RTPCCurveID;
		CAkConversionTable<AkRTPCGraphPoint, AkReal32>	ConversionTable;
	};
	typedef AkArray<RTPCSubs, const RTPCSubs&, ArrayPoolDefault, 2> RTPCSubsArray;


	CAkAttenuation( AkUniqueID in_ulID ):CAkIndexable( in_ulID ){}
	virtual ~CAkAttenuation();

	//Thread safe version of the constructor
	static CAkAttenuation* Create( AkUniqueID in_ulID = 0 );

	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize );
	AKRESULT SetAttenuationParams( AkWwiseAttenuation& in_rParams );

	///////////////////////////
	// Internal use only
	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();
	///////////////////////////

	typedef CAkConversionTable<AkRTPCGraphPoint, AkReal32> AkAttenuationCurve;

	// If no curve is available for the selected index(none) the function returns NULL.
	AkAttenuationCurve* GetCurve( AkUInt8 in_parameterIndex )
	{
		AKASSERT( in_parameterIndex < AK_MAX_NUM_ATTENUATION_CURVE );

		// Get the curve selected based on the index
		AkUInt8 l_crveToUse = m_curveToUse[in_parameterIndex];

		return ( l_crveToUse != AttenuationCurveID_None && m_curves[l_crveToUse].m_pArrayGraphPoints ) ? &m_curves[l_crveToUse] : NULL;
	}

	AkForceInline const RTPCSubsArray &GetRTPCSubscriptionList() { return m_rtpcsubs; }

#ifndef AK_OPTIMIZED
	AKRESULT AddPBI( CAkPBI* in_pPBI );
	void RemovePBI( CAkPBI* in_pPBI );
#endif

	// public members
	ConeParams m_ConeParams;
	
	AkAttenuationCurve	m_curves[ AK_MAX_NUM_ATTENUATION_CURVE ];
	AkUInt8				m_curveToUse[ AK_MAX_NUM_ATTENUATION_CURVE ];
	AkUInt8				m_bIsConeEnabled :1;

protected:
	AKRESULT Init();

private:
	void ClearRTPCs();
#ifndef AK_OPTIMIZED
	void UpdatePBIs();
#endif

	// Adds the Attenuation in the General index
	void AddToIndex();

	// Removes the Attenuation from the General index
	void RemoveFromIndex();

	AKRESULT SetRTPC(
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,		// NULL if none
		AkUInt32					in_ulConversionArraySize	// 0 if none
		);

	void UnsetRTPC(
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID in_RTPCCurveID
		);

	RTPCSubsArray m_rtpcsubs;

#ifndef AK_OPTIMIZED
	typedef AkArray<CAkPBI*, CAkPBI*, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof( CAkPBI* )> PBIArray;
	PBIArray m_PBIList;
#endif
};

#endif // _ATTENUATION_MGR_H_
