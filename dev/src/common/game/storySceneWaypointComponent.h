/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CStorySceneWaypointComponent : public CComponent, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CStorySceneWaypointComponent, CComponent, 0 );

private:
	CName										m_dialogsetName;

	THandle< CStorySceneDialogset >				m_dialogset;
	Bool										m_showCameras;
	Bool										m_useDefaultDialogsetPositions;

public:
	CStorySceneWaypointComponent();

	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

public:
	virtual void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties );

	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnPostLoad();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

private:
	void RefreshPositions();
};


BEGIN_CLASS_RTTI( CStorySceneWaypointComponent );
	PARENT_CLASS( CComponent );
	//PROPERTY_CUSTOM_EDIT( m_dialogsetName, TXT( "Name of dialogset for preview" ), TXT( "2daValueSelection" ) );
	PROPERTY_NOT_COOKED( m_dialogsetName );
	PROPERTY_EDIT_NOT_COOKED( m_dialogset, TXT( "Scene Dialogset" ) );
	PROPERTY_EDIT_NOT_COOKED( m_showCameras, TXT( "Show Cameras positions" ) );
	PROPERTY_EDIT_NOT_COOKED( m_useDefaultDialogsetPositions, TXT( "Always show dialogset positions" ) )
END_CLASS_RTTI();