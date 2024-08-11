/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

enum ERenderParticleType : Int32  //! KEEP NUMERATION THE SAME AS EParticleType
{
	RPT_Simple			= 0,
	RPT_Trail			= 1,
	RPT_FacingTrail		= 2,
	RPT_MeshParticle	= 3,
	RPT_Beam			= 4,

	RPT_Invalid			= 5
};

const Int32 PFS_Velocity		= FLAG( 1 );
const Int32 PFS_Color			= FLAG( 2 );
const Int32 PFS_Size2D		= FLAG( 3 );
const Int32 PFS_Size3D		= FLAG( 4 );
const Int32 PFS_Frame			= FLAG( 5 );
const Int32 PFS_Rotation2D	= FLAG( 6 );
const Int32 PFS_Rotation3D	= FLAG( 7 );
const Int32 PFS_Turbulence	= FLAG( 8 );
const Int32 PFS_EmitterAxis	= FLAG( 9 );

/// Particle
// WARNING: Particle structure is memory critical... structure has been organized to minimize gaps between members
// Additionally most used members are at the beginning.

// Particle fields offsets
static const Int32 PARTICLE_POSITION				= 0;
static const Int32 PARTICLE_LIFE					= 12;
static const Int32 PARTICLE_LIFE_SPAN_INV			= 16;
static const Int32 PARTICLE_VELOCITY				= 20;
static const Int32 PARTICLE_BASE_VELOCITY			= 32;
static const Int32 PARTICLE_FRAME					= 44;
static const Int32 PARTICLE_BASE_FRAME			= 48;
static const Int32 PARTICLE_COLOR					= 52;
static const Int32 PARTICLE_ALPHA					= 64;
static const Int32 PARTICLE_BASE_COLOR			= 68;
static const Int32 PARTICLE_BASE_ALPHA			= 80;
static const Int32 PARTICLE_SIZE					= 84;
static const Int32 PARTICLE_BASE_SIZE				= 92;
static const Int32 PARTICLE_ROTATION				= 100;
static const Int32 PARTICLE_ROTATION_RATE			= 104;
static const Int32 PARTICLE_BASE_ROTATION_RATE	= 108;
static const Int32 PARTICLE_TURBULENCE			= 112;
static const Int32 PARTICLE_TURBULENCE_COUNTER	= 124;
static const Int32 PARTICLE_TEXCOORD				= 100;
static const Int32 PARTICLE_PREV_TEXCOORD			= 104;
static const Int32 PARTICLE_SIZE3D				= 120;
static const Int32 PARTICLE_BASE_SIZE3D			= 132;
static const Int32 PARTICLE_ROTATION3D			= 84;
static const Int32 PARTICLE_ROTATION_RATE3D		= 96;
static const Int32 PARTICLE_BASE_ROTATION_RATE3D	= 108;
static const Int32 PARTICLE_EMITTER_AXIS			= 20;
static const Int32 PARTICLE_LAST_EMITTER_AXIS		= 32;

static const Int32 PARTICLE_BEAM_SPREAD			= 100;

struct SimpleParticle
{
public:
	Vector3		m_position;			//!< (0 byte) Current particle position
	Float		m_life;				//!< (12 byte) Particle life
	Float		m_lifeSpanInv;		//!< (16 byte) Relative scale of particle life span ( 0.0f for dead particles )
	Vector3		m_velocity;			//!< (20 byte) Current particle velocity
	Vector3		m_baseVelocity;		//!< (32 byte) Initial velocity

	Float		m_frame;			//!< (44 byte) Current particle animation frame
	Float		m_baseFrame;		//!< (48 byte) initial particle animation frame
	Vector3		m_color;			//!< (52 byte) Current particle color
	Float		m_alpha;
	Vector3		m_baseColor;		//!< (68 byte) Initial particle color
	Float		m_baseAlpha;

	Vector2		m_size;				//!< (84 byte) Current particle size
	Vector2		m_baseSize;			//!< (92 byte) Initial particle size

	Float		m_rotation;			//!< (100 byte) Current particle rotation
	Float		m_rotationRate;		//!< (104 byte) Current particle rotation speed
	Float		m_baseRotationRate;	//!< (108 byte) Initial particle rotation speed

	Vector3		m_turbulence;		//!< (112 byte) Current particle turbulence
	Float		m_turbulenceCounter;//!< (124 byte) Current particle turbulence timeout

	static const Int32 m_fieldSet = PFS_Velocity | PFS_Color | PFS_Size2D | PFS_Frame | PFS_Rotation2D | PFS_Turbulence;
};

struct TrailParticle
{
	Vector3		m_position;			//!< (0 byte) Current particle position
	Float		m_life;				//!< (12 byte) Particle life
	Float		m_lifeSpanInv;		//!< (16 byte) Relative scale of particle life span ( 0.0f for dead particles )
	Vector3		m_emitterAxis;		//!< (20 byte) Emitter orientation vector from the moment of spawn (forward,right, or up)
	Vector3		m_lastEmitterAxis;	//!< (32 byte) Prev

