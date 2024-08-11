// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventInterpolation.h"
#include "storySceneElement.h"
#include "storySceneSection.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventInterpolation  );

/*
Ctor.
*/
CStorySceneEventInterpolation::CStorySceneEventInterpolation()
: m_interpolationMethod( EInterpolationMethod::IM_Bezier )
, m_easeInStyle( EInterpolationEasingStyle::IES_Smooth )
, m_easeInParameter( 1.0f / 3.0f )
, m_easeOutStyle( EInterpolationEasingStyle::IES_Smooth )
, m_easeOutParameter( 1.0f / 3.0f )
{}

/*
Ctor.
*/
CStorySceneEventInterpolation::CStorySceneEventInterpolation( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName )
: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
, m_interpolationMethod( EInterpolationMethod::IM_Bezier )
, m_easeInStyle( EInterpolationEasingStyle::IES_Smooth )
, m_easeInParameter( 1.0f / 3.0f )
, m_easeOutStyle( EInterpolationEasingStyle::IES_Smooth )
, m_easeOutParameter( 1.0f / 3.0f )
{}

/*
Dtor.
*/
CStorySceneEventInterpolation::~CStorySceneEventInterpolation()
{}

Bool CStorySceneEventInterpolation::Initialize( const CreationArgs& creationArgs )
{
	// We should never be here. CStorySceneEventInterpolation should never be created
	// as standalone object. This class acts as interface. Initialize() would be
	// pure if our RTII supported this.
	ASSERT( false );
	return false;
}

void CStorySceneEventInterpolation::RecalculateBezierHandles( CSceneEventFunctionSimpleArgs& args )
{
	ImplRecalculateBezierHandles( args );
}

Uint32 CStorySceneEventInterpolation::GetNumKeys() const
{
	return 0;
}

CGUID CStorySceneEventInterpolation::GetKeyGuid( Uint32 keyIndex ) const
{
	return m_keyGuids[ keyIndex ];
}

void CStorySceneEventInterpolation::OnKeyChanged( CGUID keyGuid )
{
	ImplOnKeyChanged( keyGuid );
}

void CStorySceneEventInterpolation::AttachKey( CStorySceneEvent* keyEvent )
{
	ImplAttachKey( keyEvent );
}

void CStorySceneEventInterpolation::DetachKey( CStorySceneEvent* keyEvent )
{
	ImplDetachKey( keyEvent );
}

void CStorySceneEventInterpolation::SampleData( Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args ) const
{
	ImplSampleData( timeStep, outData, args );
}

void CStorySceneEventInterpolation::OnGuidChanged( CGUID oldGuid, CGUID newGuid )
{
	CStorySceneEvent::OnGuidChanged( oldGuid, newGuid );

	for( Uint32 i = 0, numKeys = m_keyGuids.Size(); i < numKeys; ++i )
	{
		if( m_keyGuids[ i ] == oldGuid )
		{
			m_keyGuids[ i ] = newGuid;
		}
	}
}

void CStorySceneEventInterpolation::ImplOnKeyChanged( CGUID keyGuid )
{
	// This function should be pure.
	ASSERT( false );
}

void CStorySceneEventInterpolation::ImplAttachKey( CStorySceneEvent* keyEvent )
{
	// This function should be pure.
	ASSERT( false );
}

void CStorySceneEventInterpolation::ImplDetachKey( CStorySceneEvent* keyEvent )
{
	// This function should be pure.
	ASSERT( false );
}

void CStorySceneEventInterpolation::ImplSampleData( Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args ) const
{
	// This function should be pure.
	ASSERT( false );
}

void CStorySceneEventInterpolation::ImplRecalculateBezierHandles( CSceneEventFunctionSimpleArgs& args )
{
	// This function should be pure.
	ASSERT( false );
}
