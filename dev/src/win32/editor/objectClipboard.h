/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdObjectClipboard
{
	wxDataFormat				m_format;
	Bool DoCopy( const TDynArray<CObject*>& objs );
public:
	CEdObjectClipboard();
	Bool Copy( CObject* obj );
	template<class T> Bool Copy( const TDynArray<T>& objs )
	{
		TDynArray<CObject*> tmp;
		for ( Uint32 i=0; i<objs.Size(); ++i ) tmp.PushBack( objs[i] );
		return DoCopy( tmp );
	}
	Bool Paste( CObject*& obj, CObject* parent = NULL );
	Bool Paste( TDynArray<CObject*>& objs, CObject* parent = NULL );
	Bool HasObjects() const;
};

extern CEdObjectClipboard GObjectClipboard;
