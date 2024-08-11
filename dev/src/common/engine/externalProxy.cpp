#include "build.h"
#include "externalProxy.h"
#include "componentIterator.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CExternalProxyAttachment )

void CExternalProxyAttachment::OnSerialize( IFile& file )
{
    if (file.IsWriter() && !file.IsGarbageCollector() && m_originalAttachment)
        m_originalAttachment->SetParent(this->GetParent());

    // Default serialization
	TBaseClass::OnSerialize( file );

    if (file.IsReader() && m_originalAttachment)
        m_originalAttachment->SetParent(this->GetParent());
}

IMPLEMENT_ENGINE_CLASS( CExternalProxyComponent )

void CExternalProxyComponent::OnSerialize( IFile& file )
{
    // Default serialization
	TBaseClass::OnSerialize( file );
}

Bool CExternalProxyComponent::IsProxyFor( CComponent &component )
{
#if VER_MINIMAL < VER_EXTERNAL_PROXIES_USE_GUIDS
	return ( GetGUID() != CGUID::ZERO && GetGUID() == component.GetGUID() ) ||
		   ( GetGUID() == CGUID::ZERO && component.GetClass()->GetName() == m_originalClassName && GetName() == component.GetName() );
#else
	return GetGUID() == component.GetGUID();
#endif
}

CComponent* CExternalProxyComponent::GetProxyIfNeeded(CEntity &entity, CComponent &component)
{
	// Component is not referenced so it does not need proxy
	if ( !component.HasFlag( OF_Referenced ) )
	{
		return &component;
	}

	// Get proxies from the entity
	for ( ComponentIterator<CExternalProxyComponent> it( &entity ); it; ++it )
	{
		CExternalProxyComponent* proxy = *it;
        if ( proxy->IsProxyFor( component ) )
		{
            return proxy;
		}
	}

	// Create proxy with matching GUID
	CExternalProxyComponent *proxy = Cast<CExternalProxyComponent>( entity.CreateComponent(ClassID<CExternalProxyComponent>(), SComponentSpawnInfo() ) );
    proxy->SetGUID( component.GetGUID() );
    
	// Done
    return proxy;
}

void CExternalProxyComponent::SuckDataFromDestination( CEntity &entity, CComponent &destination )
{
	// Copy position
#ifndef NO_COMPONENT_GRAPH
	Int32 posX=0, posY=0;
	destination.GetGraphPosition( posX, posY );
    SetGraphPosition( posX, posY );
#endif

	// Create a unique name across all components
	SetName( TXT("proxy_") + destination.GetName() + TXT("_") + ::ToString( CGUID::Create() ) );

	// Copy child attachments
	{
		TList< IAttachment* > attachments = destination.GetChildAttachments();
		for ( TList< IAttachment* >::iterator it=attachments.Begin(); it!=attachments.End(); ++it )
		{
			IAttachment* attachment = *it;

			Bool rettached = false;
			// Get the proxy for the child component
			CComponent *comp = Cast< CComponent >( attachment->GetChild() );
			if ( comp )
			{
				CComponent *child = GetProxyIfNeeded( entity, *comp );
				if ( child )
				{
					// Create attachment to the child component
					CExternalProxyAttachment *newAttachment = Cast< CExternalProxyAttachment >( Attach( child, ClassID<CExternalProxyAttachment>() ) );

					// Relink attachment
					newAttachment->SetOriginalLink( attachment );
					attachment->Break();
					attachment->SetParent( newAttachment );
					rettached = true;
				}
			}
			if ( !rettached )
			{
				attachment->Break();
			}
		}
	}

	// Copy parent attachments
	{
		TList< IAttachment* > attachments = destination.GetParentAttachments();
		for ( TList< IAttachment* >::iterator it=attachments.Begin(); it!=attachments.End(); ++it )
		{
			IAttachment* attachment = *it;

			// Get the proxy for the parent component
			CComponent *parent = GetProxyIfNeeded( entity, *SafeCast< CComponent >( attachment->GetParent() ) );
			if ( parent )
			{
				// Create attachment to the parent component
				CExternalProxyAttachment *newAttachment = Cast< CExternalProxyAttachment >( parent->Attach( this, ClassID<CExternalProxyAttachment>() ) );

				// Relink attachment
				newAttachment->SetOriginalLink( attachment );
				attachment->Break();
				attachment->SetParent( newAttachment );
			}
		}
	}
}

void CExternalProxyComponent::DumpDataToDestination( CEntity &entity, CComponent &destination )
{
	// Copy position
#ifndef NO_COMPONENT_GRAPH
	Int32 posX=0, posY=0;
	GetGraphPosition( posX, posY );
	destination.SetGraphPosition( posX, posY );
#endif

	// Dump child attachments
	{
		TList< IAttachment* > attachments = GetChildAttachments();
		for ( TList< IAttachment* >::iterator it=attachments.Begin(); it!=attachments.End(); ++it )
	    {
		    CExternalProxyAttachment *attachment = Cast<CExternalProxyAttachment>( *it );
	        if ( attachment && attachment->GetChild() )
			{
				CComponent *child = SafeCast< CComponent >( attachment->GetChild() );
				
				// Clear old attachment
				attachment->Break();
				// DM: Uncomment this to enable parent attachment override
 				//if ( ! child->GetParentAttachments().Empty() )
 				//{
 				//	child->GetParentAttachments().Front()->Break();
 				//}
				
				// Init new attachment
				if ( child->GetParentAttachments().Empty() ) // Do not override existing attachments, only add new ones
				{
					if ( destination.IsA<CExternalProxyComponent>() || child->IsA<CExternalProxyComponent>() )
					{
						attachment->Init(&destination, child, NULL);
						attachment->SetParent( &destination );
						ASSERT( attachment->GetOriginalLink() );
						if ( attachment->GetOriginalLink() )
						{
							attachment->GetOriginalLink()->SetParent( &destination );
						}
					}
					else
					{
						ASSERT( attachment->GetOriginalLink() );
						if ( attachment->GetOriginalLink() )
						{
							attachment->GetOriginalLink()->Init(&destination, child, NULL);
							attachment->GetOriginalLink()->SetParent(&destination);
						}
					}
				}
			}
		}
    }
    
	// Dump parent attachments
	{
		TList< IAttachment* > attachments = GetParentAttachments();
		for ( TList< IAttachment* >::iterator it=attachments.Begin(); it!=attachments.End(); ++it )
		{
			CExternalProxyAttachment *attachment = Cast<CExternalProxyAttachment>( *it );
	        if( attachment && attachment->GetParent() )
		    {
				CComponent *parent = SafeCast< CComponent >( attachment->GetParent() );
				
				// Clear old attachment
				attachment->Break();
				// DM: Uncomment this to enable parent attachment override
 				//if ( ! destination.GetParentAttachments().Empty() )
 				//{
 				//	destination.GetParentAttachments().Front()->Break();
 				//}

				// Init new attachment
				if ( destination.GetParentAttachments().Empty() ) // Do not override existing attachments, only add new ones
				{
					if ( parent->IsA<CExternalProxyComponent>() || destination.IsA<CExternalProxyComponent>() )
					{
						attachment->Init(parent, &destination, NULL);
						ASSERT( attachment->GetOriginalLink() );
					}
					else
					{
						ASSERT( attachment->GetOriginalLink() );
						attachment->GetOriginalLink()->Init(parent, &destination, NULL);
						attachment->GetOriginalLink()->SetParent(parent);
					}
				}
			}
        }
    }
}
