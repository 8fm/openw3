#pragma once

#include "dialogEditorHandler.h"

class IEdDialogHandlerAware
{
private:
	TDynArray< IEdDialogEditorHandler* > m_registeredDialogEditorHandlers;

public:
	virtual ~IEdDialogHandlerAware()
	{
		while ( m_registeredDialogEditorHandlers.Empty() == false )
		{
			IEdDialogEditorHandler* handler = m_registeredDialogEditorHandlers.PopBack();
			//delete handler;
		}
	}

	inline void RegisterDialogHandler( IEdDialogEditorHandler* handler )
	{
		m_registeredDialogEditorHandlers.PushBack( handler );
	}
};