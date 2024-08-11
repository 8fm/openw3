/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "fxBase.h"
#include "fxState.h"
#include "fxTrack.h"
#include "fxDefinition.h"

class CurveParameter;

/// Track item playing context
class IFXTrackItemPlayData
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

protected:
	CFXTrackItem*				m_trackItem;		//!< Related track item
	CNode*						m_node;				//!< Node we are playing on	

public:
	//! Get related node
	RED_INLINE CNode* GetNode() const { return m_node; }

	//! Get related track item
	RED_INLINE CFXTrackItem* GetTrackItem() const { return m_trackItem; }

public:
	IFXTrackItemPlayData( CNode* node, const CFXTrackItem* trackItem );

	//! Destructor, called when track item effect should be killed
	virtual ~IFXTrackItemPlayData();

	//! Called every frame while the effect is active
	virtual void OnTick( const CFXState& fxState, Float timeDelta )=0;

	//! Called when effect enters into the stopping phase
	virtual void OnStop() = 0;

	//! Called when entity is partially unstreamed, causing certain components to be destroyed
	virtual void OnPreComponentStreamOut( CComponent* component ) {}

	//! Get effect entity for effects spawned as a separate entity
	virtual CEntity* GetEffectEntity() { return NULL; }

#ifndef NO_DEBUG_PAGES
	virtual void CollectBoundingBoxes( Box& box ) const {}
	virtual void GetDescription( TDynArray< String >& descriptionLines ) const {}
	virtual Bool ValidateOptimization( TDynArray< String >* commentLines ) const { return true; }
#endif
};

/// Element in FX track
class CFXTrackItem : public CFXBase
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CFXTrackItem, CFXBase );

protected:
	Float   m_timeBegin;			//!< Start of the item
	Float   m_timeDuration;			//!< Duration of effect item

public:
	//! Get the time of the item start
	RED_INLINE Float GetTimeBegin() const { return m_timeBegin; }

	//! Get the time of the end of this track item
	RED_INLINE Float GetTimeEnd() const { return m_timeBegin + m_timeDuration; }

	//! Get the item duration
	RED_INLINE Float GetTimeDuration() const { return m_timeDuration; }

	//! Get the track we are in
	RED_INLINE CFXTrack* GetTrack() const { return SafeCast< CFXTrack >( GetParent() ); }

public:
	struct CompareFunc
	{
		static RED_INLINE Bool Less( CFXTrackItem* key1, CFXTrackItem* key2 )
		{
			return key1->GetTimeBegin() <= key2->GetTimeBegin();
		}	
	};

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const = 0;
	
	virtual void PrefetchResources(TDynArray< TSoftHandle< CResource > >& requiredResources) const { /* left intentionally empty */ }

	//! Does this effect support curved value edit
	virtual Bool SupportsCurve() { return false; }

	//! Does this effect support color value edit
	virtual Bool SupportsColor() { return false; }

	//! Get editable curve parameter
	virtual CurveParameter *GetCurveParameter() { return NULL; }

	//! Get editable curve
	virtual CCurve *GetCurve( Uint32 i = 0 ) { return NULL; }

	//! Get the number of curves
	virtual Uint32 GetCurvesCount() const { return 1; }

	//! Internal effect data has changed
	virtual void DataChanged() {}

	//! Get track item name
	virtual String GetName() const = 0;

	//! Get curve name
	virtual String GetCurveName( Uint32 i = 0 ) const { return String::EMPTY; }

	//! Is this a zero-time track item
	virtual Bool IsTick() { return false; }

	//! Change the time this track item starts without moving the end point
	virtual void SetTimeBeginWithConstEnd( Float timeBegin );

	//! Move the effect to different starting time
	virtual void SetTimeBeginWithConstDuration( Float timeBegin );

	//! Change the time this effect ends
	virtual void SetTimeEnd( Float timeEnd );

	//! Change duration of the effect
	virtual void SetTimeDuration( Float timeDuration );

	//! Remove this item from the track
	virtual void Remove();

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const;

public:
	CFXTrackItem();

private:
	//! Update duration of an track item
	void UpdateDuration( Float timeEnd );
};

BEGIN_ABSTRACT_CLASS_RTTI( CFXTrackItem );
	PARENT_CLASS( CFXBase );
	PROPERTY_EDIT( m_timeBegin, TXT("Time begin") );
	PROPERTY_EDIT( m_timeDuration, TXT("Time end") );
END_CLASS_RTTI();
