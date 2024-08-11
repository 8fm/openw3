/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CTeleporter : public CEntity
{
	DECLARE_ENGINE_CLASS( CTeleporter, CEntity, 0 );

private:
	TagList								m_destinationNodesTags;
	TagList								m_teleportedActorsTags;

	THashSet< CActor* >					m_teleportedActors;

public:
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator );

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

private:
	void funcUseTeleporter( CScriptStackFrame& stack, void* result );
};
BEGIN_CLASS_RTTI( CTeleporter );
	PARENT_CLASS( CEntity );
	PROPERTY_CUSTOM_EDIT( m_destinationNodesTags, TXT( "Tags of the destination nodes" ), TXT( "TagListEditor" ) );
	PROPERTY_CUSTOM_EDIT( m_teleportedActorsTags, TXT( "Tags of actors that this teleporter will teleport" ), TXT( "TagListEditor" ) );
	NATIVE_FUNCTION( "UseTeleporter", funcUseTeleporter );
END_CLASS_RTTI();
