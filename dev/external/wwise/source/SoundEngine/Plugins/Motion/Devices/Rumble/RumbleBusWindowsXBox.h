
#include "IAkRumbleController.h"
#include "RumbleDeviceHelper.h"
#include "RumbleBusBase.h"

#undef CHANNEL_COUNT
#define CHANNEL_COUNT 4

#undef BUFFER_SIZE
#define BUFFER_SIZE (BUFFER_COUNT * CHANNEL_COUNT * SAMPLE_COUNT)

class RumbleMixBus : public RumbleBusBase
{

public:
	RumbleMixBus();
	AKRESULT Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice );
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	bool IsActive();

protected:

	bool SendSample();
	
	struct MotorPower
	{
		AkReal32			m_fSmall;
		AkReal32			m_fLarge;
	};
	MotorPower				m_oCurrent;		//Current motor speeds
	IAkRumbleController *	m_pRumbleDevice;
};