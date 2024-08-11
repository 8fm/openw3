/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "boundedComponent.h"
#include "renderProxyInterface.h"

class CRenderFrameFragmentCollector;

/// Light channels
enum ELightChannel
{
	LC_DynamicObject	= FLAG( 0 ),		//!< Represents a dynamic object (animated, or otherwise moveable). Set by render proxy at creation, based on the component.
	LC_Characters		= FLAG( 1 ),		//!< Characters [ duplicated in shaders ]

	// Gameplay effects
	LC_Interactive		= FLAG( 2 ),
	LC_Custom0			= FLAG( 3 ),		//!< Should be highlighted while in focus mode. TODO: Rename this... except not that easy, since it's serialized by name...

	// These three are largely deprecated. The outline effect has not been fully removed, in case someone decides they want it back, but
	// it has been disabled. So, in an emergency one of these might be able to be re-purposed.
	LC_FoliageOutline	= FLAG( 4 ),		//!< Should be outlined behind foliage.
	// LC_OutlineOccluder	= FLAG( 5 ),		//!< Should allow outlining effect when occluding characters. Used internally, doesn't work when applied to an arbitrary Drawable Component.

	// Sorry Tim, stealing one of these for CatView mode.
	LC_VisibleThroughtWalls	= FLAG( 5 ),

	LC_Terrain			= FLAG( 6 ),

	LC_ForwardShaded	= FLAG( 7 ),

	LC_Default			= 0,				//!< Default flags. OR'd combination of the above (or 0 for no flags by default).
};

BEGIN_BITFIELD_RTTI( ELightChannel, 1 );
	//BITFIELD_OPTION( LC_DynamicObject );		// Not available for setting on a component, used internally.
	BITFIELD_OPTION( LC_Characters );
	BITFIELD_OPTION( LC_Interactive );
	BITFIELD_OPTION( LC_Custom0 );
	BITFIELD_OPTION( LC_FoliageOutline );
	BITFIELD_OPTION( LC_VisibleThroughtWalls );
	//BITFIELD_OPTION( LC_OutlineOccluder );	// Not available for setting on a component, used internally.
	//BITFIELD_OPTION( LC_Terrain );			// Not available for setting on a component, used internally.
	//BITFIELD_OPTION( LC_ForwardShaded );	// Not available for setting on a component, used internally.
END_BITFIELD_RTTI();

/// Drawable flags
enum EDrawableFlags
{
	DF_IsVisible					= FLAG( 0 ),		//!< Draw this component
	DF_CastShadows					= FLAG( 1 ),		//!< This mesh is casting shadows
	DF_NoLighting					= FLAG( 2 ),		//!< Do not light this mesh ( will use non defered rendering )
	DF_LocalWindSimulation			= FLAG( 3 ),		//!< Use local wind simulation for this mesh
	DF_UseInAllApperances			= FLAG( 4 ),		//!< Show this drawable in all apperances
	DF_NoColoring					= FLAG( 5 ),		//!< Do not allow any coloring for this drawable component
	DF_NoDissolves					= FLAG( 6 ),		//!< Do not perform dissolves on component
	DF_CameraTransformRotate		= FLAG( 7 ),		//!< Perform a camera transform on this component with rotation (renderer-side)
	DF_CameraTransformOnlyPosition	= FLAG( 8 ),		//!< Perform a camera transform on this component (translate only) (renderer-side)
	DF_CastShadowsWhenNotVisible	= FLAG( 9 ),		//!< Proxy cast shadows even if is not visible	
	DF_CastShadowsFromLocalLightsOnly	= FLAG( 10 ),	//!< Proxy cast shadows only from local lights (no global shadow map)
	DF_ForceNoAutohide				= FLAG( 11 ),		//!< Do not hide after autohide distance	
	DF_DynamicGeometry				= FLAG( 14 ),		//!< Mark this object as dynamic geometry ( for dynamic shadow casting )
	DF_ScheduledRenderingResourceChanged	= FLAG( 15 ),
	DF_ForceTwoSided				= FLAG( 16 ),
	DF_ForceHighestLOD				= FLAG( 17 ),
	DF_MissedUpdateTransform		= FLAG( 18 ),		//!< Parent entity has been moved while invisible need to update transform if made visible again
	DF_Collapsed					= FLAG( 19 ),		//!< Uses vertex collapse
	DF_ClimbBlock					= FLAG( 20 ),		//!< Don't allow climb
	DF_ClimbabUnlock				= FLAG( 21 ),		//!< Force Allow climb
	DF_IsCharacterShadowFallback	= FLAG( 23 ),		//!< Is character shadow fallback
	DF_UseWithSimplygonOnly			= FLAG( 24 ),		//!< For use with Simplygon during proxy mesh generation (eg.: covering geometry holes)
};

