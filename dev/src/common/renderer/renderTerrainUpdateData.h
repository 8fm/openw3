/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/renderObject.h"
#include "../engine/terrainStructs.h"
#include "renderProxyTerrain.h"

struct SClipmapLevelUpdate;
struct SClipmapStampDataUpdate;
struct STerrainTextureParameters;

class CRenderTerrainUpdateData : public IRenderObject
{
protected:
	TDynArray< SClipmapLevelUpdate* >	m_update;				//! Per level updates
	SClipmapStampDataUpdate*			m_stampUpdate;
	Vector								m_textureParams[NUM_TERRAIN_TEXTURES_AVAILABLE*2];	//!< Texture parameters (TODO: only when needed)
	Vector								m_colormapParams;

#ifndef NO_EDITOR
	Bool								m_isEditing;
#endif
	
public:
	IRenderObject*						m_material;

	CRenderTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampUpdate, Vector* colormapParams );
	~CRenderTerrainUpdateData();

#ifndef NO_EDITOR
	void SetIsEditing() { m_isEditing = true; }
	Bool IsEditing() const { return m_isEditing; }
#endif

	const TDynArray< SClipmapLevelUpdate* >&	GetUpdates()		const { return m_update; }
	const Vector*								GetTextureParams()  const { return &m_textureParams[0]; }
	const Vector								GetColormapParams() const { return m_colormapParams; }

	Bool					IsStampUpdateValid()		const { return m_stampUpdate && m_stampUpdate->IsValid(); }
	const Uint16*			GetStampData()				const { return m_stampUpdate ? m_stampUpdate->m_heightData			: NULL;			}
	Uint32					GetStampDataPitch()			const { return m_stampUpdate ? m_stampUpdate->m_heightDataPitch		: 0;			}
	Bool					IsStampDataDirty()			const { return m_stampUpdate ? m_stampUpdate->m_heightDataDirty		: false;		}
	const TColorMapType*	GetStampColorData()			const { return m_stampUpdate ? m_stampUpdate->m_colorData			: NULL;			}
	Uint32					GetStampColorDataPitch()	const { return m_stampUpdate ? m_stampUpdate->m_colorPitch			: 0;			}
	Bool					IsStampColorDataDirty()		const { return m_stampUpdate ? m_stampUpdate->m_colorDataDirty		: false;		}
	const TControlMapType*	GetStampControlData()		const { return m_stampUpdate ? m_stampUpdate->m_controlData			: NULL;			}
	Uint32					GetStampControlDataPitch()	const { return m_stampUpdate ? m_stampUpdate->m_controlPitch		: 0;			}
	Bool					IsStampControlDataDirty()	const { return m_stampUpdate ? m_stampUpdate->m_controlDataDirty	: false;		}
	Vector2					GetStampCenter()			const { return m_stampUpdate ? m_stampUpdate->m_center				: Vector2( 0.0f, 0.0f ); }
	Float					GetStampSize()				const { return m_stampUpdate ? m_stampUpdate->m_size				: 0.0f;			}
	Float					GetStampHeightScale()		const { return m_stampUpdate ? m_stampUpdate->m_heightScale			: 0.0f;			}
	Float					GetStampHeightOffset()		const { return m_stampUpdate ? m_stampUpdate->m_heightOffset		: 0.0f;			}
	Float					GetStampRotation()			const { return m_stampUpdate ? m_stampUpdate->m_radians				: 0.0f;			}
	Bool					GetStampModeAdditive()		const { return m_stampUpdate ? m_stampUpdate->m_additive			: true;			}
	Uint32					GetStampOriginalTexelSize()	const { return m_stampUpdate ? m_stampUpdate->m_originalDataSize	: 0;			}
};
