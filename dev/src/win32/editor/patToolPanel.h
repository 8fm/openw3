
#pragma once

class pat_tool
{
public:
	pat_tool(){}
	virtual ~pat_tool(){}
	virtual void release(){}
	virtual void attach( HWND o, char* buffer, int size ){}
	virtual void setdata( char* buffer, int size ){}
	virtual void lbuttondown( float x, float y ){}
	virtual void lbuttonup( float x, float y ){}
	virtual void rbuttondown( float x, float y ){}
	virtual void rbuttonup( float x, float y ){}
	virtual void mbuttondown( float x, float y ){}
	virtual void mbuttonup( float x, float y ){}
	virtual void move( float x, float y ){}
	virtual void key(char c){}
	virtual void keyup(char c){}
	virtual void wheel(float w){}
	virtual void draw(){} 
	virtual void resize( int x, int y ){}
};

class IPatToolCallback
{
public:
	virtual void OnPatToolControlsPreChanged() {}
	virtual void OnPatToolControlsChanging() {}
	virtual void OnPatToolControlsPostChanged() {}
};

class CPatToolPanel : public wxPanel
{
	DECLARE_EVENT_TABLE();

private:
	pat_tool*			m_tool;
	HINSTANCE			m_handle;
	IPatToolCallback*	m_callback;
	
protected:
	Bool				m_working;

public:
	CPatToolPanel( wxWindow* parent, const AnsiChar* funcName, void* data = nullptr, Uint32 dataSize = 0, IPatToolCallback* callback = nullptr );
	~CPatToolPanel();

	void SetData( void* data, Uint32 dataSize );

protected:
	virtual void OnControlsPreChanged();
	virtual void OnControlsChanging();
	virtual void OnControlsPostChanged();

protected:
	void OnClear( wxEraseEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnKeyUp( wxKeyEvent& event );
	void OnMouseLeftDown( wxMouseEvent& event );
	void OnMouseLeftUp( wxMouseEvent& event );
	void OnMouseMidDown( wxMouseEvent& event );
	void OnMouseMidUp( wxMouseEvent& event );
	void OnMouseMotion( wxMouseEvent& event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnPaint( wxPaintEvent& event );
	void OnMouseRightDown( wxMouseEvent& event );
	void OnMouseRightUp( wxMouseEvent& event );
	void OnSize( wxSizeEvent& event );
};
