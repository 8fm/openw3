/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "flashPlayer.h"

#ifdef USE_SCALEFORM
# include "flashPlayerScaleform.h"
#endif

//////////////////////////////////////////////////////////////////////////
//SFlashMovieInitParams
//////////////////////////////////////////////////////////////////////////
SFlashMovieInitParams::SFlashMovieInitParams()
	: m_layer( 0 )
	, m_renderGroup( eFlashMovieRenderGroup_Default )
	, m_attachOnStart( true )
	, m_notifyPlayer( true )
	, m_waitForLoadFinish( false )
{}

//////////////////////////////////////////////////////////////////////////
// IFlashReference
//////////////////////////////////////////////////////////////////////////
IFlashReference::~IFlashReference()
{
	ASSERT( m_refCount == 0 );
}

IFlashReference::IFlashReference()
	: m_refCount( 1 )
{
}

Int32 IFlashReference::AddRef()
{
	++m_refCount;
	return m_refCount;
}

Int32 IFlashReference::Release()
{
	ASSERT( m_refCount > 0 );
	Int32 nonMemberRefCount = m_refCount - 1; // Save before "delete this"
	if ( --m_refCount == 0 )
	{
		OnDestroy();
		delete this;
	}
	return nonMemberRefCount;
}

void* IFlashReference::operator new( size_t size )
{
	return RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_GUI, size );
}

void IFlashReference::operator delete( void *ptr )
{	
	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_GUI, ptr );
}

//////////////////////////////////////////////////////////////////////////
// CFlashPlayerNull
//////////////////////////////////////////////////////////////////////////
class CFlashPlayerNull : public CFlashPlayer
{
public:
	virtual Bool Init( IViewport* viewport ) { return true; }
	virtual void Tick( Float timeDelta, const Rect& viewport ) {}
	virtual void Shutdown() {}
	virtual Bool IsInitialized() const { return true; }

public:
	virtual Bool OnViewportInput( EInputKey key, EInputAction action, Float data ) { return false; }
	virtual void OnViewportGenerateFragments( CRenderFrame* frame ) {}

public:
	virtual Bool RegisterStatusListener( IFlashPlayerStatusListener* statusListener ) { return true; }
	virtual Bool UnregisterStatusListener( IFlashPlayerStatusListener* statusListener ) { return true; }

public:
	virtual Bool RegisterExternalInterface( const String& methodName, IFlashExternalInterfaceHandler* flashExternalInterfaceHandler ) { return true; }
	virtual Bool UnregisterExternalInterface( const String& methodName ) { return true; }

public:
	virtual Bool RegisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler ) { return true; }
	virtual Bool UnregisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler ) { return true; }

public:
	virtual CFlashMovie* CreateMovie( const TSoftHandle< CSwfResource >& swfHandle, const SFlashMovieInitParams& initParams ) { return nullptr; }

public:
	virtual void OnLanguageChange() {}
};

//////////////////////////////////////////////////////////////////////////
// CFlashPlayer
//////////////////////////////////////////////////////////////////////////
CFlashPlayer* CFlashPlayer::CreateFlashPlayerInstance()
{
#ifdef USE_SCALEFORM
	return new CFlashPlayerScaleform;
#else
	return new CFlashPlayerNull;
#endif
}