BEGIN_BITFIELD_RTTI( EDrawableFlags, 4 );
	BITFIELD_OPTION( DF_IsVisible );
	BITFIELD_OPTION( DF_CastShadows );
	BITFIELD_OPTION( DF_NoLighting );
	BITFIELD_OPTION( DF_LocalWindSimulation );
	BITFIELD_OPTION( DF_UseInAllApperances );
	BITFIELD_OPTION( DF_NoColoring );
	BITFIELD_OPTION( DF_NoDissolves );
	BITFIELD_OPTION( DF_CameraTransformRotate );
	BITFIELD_OPTION( DF_CameraTransformOnlyPosition );
	BITFIELD_OPTION( DF_CastShadowsWhenNotVisible );
	BITFIELD_OPTION( DF_CastShadowsFromLocalLightsOnly );
	BITFIELD_OPTION( DF_ForceNoAutohide );
	BITFIELD_OPTION( DF_DynamicGeometry	);
	BITFIELD_OPTION( DF_ForceTwoSided );
	BITFIELD_OPTION( DF_ForceHighestLOD );
	BITFIELD_OPTION( DF_MissedUpdateTransform );
	BITFIELD_OPTION( DF_Collapsed );
	BITFIELD_OPTION( DF_ClimbBlock );
	BITFIELD_OPTION( DF_ClimbabUnlock );
	BITFIELD_OPTION( DF_IsCharacterShadowFallback );
	BITFIELD_OPTION( DF_UseWithSimplygonOnly );
END_BITFIELD_RTTI();


// bwr todo: if needed more planes - eg. foreground, need to use Z viewport transform inside some shaders eg. terrain
enum ERenderingPlane
{
	RPl_Scene,
	RPl_Background,
	RPl_Max,
};

BEGIN_ENUM_RTTI( ERenderingPlane );
ENUM_OPTION( RPl_Scene );
ENUM_OPTION( RPl_Background );
END_ENUM_RTTI();

/// Component that can be drawn
class CDrawableComponent : public CBoundedComponent, public IRenderProxyInterface
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CDrawableComponent, CBoundedComponent );

	// Friend classes
	friend class CRenderScene;

protected:
	IRenderProxy*					m_renderProxy;				//!< Proxy for rendering version of this drawable component
	Uint32							m_drawableFlags;			//!< Flags for drawable component
	Uint8							m_lightChannels;			//!< Light channels
	ERenderingPlane					m_renderingPlane;			//!< Rendering plane

#ifndef NO_EDITOR
	static Bool						m_forceNoAutohideDebug;		//!< Flag used for temporary autohide disabling in editor
#endif
	
