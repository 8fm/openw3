/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once


/// Render EnvProbe dynamic data (envProbe manager's operational data)
struct SRenderEnvProbeDynamicData
{
public:
	SRenderEnvProbeDynamicData ();
	~SRenderEnvProbeDynamicData ();

	/// Is env probe ready to be shown
	Bool IsReadyToDisplay( const class CRenderEnvProbeManager &manager ) const;

	/// Called when envProbe get's detached from cube array
	void DiscardArraySlot();

public:
	Float					m_lastUpdateTime;
	Float					m_lastFullUpdateStartUpdateTime;
	Float					m_lastFullUpdateFinishUpdateTime;
	Int32					m_arraySlotIndex;
	Bool					m_isArraySlotInit;
	Bool					m_isFaceTexturesInit;
	Bool					m_isFaceTexturesUnpacked;
	Bool					m_prefetchFailed;
	Float					m_failereRecoveryTimeStamp;
};
