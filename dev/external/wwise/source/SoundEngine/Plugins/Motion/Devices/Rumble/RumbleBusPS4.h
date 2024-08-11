
#include "IAkRumbleController.h"
#include "RumbleBusBase.h"
#include "pad.h"

class RumbleMixBus : public RumbleBusBase
{

public:
	RumbleMixBus();
	AKRESULT Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice );
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	bool IsActive();

protected:

	bool SendSample();
	ScePadVibrationParam*	m_oCurrent;		//Current motor speeds
	int32_t					m_handle;
};