public:
	// Get visibility status
	RED_INLINE Bool IsVisible() const { return 0 != ( m_drawableFlags & DF_IsVisible ); }

	// Has this component disabled lighting ?
	RED_INLINE Bool NoLighting() const { return 0 != ( m_drawableFlags & DF_NoLighting ); }

	// Is component casting shadows ?
	RED_INLINE Bool IsCastingShadows() const { return 0 != ( m_drawableFlags & DF_CastShadows ); }

	// Is component using local wind simulation ?
	RED_INLINE Bool IsLocalWindSimulation() const { return 0 != ( m_drawableFlags & DF_LocalWindSimulation ); }

	// Is this component visible in all appearances ?
	RED_INLINE Bool IsUsedInAllAppearances() const { return 0 != ( m_drawableFlags & DF_UseInAllApperances ); }

	// Is component transformed by camera
	RED_INLINE Bool IsCameraTransformComponentWithRotation() const { return 0 != ( m_drawableFlags & DF_CameraTransformRotate ); }

	// Is component transformed by camera
	RED_INLINE Bool IsCameraTransformComponentWithoutRotation() const { return 0 != ( m_drawableFlags & DF_CameraTransformOnlyPosition ); }

	// Is castin' shadows even if not visible
	RED_INLINE Bool IsCastingShadowsEvenIfNotVisible() const { return 0 != ( m_drawableFlags & DF_CastShadowsWhenNotVisible ); }

	// Is castin' shadows from local lights only (underground caves and shit)
	RED_INLINE Bool IsCastingShadowsFromLocalLightsOnly() const { return 0 != ( m_drawableFlags & DF_CastShadowsFromLocalLightsOnly ); }
		
	// Is character shadow fallback
	RED_INLINE Bool IsCharacterShadowFallback() const { return 0 != ( m_drawableFlags & DF_IsCharacterShadowFallback ); }

	// Do not use shadow LOD for this component
	RED_INLINE Bool IsForceNoAutohide() const { return 0 != ( m_drawableFlags & DF_ForceNoAutohide ); }

	RED_INLINE Bool UseWithSimplygonOnly() const { return 0 != (m_drawableFlags & DF_UseWithSimplygonOnly ); }

#ifndef NO_EDITOR
	// Do not use shadow LOD for this component ( non-serializable )
	RED_INLINE static Bool IsForceNoAutohideDebug() { return m_forceNoAutohideDebug; }
#endif
	
	// Has the component disabled coloring ?
	RED_INLINE Bool NoColoring() const { return 0 != ( m_drawableFlags & DF_NoColoring ); }

	// Does it use vertex collapsed ?
	RED_INLINE Bool UsesVertexCollapsed() const { return 0 != ( m_drawableFlags & DF_Collapsed ); }

	// Does component have DF_MissedUpdateTransform flag
	RED_INLINE Bool MissedUpdateTransform() const { return 0 != ( m_drawableFlags & DF_MissedUpdateTransform ); }

	
	// Light channels this light is shining on
	RED_INLINE Uint8 GetLightChannels() const { return m_lightChannels; }

	// Get rendering proxy for this drawable component
	RED_INLINE IRenderProxy* GetRenderProxy() const { return m_renderProxy; }

	// Get rendering plane
	RED_INLINE ERenderingPlane GetRenderingPlane() const { return m_renderingPlane; }

	// get is two sided flag
	RED_INLINE Bool IsTwoSided() const { return 0 != ( m_drawableFlags & DF_ForceTwoSided ); }

	// get is force highest LOD flag
	RED_INLINE Bool IsForcedHighestLOD() const { return 0 != ( m_drawableFlags & DF_ForceHighestLOD ); }

	// get drawable flags
	RED_INLINE const Uint32 GetDrawableFlags() const { return m_drawableFlags; }
	
public:
	// Object constructor
	CDrawableComponent();

	// Set visibility status
	void SetVisible( Bool visible );

	// Set force no autohide
	void SetForceNoAutohide( Bool forceNoAutohide );

#ifndef NO_EDITOR
	// Set force no autohide temporarily
	RED_INLINE static void SetForceNoAutohideDebug( Bool forceNoAutohide ) { m_forceNoAutohideDebug = forceNoAutohide; }

	virtual Bool RemoveOnCookedBuild() override;
#endif

	// Set visibility status
	virtual void ToggleVisibility( Bool visible );

	// Set casting shadows
	void SetCastingShadows( Bool enable, Bool enableForLocalLightsOnly = false );

	// Set local wind simulation
	void SetLocalWindSimulation( Bool enable );

	// Set light channels this light is shining on
	void SetLightChannels( Uint8 channels );

	// Enable / disable specified light channel
	void EnableLightChannels( Bool enable, Uint8 channels );

	// Enable / disable specified drawable flags
	void EnableDrawableFlags( Bool enable, Uint32 flags );

	// Check if specified light channel is enabled
	Bool AreLightChannelsEnabled( Uint8 channels );

	// Set no disolves
	void SetNoDisolves( Bool noDisolves );

	// Set is two sided flag
	void SetTwoSided( Bool twoSided );

	// Set force highest LOD flag
	void SetForcedHighestLOD( Bool forced );

	// Set all drawable flags
	void SetDrawableFlags( Uint32 flags );

	void SetUsesVertexCollapsed( bool collapsed );

