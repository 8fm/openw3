/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "flashStatusListener.h"

//////////////////////////////////////////////////////////////////////////
// CName declarations
//////////////////////////////////////////////////////////////////////////

RED_DECLARE_NAME( OnFlashLoaded );
RED_DECLARE_NAME( OnFlashUnloaded );

//////////////////////////////////////////////////////////////////////////
// CFlashProxyComponent
//////////////////////////////////////////////////////////////////////////
class CFlashProxyComponent : public CComponent, IFlashStatusListener
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CFlashProxyComponent, CComponent );
	
public:
	//! CObject functions
	virtual void OnSerialize( IFile& file ) override;
	//virtual void OnScriptReloaded();
	//virtual void OnScriptThreadKilled();

public:
	//! IFlashStatusListener functions
	virtual void OnFlashLoaded() override;
	virtual void OnFlashUnloaded() override;
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( CFlashProxyComponent, CComponent );
