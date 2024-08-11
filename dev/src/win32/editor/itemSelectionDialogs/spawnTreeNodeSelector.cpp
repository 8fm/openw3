#include "build.h"
#include "spawnTreeNodeSelector.h"
#include "../classHierarchyMapper.h"

#include "../../../common/game/spawnTreeNode.h"

namespace
{
	CClassHierarchyMapper GetHierarchy( CClass* rootClass )
	{
		class Namer : public CClassHierarchyMapper::CClassNaming
		{
			void GetClassName( CClass* classId, String& outName ) const override
			{
				if ( classId->IsAbstract() == false )
				{
					CObject *const object = classId->GetDefaultObject< CObject >();
					if ( object )
					{
						IEdSpawnTreeNode* edSpawnTreeNode = dynamic_cast< IEdSpawnTreeNode* >( object );

						if ( edSpawnTreeNode )
						{
							outName = edSpawnTreeNode->GetEditorFriendlyName();
							return;
						}
					}
				}
				outName = classId->GetName().AsString();
			}
		};

		CClassHierarchyMapper mapper;
		CClassHierarchyMapper::MapHierarchy( rootClass, mapper, Namer(), true );
		return mapper;
	}
}

CEdSpawnTreeNodeSelectorDialog::CEdSpawnTreeNodeSelectorDialog( wxWindow* parent, CClass* rootClass, IEdSpawnTreeNode *const activeSpawnTreeNode, ESpawnTreeType spawnTreeType, IEdSpawnTreeNode* childSpawnTreeNode )
	: CEdMappedClassSelectorDialog( parent, GetHierarchy( rootClass ), TXT( "/Frames/SpawnTreeNodeSelectorDialog" ), CObject::GetStaticClass() )
	, m_activeSpawnTreeNode( activeSpawnTreeNode )
	, m_childSpawnTreeNode( childSpawnTreeNode )
	, m_spawnTreeType( spawnTreeType )
{
}

Bool CEdSpawnTreeNodeSelectorDialog::IsSelectable( CClass* classId ) const 
{
	if ( false == __super::IsSelectable( classId ) )	 // IsAbstract()
	{
		return false;
	}
	
	if ( false == m_activeSpawnTreeNode->IsPossibleChildClass( classId, m_spawnTreeType ) )
	{
		return false;
	}
	if ( m_childSpawnTreeNode )
	{
		CObject* defaultObj = classId->GetDefaultObject< CObject >();
		if ( !defaultObj )
		{
			return false;
		}
		IEdSpawnTreeNode* defaultSpawnTreeNode = dynamic_cast< IEdSpawnTreeNode* >( defaultObj );
		if ( !defaultSpawnTreeNode )
		{
			return false;
		}
		if ( !defaultSpawnTreeNode->IsPossibleChildClass( m_childSpawnTreeNode->AsCObject()->GetClass(), m_spawnTreeType ) ) 
		{
			return false;
		}
	}
	return true;
}