public:
	// Property changed
	virtual void OnPropertyPostChange( IProperty* property );

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// New parent attachment was added
	virtual void OnParentAttachmentAdded( IAttachment* attachment );

	// Parent attachment was broken
	virtual void OnParentAttachmentBroken( IAttachment* attachment );

	// Update component transform
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// Initialize rendering proxy
	virtual void OnInitializeProxy();

	//! Node selection changed
	virtual void OnSelectionChanged();

	// Generate hit proxy fragments for editor
	virtual void OnGenerateEditorHitProxies( CHitProxyMap& map );

	// Color variant of parent entity has changed
	virtual void OnColorVariantChanged();

	// Rendering resource has changed
	virtual void OnRenderingResourceChanged();

	// Visibility flag somewhere below was changed
	virtual void OnRefreshVisibilityFlag();

	// TODO MATEUSZ MERGE
	// Visibility flag was forced to change immediately
	virtual void OnVisibilityForced();

	// Should performe attach/detach on dissolve?
	virtual Bool ShouldPerformProxyAttachesOnDissolve() const;

	// Estimate if this component a dynamic geometry component ? (casts dynamic shadows that can change every frame?)
	virtual Bool IsDynamicGeometryComponent() const;

	//! Is component ready to be rendered?
	virtual Bool IsRenderingReady() const { return true; }

	void SheduleRenderingResourceChange() { m_drawableFlags |= DF_ScheduledRenderingResourceChanged; }

	// [Hack], please do not use this!
	RED_INLINE void SetMissedUpdateTransformFlag() { m_drawableFlags |= DF_MissedUpdateTransform; }

#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	virtual void CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const override;
#endif

public: // IRenderProxyInterface
	virtual IRenderProxyInterface* QueryRenderProxyInterface();
	virtual const IRenderProxyInterface* QueryRenderProxyInterface() const;

	virtual Uint32 GetNumberOfRenderProxies() const;
	virtual IRenderProxy* GetRenderProxy( Uint32 i );

	virtual void SetProxiesVisible( Bool flag );

	virtual void RefreshRenderProxies();
	virtual void RefreshEntityGroupBinding();

	void RelinkProxy( SUpdateTransformContext* context = nullptr );

	virtual Bool CanAttachToRenderScene() const;

protected:
	// Attach to render scene
	void ConditionalAttachToRenderScene( CWorld* world );
	void RefreshEngineColoring();
	void InitFromAppearance();

protected:
	void funcIsVisible( CScriptStackFrame& stack, void* result );
	void funcSetVisible( CScriptStackFrame& stack, void* result );
	void funcSetCastingShadows( CScriptStackFrame& stack, void* result );
	void funcEnableLightChannels( CScriptStackFrame& stack, void* result );
	void funcAreLightChannelsEnabled( CScriptStackFrame& stack, void* result );

public:
	//! Inform all drawable components that rendering resource has changed (usable only in editor!)
	static void RecreateProxiesOfRenderableComponents();

	//! Inform all drawable components that rendering resource has changed (usable during fur settings enable/disable)
	static void RecreateProxiesOfFurComponents();

	//! Inform all drawable components that selection color should be recalculated
	static void RenderingSelectionColorChangedInEditor();
};

BEGIN_ABSTRACT_CLASS_RTTI( CDrawableComponent );
	PARENT_CLASS( CBoundedComponent );
	PROPERTY_BITFIELD_EDIT( m_drawableFlags, EDrawableFlags, TXT("Special flags for drawable components") );
	PROPERTY_BITFIELD_EDIT( m_lightChannels, ELightChannel, TXT("Light channels") );
	PROPERTY_EDIT( m_renderingPlane, TXT( "Rendering plane" ) );
	NATIVE_FUNCTION( "IsVisible", funcIsVisible );
	NATIVE_FUNCTION( "SetVisible", funcSetVisible );
	NATIVE_FUNCTION( "SetCastingShadows", funcSetCastingShadows );
	NATIVE_FUNCTION( "EnableLightChannels", funcEnableLightChannels );
	NATIVE_FUNCTION( "AreLightChannelsEnabled", funcAreLightChannelsEnabled );
END_CLASS_RTTI();
