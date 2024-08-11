#include "build.h"
#include "storySceneLinkElement.h"
#include "storyScene.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneLinkElement );

CStorySceneLinkElement::CStorySceneLinkElement()
	: m_nextLinkElement( nullptr )
#ifndef NO_EDITOR
	, m_selectedLinkedElement( 0 )
#endif
{
}

void CStorySceneLinkElement::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );
}

void CStorySceneLinkElement::ConnectToElement( CStorySceneLinkElement* linkableElement )
{
	// Break current connection
	if ( m_nextLinkElement )
	{
		CStorySceneLinkElement* currentNextLinkElement = m_nextLinkElement;
		
		m_nextLinkElement = nullptr;
		OnDisconnected( m_nextLinkElement );

		currentNextLinkElement->UnlinkElement( this );
	}

	// Set new connection
	m_nextLinkElement = linkableElement;

	// Setup new link
	if ( m_nextLinkElement )
	{
		m_nextLinkElement->LinkElement( this );
		OnConnected( m_nextLinkElement );
	}
}

void CStorySceneLinkElement::LinkElement( CStorySceneLinkElement* linkElement )
{
	SCENE_ASSERT( linkElement );
	SCENE_ASSERT( !m_linkedElements.Exist( linkElement ) );
	m_linkedElements.PushBackUnique( linkElement );	
}

void CStorySceneLinkElement::UnlinkElement( CStorySceneLinkElement* linkElement )
{
	SCENE_ASSERT( linkElement );
	SCENE_ASSERT( m_linkedElements.Exist( linkElement ) );
	VERIFY( m_linkedElements.Remove( linkElement ) );

#ifndef NO_EDITOR
	m_selectedLinkedElement = Clamp< Uint32 >( m_selectedLinkedElement, 0, m_linkedElements.Size()-1 );
#endif
}

void CStorySceneLinkElement::ResetLinks()
{
	// Disconnect links
	TDynArray< CStorySceneLinkElement* > links = m_linkedElements;
	for ( Uint32 i=0; i<links.Size(); ++i )
	{
		CStorySceneLinkElement* linkedElement = links[i];
		UnlinkElement( linkedElement );
	}
}

void CStorySceneLinkElement::ResetNextElement()
{
	m_nextLinkElement = NULL;
}

void CStorySceneLinkElement::OnConnected( CStorySceneLinkElement* linkedToElement )
{
	if ( GetParent() != NULL && GetParent()->IsA< CStorySceneLinkElement >() )
	{
		CStorySceneLinkElement* parentLinkElement = SafeCast< CStorySceneLinkElement >( GetParent() );
		parentLinkElement->OnConnected( linkedToElement );
	}
}

void CStorySceneLinkElement::OnDisconnected( CStorySceneLinkElement* linkedToElement )
{
	if ( GetParent() != NULL && GetParent()->IsA< CStorySceneLinkElement >() )
	{
		CStorySceneLinkElement* parentLinkElement = SafeCast< CStorySceneLinkElement >( GetParent() );
		parentLinkElement->OnDisconnected( linkedToElement );
	}
}

void CStorySceneLinkElement::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	MarkModified_Post();
}

Bool CStorySceneLinkElement::MarkModified_Pre()
{
	CStoryScene* scene = Cast< CStoryScene >( FindParent< CStoryScene >() );
	if ( scene )
	{
		return scene->OnLinkElementMarkModifiedPre( this );
	}

	SCENE_ASSERT( scene );

	return false;
}

void CStorySceneLinkElement::MarkModified_Post()
{
	CStoryScene* scene = Cast< CStoryScene >( FindParent< CStoryScene >() );
	if ( scene )
	{
		scene->OnLinkElementMarkModifiedPost( this );
	}

	SCENE_ASSERT( scene );
}

#ifndef NO_EDITOR

Bool CStorySceneLinkElement::SupportsInputSelection() const
{
	return false;
}

void CStorySceneLinkElement::ToggleSelectedInputLinkElement()
{
	++m_selectedLinkedElement;

	if ( m_selectedLinkedElement >= m_linkedElements.Size() )
	{
		m_selectedLinkedElement = 0;
	}
}

Uint32 CStorySceneLinkElement::GetSelectedInputLinkElement() const
{
	return m_selectedLinkedElement;
}

Bool CStorySceneLinkElement::SupportsOutputSelection() const
{
	return false;
}

void CStorySceneLinkElement::ToggleSelectedOutputLinkElement()
{
}

Uint32 CStorySceneLinkElement::GetSelectedOutputLinkElement() const
{
	return 0;
}

#endif
