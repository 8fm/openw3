/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiControl.h"
#include "redGuiLayer.h"
#include "redGuiLayerItem.h"
#include "redGuiGeneralLayer.h"
#include "redGuiLayerManager.h"
#include "redGuiGraphicContext.h"
#include "redGuiManager.h"

namespace RedGui
{
	CRedGuiLayerManager::CRedGuiLayerManager()
		: m_artistsBudgetMode( false )
	{
		CreateDefaultLayers();
	}
	
	void CRedGuiLayerManager::AttachToLayer( const String& layerName, CRedGuiControl* control )
	{
		DetachFromLayer(control);
		control->AttachToLayer(layerName);
	}

	void CRedGuiLayerManager::DetachFromLayer( CRedGuiControl* control )
	{
		control->DetachFromLayer();
	}

	void CRedGuiLayerManager::UpLayerItem( CRedGuiControl* control )
	{
		control->UpLayerItem();
	}

	void CRedGuiLayerManager::CreateLayer( const String& layerName, Bool isPickable/* = true*/ )
	{
		m_layers.PushBack( new CRedGuiGeneralLayer(layerName, isPickable) );
	}

	Bool CRedGuiLayerManager::IsExist( const String& name ) const
	{
		return GetByName( name ) != nullptr;
	}

	IRedGuiLayer* CRedGuiLayerManager::GetByName( const String& name ) const
	{
		for( Uint32 i=0; i<m_layers.Size(); ++i )
		{
			if( m_layers[i]->GetName() == name )
			{
				return m_layers[i];
			}
		}

		return nullptr;
	}

	CRedGuiControl* CRedGuiLayerManager::GetControlFromPoint( const Vector2& position )
	{
		for( Int32 i=m_layers.Size()-1; i>=0; --i )
		{
			IRedGuiLayerItem* item = m_layers[i]->GetLayerItemByPoint( position );
			if( item != nullptr )
			{
				return static_cast< CRedGuiControl* >( item );
			}
		}

		DeleteReportedLayers();

		return nullptr;
	}

	void CRedGuiLayerManager::Draw( CRenderFrame *frame )
	{
		// set new frame as render target
		GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->SetRenderTarget( frame );

		if ( m_artistsBudgetMode )
		{
			if ( IRedGuiLayer* layer = GetByName( TXT("Main") ) )
			{
				layer->DrawLayer();
			}
		}
		else
		{
			for( Uint32 i=0; i<m_layers.Size(); ++i )
			{
				m_layers[i]->DrawLayer();
			}
		}
	}

	void CRedGuiLayerManager::ResizeView( const Vector2& viewSize )
	{
		for(Uint32 i=0; i<m_layers.Size(); ++i)
		{
			m_layers[i]->ResizeView(viewSize);
		}
	}

	void CRedGuiLayerManager::CreateDefaultLayers()
	{
		// create wallpaper layer
		CreateLayer(TXT("Wallpaper"), false);

		// create main layer
		CreateLayer(TXT("Main"), true);

		// create main layer
		CreateLayer(TXT("Menus"), true);

		// create pointers layer
		CreateLayer(TXT("Pointers"), false);
	}

	CRedGuiLayerManager::~CRedGuiLayerManager()
	{
		for( Uint32 i=0; i<m_layers.Size(); ++i )
		{
			DisposeLayer( m_layers[i] );
		}
		DeleteReportedLayers();
		m_layers.Clear();
	}

	void CRedGuiLayerManager::DeleteReportedLayers()
	{
		for( Uint32 i=0; i<m_layersToDelete.Size(); ++i )
		{
			delete m_layersToDelete[i];
			m_layersToDelete[i] = nullptr;
		}
		m_layersToDelete.Clear();
	}

	void CRedGuiLayerManager::DisposeLayer( IRedGuiLayer* layer )
	{
		if( layer != nullptr )
		{
			m_layers.Remove( layer );
			m_layersToDelete.PushBack( layer );
		}
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
