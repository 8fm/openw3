/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "math.h"
#include "renderHelpers.h"
#include "../../common/engine/renderGameplayEffects.h"

#define SURFACE_POST_GROUPS_MAX	(Uint32)16

// Order of this enums is used to process and render effects
enum EPostFXEffect
{
	EPE_SURFACE,
	EPE_CAT_VIEW,
	EPE_FOCUS_MODE,
	EPE_DRUNK,
	EPE_SEPIA,
	EPE_OUTLINE,

	EPE_MAX
};


// Render planes of post processes
enum EPostFXOrder
{
	EPO_POST_OPAQUE,		//!< Some effects need to rendered during regular opaque render stuff (frost etc).
	EPO_PRE_TONEMAPPING,	//!< Before the tonemapping.
	EPO_POST_TONEMAPPING,	//!< After the tonemapping.
	EPO_POST_BLOOM,			//!< Last pass. 

	EPO_MAX
};


//////////////////////////////////////////////////////////////////////////////////////////
// Effects gropus
//////////////////////////////////////////////////////////////////////////////////////////

enum ESurfacePostFXType
{
	ES_Frost,
	ES_Burn
};

struct SSurfacePostFXGroup
{			
	Vector		m_position;					// Where in the world apply
	Float		m_fadeInTime;				// How long frost will be appearing
	Float		m_fadeOutTime;				// How long frost will be fading out
	Float		m_activeTime;				// How long frost will be active
	Float		m_range;					// Floor only!
	ESurfacePostFXType		m_type;

	Float		m_currentLifeTime;			//		
	Float		m_factor;					//
	Bool		m_enabled;		
};	


//////////////////////////////////////////////////////////////////////////////////////////
// Effect base class
//////////////////////////////////////////////////////////////////////////////////////////

class IGameplayEffect
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	CGameplayEffects*	m_parent;

	Bool				m_isEnabled;
	Bool				m_needSwapBuffer;

public:

	EPostFXEffect		m_type;
	EPostFXOrder		m_order;
	Float				m_factor;

	IGameplayEffect( CGameplayEffects* parent, EPostFXEffect type , EPostFXOrder order = EPO_POST_TONEMAPPING )
		: m_type( type )
		, m_order( order )
		, m_parent( parent )
		, m_isEnabled( false )
		, m_factor( 0.0f )
	{
	}

	virtual ~IGameplayEffect() {}

	virtual void Tick( Float time ) = 0;
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void Reset(){};

	//! Apply effetct on the screen. If returning true - buffers will be swapped
	virtual Bool Apply( CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName rtnColorSource , ERenderTargetName rtnColorTarget ) = 0;

	RED_INLINE Bool IsEnabled(){ return m_isEnabled; }

	RED_INLINE void SetEnabled( Bool enabled ){ m_isEnabled = enabled ; }

	RED_INLINE	EPostFXEffect GetType() const { return m_type; }

	RED_INLINE	EPostFXOrder GetOrder() const { return m_order; }

	RED_INLINE	CGameplayEffects* GetParent() const { return m_parent; }

	RED_INLINE	Float GetStrength() const { return m_factor; }

	RED_INLINE	void SetStrength( float strength ) { m_factor = strength; };

};


//////////////////////////////////////////////////////////////////////////////////////////
// Gameplay Effects Manager
//////////////////////////////////////////////////////////////////////////////////////////

class CGameplayEffects
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	Float					m_lastGameTime;
	Bool					m_isInitialized;

	IGameplayEffect*		m_effects[ EPE_MAX ];

public:
	CGameplayEffects();
	~CGameplayEffects();

	// Init/Shutdown called from main thread.
	void Init();
	void Shutdown();

	void SetGameTime( Float time );
	
	void AddEffect( IGameplayEffect* effect );

public:

	RED_INLINE IGameplayEffect* GetEffect( EPostFXEffect effect ) const { RED_ASSERT( effect < EPE_MAX ); return m_effects[ effect ]; }
	
	RED_INLINE Bool	IsEnabled( EPostFXEffect effect ) const { RED_ASSERT( effect < EPE_MAX ); return m_effects[ effect ] ? m_effects[ effect ]->IsEnabled() : false; }

	//! Insane level - over 9000
	Vector	GetParametersVector() const;

	//! Apply all effects on given order plane. It swaps color targets
	void Apply( EPostFXOrder order, CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName& rtnColorSource , ERenderTargetName&rtnColorTarget );

	//! Disable all effect that are attached to this manager
	void DisableAll();

};

