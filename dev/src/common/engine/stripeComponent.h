/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "drawableComponent.h"
#include "engineMetalinkTypes.h"
#include "renderSettings.h"

#ifdef USE_UMBRA
class CUmbraScene;
#endif // USE_UMBRA

#ifndef NO_EDITOR
// Editor flags - used by the stripe editor for special rendering/visualization states
// Note: check renderProxyStripe to make sure the flag values are the same
#define SCEF_NOFLAGS				0x0000
#define SCEF_STRIPE_TOOL_ACTIVE		0x0001
#define SCEF_VISUALIZE_BLEND		0x0002
#define SCEF_VISUALIZE_ALPHA		0x0004
#define SCEF_SHOW_SEGMENTS			0x0008
#define SCEF_SHOW_AIROAD_LINES		0x0010
#define SCEF_SHOW_CENTER_LINES		0x0020
#define SCEF_HIDE_CONTROLS			0x0040
extern Uint32 GStripeComponentEditorFlags;
#endif // NO_EDITOR

/// Control point for the stripes
struct SStripeControlPoint
{
	Vector		m_position;		//!< Position of the control point
	Color		m_color;		//!< Color and alpha
	Float		m_scale;		//!< Scale
	EulerAngles	m_rotation;		//!< Rotation
	Float		m_blendOffset;	//!< Blending offset

	RED_INLINE SStripeControlPoint()
		: m_position( 0.0f, 0.0f, 0.0f )
		, m_color( Color::WHITE )
		, m_scale( 1.0f )
		, m_rotation( 0.0f, 0.0f, 0.0f )
		, m_blendOffset( 0.0f )
	{
	}

	RED_INLINE SStripeControlPoint( const SStripeControlPoint& src )
		: m_position( src.m_position )
		, m_color( src.m_color )
		, m_scale( src.m_scale )
		, m_blendOffset( src.m_blendOffset )
	{
	}
	RED_INLINE SStripeControlPoint( const Vector& position, const Color& color=Color::WHITE, Float scale=1.0f, const EulerAngles& rotation=EulerAngles::ZEROS, Float blendOffset=0.0f )
		: m_position( position )
		, m_color( color )
		, m_scale( scale )
		, m_rotation( rotation )
		, m_blendOffset( blendOffset )
	{
	}

	DECLARE_RTTI_STRUCT( SStripeControlPoint );
};

BEGIN_CLASS_RTTI( SStripeControlPoint );
	PROPERTY_EDIT( m_position, TXT("Position") );
	PROPERTY_EDIT( m_color, TXT("Color") );
	PROPERTY_EDIT_RANGE( m_scale, TXT("Scale"), 0.0f, 1000.0f );
	PROPERTY_EDIT( m_rotation, TXT("Rotation") );
	PROPERTY_EDIT_RANGE( m_blendOffset, TXT("Blending offset"), -1.0f, 1.0f );
END_CLASS_RTTI();

/// Stripe components (for roads, paths, etc)
class CStripeComponent : public CDrawableComponent, public PathLib::IMetalinkComponent
{
	DECLARE_ENGINE_CLASS( CStripeComponent, CDrawableComponent, 0 )

	friend class CEdStripeEdit;
	friend class CUndoStripePoint;

protected:
	THandle< CBitmapTexture >			m_diffuseTexture;		//!< Diffuse texture
	THandle< CBitmapTexture >			m_diffuseTexture2;		//!< Second optional diffuse texture
	THandle< CBitmapTexture >			m_normalTexture;		//!< Optional normal texture
	THandle< CBitmapTexture >			m_normalTexture2;		//!< Second optional normal texture
	THandle< CBitmapTexture >			m_blendTexture;			//!< Blend texture
	THandle< CBitmapTexture >			m_depthTexture;			//!< Depth texture
	Float								m_textureLength;		//!< Length of the texture in segments
	Float								m_blendTextureLength;	//!< Length of the blend texture in segments
	TDynArray< SStripeControlPoint >	m_points;				//!< Control points
	Float								m_width;				//!< Width of the stripe
	Float								m_alphaScale;			//!< Alpha scale for the whole stripe
	Float								m_endpointAlpha;		//!< Alpha for endpoints
	Float								m_autoHideDistance;		//!< Auto hide distance
	Color								m_stripeColor;			//!< Color for the whole stripe
	Float								m_density;				//!< Stripe density
	Bool								m_rotateTexture;		//!< Rotate the texture 90 degrees
	Bool								m_projectToTerrain;		//!< Project the stripe to terrain
	Bool								m_exposedToNavigation;	//!< Expose the stripe to navigation & AI

	SMultiCurve*						m_curve;				//!< Lazy initialized curve representation used possibly by external systems (ai)
#ifndef NO_EDITOR
	TDynArray< Vector >					m_centerPoints;			//!< Center points for each segment
#endif

	Uint32			m_cookedVertexCount;			//!< Number of cooked vertices
	Uint32			m_cookedIndexCount;				//!< Number of cooked indices
	DataBuffer		m_cookedIndices;				//!< Index buffer data
	DataBuffer		m_cookedVertices;				//!< Vertex buffer data
	Box				m_cookedBoundingBox;			//!< Stripe bounding box

