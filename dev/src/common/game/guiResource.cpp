/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "guiObject.h"
#include "guiResource.h"

IMPLEMENT_ENGINE_CLASS( IGuiResource );
IMPLEMENT_ENGINE_CLASS( IGuiResourceBlock );

//////////////////////////////////////////////////////////////////////////
// IGuiResourceBlock
//////////////////////////////////////////////////////////////////////////
#ifndef NO_EDITOR_GRAPH_SUPPORT
void IGuiResourceBlock::OnRebuildSockets()
{
	TBaseClass::OnRebuildSockets();
}
#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT
EGraphBlockShape IGuiResourceBlock::GetBlockShape() const
{
	return GBS_Slanted;
}
#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT
Color IGuiResourceBlock::GetClientColor() const
{
	return Color::NORMAL;
}
#endif

String IGuiResourceBlock::GetCaption() const
{
	return String::EMPTY;
}

void IGuiResourceBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

//////////////////////////////////////////////////////////////////////////
// IGuiResource
//////////////////////////////////////////////////////////////////////////
void IGuiResource::OnPostLoad()
{
}

Bool IGuiResource::MarkModified()
{
	return TBaseClass::MarkModified();
}

void IGuiResource::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

Vector IGuiResource::GraphGetBackgroundOffset() const
{
	return m_backgroundOffset;
}

void IGuiResource::GraphSetBackgroundOffset( const Vector& offset )
{
	m_backgroundOffset = offset;
}

CObject *IGuiResource::GraphGetOwner()
{
	return this;
}

void IGuiResource::GraphStructureModified()
{
}

