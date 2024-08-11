#pragma once

class CResLinkerEngine;
class CVersionControlInterface
{
	friend class CResLinkerEngine;
private:
	Bool m_silentMode;

public:
	void OnNotOperational();

	// Resolves situation, when connection could not be established
	Int OnNoConnection();

	// Resolves situation, when file is checked out already by some other users
	Int OnParallelUsers( const TDynArray< String > &users );

	// Resolves situation, when file cannot be saved due to lack of check out
	Bool OnSaveFailure( CDiskFile &file );

	// Resolves situation, when file must be checked out
	Int OnCheckOutRequired( const String &path, const TDynArray< String > &users );

	// Resolves situation, when file was already checked out
	void OnDoubleCheckOut();

	// Resolves situation, when file was not checked out before other operation
	void OnNotEdited( const CDiskFile &file );

	// Resolves situation, when single file is about to be submitted
	Int OnSubmit( String &description, CDiskFile &resource );

	// Resolves situation, when multiple files are about to be submitted
	Int OnMultipleSubmit( String &, const TDynArray< CDiskFile * > &, TSet< CDiskFile * > &);

	// Resolves situation, when file could not be submitted
	void OnFailedSubmit();

	// Resolves situation, when file could not be checked out
	void OnFailedCheckOut();

	// Resolves situation, when file is local
	void OnLocalFile();

	// Resolves situation, when loaded asset is about to be deleted
	void OnLoadedDelete();

	// Resolves situation, when asset could not be deleted
	void OnFailedDelete();

	// Resolves situation, when asset has to be synced
	void OnSyncRequired();

	// Resolves situation, when asset cannot be synced due to writable/checked out state
	Int OnSyncFailed();

	//void OnCaptureLost(wxMouseCaptureLostEvent &event);

	Bool OnConfirmModifiedDelete();

	Bool OnConfirmDelete();

	void OnFileLog(CDiskFile &file, TDynArray< THashMap< String, String > > &history );

	void OnFilesIdentical();

	Bool OnConfirmRevert();
};