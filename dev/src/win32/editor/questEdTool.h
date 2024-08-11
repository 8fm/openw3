#pragma once

class CEdQuestEditor;

class IToolCommandWrapper : public wxObject
{
public:
	virtual ~IToolCommandWrapper() {}

	virtual void OnBlock( CQuestGraphBlock* block ) = 0;

	virtual void OnSocket( CQuestGraphBlock* block, const CQuestGraphSocket* socket ) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct SToolMenu
{
	enum ActivationType
	{
		AT_BLOCK			= 1,
		AT_OUT_SOCKET		= 2,
		AT_IN_SOCKET		= 4,
	};

	String					name;
	IToolCommandWrapper*	command;
	Uint32					activationType;

	SToolMenu( const String& _name, IToolCommandWrapper* _command, Uint32	_activationType )
		: name( _name )
		, command( _command )
		, activationType( _activationType )
	{}
};

///////////////////////////////////////////////////////////////////////////////

class IQuestEdTool
{
public:
	virtual ~IQuestEdTool() {}

	virtual void OnAttach( CEdQuestEditor& host, wxWindow* parent ) = 0;

	virtual void OnDetach() = 0;

	virtual void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  ) = 0;

	virtual void OnGraphSet( CQuestGraph& graph ) = 0;

	virtual wxPanel* GetPanel() = 0;

	virtual String GetToolName() const = 0;
};
