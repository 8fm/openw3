#include "build.h"
#include "dummyVersionControlInterface.h"


void CVersionControlInterface::OnNotOperational() {};

// Resolves situation, when connection could not be established
Int CVersionControlInterface::OnNoConnection() { return 0; };

// Resolves situation, when file is checked out already by some other users
Int CVersionControlInterface::OnParallelUsers( const TDynArray< String > &users ){ return 0; };

// Resolves situation, when file cannot be saved due to lack of check out
Bool CVersionControlInterface::OnSaveFailure( CDiskFile &file ){ return false; };

// Resolves situation, when file must be checked out
Int CVersionControlInterface::OnCheckOutRequired( const String &path, const TDynArray< String > &users ){ return 0; };

// Resolves situation, when file was already checked out
void CVersionControlInterface::OnDoubleCheckOut(){};

// Resolves situation, when file was not checked out before other operation
void CVersionControlInterface::OnNotEdited( const CDiskFile &file ){};

// Resolves situation, when single file is about to be submitted
Int CVersionControlInterface::OnSubmit( String &description, CDiskFile &resource ){ return 0; };

// Resolves situation, when multiple files are about to be submitted
Int CVersionControlInterface::OnMultipleSubmit( String &, const TDynArray< CDiskFile * > &, TSet< CDiskFile * > &){ return 0; };

// Resolves situation, when file could not be submitted
void CVersionControlInterface::OnFailedSubmit(){};

// Resolves situation, when file could not be checked out
void CVersionControlInterface::OnFailedCheckOut(){};

// Resolves situation, when file is local
void CVersionControlInterface::OnLocalFile(){};

// Resolves situation, when loaded asset is about to be deleted
void CVersionControlInterface::OnLoadedDelete(){};

// Resolves situation, when asset could not be deleted
void CVersionControlInterface::OnFailedDelete(){};

// Resolves situation, when asset has to be synced
void CVersionControlInterface::OnSyncRequired(){};

// Resolves situation, when asset cannot be synced due to writable/checked out state
Int CVersionControlInterface::OnSyncFailed(){ return 0; };

//void OnCaptureLost(wxMouseCaptureLostEvent &event);

Bool CVersionControlInterface::OnConfirmModifiedDelete() { return false;} ;

Bool CVersionControlInterface::OnConfirmDelete() { return false;} ;

void CVersionControlInterface::OnFileLog(CDiskFile &file, TDynArray< THashMap< String, String > > &history ) {};

void CVersionControlInterface::OnFilesIdentical() {};

Bool CVersionControlInterface::OnConfirmRevert() { return true; }
