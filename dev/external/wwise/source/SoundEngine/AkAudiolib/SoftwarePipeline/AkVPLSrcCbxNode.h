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
// AkVPLSrcCbxNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_SRC_CBX_NODE_H_
#define _AK_VPL_SRC_CBX_NODE_H_

#include "AkPBI.h"
#include "AkSrcLpFilter.h"
#include "AkResampler.h"
#include "AkVPLNode.h"
#include "AkVPLLPFNode.h"
#include "AkVPLPitchNode.h"
#include "Ak3DParams.h"
#include "AkVPLSrcNode.h"
#include "AkVPL.h"
#include "AkCommon.h"

#define MAX_NUM_SOURCES			2			// Max no. of sources in sample accurate container. 

#define MIMINUM_SOURCE_STARVATION_DELAY		(20)	//~400ms hysteresis on starvation logs.

class CAkVPLNode;
class CAkVPLFilterNodeBase;
class CAkVPLSrcCbxNode;
class AkHdrBus;

struct AkVPLSrcCbxRec
{
	AkVPLSrcCbxRec( CAkVPLSrcCbxNode * in_pCbxNode );
	void ClearVPL();

	CAkVPLPitchNode 			m_Pitch;
	CAkVPLLPFNode 				m_LPF;
	CAkVPLFilterNodeBase *		m_pFilter[AK_NUM_EFFECTS_PER_OBJ];
	
	AkForceInline CAkVPLLPFNode * Head() { return &m_LPF; }
};


// Contains everything that is common to a software- and hardware- based cbx node.
class CAkVPLSrcCbxNodeBase
{
	friend class AkVPL;
	friend class CAkLEngine;
	friend class CAkLEngineHw;

public:
	CAkVPLSrcCbxNodeBase * pNextItem; // for AkListVPLSrcs

#ifndef AK_OPTIMIZED
	AkUniqueID m_PipelineID; // Profiling id.
#endif

	CAkVPLSrcCbxNodeBase();
	virtual ~CAkVPLSrcCbxNodeBase();

	virtual void Term() = 0;
	void Start();
	virtual void Stop() = 0;
	virtual void Pause();
	virtual void Resume() = 0;
	virtual void StopLooping( CAkPBI * in_pCtx ) = 0;
	virtual void SeekSource() = 0;
	virtual bool IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot ) = 0;
	virtual bool IsUsingThisSlot( const AkUInt8* in_pData ) = 0;
	AKRESULT FetchStreamedData( CAkPBI * in_pCtx );

	virtual void UpdateFx( AkUInt32 in_uFXIndex ) = 0;

	virtual void SetFxBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ 
		) = 0;

	virtual void RefreshBypassFx() = 0;
	virtual void RefreshBypassFx( AkUInt32 in_uFXIndex ) = 0;
	
	inline CAkPBI * GetContext()
	{				
		if( m_pSources[ 0 ] )
			return m_pSources[ 0 ]->GetContext();
		return NULL;
	}

	inline VPLNodeState GetState() { return m_eState; }

	inline void	HandleSourceStarvation()
	{
		if ( !m_pSources[ 0 ]->IsPreBuffering() )
		{
			m_bHasStarved = true;
#ifndef AK_OPTIMIZED
			if ( !m_iWasStarvationSignaled ) 
			{
				m_pSources[0]->NotifySourceStarvation();
				m_iWasStarvationSignaled = MIMINUM_SOURCE_STARVATION_DELAY;
			}
#endif
		}
	}

	// For monitoring.
	inline bool			IsVirtualVoice() const { return !m_bAudible; }
#ifndef AK_OPTIMIZED
	inline AkReal32		BaseVolume() { return m_fBehavioralVolume; }	// Linear
	inline AkReal32		GetHdrWindowTop() { return m_fWindowTop; }	// dB
	inline AkReal32		GetEnvelope() { return m_fLastEnvelope; }
	inline AkReal32		GetNormalizationMonitoring() { return AkMath::FastLinTodB( m_fNormalizationGain ); }
	inline const AkSpeakerVolumes & LastVolumes( AkOutputDeviceID in_uDeviceID, AkUInt32 in_uChannel )
	{
		AkDeviceInfo* pDevice = m_OutputDevices.GetVolumesByID( in_uDeviceID );
		if ( pDevice )
		{
			return pDevice->mxDirect[in_uChannel].Previous.volumes;
		}
		else
		{
			static const AkSpeakerVolumes zeros = { 0, };
			return zeros;
		}
	}
