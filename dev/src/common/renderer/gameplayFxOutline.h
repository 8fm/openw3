
#pragma once

#include "gameplayFx.h"

struct SOutlineGroup
{
	CName m_name;
	Vector m_fillColor;
	Vector m_outlineColor;
	Float m_outlineWidth;
	Bool m_internalEdges;
};

/// Draw outlines around different objects (usually characters). Two different outlines are supported:
///
///		FXOUTLINE_Target	-- drawn around objects at all times (visible, occluded, everything).
///		FXOUTLINE_Occlusion	-- drawn around objects that have been occluded by certain geometry (currently SpeedTree foliage).
///
///
/// This effect is currently not being used. It hasn't been totally removed, in case someone wants it back. If the time comes to
/// actually remove it, or do something different with it, there are a few other places to consider:
///    - renderProxySpeedTree.cpp, Render has special light-channel checks for LC_OutlineOccluder on trees
///    - stateBlock.cpp in renderer, speedtree state blocks set LC_OutlineOccluder for trees
///    - gpuApiRenderState.cpp in gpuApi*, some depth-stencil functions have special cases for LC_FoliageOutline
///    - gpuApiInterface.h in gpuApiUtils, DSSM_ST_WriteStencil_* have possible ranges, for stencil write mask. specific to LC_FoliageOutline.
///    - drawableComponent.h in engine, LC_FoliageOutline LC_OutlineOccluder and LC_TargetOutline may or may not be needed.
///    - renderGameplayEffects.h in engine, still has EGameplayEffectOutlineType enum.
///    - and of course the shader itself, gameplayCharacterOutline.fx and m_gameplayCharacterOutline in renderShaders.h
/// That should be it...
class COutlineEffectPostFX : public IGameplayEffect
#ifndef NO_EDITOR_EVENT_SYSTEM
	, public IEdEventListener
#endif
{
private:

	struct OutlineState
	{
		Bool	m_isEnabled;
		Float	m_fadeSpeed;
		Float	m_strength;

		CName	m_groupName;				//<! Which outline group (coloring etc) this outline should use.

		Uint8	m_maskStencilBits;			//<! Bits for stencil test when filling mask.
		Uint8	m_maskInternalStencilBits;	//<! Bits for stencil test when filling mask, with internal edges enabled.

		Uint8	m_edgeDetectStencilBits;	//<! Bits for stencil test when rendering the outline shader.

		Bool	m_useStencilBuffer;			//<! Whether this outline should use stencil buffer, or a geometry pass

		OutlineState()
			: m_isEnabled( false )
			, m_fadeSpeed( 0.0f )
			, m_strength( 0.0f )
			, m_maskStencilBits( 0 )
			, m_maskInternalStencilBits( 0 )
			, m_edgeDetectStencilBits( 0 )
			, m_useStencilBuffer( true )
		{}
	};

	CGameplayEffects*				m_parent;

	OutlineState					m_outlineStates[ FXOUTLINE_MAX ];
	TDynArray< SOutlineGroup >		m_outlineGroups;

#ifndef NO_EDITOR_EVENT_SYSTEM
	mutable Red::Threads::CMutex	m_groupsMutex;
#endif

public:
	Uint8 GetUsedStencilBits() const;

public:
	COutlineEffectPostFX( CGameplayEffects* parent );
	~COutlineEffectPostFX();

	//-------------------------------------------
	// Common FX methods

	virtual void Init();

	virtual void Shutdown();

	virtual void Tick( Float time );

	virtual Bool Apply( CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName rtnColorSource, ERenderTargetName rtnColorTarget );

	//-------------------------------------------

	void Enable( EGameplayEffectOutlineType type, Float fadeTime );

	void Disable( EGameplayEffectOutlineType type, Float fadeTime );

#ifndef NO_EDITOR_EVENT_SYSTEM
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

protected:

	void ReloadOutlineSettings();

	// In editor, must lock m_groupsMutex before calling, and keep locked while reference is held!
	const SOutlineGroup& FindOutlineGroup( const CName& name ) const;
};