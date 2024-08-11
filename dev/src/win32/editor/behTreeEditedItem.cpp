#include "build.h"
#include "behTreeEditedItem.h"

#include "../../common/game/behTree.h"
#include "../../common/game/behTreeNode.h"
#include "../../common/core/diskFile.h"

////////////////////////////////////////////////////////////////////////////////
// IBehTreeEditedItem
////////////////////////////////////////////////////////////////////////////////
CBehTree* IBehTreeEditedItem::GetRes()
{
	return NULL;
}
void IBehTreeEditedItem::SetRes( CBehTree* res )
{
}
Bool IBehTreeEditedItem::CheckOut()
{
	return false;
}
////////////////////////////////////////////////////////////////////////////////
// CBehTreeEditedResource
////////////////////////////////////////////////////////////////////////////////
CBehTreeEditedResource::CBehTreeEditedResource( CBehTree* res )
	: m_res( res )
{
	m_res->AddToRootSet();
}
CBehTreeEditedResource::~CBehTreeEditedResource()
{
	if ( m_res )
	{
		m_res->RemoveFromRootSet();
	}
}

void CBehTreeEditedResource::MarkModified()
{
	m_res->MarkModified();
}
void CBehTreeEditedResource::Save()
{
	m_res->Save();
}
void CBehTreeEditedResource::SetRootNode( IBehTreeNodeDefinition* node )
{
	m_res->SetRootNode( node );
}
IBehTreeNodeDefinition* CBehTreeEditedResource::GetRootNode()
{
	return m_res->GetRootNode();
}
Bool CBehTreeEditedResource::CanModify()
{
	return m_res->CanModify();
}
void CBehTreeEditedResource::GetName( wxString& outName, Bool shortName )
{
	CDiskFile* file = m_res->GetFile();
	if ( shortName )
	{
		outName = file->GetFileName().AsChar() + wxString(TXT(" - Behavior Tree Editor"));
	}
	else
	{
		outName = file->GetFileName().AsChar()
			+ wxString( TXT(" - ") ) + file->GetDepotPath().AsChar()
			+ wxString( TXT(" - Behavior Tree Editor") );
	}
}
void CBehTreeEditedResource::OnResourceReload( CResource* res )
{

}
CBehTree* CBehTreeEditedResource::GetRes()
{
	return m_res;
}
void CBehTreeEditedResource::SetRes( CBehTree* res )
{
	m_res = res;
}
Bool CBehTreeEditedResource::CheckOut()
{
	if ( m_res )
	{
		return m_res->GetFile()->CheckOut();
	}
	return false;
}
CObject* CBehTreeEditedResource::GetParentObject()
{
	return m_res;
}


////////////////////////////////////////////////////////////////////////////////
// CBehTreeEditedProperty
////////////////////////////////////////////////////////////////////////////////
CBehTreeEditedProperty::CBehTreeEditedProperty( CObject* parentObject, CProperty* prop )
	: m_object( parentObject )
	, m_property( prop )
	, m_name( parentObject->GetFriendlyName().AsChar() )
{
	RefreshRootNode();
}
CBehTreeEditedProperty::~CBehTreeEditedProperty()
{
}
void CBehTreeEditedProperty::MarkModified()
{
	// TODO: Get res editor for current root object and mark it modified
}
void CBehTreeEditedProperty::Save()
{
	// TODO: Get res editor for current root object
}
void CBehTreeEditedProperty::SetRootNode( IBehTreeNodeDefinition* node )
{
	CObject* obj = m_object.Get();
	if ( obj )
	{
		if ( m_property->GetType()->GetType() == RT_Pointer )
		{
			m_property->Set( obj, &node );
		}
		else if ( m_property->GetType()->GetType() == RT_Handle )
		{
			THandle< IBehTreeNodeDefinition > nodeHandle( node );
			m_property->Set( obj, &nodeHandle );
		}

		m_rootNode = node;
	}
}
IBehTreeNodeDefinition* CBehTreeEditedProperty::GetRootNode()
{
	IBehTreeNodeDefinition* def = m_rootNode.Get();
	if ( !def )
	{
		RefreshRootNode();
		def = m_rootNode.Get();
	}
	return def;
}
Bool CBehTreeEditedProperty::CanModify()
{
	return true;
}
void CBehTreeEditedProperty::GetName( wxString& outName, Bool shortName )
{
	outName = m_name;
}
void CBehTreeEditedProperty::OnResourceReload( CResource* res )
{
	m_rootNode = NULL;
}
CObject* CBehTreeEditedProperty::GetParentObject()
{
	return m_object.Get();
}
void CBehTreeEditedProperty::RefreshRootNode()
{
	m_rootNode = NULL;
	CObject* obj = m_object.Get();
	if ( !obj )
	{
		return;
	}

	if ( m_property->GetType()->GetType() == RT_Pointer )
	{
		IBehTreeNodeDefinition* def = NULL;
		m_property->Get( obj, &def );
		m_rootNode = def;
	}
	else if ( m_property->GetType()->GetType() == RT_Handle )
	{
		m_property->Get( obj, &m_rootNode );
	}
}
