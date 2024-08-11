#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/MotionEngine/Common/IAkMotionMixBus.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AkSettings.h>
#include "AkCaptureMgr.h"
#include "AkProfile.h"

#ifdef AK_XBOXONE
#define CHANNEL_COUNT 4
#else
#define CHANNEL_COUNT 2
#endif

#define SAMPLE_COUNT 2
#define BUFFER_COUNT 8
#define BUFFER_SIZE (BUFFER_COUNT * CHANNEL_COUNT * SAMPLE_COUNT)
#define BUFFER_MASK (BUFFER_SIZE - 1)

class RumbleBusBase : public IAkMotionMixBus
{
public:
	RumbleBusBase();

	//IAkPlugin
	/// Release the resources upon termination of the plug-in.
	/// \return AK_Success if successful, AK_Fail otherwise
	/// \aknote The self-destruction of the plug-in must be done using AK_PLUGIN_DELETE() macro. \endaknote
	/// \sa
	/// - \ref iakeffect_term
	virtual AKRESULT Term( 
		AK::IAkPluginMemAlloc * in_pAllocator 	///< AkInterface to memory allocator to be used by the plug-in
		) ;

	/// The reset action should perform any actions required to reinitialize the state of the plug-in 
	/// to its original state.
	/// \return AK_Success if successful, AK_Fail otherwise.
	/// \sa
	/// - \ref iakeffect_reset
	virtual AKRESULT Reset( ) ;

	/// Plug-in information query mechanism used when the sound engine requires information 
	/// about the plug-in to determine its behavior
	/// \return AK_Success if successful.
	/// \sa
	/// - \ref iakeffect_geteffectinfo
	virtual AKRESULT GetPluginInfo( 
		AkPluginInfo & out_rPluginInfo	///< Reference to the plug-in information structure to be retrieved
		) ;

	//IAkMotionMixBus
	virtual AKRESULT 	Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer, void * in_pDevice = NULL );

	virtual AKRESULT	MixAudioBuffer( AkAudioBuffer &io_rBuffer );
	virtual AKRESULT	MixFeedbackBuffer( AkAudioBuffer &io_rBuffer );
	virtual AKRESULT	RenderData();
	virtual void		CommandTick();
	virtual void		Stop(){};

	virtual AkReal32	GetPeak();
	virtual	bool		IsStarving();
	virtual bool		IsActive() = 0;
	virtual AkChannelMask GetMixingFormat();
	virtual void		SetMasterVolume(AkReal32 in_fVol);

	// Execute effect processing.
	virtual void Execute( 
		AkAudioBuffer * //io_pBufferOut // Output buffer interface
#ifdef AK_PS3
		, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	///< Asynchronous DSP process utilities on PS3
#endif 
		) {};

	virtual void		StartOutputCapture(const AkOSChar* in_CaptureFileName);
	virtual void		StopOutputCapture();

protected:

	virtual bool		SendSample() = 0;

	template <class T>
	AkForceInline bool PulseMod( AkReal32 in_fControl, T in_valLow, T in_valHigh, T & out_val );
	AkReal32			m_fAverageSpeed;	//For PulseMod

	AkCaptureFile*		m_pCapture;

	AkReal32			m_pData[BUFFER_SIZE];		//Mixing buffer
	AkReal32			m_fLastBufferPower;	//Last audio buffer value
	AkUInt16			m_usWriteBuffer;//Next buffer for mixing
	AkUInt16			m_usReadBuffer;	//Next buffer to send
	AkReal32			m_fPeak;		//Peak value
	AkReal32			m_fVolume;		//Master volume for this player.
	AkInt16				m_iDrift;		//Drift between the Render calls and the Tick calls
	AkUInt8				m_iPlayer;		//Player controller port number
	bool				m_bStopped;		//Are the motors stopped.
	bool				m_bGotData;		//Do we have anything to send

};

template <class T>
bool RumbleBusBase::PulseMod( AkReal32 in_fControl, T in_valLow, T in_valHigh, T & out_val )
{
	T iLastVal = out_val;
	//Try to maintain an average speed by weighting the ON and OFF states differently.
	//Below 15% the motor only clicks uselessly.  Rescale the control value so we can use the full range.
	if (in_fControl > 0.15f)
		in_fControl = in_fControl * 0.85f + 0.15f;

	if (m_fAverageSpeed < in_fControl && in_fControl > 0.15f)
	{
		//Below the desired speed, start the motor.
		m_fAverageSpeed += (1 - in_fControl);
		out_val = in_valHigh;
		if (m_fAverageSpeed > 1)
			m_fAverageSpeed = 1;
	}
	else
	{
		//Above the desired speed, stop the motor.
		m_fAverageSpeed -= in_fControl;
		if (m_fAverageSpeed < 0 || in_fControl == 0)
			m_fAverageSpeed = 0;

		out_val = in_valLow;
	}

	return iLastVal != out_val;
}
