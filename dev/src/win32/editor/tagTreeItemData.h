#pragma once

class wxInt32TreeItemData : public wxTreeItemData
{
private:
	Int32 m_data;

public:
	wxInt32TreeItemData(Int32 data)
		: m_data(data)
	{}

	void  SetData(Int32 data) { m_data = data; }
	Int32 GetData() const     { return m_data; }
};
