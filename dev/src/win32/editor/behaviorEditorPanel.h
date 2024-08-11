/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "perlinGenerator.h"

class CEdBehaviorEditor;

class CEdBehaviorEditorPanel
{
public:
	CEdBehaviorEditorPanel( CEdBehaviorEditor* editor );

	CEdBehaviorEditor*			GetEditor() const;

	CBehaviorGraph*				GetBehaviorGraph() const;
	CBehaviorGraphInstance*		GetBehaviorGraphInstance() const;

	CAnimatedComponent*			GetAnimatedComponent() const;
	CEntity*					GetEntity() const;

	virtual wxWindow*			GetPanelWindow() = 0;
	virtual wxString			GetPanelName() const = 0;
	virtual wxString			GetPanelCaption() const = 0;
	virtual wxString			GetInfo() const { return wxT(""); }

	virtual wxAuiPaneInfo		GetPaneInfo() const { return wxAuiPaneInfo().Name( GetPanelName() ).Caption( GetPanelCaption() ).DestroyOnClose( false ); }

	virtual void SaveSession( CConfigurationManager &config, const String& path )	 {}
	virtual void RestoreSession( CConfigurationManager &config, const String& path ) {}

public:
	virtual void OnClose()			{}
	virtual void OnReset()			{}
	virtual void OnDebug( Bool flag ){}
	virtual void OnPanelClose()		{}
	virtual void OnLoadEntity()		{}
	virtual void OnUnloadEntity()	{}
	virtual void OnPreInstanceReload() {}
	virtual void OnInstanceReload() {}
	virtual void OnGraphModified()	{}
	virtual void OnNodesSelect( const TDynArray< CGraphBlock* >& nodes )	{}
	virtual void OnNodesDeselect()	{}
	virtual void OnTick( Float dt ) {}
	virtual Bool RequiresCustomTick() const { return false; }
	virtual void OnCustomTick( Float dt ) {}
	virtual void OnDebuggerPostSamplePose( const SBehaviorGraphOutput& pose ) {}
	virtual void OnPrintNodes( CEdGraphEditor* graphCanvas ) {}

private:
	CEdBehaviorEditor* m_editor;
};

class CEdBehaviorEditorSimplePanel  : public wxPanel
									, public CEdBehaviorEditorPanel
{
	DECLARE_EVENT_TABLE()

public:
	CEdBehaviorEditorSimplePanel( CEdBehaviorEditor* editor, wxWindow* parent = NULL );

	virtual wxWindow* GetPanelWindow() { return this; }
};

class BehaviorBackground
{
	Bool				m_init;
	Gdiplus::Bitmap*	m_image;
	Perlin*				m_generator;

public:
	BehaviorBackground()
		: m_init( false )
		, m_image( NULL )
		, m_generator( NULL )
	{

	}

	~BehaviorBackground()
	{
		if ( m_image )
		{
			delete m_image;
			m_image = NULL;
		}

		if ( m_generator )
		{
			delete m_generator;
			m_generator = NULL;
		}
	}

	Gdiplus::Bitmap* Draw( Int32 width, Int32 height )
	{
		if ( !m_init )
		{
			Init();
		}

		CheckImage( width, height );

		for ( Int32 i=width-1; i>=0; --i )
		{
			for ( Int32 j=height-1; j>=0; --j )
			{
				Uint8 x = 255.f * m_generator->Get( i, j );
				
				Gdiplus::Color color(255, x, x, x );

				m_image->SetPixel( i, j, color );
			}
		}

		return m_image;
	}

	void OnTick( Float dt )
	{
		//...
	}

	void OnClick()
	{
		if ( m_generator )
		{
			m_generator->Reinit();
		}
	}

private:
	void Init()
	{
		m_generator = new Perlin( 8, 0.0033f, 1.f );
		m_init = true;
	}

	void CheckImage( Int32 width, Int32 height )
	{
		if ( m_image )
		{
			if ( m_image->GetWidth() != width || m_image->GetHeight() != height )
			{
				delete m_image;
				m_image = new Gdiplus::Bitmap( width, height, PixelFormat32bppARGB );
			}
		}
		else
		{
			m_image = new Gdiplus::Bitmap( width, height, PixelFormat32bppARGB );
		}
	}
};
