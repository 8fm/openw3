/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "evaluatorVector.h"
#include "drawableComponent.h"
#include "materialCompiler.h"

enum EParticleType : Int32	//! KEEP NUMERATION THE SAME AS ERenderParticleType
{
	PT_Simple		= 0,
	PT_Trail		= 1,
	PT_FacingTrail	= 2,
	PT_Mesh			= 3,
	PT_Beam			= 4,

	PT_Invalid		= 5
};

enum EParticleVertexDrawerType : Int32
{
	PVDT_Billboard,			//!< Using Billboard Drawer
	PVDT_Rain,				//!< Using Rain Drawer
	PVDT_Screen,			//!< Using Screen Drawer
	PVDT_EmitterOrientation,//!< Using Emitter Orientation Drawer
	PVDT_SphereAligned,		//!< Using SphereAligned Drawer
	PVDT_VerticalFixed,		//!< Using SphereAligned Drawer with modified vertex factory
	PVDT_MotionBlurred,		//!< Using MotionBlur Drawer
	PVDT_Trail,				//!< Using Trail Drawer
	PVDT_Mesh,				//!< Using Mesh Drawer
	PVDT_FacingTrail,		//!< Using Facing Trail Drawer
	PVDT_Beam,				//!< Using Beam Drawer

	PVDT_Invalid
};

BEGIN_ENUM_RTTI( EParticleVertexDrawerType );
	ENUM_OPTION( PVDT_Billboard );
	ENUM_OPTION( PVDT_Rain );
	ENUM_OPTION( PVDT_Screen );
	ENUM_OPTION( PVDT_EmitterOrientation );
	ENUM_OPTION( PVDT_SphereAligned );
	ENUM_OPTION( PVDT_MotionBlurred );
	ENUM_OPTION( PVDT_Trail );
	ENUM_OPTION( PVDT_Mesh );
	ENUM_OPTION( PVDT_FacingTrail );
	ENUM_OPTION( PVDT_Beam );
	ENUM_OPTION( PVDT_Invalid );
END_ENUM_RTTI();

namespace
{
	EMaterialVertexFactory ParticleVertexDrawerToMaterialVertexFactory( EParticleVertexDrawerType pvdt )
	{
		EMaterialVertexFactory ret = MVF_Invalid;
		switch ( pvdt )
		{
		case PVDT_Billboard:			ret = MVF_ParticleBilboard;	break;
		case PVDT_SphereAligned:		ret = MVF_ParticleSphereAligned; break;
		case PVDT_VerticalFixed:		ret = MVF_ParticleVerticalFixed; break;
		case PVDT_MotionBlurred:		ret = MVF_ParticleMotionBlur; break;
		case PVDT_Rain:					ret = MVF_ParticleBilboardRain;	break;
		case PVDT_Trail:				ret = MVF_ParticleTrail; break;
		case PVDT_EmitterOrientation:	ret = MVF_ParticleParallel; break;
		case PVDT_Screen:				ret = MVF_ParticleScreen; break;
		case PVDT_Mesh:					ret = MVF_MeshStatic; break;
		case PVDT_FacingTrail:			ret = MVF_ParticleFacingTrail; break;
		case PVDT_Beam:					ret = MVF_ParticleBeam; break;
		default: ASSERT( !"Invalid particle type" );
		}

		return ret;
	}
}

// Interface for particle drawers
// drawers take care of preparing data fed into particle batcher
// basically it generates fragment data from Particle array
class IParticleDrawer : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IParticleDrawer, IParticleModule );

public:
	typedef TDynArray< THandle< CMesh > >	TMeshList;

public:
	IParticleDrawer() {}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { ASSERT( 0 ); return PT_Invalid; }

	// Get a list of mesh resources this drawer can use. Mesh drawer specific
	virtual const TMeshList* GetMeshes()					const { return nullptr; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_Invalid; }
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IParticleDrawer, CObject );


