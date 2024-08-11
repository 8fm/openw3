#pragma once

#include "../../common/core/versionControl.h"

// Editor version control interface implementation
class CEdVersionControlInterface : public IVersionControlEditorInterface
{
public:
	// Not operational
	virtual void OnNotOperational();

	// Resolves situation, when connection could not be established
	virtual Int32 OnNoConnection();

	// Resolves situation, when some files on list are already checked out by some other users
	virtual Int32 OnParallelUsers( const TDynArray< CDiskFile* > &fileList, const TDynArray< String > &users, Bool exclusiveAccess );

	// Resolves situation, when file is checked out already by some other users
	virtual Int32 OnParallelUsers( CDiskFile &file, const TDynArray< String > &users, Bool exclusiveAccess );

	// Resolves situation, when file cannot be saved due to lack of check out
	virtual Bool OnSaveFailure( CDiskFile &file );

	// Resolves situation, when file must be checked out
	virtual Int32 OnCheckOutRequired( const String &path, const TDynArray< String > &users );

	// Resolves situation, when file was already checked out
	virtual void OnDoubleCheckOut();

	// Resolves situation, when file was not checked out before other operation
	virtual void OnNotEdited( const CDiskFile &file );

	// Resolves situation, when single file is about to be submitted
	virtual Int32 OnSubmit( String &description, CDiskFile &resource );

	// Resolves situation, when multiple files are about to be submitted
	virtual Int32 OnMultipleSubmit( String &, const TDynArray< CDiskFile * > &, TSet< CDiskFile * > &);

	// Resolves situation, when file could not be submitted
	virtual void OnFailedSubmit();

	// Resolves situation, when file could not be checked out
	virtual void OnFailedCheckOut();

	// Resolves situation, when file is local
	virtual void OnLocalFile();

	// Resolves situation, when loaded asset is about to be deleted
	virtual void OnLoadedDelete();

	// Resolves situation, when asset could not be deleted
	virtual void OnFailedDelete();

	// Resolves situation, when asset has to be synced
	virtual void OnSyncRequired();

	// Resolves situation, when asset cannot be synced due to writable/checked out state
	virtual Int32 OnSyncFailed();

	// Confirm deletion of modified file
	virtual Bool OnConfirmModifiedDelete();

	// Confirm deletion of file
	virtual Bool OnConfirmDelete();

	// Log file
	virtual void OnFileLog(CDiskFile &file, TDynArray< THashMap< String, String > > &history );

	// File is identical
	virtual void OnFilesIdentical();

	// Confirm revert of file
	virtual Bool OnConfirmRevert();
};
