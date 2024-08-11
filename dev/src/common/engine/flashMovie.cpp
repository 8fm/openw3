/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "flashMovie.h"

//////////////////////////////////////////////////////////////////////////
// CFlashMovie
//////////////////////////////////////////////////////////////////////////
CFlashMovie::CFlashMovie( const SFlashMovieLayerInfo& layerInfo, Uint32 flags /*= ISF_All*/ )
	: m_userData( 0 )
	, m_layerInfo( layerInfo )
	, m_inputSourceFlags( flags )
	, m_catchupFrames( 0 )
	, m_externalInterfaceOverride( nullptr )
	, m_status( Uninitialized )
{
}

void CFlashMovie::SetLayerInfo( const SFlashMovieLayerInfo& layerInfo )
{
	if ( m_layerInfo != layerInfo )
	{
		m_layerInfo = layerInfo;
		OnLayerInfoChanged( layerInfo );
	}
}

void CFlashMovie::SetIgnoreKeys( const TDynArray< EInputKey >& keysToSet )
{
	ForEach( keysToSet.Begin(), keysToSet.End(), [ this ]( EInputKey key ) { m_ignoreKeys.Set( key ); } );
}

void CFlashMovie::ClearIgnoreKeys( const TDynArray< EInputKey >& keysToClear )
{
	ForEach( keysToClear.Begin(), keysToClear.End(), [ this ]( EInputKey key ) { m_ignoreKeys.Clear( key ); } );
}