//////////////////////////////////////////////////////////////////////////
// Drawer setup data for Beams
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerBeam : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerBeam, IParticleDrawer, 0);

public:
	Float				m_texturesPerUnit;
	Uint32				m_numSegments;
	IEvaluatorVector*	m_spread;

public:
	CParticleDrawerBeam()
		: m_texturesPerUnit( 1.0f )
		, m_numSegments( 10 )
	{
		m_spread = new CEvaluatorVectorConst( this, Vector::ZEROS );
	}

	Uint32 GetNumSegments()									const { return m_numSegments; }

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const {	return PT_Beam; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_Beam; }
};

BEGIN_CLASS_RTTI( CParticleDrawerBeam );
PARENT_CLASS( IParticleDrawer );
PROPERTY_EDIT( m_texturesPerUnit, TXT( "Textures per length unit in world space" ) )
PROPERTY_INLINED( m_spread, TXT( "Spread factor of beams" ) )
PROPERTY_EDIT( m_numSegments, TXT( "Number of segments in each beam" ) )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Drawer setup for standard, billboard particles
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerBillboard : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerBillboard, IParticleDrawer, 0);

public:
	CParticleDrawerBillboard() {}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { return PT_Simple; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_Billboard; }
};
DEFINE_SIMPLE_RTTI_CLASS( CParticleDrawerBillboard, IParticleDrawer );

//////////////////////////////////////////////////////////////////////////
// Drawer setup for motion blurred particles, with additional rain logic
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerRain : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerRain, IParticleDrawer, 0);

public:
	Float m_stretchPerVelocity;
	Float m_blendStartVelocity;
	Float m_blendEndVelocity;

public:
	CParticleDrawerRain() 
		: m_stretchPerVelocity(0.5f)
		, m_blendStartVelocity(1.0f)
		, m_blendEndVelocity(5.0f)
	{}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { return PT_Simple; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_Rain; }
};

BEGIN_CLASS_RTTI( CParticleDrawerRain );
PARENT_CLASS( IParticleDrawer );
PROPERTY_EDIT( m_stretchPerVelocity, TXT( "Stretch per velocity" ) );
PROPERTY_EDIT( m_blendStartVelocity, TXT( "Blend start velocity" ) );
PROPERTY_EDIT( m_blendEndVelocity, TXT( "Blend end velocity" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Drawer setup for particles aligned to the XY plane of emitter
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerEmitterOrientation : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerEmitterOrientation, IParticleDrawer, 0);

public:
	CParticleDrawerEmitterOrientation() {}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { return PT_Simple; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_EmitterOrientation; }
};

DEFINE_SIMPLE_RTTI_CLASS( CParticleDrawerEmitterOrientation, IParticleDrawer );

//////////////////////////////////////////////////////////////////////////
// Drawer setup for particles aligned to the motion direction and speed-blurred
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerMotionBlur : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerMotionBlur, IParticleDrawer, 0);

public:
	Float m_stretchPerVelocity;
	Float m_blendStartVelocity;
	Float m_blendEndVelocity;

public:
	CParticleDrawerMotionBlur()
		: m_stretchPerVelocity(0.5f)
		, m_blendStartVelocity(1.0f)
		, m_blendEndVelocity(5.0f)
	{}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType() const { return PT_Simple; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_MotionBlurred; }
};

BEGIN_CLASS_RTTI( CParticleDrawerMotionBlur );
PARENT_CLASS( IParticleDrawer );
PROPERTY_EDIT( m_stretchPerVelocity, TXT( "Stretch per velocity" ) );
PROPERTY_EDIT( m_blendStartVelocity, TXT( "Blend start velocity" ) );
PROPERTY_EDIT( m_blendEndVelocity, TXT( "Blend end velocity" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Drawer setup for particles aligned to the sphere surrounding the viewer
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerSphereAligned : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerSphereAligned, IParticleDrawer, 0);

public:
	Bool m_verticalFixed;

public:
	CParticleDrawerSphereAligned()
		: m_verticalFixed( false )
	{}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { return PT_Simple; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const
	{ 
		if ( m_verticalFixed )
		{
			return PVDT_VerticalFixed;
		}
		else
		{
			return PVDT_SphereAligned; 
		}
	}
};

BEGIN_CLASS_RTTI( CParticleDrawerSphereAligned );
	PARENT_CLASS( IParticleDrawer );
	PROPERTY_EDIT( m_verticalFixed, TXT("Set vertical orientation uniform") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Drawer setup for screen decals
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerScreen : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerScreen, IParticleDrawer, 0);

public:
	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { return PT_Simple; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_Screen; }
};

