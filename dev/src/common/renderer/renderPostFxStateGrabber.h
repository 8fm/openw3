/**
*	Copyright © 2015 CD Projekt Red. All Rights Reserved.
	Kamil Nowakowski
*/

#pragma once

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// This class contains some coherent info from frame to frame
// Its needed to keep some parametrs from the last frame to track 
// proper configs if there are blending or time related shit
// or blending and we cannot acces to the previous data sets
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

enum EPostFxDirtyMask
{
	EPDM_None			= 0,
	EPDM_DofParams		= FLAG(0),
	EPDM_BokehDofParams	= FLAG(1),

	EPDM_All			= EPDM_DofParams | EPDM_BokehDofParams
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

class CPostFxStateGrabber
{

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

	CEnvDepthOfFieldParametersAtPoint		m_dofParams;			//! Simple dof parameters

	SBokehDofParams							m_bokehDofParams;		//! Bokeh dof parameters (mostly cutscenes)

	Uint32									m_dirtyMask;

public:

	CPostFxStateGrabber()
		: m_dirtyMask( EPDM_None )
	{

	}

	~CPostFxStateGrabber()
	{

	}

public:

	RED_INLINE void AddDirtyMask( Uint32 flag ) { m_dirtyMask |= flag; }

	RED_INLINE void ClearDirtyMask( Uint32 flag ) { m_dirtyMask &= ~flag; }

	RED_INLINE Bool HasDirtyMask( EPostFxDirtyMask flag ) const { return ( m_dirtyMask & flag ) != 0; }

	RED_INLINE void ClearDirtyMask( ) { m_dirtyMask = EPDM_None; }

	RED_INLINE Uint32 GetDirtyMask() const { return m_dirtyMask; } ;

public:

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	//! Set new dof parameters
	RED_INLINE void SetLastDofParams( const CEnvDepthOfFieldParametersAtPoint& dofParams ) 
	{ 
		// Don't write to ourselves
		if( &m_dofParams != &dofParams )
		{
			m_dofParams = dofParams; 
			AddDirtyMask( EPDM_DofParams );
		}
	}

	//! Get old dof parameters
	RED_INLINE const CEnvDepthOfFieldParametersAtPoint& GetLastDofParams( ) const { return m_dofParams; }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	//! Set new bokeh parameters
	RED_INLINE void SetLastBokehDofParams( const SBokehDofParams& bokehDofParams ) 
	{ 
		// Don't write to ourselves
		if( &m_bokehDofParams != &bokehDofParams )
		{
			m_bokehDofParams = bokehDofParams; 
			AddDirtyMask( EPDM_BokehDofParams );
		}
	}

	//! Get old bokeh parameters
	RED_INLINE const SBokehDofParams& GetLastBokehDofParams( ) const { return m_bokehDofParams; }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

};
