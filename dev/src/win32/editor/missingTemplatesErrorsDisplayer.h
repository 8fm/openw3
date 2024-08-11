/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef EDITORMISSINGTEMPLATESERRORS_H
#define EDITORMISSINGTEMPLATESERRORS_H

class CEdMissingTemplatesErrorsDisplayer
{
public:
	CEdMissingTemplatesErrorsDisplayer( wxWindow* parent );
	Bool Execute( const TSortedMap< String, TDynArray< String > >& layersMissingTemplates );

private:
	String m_webString;
	wxWindow* m_parent;

	void AppendEntries( const TSortedMap< String, TDynArray< String > >& layersMissingTemplates );
	void AppendLayerEntry( const String& layer, const TDynArray< String >& missingTemplates );
	void GenerateWebString( const TSortedMap< String, TDynArray< String > >& layersMissingTemplates );
};

#endif //EDITORMISSINGTEMPLATESERRORS_H
