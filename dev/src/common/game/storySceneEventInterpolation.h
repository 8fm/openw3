// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "storySceneEvent.h"
#include "interpolationBasics.h"

/*
Base class for all interpolation events.
*/
class CStorySceneEventInterpolation : public CStorySceneEvent
{
public:
	class CreationArgs
	{
	public:
		String m_eventName;																// Interpolation event name.
		TDynArray< CStorySceneEvent* > m_keyEvents;										// List of key events, sorted by abs start time. Must contain at least two key events.
	};

	CStorySceneEventInterpolation();
	CStorySceneEventInterpolation( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName );
	virtual ~CStorySceneEventInterpolation() override;

	// compiler generated cctor is ok

	virtual Bool Initialize( const CreationArgs& creationArgs );
	void RecalculateBezierHandles( CSceneEventFunctionSimpleArgs& args );

	virtual Uint32 GetNumKeys() const;

	CGUID GetKeyGuid( Uint32 keyIndex ) const;

	void OnKeyChanged( CGUID keyGuid );
	void AttachKey( CStorySceneEvent* keyEvent );
	void DetachKey( CStorySceneEvent* keyEvent );

	void SampleData( Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args ) const;

	EInterpolationMethod GetInterpolationMethod() const;

	EInterpolationEasingStyle GetEaseInStyle() const;
	Float GetEaseInParameter() const;

	EInterpolationEasingStyle GetEaseOutStyle() const;
	Float GetEaseOutParameter() const;

	void SetInterpolationMethod( EInterpolationMethod interpolationMethod );

	void SetEaseInStyle( EInterpolationEasingStyle easeInStyle );
	void SetEaseInParameter( Float easeInParameter );

	void SetEaseOutStyle( EInterpolationEasingStyle easeOutStyle );
	void SetEaseOutParameter( Float easeOutParameter );

	virtual void OnGuidChanged( CGUID oldGuid, CGUID newGuid ) override;

protected:
	virtual void ImplOnKeyChanged( CGUID keyGuid );
	virtual void ImplAttachKey( CStorySceneEvent* keyEvent );
	virtual void ImplDetachKey( CStorySceneEvent* keyEvent );

	virtual void ImplSampleData( Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args ) const;

	virtual void ImplRecalculateBezierHandles( CSceneEventFunctionSimpleArgs& args );

	TDynArray< CGUID > m_keyGuids;

	EInterpolationMethod m_interpolationMethod;		// Interpolation method to use for all keys and for all channels.
													// TODO: Note that classes deriving from CStorySceneEventInterpolation (see interpolationEventClassGenerator.h)
													// allow different interpolation method to be used for each key and for each channel (see m_interpolationTypes
													// member of InterpolationEventClass##Key) but that settings are not yet used.

	EInterpolationEasingStyle m_easeInStyle;
	Float m_easeInParameter;

	EInterpolationEasingStyle m_easeOutStyle;
	Float m_easeOutParameter;

	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventInterpolation, CStorySceneEventDuration )
};

BEGIN_CLASS_RTTI( CStorySceneEventInterpolation )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY( m_keyGuids )
	PROPERTY_EDIT( m_interpolationMethod, TXT("Interpolation method.") );
	PROPERTY_EDIT( m_easeInStyle, TXT("Ease in style.") );
	PROPERTY_EDIT_RANGE( m_easeInParameter, TXT("Ease in parameter, range (0.0, 1.0f)."), 0.0f, 1.0f );
	PROPERTY_EDIT( m_easeOutStyle, TXT("Ease out style.") );
	PROPERTY_EDIT_RANGE( m_easeOutParameter, TXT("Ease out parameter, range (0.0, 1.0f)."), 0.0f, 1.0f );
END_CLASS_RTTI()

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE EInterpolationMethod CStorySceneEventInterpolation::GetInterpolationMethod() const
{
	return m_interpolationMethod;
}

RED_INLINE EInterpolationEasingStyle CStorySceneEventInterpolation::GetEaseInStyle() const
{
	return m_easeInStyle;
}

RED_INLINE Float CStorySceneEventInterpolation::GetEaseInParameter() const
{
	return m_easeInParameter;
}

RED_INLINE EInterpolationEasingStyle CStorySceneEventInterpolation::GetEaseOutStyle() const
{
	return m_easeOutStyle;
}

RED_INLINE Float CStorySceneEventInterpolation::GetEaseOutParameter() const
{
	return m_easeOutParameter;
}

RED_INLINE void CStorySceneEventInterpolation::SetInterpolationMethod( EInterpolationMethod interpolationMethod )
{
	m_interpolationMethod = interpolationMethod;
}

RED_INLINE void CStorySceneEventInterpolation::SetEaseInStyle( EInterpolationEasingStyle easeInStyle )
{
	m_easeInStyle = easeInStyle;
}

RED_INLINE void CStorySceneEventInterpolation::SetEaseInParameter( Float easeInParameter )
{
	m_easeInParameter = easeInParameter;
}

RED_INLINE void CStorySceneEventInterpolation::SetEaseOutStyle( EInterpolationEasingStyle easeOutStyle )
{
	m_easeOutStyle = easeOutStyle;
}

RED_INLINE void CStorySceneEventInterpolation::SetEaseOutParameter( Float easeOutParameter )
{
	m_easeOutParameter = easeOutParameter;
}
