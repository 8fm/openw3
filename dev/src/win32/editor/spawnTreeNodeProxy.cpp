/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeNodeProxy.h"

IMPLEMENT_ENGINE_CLASS( CEdSpawntreeNodeProxy )


CEdSpawntreeNodeProxy::~CEdSpawntreeNodeProxy()
{

}
void CEdSpawntreeNodeProxy::Initialize( IEdSpawnTreeNode* node, Bool isExternal, CEdSpawntreeNodeProxy* instanceHolder )
{
	m_spawnNodeInterface = node;
	m_engineObject = node->AsCObject();
	m_isValid = true;
	m_isExternal = isExternal;
	m_instanceHolder = instanceHolder;
}

void CEdSpawntreeNodeProxy::Invalidate()
{
	m_isValid = false;
	m_engineObject = nullptr;
	m_instanceHolder = nullptr;
}

CSpawnTreeInstance*	CEdSpawntreeNodeProxy::GetProvidedInstanceBuffer()
{
	ASSERT( m_isValid );
	CSpawnTreeInstance* buffer = m_instanceHolder ? m_instanceHolder->GetProvidedInstanceBuffer() : nullptr;
	if ( m_spawnNodeInterface->HoldsInstanceBuffer() )
	{
		buffer = m_spawnNodeInterface->GetInstanceBuffer( buffer );
	}
	return buffer;
}

CSpawnTreeInstance* CEdSpawntreeNodeProxy::GetInstanceBuffer() const
{
	ASSERT( m_isValid );
	return m_instanceHolder ? m_instanceHolder->GetProvidedInstanceBuffer() : nullptr;
}