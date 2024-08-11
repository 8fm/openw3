/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Version control wrapper for copy/delete files
// All input resources must be in the same directory
class CVersionControlWrapper 
{
public:
	void CopyFiles(CDirectory* dir, TDynArray<CResource*>& data);
	void CopyAsFiles(CDirectory* dir, TDynArray<CResource*>& data, TDynArray<String>& names);
	void DeleteFiles(TDynArray<CResource*>& data);

	void MoveFiles(CDirectory* dir, TDynArray<CResource*>& data);
	void RenameFiles(TDynArray<CResource*>& data, TDynArray<String>& names);

	void CreateLinksForFiles(CDirectory* dir, TDynArray<CResource*>& data);
	void CreateLinksForFiles(CDirectory* dir, TDynArray<CResource*>& data, TDynArray<String>& names);
	void DeleteLinksForFiles(CDirectory* dir, TDynArray<CResource*>& data);

protected:
	Bool UndoCopyFiles(CDirectory* dir, TDynArray<CResource*>& data, TDynArray<String>& names, TDynArray<CResource*>::iterator end);
	Bool UndoCreateLinksForFiles(TDynArray<CResource*>& data, TDynArray<CResource*>::iterator end);
};

typedef TSingleton<CVersionControlWrapper> SVersionControlWrapper;