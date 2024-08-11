/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once 

#include "budgetedTickContainer.h"

class CFXDefinition;
class CEntity;
class IFXTrackItemPlayData;
class EntitySpawnInfo;
class CTickManager;

/// Effect editor message 
struct SEditorEffectMessage
{
	const CFXDefinition*	m_definition;
	CEntity*				m_entity;
	CName					m_effectTag;
	Float					m_effectTime;
};

/// FX playing context
class CFXState : public ILODable
{
	friend class CTickManager;
	friend class CEffectManager;

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_EntityFxState );
	DEFINE_TICK_BUDGETED_LIST_ELEMENT( CFXState, tick );

protected:
	CName								m_tag;						//!< Effect tag
	Float								m_length;					//!< Length of the effect
	Float								m_currentTime;				//!< Current time
	Float								m_tickEffectStartTime;		//!< Current time
	Float								m_showDistanceSquared;		//!< Show distance of the effect squared for optimized calculations
	THandle< CFXDefinition >			m_fxDefinition;				//!< Effect definition
	CEntity*							m_entity;					//!< Entity we are playing the effect on
	THandle< class CNode >				m_target;					//!< Node being a particle emission target
	TDynArray< IFXTrackItemPlayData* >	m_activeItems;				//!< Active track items
	TDynArray< const CFXTrackItem* >    m_pendingTrackItems;		//!< Not active track items because some components were not streamed on time
	TDynArray< IFXTrackItemPlayData* >	m_playDataToRemove;
	TDynArray< const CFXTrackItem* >	m_trackItemsToActivate;
	CName								m_boneName;					//!< Name of the bone on which fire the effect
	CName								m_targetBone;				//!< Name of the target bone being a particle emission target
	Bool								m_isPreviewEffect	: 1;	//!< Effect was played from the preview window
	Bool								m_isPaused			: 1;	//!< Effect is paused
	Bool								m_isAlive			: 1;	//!< Effect is still alive
	Bool								m_isStopping		: 1;	//!< Effect is stopping, will not loop on end point
	Bool								m_isInitialized		: 1;	//!< Initialized
	Bool								m_stoppingNotified	: 1;	//!< Was Stop() notified?
	Bool								m_forAnim			: 1;	//!< Is effect for anim?
	Uint32													: 0;	//!< Padding
public:
	//! Get effect tag
	RED_INLINE CName GetTag() const { return m_tag; }

	//! Get current effect time
	RED_INLINE Float GetCurrentTime() const { return m_currentTime; }

	//! Get FX definition
	RED_INLINE const CFXDefinition* GetDefinition() const { return m_fxDefinition; }

	//! Get affected entity
	RED_INLINE CEntity* GetEntity() const { return m_entity; }

	//! Is effect initialized
	RED_INLINE Bool IsInitialized() const { return m_isInitialized; }

	//! Is the effect paused
	RED_INLINE Bool IsPaused() const { return m_isPaused; }

	//! Is this effect alive ?
	RED_INLINE Bool IsAlive() const { return m_isAlive; }

	//! Is this effect stopping ?
	RED_INLINE Bool IsStopping() const { return m_isStopping; }

	//! Is this effect notified about stopping ?
	RED_INLINE Bool IsStopNotified() const { return m_stoppingNotified; }

	//! Get name of the bone on which fire the effect
	RED_INLINE const CName& GetBoneName() const { return m_boneName; }

	//! Get target node
	RED_INLINE const CNode* GetTargetNode() const { return m_target.Get(); }

	RED_INLINE const CName GetTargetBone() const { return m_targetBone; }
public:
	CFXState( const CFXDefinition* fxDefinition, Float showDistanceSqr, CName tag, CEntity* entity,
		Float startTime, Bool isPreview, const CName& boneName = CName::NONE, const CNode* targetNode = NULL, const CName& targetBone = CName::NONE, Bool forAnim = false );
	~CFXState();

	//! Stop effect, can take some time
	void Stop();

	//! Pause effect
	void Pause( Bool pause );

	//! Cleans all the play data associated with this effect
	void Cleanup();

	//! Kill the effect now. Calls Cleanup() and removes the effect from effect manager & entity active effects struct.
	//! Must be the last method called on the FXState as it implements suicide pointer (deletes itself at the end of method)
	void Destroy();

	//! Update
	void Tick( Float timeDelta );

	//! Create dynamic entity related to this effect
	CEntity* CreateDynamicEntity( const EntitySpawnInfo& info );

	//! Fill with fx definition stuff
	void FillFxDefinition( CFXDefinition* definition );

	//! Stop effects that use specified component
	void OnComponentStreamOut( CComponent* component );

	//! Set effect parameter value for specified component (or all if set to CName::NONE) and parameter name
	void SetEffectsParameterValue( Float intensity, const CName &specificComponentName, CName effectIntensityParameterName );

protected:
	//! Collect active track play data withing given time range
	void CollectActivePlayDataToKill( Float timeStart, Float timeEnd, TDynArray< IFXTrackItemPlayData* >& data );

	//! Deletes play data. Used in situations where the effect is further than show distance.
	void ClearLoopedPlayData();

#ifndef NO_DEBUG_PAGES
public:
	//! DEBUG
	Box GetDebugBoundingBox() const;
	void GetPlayData( TDynArray< IFXTrackItemPlayData* >& playData ) const { playData = m_activeItems; }
#endif

	// ILODable
	void UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager ) override;
	ILODable::LOD ComputeLOD( CLODableManager* manager ) const override;

	RED_FORCE_INLINE Bool IsTickEnabled() const { return GetCurrentLOD() == ILODable::LOD_0; }
	RED_FORCE_INLINE Bool IsTickBudgeted() const { return GetCurrentLOD() == ILODable::LOD_1; }
	RED_FORCE_INLINE Bool IsTickDisabled() const { return GetCurrentLOD() == ILODable::LOD_2; }
};

class CEffectManager : public CLODableManager
{
public:
	CEffectManager();
	void AddEffect( CFXState* effect );
	void RemoveEffect( CFXState* effect );
};
