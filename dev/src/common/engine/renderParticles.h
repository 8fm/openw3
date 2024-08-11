/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Type of particle data
enum EParticleDataType
{
	PDT_Simple,				//!< Using RenderSimpleParticle
	PDT_Rain,				//!< Using RenderSimpleParticle
	PDT_Screen,				//!< Using RenderSimpleParticle
	PDT_Parallel,			//!< Using RenderSimpleParticle
	PDT_SphereAligned,		//!< Using RenderSphereAlignedParticle
	PDT_MotionBlurred,		//!< Using RenderMotionBlurredParticle
	PDT_Trail,				//!< Using RenderTrailParticle
	PDT_Mesh,				//!< Using RenderMeshParticle
	PDT_FacingTrail,		//!< Using RenderFacingTrailParticle
	PDT_Beam				//!< Using RenderBeamParticle
};

/// Bilboard particle
struct RenderSimpleParticle
{
	Float	m_position[3];		// XYZ - position
	Uint8	m_color[4];			// Color + alpha
	float	m_rotation;			// Particle rotation
	float	m_frame;			// Render frame
	Float	m_size[2];			// Size ( not normalized )
	Float	m_uv[2];			// UV flip
};

/// sphere aligned particle
struct RenderSphereAlignedParticle
{
	Float	m_position[3];		// XYZ - position
	Uint8	m_color[4];			// Color + alpha
	float	m_rotation;			// Particle rotation
	float	m_frame;			// Render frame
	Float	m_size[2];			// Size ( not normalized )
	Float	m_verticalFixed;	// is vertical alignment fixed
	Float	m_uv[2];			// UV flip
};

/// Motion blurred particle
struct RenderMotionBlurredParticle
{
	Float	m_position[3];		// XYZ - position
	Uint8	m_color[4];			// Color + alpha
	float	m_rotation;			// Particle rotation
	float	m_frame;			// Render frame
	Float	m_size[2];			// Size ( not normalized )
	Float	m_velocity[3];		// Particle motion direction
	Float	m_stretch;			// stretch per velocity
	Float	m_motionBlend;		// motion blend ratio
	Float	m_uv[2];			// UV flip
};

/// Trail particle
struct RenderTrailParticle
{
	Float	m_position[3];		// XYZ - position
	Uint8	m_color[4];			// Color + alpha
	Float	m_lastPos[3];		// previous emitter position
	Float	m_emitterAxis[3];	// transformed emitter orientation vector (forward, right or up)
	Float	m_lemitterAxis[3];	// previous transformed emitter orientation vector (forward, right or up)
	Float	m_stretch;			// emit edge width
	Float	m_texCoord;			// vertical tex coord
	Float	m_lastTexCoord;		// previous particle vertical coord
	float	m_frame;			// Render frame
	Float	m_particleLen;		// distance between this and previous particle
	Uint8	m_lastAlpha;		// previous particle (edge) alpha value
};

/// Trail particle
struct RenderFacingTrailParticle
{
	Float	m_position[3];		// XYZ - position
	Uint8	m_color[4];			// Color + alpha
	Float	m_N1Pos[3];			// previous emitter position
	Float	m_N2Pos[3];			// previous previous emitter position :)
	Float	m_stretch;			// emit edge width
	Float	m_texCoord;			// vertical tex coord
	Float	m_lastTexCoord;		// previous particle vertical coord
	float	m_frame;			// Render frame
	Uint8	m_lastAlpha;		// previous particle (edge) alpha value
};

/// Beam particle
struct RenderBeamParticle
{
	Float	m_position[3];		// XYZ - position
	Uint8	m_color[4];			// Color + alpha
	Float	m_N1Pos[3];			// previous emitter position
	Float	m_N2Pos[3];			// previous previous emitter position :)
	Float	m_stretch;			// emit edge width
	Float	m_texCoord;			// vertical tex coord
	Float	m_lastTexCoord;		// previous particle vertical coord
	float	m_frame;			// Render frame
	Uint8	m_lastAlpha;		// previous particle (edge) alpha value
};

class CRenderMesh;

/// Mesh particle
struct RenderMeshParticle
{
	Matrix			m_localToWorld;
	Vector			m_color;
	CRenderMesh*	m_renderMesh;
};

/// Different types of passed additional info - feel free to expand it :) 
enum ERenderParticleDataAdditionalInfo
{
	RPDI_LightChannel,
};

/// Particle data
class IRenderParticleData : public IRenderObject
{
public:
	virtual ~IRenderParticleData(){};
	//! Get type of data
	virtual EParticleDataType GetDataType() const=0;

	//! Get number of particles
	virtual Uint32 GetNumParticles() const=0;

	//! Get data buffer
	virtual void* GetData()=0;	

	//! Set some additional data info
	virtual void SetAdditionalDataInfo( ERenderParticleDataAdditionalInfo infoType, Uint32 data ) = 0;
	virtual void SetAdditionalDataInfo( ERenderParticleDataAdditionalInfo infoType, Float data ) = 0;

public:
	//! Generate internal vertex data
	virtual void GenerateVertexData()=0;
};