	void GenerateStripeGeometry( struct SRenderProxyStripeProperties* properties, Float extraWidth=0.0f );
	void ComputeCurve( SMultiCurve& curve, Float *totalDistance = nullptr );

public:
	CStripeComponent();
	virtual ~CStripeComponent();
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
	virtual void OnUpdateBounds();
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

#ifndef NO_EDITOR
			void Highlight( CRenderFrame* frame, Color stripeColor = Color::WHITE, Color frameColor = Color::DARK_RED ) const;
#endif

#ifdef USE_UMBRA
	virtual Bool ShouldBeCookedAsOcclusionData() const { return true; }
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
	virtual Uint32 GetOcclusionId() const override;
#endif

	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnVisibilityForced();
	virtual void RefreshRenderProxies();

	virtual Float GetAutoHideDistance() const {	return CalculateAutoHideDistance( m_autoHideDistance, IsCooked() ? m_cookedBoundingBox : m_boundingBox ); }
	virtual Float GetDefaultAutohideDistance() const { return Config::cvStripeHideDistance.Get(); }
	virtual Float GetMaxAutohideDistance() const { return 300.0f; }

	virtual void GetStorageBounds( Box& box ) const override { box = GetBoundingBox(); }

	const SMultiCurve& RequestCurve();

	// PathLib::IMetalinkComponent interface
	virtual CComponent*					AsEngineComponent() override;
	virtual PathLib::IComponent*		AsPathLibComponent() override;
	virtual Bool						IsNoticableInGame( PathLib::CProcessingEvent::EType eventType ) const override;
	virtual Bool						IsNoticableInEditor( PathLib::CProcessingEvent::EType eventType ) const override;
	virtual Bool						ConfigureGraph( GraphConfiguration& configuration, CPathLibWorld& pathlib ) override;
	virtual PathLib::IMetalinkSetup::Ptr CreateMetalinkSetup() const override;

	void GetRenderProxyStripeProperties( struct SRenderProxyStripeProperties* properties );
	void UpdateStripeProxy();

	RED_INLINE Bool GetProjectToTerrain() const { return m_projectToTerrain; }
	RED_INLINE Bool IsExposedToAI() const { return m_exposedToNavigation; }
	RED_INLINE Uint32 GetControlPointCount() const { return m_points.Size(); }
	RED_INLINE const SStripeControlPoint& GetControlPoint( Uint32 index ) const { return m_points[index]; }
	RED_INLINE SStripeControlPoint& GetControlPoint( Uint32 index ) { return m_points[index]; }
	RED_INLINE Float GetStripeWidth() const { return m_width; }

#ifndef NO_EDITOR
	void SetProjectToTerrain( bool projectToTerrain );
	void SetExposeToAI( bool exposeToAI );
	virtual void EditorPreDeletion() override;
#endif

#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif
};

BEGIN_CLASS_RTTI( CStripeComponent );
	PARENT_CLASS( CDrawableComponent );
	PROPERTY_EDIT( m_diffuseTexture, TXT("First Diffuse texture") );
	PROPERTY_EDIT( m_diffuseTexture2, TXT("Second optional diffuse texture (needs blend texture)") );
	PROPERTY_EDIT( m_normalTexture, TXT("Optional first diffuse texture (needs first diffuse texture)") );
	PROPERTY_EDIT( m_normalTexture2, TXT("Optional second diffuse texture (needs second diffuse texture and blend texture)") );
	PROPERTY_EDIT( m_blendTexture, TXT("Texture controlling the blending between the first and second pair of textures") );
	PROPERTY_EDIT( m_depthTexture, TXT("Texture controlling parallax") );
	PROPERTY_EDIT( m_rotateTexture, TXT("Rotate texture 90 degrees") );
	PROPERTY_EDIT( m_autoHideDistance, TXT( "Auto hide distance. -1 will set default value from ini file" ) );
	PROPERTY_EDIT_RANGE( m_textureLength, TXT("Texture length in segments"), 0, 1000 );
	PROPERTY_EDIT_RANGE( m_blendTextureLength, TXT("Blend texture length in segments"), 0, 1000 );
	PROPERTY( m_points );
	PROPERTY_EDIT_RANGE( m_width, TXT("Stripe width"), 0.01f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_alphaScale, TXT("Alpha scale for the whole stripe"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_endpointAlpha, TXT("Alpha at endpoints"), 0.0f, 1.0f );
	PROPERTY_EDIT( m_stripeColor, TXT("Color for the whole stripe") );
	PROPERTY_EDIT_RANGE( m_density, TXT("Stripe density multiplier"), 0.5f, 32.0f );
	PROPERTY_EDIT( m_projectToTerrain, TXT("Project the stripe to terrain (for roads, etc)") );
	PROPERTY_EDIT( m_exposedToNavigation, TXT("Expose the stripe to AI") );
	PROPERTY( m_cookedVertices );
	PROPERTY( m_cookedIndices );
	PROPERTY( m_cookedVertexCount );
	PROPERTY( m_cookedIndexCount );
	PROPERTY( m_cookedBoundingBox );
END_CLASS_RTTI();

class CStripeComponentSetup : public PathLib::IMetalinkSetup
{
public:
	virtual PathLib::MetalinkClassId GetClassId() const override;

	virtual Uint32					GetMetalinkPathfollowFlags() const override;
	virtual Bool					AgentPathfollowUpdate( RuntimeData& r, PathLib::CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint ) override;
	virtual Bool					AgentPathfollowOver( RuntimeData& r, PathLib::CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint ) override;
	virtual Bool					AgentPathfollowIgnore( PathLib::CAgent* pathAgent ) override;
};

