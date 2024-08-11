/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"

class CUndoCreateDestroy : public IUndoStep
{
	CUndoCreateDestroy() {}
	DECLARE_ENGINE_CLASS( CUndoCreateDestroy, IUndoStep, 0 );

public:
	virtual String GetName();
	virtual String GetTarget();

	static void CreateStep( CEdUndoManager* undoManager, CEntity * entity, Bool undoCreation );
	static void CreateStep( CEdUndoManager* undoManager, CComponent * component, Bool undoCreation, Bool updateStreaming );
	static void CreateStep( CEdUndoManager* undoManager, IAttachment * attachment, Bool undoCreation );
	static void FinishStep( CEdUndoManager* undoManager, const String& stepName = String::EMPTY );

	virtual void OnObjectRemoved( CObject *object ) override;

private:
	struct EntityInfo 
	{
 		CEntity *				m_entity;
		CEntityGroup*			m_parentGroup;
		THandle< CLayer >		m_layer;

		EntityInfo() {}
	};
	struct ComponentInfo
	{
		CComponent *			m_component;
		THandle< CEntity >		m_entity;

		ComponentInfo() : m_component( NULL ) {}
	};
	struct AttachmentInfo 
	{
		THandle< CEntity >		m_entity;
		IAttachment *			m_attachment;
		String					m_parentComponent;
		String					m_childComponent;

		AttachmentInfo() : m_attachment( NULL ) {}
	};
	
	struct Data
	{
		TDynArray< EntityInfo >		m_entities;
		TDynArray< ComponentInfo >	m_components;
		TDynArray< AttachmentInfo >	m_attachments;
	};

	Data m_createdData;
	Data m_removedData;

	String	  m_targetString;
	String	  m_stepName;

	static Data& PrepareStep( CEdUndoManager* undoManager, CObject* object, Bool undoCreation );

	void DoRemoveObjectOn( Data& data, CObject *object );
	void DoCreationOn( Data& data );
	void DoDeletionOn( Data& data );
	void BuildTargeStringOn( Data& data );

	virtual void DoUndo();
	virtual void DoRedo();

	CUndoCreateDestroy( CEdUndoManager& undoManager, Bool undoCreation )
		: IUndoStep ( undoManager )
	{}

	// Object serialization interface
	virtual void OnSerialize( IFile& file );
};

BEGIN_CLASS_RTTI( CUndoCreateDestroy )
	PARENT_CLASS( IUndoStep );
END_CLASS_RTTI();
