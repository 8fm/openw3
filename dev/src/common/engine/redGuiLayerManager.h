/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

namespace RedGui
{
	class CRedGuiLayerManager
	{
	public:
		CRedGuiLayerManager();
		virtual ~CRedGuiLayerManager();

		// Attach control to layer
		void AttachToLayer( const String& layerName, CRedGuiControl* control );

		// Detach control from layer
		void DetachFromLayer( CRedGuiControl* control );

		// Up control to be on top (top for it's layer)
		void UpLayerItem( CRedGuiControl* control );

		// Add new layer
		void CreateLayer( const String& layerName, Bool isPickable = true );

		// Check is layer exist
		Bool IsExist( const String& name ) const;

		// get layer by name
		IRedGuiLayer* GetByName( const String& name ) const;

		// Get top visible and enable control at position
		CRedGuiControl* GetControlFromPoint( const Vector2& position );

		// Draw all layers
		void Draw( CRenderFrame *frame );

		void ResizeView( const Vector2& viewSize );

		void DisposeLayer( IRedGuiLayer* layer );

		Bool IsBudgetMode() { return m_artistsBudgetMode; }
		void SetBudgetMode( Bool budgetMode ) { m_artistsBudgetMode = budgetMode; }

	private:
		void CreateDefaultLayers();
		void DeleteReportedLayers();

		ArrayLayerPtr m_layers;
		ArrayLayerPtr m_layersToDelete;
		Bool m_artistsBudgetMode;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
