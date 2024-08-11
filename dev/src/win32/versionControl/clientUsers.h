#pragma once

class StrDict;
struct StatFields;

class CWatcher : public ClientUser
{
	private:
		Int32 m_result;

		// FIXME: quickfix
		// This operator is to deal with the C4512 "'class' : assignment operator could not be generated" 
		// Child classes tend to use a reference member for some reason.
		CWatcher& operator= (const CWatcher &)
		{
			return *this;
		}

	public:
		CWatcher();
		CWatcher(Int32 result);
		void SetResult(Int32 result){
			m_result = result;
		};
		Int32 GetResult(){
			return m_result;
		};
		void HandleError(Error *error);
};

class CTaggedWatcher : public CWatcher
{
	protected:
		StrDict* GetTaggedData(StrDict *results);
	public:
		CTaggedWatcher(Int32 result) : CWatcher(result) { };
};

class CFStatWatcher : public CTaggedWatcher
{
	private:
		String m_name;
		struct StatFields *m_stats;
	public:
		CFStatWatcher(Int32 result, const String &name, struct StatFields *stats) 
			: CTaggedWatcher(result)
			, m_name(name)
			, m_stats(stats) { };
		void OutputStat(StrDict *results);
};

class CFileLogWatcher : public CWatcher 
{
private:
	TDynArray< THashMap< String, String > > &m_history;
public:
	CFileLogWatcher( TDynArray< THashMap< String, String > > &history )
		: CWatcher()
		, m_history(history) { };
	void OutputStat( StrDict *varList );
};

class CNewChangelistWatcher : public CTaggedWatcher
{
private:
	const String m_user;
	const String m_client;
	const String m_description;
	Uint32 m_number;

public:
	CNewChangelistWatcher( Int32 result, const String &user, const String &client, const String &description )
		: CTaggedWatcher( result )
		, m_user(user)
		, m_client(client)
		, m_description(description) { };
	void InputData( StrBuf *strbuf, Error *e );
	void OutputInfo( char level, const char *data );

	RED_INLINE Uint32 GetChangelistNumber() const { return m_number; }
};

class CSubmitWatcher : public CTaggedWatcher
{
private:
	const String m_user;
	const String m_client;
	const String m_description;
	const TDynArray< String > &m_chosen;

public:
	CSubmitWatcher(Int32 result, const String &user, const String &client, 
				   const String &description, const TDynArray< String > &chosen)
		: CTaggedWatcher( result )
		, m_user(user)
		, m_client(client)
		, m_description(description)
		, m_chosen(chosen) { };
	void InputData( StrBuf *strbuf, Error *e );
};

class CSyncWatcher : public CTaggedWatcher
{
public:
	CSyncWatcher(Int32 result)
		: CTaggedWatcher( result ) { }
	void OutputStat(StrDict *results);
};

class COpenedWatcher : public CWatcher
{
private:
	TDynArray< String > &m_files;
public:
	COpenedWatcher(Int32 result, TDynArray< String > &files)
		: CWatcher( result ) 
		, m_files( files ) { }
	void OutputStat(StrDict *result);
};

class CFilesWatcher : public CWatcher
{
private:
	TDynArray< String > &m_files;
public:
	CFilesWatcher(Int32 result, TDynArray< String > &files)
		: CWatcher( result ) 
		, m_files( files ) { }
	void OutputStat(StrDict *result);
};

class CWhereWatcher : public CWatcher
{
private:
	String m_depot;
	String m_local;
public:
	CWhereWatcher(Int32 result)
		: CWatcher(result) { };
	void OutputStat(StrDict *result);
	String GetDepot(){ return m_depot; }
	String GetLocal(){ return m_local; }
};

class CLogWatcher : public CWatcher
{
public:
	CLogWatcher(Int32 result)
		: CWatcher(result) { };

	void OutputStat(StrDict *result);
};

class CChangelistWatcher : public CWatcher
{
	ChangelistDescription& m_data;

public:
	CChangelistWatcher(Int32 result, ChangelistDescription& data)
		: CWatcher(result), m_data( data ) { };

	void OutputStat(StrDict *result);
};

class CChangesWatcher : public CWatcher
{
	TDynArray< ChangeDescription >& m_list;

public:
	CChangesWatcher(Int32 result, TDynArray< ChangeDescription >& list )
		: CWatcher(result), m_list( list ) { };

	void OutputStat(StrDict *result);


};

class CKeepAlive : public KeepAlive
{
private:
	Int32 m_counter;
	Bool m_broken;

public:
	RED_INLINE CKeepAlive() : KeepAlive(), m_counter(0), m_broken(false) { };
	RED_INLINE Bool WasBroken(){ return m_broken; }
	RED_INLINE void SetBroken( Bool value ){ m_broken = value; }
	
	int IsAlive();
};