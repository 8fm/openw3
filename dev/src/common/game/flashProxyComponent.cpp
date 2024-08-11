/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "flashProxyComponent.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CFlashProxyComponent );

RED_DEFINE_NAME( OnFlashLoaded );
RED_DEFINE_NAME( OnFlashUnloaded );

//////////////////////////////////////////////////////////////////////////
// CFlashProxyComponent
//////////////////////////////////////////////////////////////////////////
void CFlashProxyComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

void CFlashProxyComponent::OnFlashLoaded()
{
	CallEvent( CNAME( OnFlashLoaded ) );
}

void CFlashProxyComponent::OnFlashUnloaded()
{
	CallEvent( CNAME( OnFlashUnloaded ) );
}
