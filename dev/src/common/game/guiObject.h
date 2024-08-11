/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashValue;
class CFlashValueStorage;

//////////////////////////////////////////////////////////////////////////
// CGuiObject
//////////////////////////////////////////////////////////////////////////
class CGuiObject : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS_WITH_ALLOCATOR( CGuiObject, CObject, MC_GUI );

public:
	//! CObject functions
	virtual void		OnSerialize( IFile& file ) override;

public:
	virtual CGuiObject* GetChild( const String& childName ) { return nullptr; }

public:
	virtual void		Tick( float timeDelta ) {}

public:
	virtual Bool		RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage )=0;
	virtual Bool		UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage )=0;

public:
	virtual Bool		InitWithFlashSprite( const CFlashValue& flashSprite );
	virtual Bool		IsInitWithFlashSprite() const;

public:
	virtual Bool		RegisterRenderTarget( const String& targetName, Uint32 width, Uint32 height );
	virtual Bool		UnregisterRenderTarget( const String& targetName );

public:
						CGuiObject();

	virtual				~CGuiObject() {}
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_ABSTRACT_CLASS_RTTI( CGuiObject );
PARENT_CLASS( CObject );
END_CLASS_RTTI();