	Float		m_frame;			//!< (44 byte) Current particle animation frame
	Float		m_baseFrame;		//!< (48 byte) initial particle animation frame
	Vector3		m_color;			//!< (52 byte) Current particle color
	Float		m_alpha;			//!< (64 byte) Current particle alpha
	Vector3		m_baseColor;		//!< (68 byte) Initial particle color
	Float		m_baseAlpha;		//!< (80 byte) Initial particle alpha

	Vector2		m_size;				//!< (84 byte) Particle stretch along axis
	Vector2		m_baseSize;			//!< (92 byte) Initial particle size

	Float		m_texCoord;			//!< (100 byte) Particle texture coordinate in trail spawn direction
	Float		m_prevTexCoord;		//!< (104 byte) Previous particle texture coordinate in trail spawn direction

	static const Int32 m_fieldSet = PFS_Color | PFS_Size2D | PFS_Frame | PFS_EmitterAxis;
};

struct FacingTrailParticle
{
	Vector3		m_position;			//!< (0 byte) Current particle position
	Float		m_life;				//!< (12 byte) Particle life
	Float		m_lifeSpanInv;		//!< (16 byte) Relative scale of particle life span ( 0.0f for dead particles )
	Vector3		m_velocity;			//!< (20 byte) Current particle velocity
	Vector3		m_baseVelocity;		//!< (32 byte) Initial velocity

	Float		m_frame;			//!< (44 byte) Current particle animation frame
	Float		m_baseFrame;		//!< (48 byte) initial particle animation frame
	Vector3		m_color;			//!< (52 byte) Current particle color
	Float		m_alpha;			//!< (64 byte) Current particle alpha
	Vector3		m_baseColor;		//!< (68 byte) Initial particle color
	Float		m_baseAlpha;		//!< (80 byte) Initial particle alpha

	Vector2		m_size;				//!< (84 byte) Particle stretch along axis
	Vector2		m_baseSize;			//!< (92 byte) Initial particle size

	Float		m_texCoord;			//!< (100 byte) Particle texture coordinate in trail spawn direction
	Float		m_prevTexCoord;		//!< (104 byte) Previous particle texture coordinate in trail spawn direction

	static const Int32 m_fieldSet = PFS_Color | PFS_Size2D | PFS_Frame | PFS_Velocity;
};

struct MeshParticle
{
	Vector3		m_position;			//!< (0 byte) Current particle position
	Float		m_life;				//!< (12 byte) Particle life
	Float		m_lifeSpanInv;		//!< (16 byte) Relative scale of particle life span ( 0.0f for dead particles )
	Vector3		m_velocity;			//!< (20 byte) Current particle velocity
	Vector3		m_baseVelocity;		//!< (32 byte) Initial velocity

	Float		m_frame;			//!< (44 byte) Current particle animation frame
	Float		m_baseFrame;		//!< (48 byte) initial particle animation frame
	Vector3		m_color;			//!< (52 byte) Current particle color
	Float		m_alpha;			//!< (64 byte) Current particle alpha
	Vector3		m_baseColor;		//!< (68 byte) Initial particle color
	Float		m_baseAlpha;		//!< (80 byte) Initial particle alpha

	Vector3		m_rotation;			//!< (84 byte) Current particle rotation
	Vector3		m_rotationRate;		//!< (96 byte) Current particle rotation speed
	Vector3		m_baseRotationRate; //!< (108 byte) Initial particle rotation speed

	Vector3		m_size;				//!< (120 byte) 3D size of the particle	
	Vector3		m_baseSize;			//!< (132 byte) Initial 3D size of the particle	



	//Vector3		m_turbulence;		//!< (72 byte) Current particle turbulence
	//Float		m_turbulenceCounter;//!< (84 byte) Current particle turbulence timeout

	static const Int32 m_fieldSet = PFS_Velocity | PFS_Size3D | PFS_Frame | PFS_Rotation3D | PFS_Color;
};

struct BeamParticle
{
public:
	Vector3		m_position;			//!< (0 byte) Current particle position
	Float		m_life;				//!< (12 byte) Particle life
	Float		m_lifeSpanInv;		//!< (16 byte) Relative scale of particle life span ( 0.0f for dead particles )
	Vector3		m_velocity;			//!< (20 byte) Current particle velocity
	Vector3		m_baseVelocity;		//!< (32 byte) Initial velocity

	Float		m_frame;			//!< (44 byte) Current particle animation frame
	Float		m_baseFrame;		//!< (48 byte) initial particle animation frame
	Vector3		m_color;			//!< (52 byte) Current particle color
	Float		m_alpha;
	Vector3		m_baseColor;		//!< (68 byte) Initial particle color
	Float		m_baseAlpha;

	Vector2		m_size;				//!< (84 byte) Current particle size
	Vector2		m_baseSize;			//!< (92 byte) Initial particle size

	Vector3		m_beamSpread;		//!< (100 byte) Current particle rotation

	Vector3		m_turbulence;		//!< (112 byte) Current particle turbulence
	Float		m_turbulenceCounter;//!< (124 byte) Current particle turbulence timeout

	static const Int32 m_fieldSet = PFS_Velocity | PFS_Color | PFS_Size2D | PFS_Frame | PFS_Turbulence;
};