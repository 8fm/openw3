/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../engine/flashValue.h"
#include "../engine/flashValueStorage.h"

#include "guiObject.h"
#include "guiResource.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CGuiObject );

//////////////////////////////////////////////////////////////////////////
// CGuiObject
//////////////////////////////////////////////////////////////////////////
CGuiObject::CGuiObject()
{
}

void CGuiObject::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

Bool CGuiObject::InitWithFlashSprite( const CFlashValue& flashSprite )
{
	RED_UNUSED( flashSprite );
	return false;
}

Bool CGuiObject::IsInitWithFlashSprite() const
{
	return false;
}

Bool CGuiObject::RegisterRenderTarget( const String& sceneName, Uint32 width, Uint32 height )
{
	RED_UNUSED( sceneName );
	RED_UNUSED( width );
	RED_UNUSED( height );

	return false;
}

Bool CGuiObject::UnregisterRenderTarget( const String& sceneName )
{
	RED_UNUSED( sceneName );

	return false;
}
