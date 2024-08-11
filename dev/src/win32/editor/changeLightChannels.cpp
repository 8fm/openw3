/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "changeLightChannels.h"
#include "../../common/core/feedback.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/drawableComponent.h"
#include "resourceIterator.h"

BEGIN_EVENT_TABLE( CEdChangeLightChannelsDlg, wxDialog )
	EVT_BUTTON( XRCID("m_apply"), CEdChangeLightChannelsDlg::OnBatch )
END_EVENT_TABLE()

CEdChangeLightChannelsDlg::CEdChangeLightChannelsDlg( wxWindow* parent, CContextMenuDir* contextMenuDir )
	: m_bitField( nullptr )
	, m_world( nullptr )
	, m_contextMenuDir( *contextMenuDir )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("ChangeFlags") );
	SetTitle( TXT("Change light channels") );
	
	m_lightChannels = XRCCTRL( *this, "m_flags", wxCheckListBox );

	// add options to light channels box
	static CName typeName( TXT("ELightChannel" ) );
	m_bitField = static_cast< CBitField* >( SRTTI::GetInstance().FindType( typeName, RT_BitField ) );

	if( m_bitField != nullptr )
	{
		for ( Uint32 i=0; i<32; i++ )
		{
			CName bitName = m_bitField->GetBitName( i );
			if ( bitName )
			{
				Uint32& value = m_mapCheckBoxToBit.GetRef( m_lightChannels->GetCount() );
				value = i;
				m_lightChannels->Append( bitName.AsChar() );
			}
		}
	}
	else
	{
		GFeedback->ShowError( TXT(" Change light channels editor did not find ELightChannel type. Editor will be closed.") );
		Close();
	}
}

CEdChangeLightChannelsDlg::~CEdChangeLightChannelsDlg()
{
	if( m_world != nullptr )
	{
		m_world->Discard();
	}
}

void CEdChangeLightChannelsDlg::OnBatch( wxCommandEvent &event )
{
	for ( CResourceIteratorAdapter< CEntityTemplate > entityTemplate( m_contextMenuDir, TXT("Checking out entities...") ); entityTemplate; ++entityTemplate )
	{
		ChangeLightChannels( entityTemplate.Get() );
	}

	Close();
}

void CEdChangeLightChannelsDlg::ChangeLightChannels( CEntityTemplate* entityTemplate )
{
	CEntity* entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );

	if( entity != nullptr )
	{
		if ( m_world == nullptr )
		{
			m_world = CreateObject< CWorld >();
			m_world->AddToRootSet();

			WorldInitInfo initInfo;
			initInfo.m_previewWorld = true;
			m_world->Init(initInfo);
		}

		CLayer* layer = m_world->GetDynamicLayer();
		layer->AddEntity( entity );
		entity->AttachToWorld( m_world );
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		entity->PrepareEntityForTemplateSaving();
		entity->DetachTemplate();

		const Uint32 channelCount = m_lightChannels->GetCount();

		const TDynArray< CComponent* >& components = entity->GetComponents();
		const Uint32 componentCount = components.Size();
		for( Uint32 i=0; i<componentCount; ++i )
		{
			CDrawableComponent* drawableComponent = Cast< CDrawableComponent >( components[i] );
			if( drawableComponent != nullptr )
			{
				for( Uint32 j=0; j<channelCount; ++j )
				{
					Uint32 bitField = m_mapCheckBoxToBit.GetRef( j );
					drawableComponent->EnableLightChannels( m_lightChannels->IsChecked( j ), FLAG( bitField ) );
				}
			}
		}

		entityTemplate->CaptureData( entity );

		// Destroy instance
		layer->RemoveEntity( entity );
		entity->Discard();

		// Second instance to convert any new properties
		entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
		if ( entity != nullptr )
		{
			CLayer* layer = m_world->GetDynamicLayer();
			layer->AddEntity( entity );
			entity->AttachToWorld( m_world );
			entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
			entity->UpdateStreamedComponentDataBuffers();

			layer->RemoveEntity( entity );
			entity->Discard();
		}
	}
}
