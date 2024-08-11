/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/
#pragma once


enum EFrameUpdateState
{
	FUS_AlreadyUpdated,				// UpdateOncePerFrame was already called in this frame.
	FUS_NotUpdatedLastFrame,		// First update in this frame, and did not get an update in the previous frame.
	FUS_UpdatedLastFrame			// First update in this frame, and was updated in the previous frame.
};


class SFrameTracker
{

	//!< Last frame id this object was updated
	Red::Threads::CAtomic< Int32 >				m_updateFrameIndex;						

public:

	RED_FORCE_INLINE SFrameTracker() : m_updateFrameIndex(0) {}

	RED_FORCE_INLINE ~SFrameTracker() {}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	//! Get last successful update frame index
	RED_INLINE const Uint32 GetLastUpdateFrameIndex() const { return m_updateFrameIndex.GetValue(); }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	/// Chain for update function that should happen only once per frame
	const EFrameUpdateState UpdateOncePerFrame( Int32 frameIndex );

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	// Sets first frame (usually use it with AttachOnScene )
	void SetFrameIndex( Int32 frameIndex );

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
};

// Many systems need to be pulled/ticked one more time even if the current frame tend
// to be paused/disabled. This struct allows to execute task one more time even if 
// current frame ask for disable feature.
struct SCoherentFrameTracker
{

	Bool						m_executedLastFrame;

	SCoherentFrameTracker()
		: m_executedLastFrame( false )
	{}

	RED_INLINE Bool Check( const Bool isExecutingThisFrame )
	{
		const Bool allowExecute = isExecutingThisFrame || m_executedLastFrame;
		m_executedLastFrame = isExecutingThisFrame;
		return allowExecute;
	}

};