#endif

	virtual void RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia ){ }

	CAkVPLSrcNode**		GetSources(){ return m_pSources; }

	AKRESULT			AddSrc( CAkPBI * in_pCtx, bool in_bActive );	
	AKRESULT			AddSrc( CAkVPLSrcNode * in_pSrc, bool in_bActive, bool in_bFirstTime );	


	// Compute volumes of all emitter-listener pairs for this sound. 
	// Returns true if routed to aux (and not virtual).
	bool ComputeVolumeRays();
	
	// Compute max volume of all paths (auxiliary and dry), taking pre-computed
	// bus gains into account.
	void ComputeMaxVolume();

	// Aux send values.
	inline AkMergedEnvironmentValue * GetSendValues( AkUInt8 & out_uNumSends ) { out_uNumSends = m_uNumSends; return m_arSendValues; }
	inline AkUInt32 GetNumSends() { return m_uNumSends; }
	inline void CleanupAuxBusses()
	{
		for(AkUInt32 i = 0; i < m_uNumSends; i++)
			m_arSendValues[i].PerDeviceAuxBusses.Term();
	}

#ifdef AK_PS3
	inline const AkMergedEnvironmentValue * GetSendValue( AkInt32 in_iEnv ) { return ( ( in_iEnv < m_uNumSends ) ? &m_arSendValues[in_iEnv] : NULL ); }
#endif
	inline void ForceNumSends( AkUInt8 in_uNumSends ) { m_uNumSends = in_uNumSends; }	// force smaller num sends in case of error.

	// Returns the number of devices to which this sound is routed.
	inline AkUInt32 GetNumDevices() { return m_OutputDevices.GetNumDevices(); }

	// Get speaker matrices.
	inline AkDeviceInfo * GetDeviceInfo( AkOutputDeviceID in_uDeviceId ) { return m_OutputDevices.GetVolumesByID( in_uDeviceId ); }

	inline void SetHdrBus( AkHdrBus * in_pHdrBus ) { m_pHdrBus = in_pHdrBus; }

	// Add output bus: if it fails the voice needs to remain in a consistent state. It will just not output sound in this graph.
	void AddOutputBus(AkVPL *in_pVPL, AkOutputDeviceID in_uDeviceID, bool bCrossDeviceSend);

	inline AkDeviceInfoList::IteratorEx RemoveOutputBus(AkDeviceInfoList::IteratorEx in_itBus) 
	{		
		m_bDeviceChange = true;		

		for(AkUInt32 i = 0; i < m_uNumSends; i++)
		{
			for(AkDeviceVPLArray::Iterator it = m_arSendValues[i].PerDeviceAuxBusses.Begin(); it != m_arSendValues[i].PerDeviceAuxBusses.End();/**/)
			{
				if ((*in_itBus)->pMixBus == (*it).item)
					it = m_arSendValues[i].PerDeviceAuxBusses.EraseSwap(it);
				else
					++it;
			}

			if (m_arSendValues[i].PerDeviceAuxBusses.IsEmpty())
			{
				//No more send.  Remove from array.
				m_arSendValues[i] = m_arSendValues[m_uNumSends-1];
				m_arSendValues[m_uNumSends-1].auxBusID = 0;
				m_uNumSends--;
				i--;
			}		
		}

		return m_OutputDevices.Remove(in_itBus);
	}

	inline AkVPL* GetOutputBus(AkUInt32 in_uDevice) {return m_OutputDevices.GetNumDevices() > 0 ? m_OutputDevices.GetVolumesByID(in_uDevice)->pMixBus : NULL;}
	inline AkDeviceInfoList::Iterator BeginBus(){return m_OutputDevices.Begin();}
	inline AkDeviceInfoList::IteratorEx BeginBusEx(){return m_OutputDevices.BeginEx();}
	inline AkDeviceInfoList::Iterator EndBus(){return m_OutputDevices.End();}
	inline bool HasOutputDevice() { return m_OutputDevices.HasOutputDevice(); }

protected:

	// Overridable methods (SW vs HW pipelines).
	//
	virtual AkReal32 GetAnalyzedEnvelope() = 0;

	// Helpers.
	// 
	void SetAudible( CAkPBI * in_pCtx, bool in_bAudible );

	// Evaluate max volume against threshold and compute complete set of 
	// speaker volumes if needed.
	void GetVolumes( 
		bool in_bIsAuxRoutable, 
		CAkPBI* AK_RESTRICT in_pContext, 
		AkChannelMask in_uInputConfig,
		bool & out_bNextSilent,		// True if sound is under threshold at the end of the frame
		bool & out_bAudible,		// False if sound is under threshold for whole frame: then it may be stopped or processsed as virtual.
		AkReal32 &out_fLPF,
		AkReal32 &out_fObsLPF
		);

	// Compute per-speaker volume distribution for 2D positioning.
	void ComputeSpeakerMatrix2D(
		bool in_bIsAuxRoutable, 
		CAkPBI*	AK_RESTRICT	in_pContext,
		const AkVolumeDataArray & in_arVolumeData,
		AkChannelMask in_uInputConfig,
		AkReal32 in_fBehavioralVolume	// Collapsed volumes of actor and voice bus hierarchies, and fades.
		);

	inline AkHdrBus * GetHdrBus() { return m_pHdrBus; }

	CAkVPLSrcNode *		m_pSources[MAX_NUM_SOURCES];	// [0] == Current, [1] == Next

	AkHdrBus *			m_pHdrBus;

	// Array of emitter-listener rays, containing listener-dependent linear volumes and optionally, angles.
	AkVolumeDataArray	m_arVolumeData;
	AkReal32			m_fBehavioralVolume;		// listener-independent volume (common to all rays), linear.
	AkReal32			m_fMaxVolumeDB;				// Max absolute nominal volume of this voice among all signal paths, in dB. Used with HDR only.
