
#include "RumbleBusBase.h"

#ifdef AK_WII
#include <revolution/wpad.h>
#endif

#ifdef AK_WIIU
#include <cafe/pads/wpad/wpad.h>
#define WPAD_STATE_SETUP WPAD_STATE_ENABLED
#endif

class RumbleMixBus : public RumbleBusBase
{
public:
	RumbleMixBus();
	AKRESULT Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice );
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	AKRESULT RenderData();
	AkChannelMask GetMixingFormat() {return AK_SPEAKER_SETUP_MONO;}

	bool IsActive();

protected:

	bool SendSample();	

	AkUInt32			m_oCurrent;		//Current motor speeds
	AkInt8				m_iStopping;	//Make sure we send 5 stops on the Wii.  Sometimes the Wiimote communication fails without us knowing.
	bool				m_bIsWiimote ;	//Is this a Wiimote or a DRC
	AkReal32			m_fVPADRequired;
	AkUInt32			m_uVPADProduced;
};