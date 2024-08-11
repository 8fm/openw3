

struct SPostFxEffects
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_RenderData );
	SPostFxEffects ();

	void CreateEffects( const CRenderSurfaces* surfaces );
	void DestroyEffects();
	void NullEffects();

	class CPostFxPresent			*fxPresent;
	class CPostFxCopy				*fxCopy;
	class CPostFxDepthOfField		*fxDOF;
	class CPostFxBlurGauss			*fxBlurGauss;
	class CPostFxToneMapping		*fxToneMapping;
	class CPostFxBlur				*fxBlurFilter;
	class CPostFxDebug				*fxDebug;
	class CPostFxFlare				*fxFlare;
	class CPostFXHorizonBasedSSAO	*fxHBSSAO;
	class CPostFXMicrosoftSSAO		*fxMSSSAO;
	class CPostFxMotionBlur			*fxMotionBlur;
	class CPostFxUnderwater			*fxUnderwater;
	class CPostFxBokehDof			*fxBokehDof;
	class CPostFxSurfaceFlow		*fxSurfaceFlow;	
	class CPostFxStateGrabber		*fxStateGrabber;
};

/// Post-process quad array
class CPostProcessQuadArray
{
private:
	Uint32							m_numQuads;
	Uint32							m_width;
	Uint32							m_height;
	GpuApi::BufferRef				m_vertices;
	GpuApi::BufferRef				m_indices;
	Bool							m_isValid;

public:
	//! Get width of quad grid
	RED_INLINE Uint32 GetWidth() const { return m_width; }

	//! Get height of quad grid
	RED_INLINE Uint32 GetHeight() const { return m_height; }

	//! Get number of quads
	RED_INLINE Uint32 GetNumQuads() const { return m_numQuads; }

public:
	CPostProcessQuadArray( Uint32 width, Uint32 height, const Float z = 0.5f );
	~CPostProcessQuadArray();

	//! Draw the quad array
	void Draw() const;
};

/// Post-process drawer
class CPostProcessDrawer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
private:
	SPostFxEffects 			*m_pEffects;
	
public:
	CPostProcessDrawer ( const CRenderSurfaces* surfaces );
	~CPostProcessDrawer ();

	/// Draw screen aligned quad on given screen area with given shader (no shader will be bound in case of passing NULL)
	void DrawQuad( const PostProcessUtilities::TexelArea &renderArea, CRenderShaderPair *shader, Float z = 0.5f ) const;

	/// Draw quad that covers whole viewport
	void DrawQuad( CRenderShaderPair *shader, Float z = 0.5f, const Color& color = Color::WHITE ) const;

	/// Draw array of quads
	void DrawQuadArray( CRenderShaderPair *shader, const CPostProcessQuadArray& quads ) const;

	/// Get fullscreen effects
	const SPostFxEffects& GetEffects() const { return *m_pEffects; }

	/// On lost device
	void OnLostDevice();
};


#include "renderPostFxCopy.h"
#include "renderPostFxDebug.h"
#include "renderPostFxBlurGauss.h"
#include "renderPostFxPresent.h"
#include "renderPostFxDepthOfField.h"
#include "renderPostFxToneMapping.h"
#include "renderPostFXBlur.h"
#include "renderPostFxFlare.h"
#include "renderPostFxHBSSAO.h"
#include "renderPostFxMicrosoftSSAO.h"
#include "renderPostFxMotionBlur.h"
#include "renderPostFxUnderwater.h"
#include "renderPostFxBokehDof.h"
#include "renderPostFxSurfaceFlow.h"