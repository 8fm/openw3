#pragma once

#include "dialogEditorHandler.h"
class CEdSceneEditorScreenplayPanel;
class LocalizedString;

//////////////////////////////////////////////////////////////////////////

class CEdDialogScrollOnFocusHandler : public IEdDialogEditorHandler
{
protected:
	CEdSceneEditorScreenplayPanel* m_storySceneEditor;

public:
	CEdDialogScrollOnFocusHandler( CEdSceneEditorScreenplayPanel* dialogEditor );
	void ConnectTo( wxWindow* window );

protected:
	void OnSetFocus( wxFocusEvent& event );
	void OnChildSetFocus( wxChildFocusEvent& event );
	void OnCharPressed( wxKeyEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogArrowTraverseHandler : public IEdDialogEditorHandler
{
public:
	void ConnectTo( wxWindow* window );

protected:
	void OnCharPressed( wxKeyEvent& event );

	bool ShouldIgnoreHandlingEvent( wxWindow* eventSource, wxKeyEvent &event );
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogCaretOnFocusHandler : public IEdDialogEditorHandler
{
private:
	Bool m_manualFocusOnMouseClick;

public:
	CEdDialogCaretOnFocusHandler() : m_manualFocusOnMouseClick( false ) {}
	void ConnectTo( wxWindow* window );

protected:
	void OnFocus( wxFocusEvent& event );
	void OnMultilineMouseDown( wxMouseEvent& event );
	void OnMultilineMouseUp( wxMouseEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogHyperlinkHandler : public IEdDialogEditorHandler
{
public:
	void ConnectTo( wxWindow* window );

protected:
	void OnLinkClick( wxMouseEvent& event );
	void OnMouseEnter( wxMouseEvent& event );
	void OnMouseLeave( wxMouseEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogAutoExpandHandler : public IEdDialogEditorHandler
{
private:
	CEdSceneEditorScreenplayPanel* m_storySceneEditor;

public:
	CEdDialogAutoExpandHandler( CEdSceneEditorScreenplayPanel* dialogEditor );
	void ConnectTo( wxWindow* window );

protected:
	void OnTextChanged( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogManualScriptScrollHandler : public IEdDialogEditorHandler
{
private:
	wxScrolledWindow* m_scrollWindow;

public:
	CEdDialogManualScriptScrollHandler( wxScrolledWindow* scrollWindow );
	void ConnectTo( wxWindow* window );

protected:
	void OnMouseScroll( wxMouseEvent& event );
};

class CEdPseudoButtonHandler : public IEdDialogEditorHandler
{
public:
	void ConnectTo( wxWindow* window );
	
protected:
	void OnMouseLeftButtonUp( wxMouseEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogTranslationHelperHandler : public IEdDialogEditorHandler
{
private:
	LocalizedString*	m_content;
	wxWindow*			m_tipWindow;
	wxWindow*			m_tooltipField;

	static Bool			m_areTooltipsEnabled;

public:
	CEdDialogTranslationHelperHandler( LocalizedString* content ) : m_content( content ), m_tipWindow( NULL ), m_tooltipField( NULL ) {}
	void ConnectTo( wxWindow* window );

	static void EnableTooltips( Bool enable ) { m_areTooltipsEnabled = enable; }
	static Bool AreTooltipsEnabled() { return m_areTooltipsEnabled; }

protected:
	void OnFocusSet( wxFocusEvent& event );
	void OnFocusLost( wxFocusEvent& event );
};

//////////////////////////////////////////////////////////////////////////
