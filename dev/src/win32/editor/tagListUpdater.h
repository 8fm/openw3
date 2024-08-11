#pragma once

class CTagListProvider;

struct STagNode
{
	STagNode(String name, Uint32 count = 0, const CTagListProvider *provider = NULL);

	const String& GetName() const { return m_name; }
	const CTagListProvider *GetProvider() const { return m_provider; }
	Int32 GetCount() const { return m_count; }
	const TDynArray<STagNode>& GetChildNodes() const { return m_childNodes; }

	STagNode &AppendChild(String name, Uint32 count = 0, const CTagListProvider *provider = NULL)
	{
		m_childNodes.PushBack(STagNode(name, count, provider));
		return m_childNodes.Back();
	}

private:

	String m_name;
	const CTagListProvider *m_provider;
	Uint32 m_count;
	TDynArray<STagNode> m_childNodes;
	
};

class CTagListProvider
{
public:

	virtual String GetTagGroupName() const = 0;

	void GetTags( STagNode &root, String& filter );

	virtual void RemeberTag( const String& tag ) {}
	virtual void ForgetTag( const String& tag ) {}
	virtual Bool IsTagAllowed( const String& tag ) { return true; }

	virtual void SaveSession() {}
	virtual void LoadSession() {}

protected:

	virtual Int32 DoGetTags( STagNode &node, String& filter ) = 0;
};


class CHistoryTagListProvider : public CTagListProvider
{
public:

	virtual String GetTagGroupName() const { return TXT("History"); }
	virtual void RemeberTag( const String& tag ) { m_historyTags.Insert( tag ); }
	virtual void ForgetTag( const String& tag ) { m_historyTags.Erase( tag ); }
	
	virtual void SaveSession();
	virtual void LoadSession();

protected:

	virtual Int32 DoGetTags( STagNode &node, String& filter );

private:

	TSet< String > m_historyTags;
};

class CWorldTagListProvider : public CTagListProvider
{

public:

	virtual String GetTagGroupName() const { return TXT( "World" ); }

protected:

	virtual Int32 DoGetTags( STagNode &node, String& filter );
};


class wxTagListProviderTreeItemData : public wxTreeItemData
{
private:
	CTagListProvider *m_data;

public:
	wxTagListProviderTreeItemData(CTagListProvider *data)
		: wxTreeItemData()
		, m_data(data)
	{}

	//void  SetData(Int32 data) { m_data = data; }
	CTagListProvider * GetData() const { return m_data; }
};