/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderElement.h"
#include "renderHelpers.h"
#include "renderMaterial.h"
#include "../engine/renderFrame.h"

#ifdef DEBUG_RENDER_ELEMENT_STATS
Red::Threads::CAtomic< Int32 > GNumElements = 0;
#endif

IRenderElement::IRenderElement( ERenderElementType type, IRenderProxyDrawable* proxy, const IMaterial* material )
	: m_proxy( proxy )
	, m_material( NULL )
	, m_parameters( NULL )
	, m_batchNext( NULL )
	, m_sortGroup( RSG_Unlit )
	, m_type( type )
{
	ExtractMaterial( material );

	RED_ASSERT( m_material );
	RED_ASSERT( m_parameters );

#ifdef DEBUG_RENDER_ELEMENT_STATS
	GNumElements.Increment();
#endif
}

IRenderElement::IRenderElement( ERenderElementType type, IRenderProxyDrawable* proxy, CRenderMaterial* material, CRenderMaterialParameters* parameters )
	: m_proxy( proxy )
	, m_material( material )
	, m_parameters( parameters )
	, m_batchNext( NULL )
	, m_sortGroup( material->GetRenderSortGroup() )
	, m_type( type )
{
	RED_ASSERT( m_material );
	RED_ASSERT( m_parameters );

	m_material->AddRef();
	m_parameters->AddRef();

#ifdef DEBUG_RENDER_ELEMENT_STATS
	GNumElements.Increment();
#endif
}

IRenderElement::~IRenderElement()
{
	SAFE_RELEASE( m_material );
	SAFE_RELEASE( m_parameters );

#ifdef DEBUG_RENDER_ELEMENT_STATS
	GNumElements.Decrement();
#endif
}

void IRenderElement::ExtractMaterial( const IMaterial* material )
{
	if ( material )
	{
		// Get material definition ( shader )
		const IMaterialDefinition* definition = const_cast< IMaterial* >( material )->GetMaterialDefinition();
		if ( definition )
		{
			// Extract material settings
			ExtractRenderResource( definition, m_material );
			ExtractRenderResource( material, m_parameters );

			// Get sort group to use for this element
			m_sortGroup = definition->GetRenderingSortGroup();
		}
	}
}

Bool IsSortGroupCastingShadow( ERenderingSortGroup group )
{
	switch ( group )
	{
	case RSG_LitOpaque:
	case RSG_LitOpaqueWithEmissive:
	case RSG_Forward:
	case RSG_Hair:
	case RSG_Skin:
	case RSG_Unlit:
		return true;
	}

	return false;
}