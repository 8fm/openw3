/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/*
#include "storySceneSectionPlayingPlan.h"


class CStoryScenePlayer;
class CStorySceneSection;

class IStorySceneSectionOverrideCondition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IStorySceneSectionOverrideCondition, CObject );

public:
	virtual Bool ShouldOverride( const CStoryScenePlayer* scenePlayer ) const { return false; }
};

BEGIN_CLASS_RTTI( IStorySceneSectionOverrideCondition );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneSectionDistanceOverrideCondition : public IStorySceneSectionOverrideCondition
{
	DECLARE_ENGINE_CLASS( CStorySceneSectionDistanceOverrideCondition, IStorySceneSectionOverrideCondition, 0 )

public:
	virtual Bool ShouldOverride( const CStoryScenePlayer* scenePlayer ) const;

protected:
	Float	m_distance;
};

BEGIN_CLASS_RTTI( CStorySceneSectionDistanceOverrideCondition );
	PARENT_CLASS( IStorySceneSectionOverrideCondition );
	PROPERTY_EDIT_RANGE( m_distance, TXT( "Distance from player" ), 0.0f, 1000.0f );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneSectionEavesdroppingOverrideCondition : public IStorySceneSectionOverrideCondition
{
	DECLARE_ENGINE_CLASS( CStorySceneSectionEavesdroppingOverrideCondition, IStorySceneSectionOverrideCondition, 0 );

public:
	virtual Bool ShouldOverride( const CStoryScenePlayer* scenePlayer ) const;

protected:
	Float	m_distanceFromPlayer;
	Float	m_distanceFromListener;
};

BEGIN_CLASS_RTTI( CStorySceneSectionEavesdroppingOverrideCondition );
	PARENT_CLASS( IStorySceneSectionOverrideCondition );
	PROPERTY_EDIT_RANGE( m_distanceFromPlayer, TXT( "Distance between player and actors within which eavesdropping is not possible" ), 0.0f, 1000.f );
	PROPERTY_EDIT_RANGE( m_distanceFromListener, TXT( "Distance between listener and actors above which eavesdropping is not possible" ), 0.0f, 1000.f );
END_CLASS_RTTI();
*/