/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/flashReference.h"
#include "../../common/engine/flashValue.h"
#include "../../common/engine/flashFunction.h"
#include "../../common/game/guiObject.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovie;
class CFlashValueStorage;
class CFlashBindingAdapter;
class CFlashMovieAdapter;

//////////////////////////////////////////////////////////////////////////
// CFlashSpriteAdapter
//////////////////////////////////////////////////////////////////////////
class CFlashSpriteAdapter : public IFlashReference
{
private:
	typedef TDynArray< CFlashBindingAdapter* >	TFlashBindingAdapterList;

private:
	TFlashBindingAdapterList					m_flashBindingAdapterList;

private:
	CFlashValue									m_flashSprite;
	CFlashValueStorage*							m_flashValueStorage;
	CFlashMovie*								m_flashMovie;
	CFlashMovieAdapter*							m_flashMovieAdapter;

private:
	THandle< CGuiObject >						m_adaptedObject;

public:
												CFlashSpriteAdapter( CFlashMovie* flashMovie, const CFlashValue& flashSprite, CGuiObject* adaptedObject );
	virtual										~CFlashSpriteAdapter();
	virtual Bool								Init();
	virtual void								OnDestroy() override;

public:
	void										RegisterDataBinding( const String& key, const CFlashValue& flashClosure, const CFlashValue& flashBoundObject, const CFlashValue& flashIsGlobal );
	void										UnregisterDataBinding( const String& key, const CFlashValue& flashClosure, const CFlashValue& flashBoundObject );
	void										RegisterChild( const String& childName, const CFlashValue& flashChild );
	void										UnregisterChild( const CFlashValue& flashChild );
	void										CallGameEvent( const String& eventName, const CFlashValue& eventArgs );
	void										RegisterRenderTarget( const String& targetName, Uint32 width, Uint32 height );
	void										UnregisterRenderTarget( const String& targetName );

public:
	RED_INLINE CFlashMovie*					GetFlashMovie() const { return m_flashMovie; }
	RED_INLINE const CFlashValue&				GetFlashSprite() const { return m_flashSprite; }
	RED_INLINE CFlashValue&					GetFlashSprite() { return m_flashSprite; }
	RED_INLINE CFlashValueStorage*			GetFlashValueStorage() const { return m_flashValueStorage; }
	RED_INLINE CObject*						GetAdaptedObject() const { ASSERT( m_adaptedObject.Get() ); return m_adaptedObject.Get(); }

private:
	void										RegisterFlashFunctions( CFlashValue& flashSprite );
};