BEGIN_CLASS_RTTI( CParticleDrawerScreen );
PARENT_CLASS( IParticleDrawer );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Drawer setup for trails
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerTrail : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerTrail, IParticleDrawer, 0);

public:
	Float m_texturesPerUnit;
	Bool m_dynamicTexCoords;
	Int32 m_minSegmentsPer360Degrees;

public:
	CParticleDrawerTrail()
		: m_texturesPerUnit( 0.5f )
		, m_dynamicTexCoords( false )
		, m_minSegmentsPer360Degrees( 40 )
	{}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { return PT_Trail; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_Trail; }
};

BEGIN_CLASS_RTTI( CParticleDrawerTrail );
PARENT_CLASS( IParticleDrawer );
PROPERTY_EDIT( m_texturesPerUnit, TXT( "Textures per length unit in world space" ) )
PROPERTY_EDIT( m_dynamicTexCoords, TXT( "Texture coordinates follow emitter" ) )
PROPERTY_EDIT( m_minSegmentsPer360Degrees, TXT("Minimal number of particles to spawn over full (360 degrees) angle - helps smoothing out trail edges") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Drawer setup for camera facing trails
//////////////////////////////////////////////////////////////////////////
class CParticleDrawerFacingTrail : public CParticleDrawerTrail
{
	DECLARE_ENGINE_CLASS(CParticleDrawerFacingTrail, CParticleDrawerTrail, 0);

public:
	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType()					const { return PT_FacingTrail; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_FacingTrail; }
};

BEGIN_CLASS_RTTI( CParticleDrawerFacingTrail );
PARENT_CLASS( CParticleDrawerTrail );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Drawer setup for mesh based particles
//////////////////////////////////////////////////////////////////////////

enum EMeshParticleOrientationMode : Int32
{
	MPOM_Normal,
	MPOM_MovementDirection,
	MPOM_NoRotation
};

BEGIN_ENUM_RTTI( EMeshParticleOrientationMode );
ENUM_OPTION( MPOM_Normal );
ENUM_OPTION( MPOM_MovementDirection );
ENUM_OPTION( MPOM_NoRotation );
END_ENUM_RTTI();

class CParticleDrawerMesh : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerMesh, IParticleDrawer, 0);

public:
	TMeshList						m_meshes;				//<! Mesh resources
	EMeshParticleOrientationMode	m_orientationMode;		//<! Orientation mode for meshes
	Uint8							m_lightChannels;		//!< Light channels

	CParticleDrawerMesh() 
		: m_lightChannels( LC_Default )
	{
	}

public:
	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType() const { return PT_Mesh; }

	// Get proper render side routine identifier
	virtual EParticleVertexDrawerType GetVertexDrawerType() const { return PVDT_Mesh; }

	// Get a list of mesh resources this drawer can use. Mesh drawer specific
	virtual const TMeshList* GetMeshes() const { return &m_meshes; }
};

BEGIN_CLASS_RTTI( CParticleDrawerMesh );
PARENT_CLASS( IParticleDrawer );
PROPERTY_EDIT( m_meshes, TXT( "Meshes" ) );
PROPERTY_EDIT( m_orientationMode, TXT( "The way for setting mesh orientation during movement" ) );
PROPERTY_BITFIELD_EDIT( m_lightChannels, ELightChannel, TXT("Light channels") );
END_CLASS_RTTI();
