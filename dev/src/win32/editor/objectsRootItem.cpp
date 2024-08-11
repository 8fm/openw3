/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dynamicPropGroupItem.h"

CObjectsRootItem::CObjectsRootItem( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItemClass( page, parent )
	, m_baseClass( NULL )
{	
}

CObjectsRootItem::~CObjectsRootItem()
{
}

void CObjectsRootItem::SetObjects( const TDynArray< STypedObject > &objects )
{
	// Collect valid objects and determine common base class
	for ( Uint32 i=0; i<objects.Size(); i++ )
	{
		const STypedObject &obj = objects[i];
		if ( obj.m_object )
		{
			// Get common class
			if ( !m_baseClass )
			{
				m_baseClass = obj.m_class;
			}
			else
			{
				while ( !obj.m_class->IsBasedOn( m_baseClass ) )
				{
					m_baseClass = m_baseClass->GetBaseClass();
				}
			}

			// Add object to list
			m_objects.PushBack( obj );
		}
	}

	// Expand
	if ( m_objects.Size() )
	{
		Init( m_baseClass );

		m_isExpandable = true;

		if ( m_children.Empty() )
		{
			Expand();
		}
	}
}

void CObjectsRootItem::DrawLayout( wxDC& dc )
{
	DrawChildren( dc );
}

void CObjectsRootItem::UpdateLayout( Int32& yOffset, Int32 x, Int32 width )
{
	m_rect.x = x;
	m_rect.y = yOffset;
	m_rect.width = width;

	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->UpdateLayout( yOffset, x, width );
	}

	m_rect.height = yOffset - m_rect.y;
}

Int32 CObjectsRootItem::GetLocalIndent() const
{
	return 0;
}

Int32 CObjectsRootItem::GetNumObjects() const
{
	return m_objects.Size();
}

STypedObject CObjectsRootItem::GetRootObject( Int32 objectIndex ) const
{
	return GetParentObject( objectIndex );
}

STypedObject CObjectsRootItem::GetParentObject( Int32 objectIndex ) const
{
	if ( objectIndex == -1 )
	{
		void* obj = ( !m_baseClass || m_baseClass->IsAbstract() ) ? nullptr : m_baseClass->GetDefaultObjectImp();
		return STypedObject( obj, m_baseClass );
	}

	return m_objects[ objectIndex ];
}

void CObjectsRootItem::Expand()
{
	// Extra shit for entity
	if ( m_page->GetSettings().m_showEntityComponents )
	{
		CClass* baseClass = NULL;
		// gather all the components for all the entities
		TDynArray< CComponent* > componentArray;
		for( Uint32 j=0; j<m_objects.Size(); ++j )
		{
			// Is Entity ?
			if ( CEntity* entity = m_objects[j].As< CEntity >() )
			{
				// We can edit only entities with detached templates
				// Entities without templates should have only one component except terrain tiles so we need the last part
				if ( m_page->IsEntityEditorOwner() || (!entity->GetTemplate() && entity->GetComponents().Size() <= 1) )
				{
					TDynArray< CComponent* > components;
					// Expand components
					CollectEntityComponents( entity, components );

					if (components.Size() > 0)
					{
						// Get common class
						if ( !baseClass )
						{
							baseClass = components[0]->GetClass();
						}
						else
						{
							while ( !components[0]->GetClass()->IsBasedOn( baseClass ) )
							{
								baseClass = baseClass->GetBaseClass();
							}
						}

						// Add object to list
						componentArray.PushBack(components[0]);
					}

					//// create arrays by type
					//for ( Uint32 i=0; i<components.Size(); ++i )
					//{
					//	bool found = false;

					//	for ( Uint32 h=0; h<componentArrays.Size(); ++h )
					//	{
					//		if (components[i]->GetClass() == componentArrays[h][0]->GetClass())
					//		{
					//			componentArrays[h].PushBack(components[i]);
					//			found = true;
					//			break;
					//		}
					//	}
					//	if (!found)
					//	{
					//		TDynArray< CComponent* > newType;
					//		newType.PushBack(components[i]);
					//		componentArrays.PushBack(newType);
					//	}
					//}
				}
			}
		}

		// we need the components that can be found in every object
		if ( baseClass != NULL && componentArray.Size() == m_objects.Size() )
		{
			CComponentGroupItem * cgi = new CComponentGroupItem( m_page, this, baseClass );
			cgi->SetObjects(componentArray);
		}
		//CObjectsRootItem * ori = new CObjectsRootItem( m_page, this);
		//ori->SetObjects(typedObjects);
	}


	// Classical properties
	CPropertyItemClass::Expand();
}

Bool CObjectsRootItem::IsReadOnly() const
{	
	return false;
}

Bool CObjectsRootItem::IsInlined() const
{	
	return true;
}

String CObjectsRootItem::GetName() const
{	
	return String::EMPTY;
}

String CObjectsRootItem::GetCustomEditorType() const
{
	return String::EMPTY;
}