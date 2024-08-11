
#include <cell/pad.h>
#include "RumbleBusBase.h"

class RumbleMixBus : public RumbleBusBase
{
public:
	RumbleMixBus();
	AKRESULT Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice );
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	bool IsActive();

protected:

	bool SendSample();

	CellPadActParam		m_oCurrent;		//Current motor speeds

	//The PS3 controller stops vibrating after 3 seconds if no new samples are sent.
	AkUInt32			m_iRefreshCount;//Samples elapsed since the last refresh.
	bool				m_bIsGem;//A "Move" controller is connected.
};