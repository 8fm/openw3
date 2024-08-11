#ifndef AK_DEVICE_MGR_H
#define AK_DEVICE_MGR_H

#include "AkSink.h"
#include "AkSpeakerPan.h"
#include <AK/Tools/Common/AkArray.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

class CAkVPLFinalMixNode;

class AkDevice
{
public:	
	AkDevice()
	{
		memset(this, 0, sizeof(AkDevice));
	}
	~AkDevice() {Cleanup();}

	//Transfer ownership
	AkDevice & operator=(AkDevice& B)
	{
		Cleanup();
		pFinalMix = B.pFinalMix;
		pSink = B.pSink;
		pUserData = B.pUserData;
		uDeviceID = B.uDeviceID;
		uListeners = B.uListeners;
		puSpeakerAngles = B.puSpeakerAngles;
		uNumAngles = B.uNumAngles;
		fOneOverMinAngleBetweenSpeakers = B.fOneOverMinAngleBetweenSpeakers;
		ePanningRule = B.ePanningRule;
		m_mapConfig2PanPlane.Transfer(B.m_mapConfig2PanPlane);

		B.pFinalMix = NULL;
		B.pSink = NULL;
		B.pUserData = NULL;
		B.puSpeakerAngles = NULL;
		B.uNumAngles = 0;
		return *this;
	}

	void Cleanup();	

	void PushData();

	//
	// Positioning.
	// 
	AKRESULT SetSpeakerAngles( 
		const AkReal32 *	in_pfSpeakerAngles,	// Array of loudspeaker pair angles, expressed in degrees relative to azimuth ([0,180]).
		AkUInt32			in_uNumAngles		// Number of loudspeaker pair angles.
		);

	inline CAkSpeakerPan::PanPair * GetPanTable( 
		AkChannelMask	in_uOutputConfig	// config of bus to which this signal is routed.
		)
	{
		AKASSERT( ( in_uOutputConfig & AK_SPEAKER_LOW_FREQUENCY ) == 0 );

		CAkSpeakerPan::MapConfig2PanPlane::Iterator it = m_mapConfig2PanPlane.FindEx( in_uOutputConfig );
		AKASSERT( it != m_mapConfig2PanPlane.End() );	// Must have been created when voice connected to device.
		return (*it).item;
	}
	
	AKRESULT CreatePanCache( 
		AkChannelMask	in_uOutputConfig	// config of bus to which this signal is routed.
		);

	inline AKRESULT EnsurePanCacheExists( 
		AkChannelMask	in_uOutputConfig	// config of bus to which this signal is routed.
		)
	{
#ifdef AK_LFECENTER
		// Ignore LFE.
		in_uOutputConfig = in_uOutputConfig & ~AK_SPEAKER_LOW_FREQUENCY;
#endif
		AKRESULT eResult = AK_Success;
		if ( !m_mapConfig2PanPlane.Exists( in_uOutputConfig ) )
			eResult = CreatePanCache( in_uOutputConfig );	// Does not exist. Create cache now.

#ifdef AK_LFECENTER
		if ( in_uOutputConfig & AK_SPEAKER_FRONT_CENTER 
			&& eResult == AK_Success )
		{
			// There is a center channel. Need to allocate a non-center config too.
			in_uOutputConfig = in_uOutputConfig & ~AK_SPEAKER_FRONT_CENTER;
			if ( !m_mapConfig2PanPlane.Exists( in_uOutputConfig ) )
				eResult = CreatePanCache( in_uOutputConfig );	// Does not exist. Create cache now.
		}
#endif
		return eResult;
	}
	//

	CAkVPLFinalMixNode *pFinalMix;
	CAkSink*			pSink;
	void *				pUserData;
	AkOutputDeviceID	uDeviceID;		//Device ID
	AkUInt32			uListeners;		//Listener mask.  All these listeners output on this device.

	// 3D positioning.
	AkUInt32 *			puSpeakerAngles;	// Angles in uint, [0,PI] -> [0,PAN_CIRCLE/2]
	AkUInt32			uNumAngles;
	AkReal32			fOneOverMinAngleBetweenSpeakers;	// 1 / (uMinAngleBetweenSpeakers)
	AkPanningRule		ePanningRule;
	CAkSpeakerPan::MapConfig2PanPlane m_mapConfig2PanPlane;
};

typedef AkArray<AkDevice, AkDevice&, ArrayPoolLEngineDefault> AkDeviceArray;

#define AK_MAKE_DEVICE_KEY(_type, _id) (((AkUInt64)_id << 32) | _type)
#define AK_GET_SINK_TYPE_FROM_DEVICE_KEY(_key) ((AkUInt32)(_key & 0xffffffff))
#define AK_GET_DEVICE_ID_FROM_DEVICE_KEY(_key) ((AkUInt32)(_key >> 32))
#define AK_MAIN_OUTPUT_DEVICE 0		// Must be consistent with AkSink_Main.

class CAkOutputMgr
{
public:
	static void Term();
	static AKRESULT AddMainDevice( AkOutputSettings & in_outputSettings, AkSinkType in_eSinkType, AkUInt32 in_uListeners, void *in_pUserData);
	static AKRESULT AddOutputDevice( AkOutputSettings & in_outputSettings, AkSinkType in_eSinkType, AkUInt32 in_uDevice, AkUInt32 in_uListeners, void *in_pUserData);
	static AKRESULT RemoveOutputDevice( AkOutputDeviceID in_uDeviceID );
	static AKRESULT SetListenersOnDevice(AkUInt32 in_uListeners, AkOutputDeviceID in_uDeviceID);

	static AkForceInline AkDeviceArray::Iterator OutputBegin() {return m_Devices.Begin();}
	static AkForceInline AkDeviceArray::Iterator OutputEnd() {return m_Devices.End();}
	static AkForceInline AkDevice* GetDevice(AkOutputDeviceID in_uID)
	{
		for(AkUInt32 i = 0; i < m_Devices.Length(); i++)
		{
			if (m_Devices[i].uDeviceID == in_uID)
				return &m_Devices[i];
		}
		return NULL;
	}

	static AkForceInline AkUInt32 GetDeviceListeners(AkOutputDeviceID in_uID)
	{
		for(AkUInt32 i = 0; i < m_Devices.Length(); i++)
		{
			if (m_Devices[i].uDeviceID == in_uID)
				return m_Devices[i].uListeners;
		}
		return 0;
	}

	static AKRESULT ReplaceSink(AkOutputDeviceID in_uDeviceID, CAkSink* in_pSink);
	static AkUInt32 Count() {return m_Devices.Length();}

	static void StartOutputCapture(const AkOSChar* in_CaptureFileName);
	static void StopOutputCapture();

private:
	static AKRESULT _AddOutputDevice( AkOutputDeviceID in_uKey, AkOutputSettings & in_outputSettings, AkSinkType in_eSinkType, AkUInt32 in_uDevice, AkUInt32 in_uListeners, void *in_pUserData);
	static AkArray<AkDevice, AkDevice&, ArrayPoolLEngineDefault> m_Devices;
};
#endif
