/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/renderObject.h"

// Forward declarations
class IRenderProxyDrawable;
class CRenderMaterial;
class CRenderMaterialParameters;
class IRenderElement;
enum ERenderingSortGroup : CEnum::TValueType;


/// Type of render element
enum ERenderElementType
{
	RET_Invalid,
	RET_MeshChunk,	
	RET_ParticlesEmitter,
	RET_BrushFace,
	RET_Apex,
	RET_Swarm
};

/// Rendering element, renderable part of proxy
class IRenderElement : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderElement )
protected:
	IRenderProxyDrawable*			m_proxy;		//!< Owner, the proxy that created this rendering element
	CRenderMaterial*				m_material;		//!< Rendering material, every element has one
	CRenderMaterialParameters*		m_parameters;	//!< Material parameters, every element has one
	IRenderElement*					m_batchNext;	//!< Next in batch
	ERenderElementType				m_type:8;		//!< Type of render element
	ERenderingSortGroup				m_sortGroup:8;	//!< Assigned sort group, to be removed

public:
	//! Get the proxy
	RED_INLINE IRenderProxyDrawable* GetProxy() const { return m_proxy; }

	//! Get the type of element
	RED_INLINE ERenderElementType GetType() const { return m_type; }

	//! Get next element in batch
	RED_INLINE IRenderElement* GetBatchNext() const { return m_batchNext; }

	//! Set new batched element
	RED_INLINE void SetBatchNext( IRenderElement* next ) { m_batchNext = next; }

	//! Link to batch list
	RED_INLINE void LinkToBatchList( IRenderElement*& list ) { m_batchNext = list; list = this; }

	//! Get the assigned sort group
	RED_INLINE ERenderingSortGroup GetSortGroup() const { return m_sortGroup; }

	//! Get the material
	RED_INLINE CRenderMaterial* GetMaterial() const { return m_material; }

	//! Get the material rendering parameters
	RED_INLINE CRenderMaterialParameters* GetMaterialParams() const { return m_parameters; }

public:
	IRenderElement( ERenderElementType type, IRenderProxyDrawable* proxy, const IMaterial* material );
	IRenderElement( ERenderElementType type, IRenderProxyDrawable* proxy, CRenderMaterial* material, CRenderMaterialParameters* parameters );
	virtual ~IRenderElement();

protected:
	RED_INLINE void ExtractMaterial( const IMaterial* material );
};

Bool IsSortGroupCastingShadow( ERenderingSortGroup group );
