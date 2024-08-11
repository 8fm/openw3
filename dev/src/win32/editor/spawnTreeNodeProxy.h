/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IEdSpawnTreeNode;


// Spawn tree proxy that stands between editor and spawn tree node.
class CEdSpawntreeNodeProxy : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CEdSpawntreeNodeProxy )
protected:
	THandle< CObject >					m_engineObject;
	IEdSpawnTreeNode*					m_spawnNodeInterface;
	Bool								m_isValid;
	Bool								m_isExternal;
	THandle< CEdSpawntreeNodeProxy >	m_instanceHolder;

	CSpawnTreeInstance*	GetProvidedInstanceBuffer();

public:
						CEdSpawntreeNodeProxy()
							: m_isValid( false )						{ EnableReferenceCounting( true ); }
						~CEdSpawntreeNodeProxy();

	void				Initialize( IEdSpawnTreeNode* node, Bool isExternal, CEdSpawntreeNodeProxy* instanceHolder );
	void				Invalidate();

	Bool				IsExternalNode() const							{ ASSERT( m_isValid ); return m_isExternal; }
	IEdSpawnTreeNode*	GetSpawnTreeNode() const						{ ASSERT( m_isValid ); return m_spawnNodeInterface; }
	CObject*			GetEngineObject() const							{ ASSERT( m_isValid ); return m_engineObject.Get(); }
	CEdSpawntreeNodeProxy* GetInstanceHolder() const					{ ASSERT( m_isValid ); return m_instanceHolder.Get(); }
	CSpawnTreeInstance*	GetInstanceBuffer() const;
	
};

BEGIN_CLASS_RTTI( CEdSpawntreeNodeProxy )
	PARENT_CLASS( IScriptable )
END_CLASS_RTTI()