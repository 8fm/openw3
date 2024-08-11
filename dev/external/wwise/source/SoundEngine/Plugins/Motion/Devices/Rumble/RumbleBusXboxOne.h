
#include "RumbleDeviceHelper.h"
#include "RumbleBusBase.h"

class RumbleMixBus : public RumbleBusBase
{

public:
	RumbleMixBus();
	AKRESULT Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice );

	bool IsActive();
	AkChannelMask GetMixingFormat() {return AK_SPEAKER_SETUP_4_0;}

protected:

	bool SendSample();
		
	AkReal32 m_fRightMotorLevel;
	AkReal32 m_fLeftMotorLevel;
	AkReal32 m_fRightTriggerLevel;
	AkReal32 m_fLeftTriggerLevel;
	AkUInt64 m_idDevice;
};
