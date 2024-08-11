/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeStateSerializer.h"

#include "../engine/pathlibSimpleBuffers.h"

#include "generalTreeOperations.h"

typedef TGeneralTreeSerializer< TGeneralTreePersistantIterator< TGeneralTreeIteratorData< CBehTreeGeneralDescription, SGeneralTreeIteratorStackData< CBehTreeGeneralDescription::Node > > > > CGeneralTreeSerializer;

CBehTreeStateSerializer::CBehTreeStateSerializer( IBehTreeNodeInstance* node )
	: m_desc( node )
{

}

Bool CBehTreeStateSerializer::IsSaving()
{
	CGeneralTreeSerializer serializer( m_desc );

	return serializer.IsSavingTreeState();
}

Bool CBehTreeStateSerializer::Save( IGameSaver* writer )
{
	CGeneralTreeSerializer serializer( m_desc );
	serializer.SaveTreeState( writer );
	return true;
}
Bool CBehTreeStateSerializer::Load( IGameLoader* reader )
{
	CGeneralTreeSerializer serializer( m_desc );
	return serializer.LoadTreeState( reader );
}