#ifndef AK_OPTIMIZED
	AkReal32			m_fWindowTop;				// Window top for this voice in dB (used for monitoring).
	AkReal32			m_fLastEnvelope;			// Last envelope value, in dB, for monitoring.
	AkReal32			m_fNormalizationGain;		// Last normalization gain, linear, for monitoring.
#endif

	VPLNodeState m_eState;

	//Not uselessly initialized.
	AkVirtualQueueBehavior m_eVirtualBehavior;
	AkBelowThresholdBehavior m_eBelowThresholdBehavior;
	// Volume status. 
	// Note: Meanings of m_bAudible and m_bPreviousSilent differ:
	// m_bPreviousSilent is true when the punctual volume value between the previous and the current audio 
	// frame is below threshold. It is used for low-level volume fades control, and also to compute m_bAudible.
	// m_bAudible is false when the sound was completely silent for the entire previous audio frame;
	// The voice's state (stopping, pausing, virtual) depends on it.
	AkUInt8				m_bAudible:1;					// For virtual voices
	AkUInt8				m_bPreviousSilent:1;			// Volumes on previous edge were silent -- NOT equivalent to m_bAudible
	AkUInt8				m_bFirstBufferProcessed:1;
	AkUInt8				m_bFirstSendsCalculated:1;
	AkUInt8				m_bIsAuxRoutable:1;

	AkUInt8				m_bHasStarved:1;
	AkUInt8				m_bDeviceChange:1;

	CAkOutputDevices	m_OutputDevices;
	
	AkMergedEnvironmentValue m_arSendValues[AK_MAX_AUX_SUPPORTED];	
	AkUInt8				m_uNumSends;	// Actual number of valid sends	

#ifndef AK_OPTIMIZED
	AkUInt8				m_iWasStarvationSignaled;// must signal when == 0, reset to MIMINUM_SOURCE_STARVATION_DELAY.
#endif
};

// software-based cbx node.
class CAkVPLSrcCbxNode
	: public CAkVPLSrcCbxNodeBase
{
public:
	friend class AkVPL;
	friend class CAkLEngine;
	friend class CAkVPLPitchNode;

	CAkVPLSrcCbxNode();

	void				Init( AkUInt32 in_uSampleRate );
	void				Term();
	void				Stop();
	void				Resume();
	void				StopLooping( CAkPBI * in_pCtx );
	void				SeekSource();

	bool				StartRun( AkVPLState & io_state );
	void				ConsumeBuffer( AkVPLState & io_state );
	void				ReleaseBuffer();
	AKRESULT			AddPipeline();
	void				SwitchToNextSrc();

	void				UpdateFx( AkUInt32 in_uFXIndex );

	void SetFxBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ 
		);

	void RefreshBypassFx();
	void RefreshBypassFx( AkUInt32 in_uFXIndex );

	inline AkUInt32		GetSampleRate() { return m_uSampleRate; }

	// Environmental helper methods
	AkForceInline void ConsumeBufferForObstruction( AkVPLState & io_state ) { m_ObstructionLPF.ConsumeBuffer( io_state ); }
	AkForceInline void ProcessDoneForObstruction( AkVPLState & io_state ) { m_ObstructionLPF.ProcessDone( io_state ); }

	bool				PipelineAdded() { return m_cbxRec.m_Pitch.IsInitialized(); }

	bool				IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot );
	bool				IsUsingThisSlot( const AkUInt8* in_pData );

	void				RemovePipeline( AkCtxDestroyReason in_eReason );

	AkForceInline AkReal32		LastLPF() const { return m_cbxRec.m_LPF.GetLPF(); }
	AkForceInline AkReal32		LastObsLPF() const { return m_ObstructionLPF.GetLPF(); }

	virtual void RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia );

	AkChannelMask GetOutputChannelMask(){return m_uChannelMask;}

protected:
	virtual AkReal32 GetAnalyzedEnvelope()
	{
		return m_pSources[ 0 ]->GetAnalyzedEnvelope( m_cbxRec.m_Pitch.GetNumBufferedInputSamples() );
	}	

private:
	AKRESULT			SourceTimeSkip( AkUInt32 in_uFrames );
	AKRESULT			SetVirtual( bool in_fMode );
	void				RestorePreviousVolumes( AkPipelineBuffer* AK_RESTRICT io_pBuffer );
	
private:
	AkUInt32			m_uSampleRate;
	AkChannelMask		m_uChannelMask;
	AkVPLSrcCbxRec		m_cbxRec;
	CAkVPLLPFNode		m_ObstructionLPF;		    // LPF node for obstruction.
};

#endif //_AK_VPL_SRC_CBX_NODE_H_
