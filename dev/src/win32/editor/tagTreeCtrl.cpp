#include "build.h"
#include "tagTreeCtrl.h"

#include "tagTreeItemData.h"
#include "tagListUpdater.h"

#define ID_TAGS_TREE			1001
#define ID_REMOVE_HISTORY_TAG	54321

BEGIN_EVENT_TABLE(wxTagTreeCtrl, wxTreeListCtrl)
	EVT_TREE_SEL_CHANGED(ID_TAGS_TREE, wxTagTreeCtrl::OnTreeSelectionChanged)
END_EVENT_TABLE()

wxTagTreeCtrl::wxTagTreeCtrl(wxWindow *parent, const TDynArray<CName> &tagList, wxTextCtrl *editCtrl)
: wxTreeListCtrl(parent, ID_TAGS_TREE, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_NO_LINES | wxTR_HIDE_ROOT | wxBORDER_NONE)
, m_editCtrl(editCtrl)
, m_tags(tagList)
, m_refreshPending(true)
, m_lockOnEditTagChanged(false)
, m_filter(TXT("=====UNDEFINED====="))
, m_providersWillBeDisposedExternally( false )
{
	// Setup images
	wxImageList* images = new wxImageList( 16, 16, true, 2 );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TAG_ROOT") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TAG_NODE") ) );
	AssignImageList( images );

	m_availableTagsProviders.PushBack(new CHistoryTagListProvider());
	m_availableTagsProviders.PushBack(new CWorldTagListProvider());

	AddColumn(wxT("Tags"), 250);
	AddColumn(wxT("Count"), 50);

	SetSize(wxSize(320, 150));

	ASSERT(m_editCtrl);
	if (m_editCtrl == NULL)
	{
		m_editCtrl = new wxTextCtrl(parent, wxID_ANY);
	}

	wxString tagString = TXT("");
	for (Uint32 i = 0; i < tagList.Size(); ++i)
	{
		tagString += tagList[i].AsString().AsChar();
		tagString += TXT("; ");
	}

	m_editCtrl->SetValue(tagString);
	m_editCtrl->SetInsertionPointEnd();
	m_editCtrl->Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&wxTagTreeCtrl::OnEditTagChanged, NULL, this);
	Connect(ID_TAGS_TREE, wxEVT_RIGHT_DOWN, wxMouseEventHandler( wxTagTreeCtrl::OnTreeNodeContext ), NULL, this);
	
	UpdateTagTree(true);
}

wxTagTreeCtrl::~wxTagTreeCtrl()
{
	m_editCtrl->Disconnect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&wxTagTreeCtrl::OnEditTagChanged, NULL, this);
	Disconnect(ID_TAGS_TREE, wxEVT_CONTEXT_MENU, (wxObjectEventFunction)&wxTagTreeCtrl::OnTreeNodeContext, NULL, this);

	if ( m_providersWillBeDisposedExternally == false )
	{
		while ( m_availableTagsProviders.Empty() == false )
		{
			delete m_availableTagsProviders.PopBack();
		}
	}
	
}

void wxTagTreeCtrl::RememberTags()
{
	m_tags.Clear();

	TDynArray<String> tags;
	String(m_editCtrl->GetValue().wc_str()).Slice(tags, TXT(";"));

	for(TDynArray<String>::iterator it = tags.Begin(); it != tags.End(); ++it)
	{
		(*it).Trim();
		if (!it->Empty())
		{
			m_tags.PushBackUnique( CName( *it ));

			for (TDynArray<CTagListProvider *>::iterator provIt = m_availableTagsProviders.Begin(); provIt != m_availableTagsProviders.End(); ++provIt)
			{
				(*provIt)->RemeberTag(*it);
			}
		}
	}

	UpdateTagTree(true);
}

void wxTagTreeCtrl::LoadOptionsFromConfig(Bool resize)
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Dialogs/TagViewEditor") );

	// restore frame position and size
	if (resize)
	{
		Int32	x = config.Read( TXT("X"), 50 );
		Int32 y = config.Read( TXT("Y"), 50 );
		Int32 width = config.Read( TXT("Width"), 500 );
		Int32 height = config.Read( TXT("Height"), 400 );

		width = max(50, width);
		height = max(50, height);

		if( x < -1000 )
			x = 50;
		if( y < -1000 )
			y = 50;

		SetSize(x, y, width, height);
	}

	String sTags;
	if (config.Read( TXT("History"), &sTags))
	{
		TDynArray<String> tags;
		String(sTags).Slice(tags, TXT(";"));
		for(TDynArray<String>::iterator it = tags.Begin(); it != tags.End(); ++it)
		{
			(*it).Trim();
			if (!it->Empty())
			{
				//m_historyTags.Insert(*it);
				for (TDynArray<CTagListProvider *>::iterator provIt = m_availableTagsProviders.Begin(); provIt != m_availableTagsProviders.End(); ++provIt)
				{
					(*provIt)->RemeberTag(*it);
				}
			}
		}
	}
}

void wxTagTreeCtrl::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Dialogs/TagViewEditor") );

	// save frame position & size
	// get info from native window placement to keep last (not maximized) size
	WINDOWPLACEMENT wp;
	GetWindowPlacement( GetHwnd(), &wp );

	Int32 x = wp.rcNormalPosition.left;
	Int32 y = wp.rcNormalPosition.top;
	Int32 width = GetSize().x;
	Int32 height = GetSize().y;

	config.Write( TXT("X"), x );
	config.Write( TXT("Y"), y );
	config.Write( TXT("Width"), width );
	config.Write( TXT("Height"), height );

	TSet<String> historyTags;
	TDynArray<String> tags;
	String(m_editCtrl->GetValue().wc_str()).Slice(tags, TXT(";"));
	for(TDynArray<String>::iterator it = tags.Begin(); it != tags.End(); ++it)
	{
		(*it).Trim();
		if (!it->Empty())
		{
			historyTags.Insert(*it);
			for (TDynArray<CTagListProvider *>::iterator provIt = m_availableTagsProviders.Begin(); provIt != m_availableTagsProviders.End(); ++provIt)
			{
				(*provIt)->RemeberTag(*it);
			}
		}
	}

	String sTags;
	for(TSet<String>::iterator it = historyTags.Begin(); it != historyTags.End(); ++it)
	{
		sTags += (*it) + TXT(";");
	}
	config.Write(TXT("History"), sTags);
}

void wxTagTreeCtrl::OnTreeSelectionChanged(wxTreeEvent& event)
{
	// Get selected item
	wxTreeItemId id = GetSelection();
	if (id.IsOk() && GetItemImage(id) == 1)
	{
		String text = GetItemText(id, 0).wc_str();
		if (!text.Empty())
		{
			// Add name to tag list
			CName name(text.AsChar());
			if (!m_tags.PushBackUnique(name))
			{
				m_tags.Erase(Find(m_tags.Begin(), m_tags.End(), name));
				m_tags.PushBackUnique(name);
			}

			text = TXT("");
			for (Uint32 i = 0; i < m_tags.Size(); ++i)
			{
				text += m_tags[i].AsString() + TXT("; ");
			}

			/*Int32 carretPos = m_editCtrl->GetInsertionPoint();
			String tags = m_editCtrl->GetValue().wc_str();

			String leftString = tags.LeftString(carretPos).StringBefore(TXT(";"), true);
			leftString.Trim();
			if (!leftString.Empty())
			{
				leftString += TXT("; ");
			}

			String rightString = tags.MidString(carretPos).StringAfter(TXT(";"));
			rightString.Trim();
			if (!rightString.Empty())
			{
				rightString += rightString.EndsWith(TXT(";")) ? TXT(" ") : TXT("; ");
			}

			text = leftString + text;
			Int32 carretPosEnd = text.GetLength();
			text += TXT("; ") + rightString;*/

			m_lockOnEditTagChanged = true;
			m_editCtrl->SetValue(text.AsChar());
			m_editCtrl->SetInsertionPoint(text.GetLength());
			m_editCtrl->SetSelection(text.Size(), text.Size());
			
			m_lockOnEditTagChanged = false;
			SetFocus();
			UpdateTagTree();
		}
	}
}

void wxTagTreeCtrl::OnTreeNodeContext(wxMouseEvent& event)
{
	// Get selected item
	wxTreeItemId id = GetSelection();
	if (id.IsOk() && !HasChildren(id))
	{
		if (wxTagListProviderTreeItemData *data = static_cast<wxTagListProviderTreeItemData *>(GetItemData(id)))
		{
			const CTagListProvider* provider = data->GetData();
			if ( provider->GetTagGroupName() == TXT("History") )
			{
				wxMenu menu;
				menu.Append(ID_REMOVE_HISTORY_TAG, TXT("Forget tag"), wxEmptyString, false );
				menu.Connect(ID_REMOVE_HISTORY_TAG, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(wxTagTreeCtrl::OnForgetTag), NULL, this);
				PopupMenu( &menu );
			}
		}
	}
}

void wxTagTreeCtrl::OnForgetTag(wxCommandEvent& event)
{
	wxTreeItemId id = GetSelection();
	if (id.IsOk() && !HasChildren(id))
	{
		String tagName = GetItemText(id).wc_str();
		//m_historyTagListProvider->ForgetTag(tag);
		for ( TDynArray<CTagListProvider *>::iterator it = m_availableTagsProviders.Begin(); it != m_availableTagsProviders.End(); ++it)
		{
			(*it)->ForgetTag(tagName);
		}

		TDynArray<CName>::iterator tagIt = Find(m_tags.Begin(), m_tags.End(), CName(tagName.AsChar()));
		if (tagIt != m_tags.End())
		{
			m_tags.Erase(tagIt);
		}

		Delete(id);
	}
}

void wxTagTreeCtrl::OnEditTagChanged(wxCommandEvent& event)
{
	if (m_lockOnEditTagChanged)
		return;

	UpdateTagTree();

	event.Skip();
}

void wxTagTreeCtrl::SetTagListProviders( const TDynArray<CTagListProvider *> providers, Bool providersWillBeDisposedExternally /*= false */ )
{
	if ( m_providersWillBeDisposedExternally == false )
	{
		while (m_availableTagsProviders.Empty() == false)
		{
			delete m_availableTagsProviders.PopBack();
		}
	}

	for (Uint32 i = 0; i < providers.Size(); ++i)
	{
		m_availableTagsProviders.PushBack(providers);
	}

	m_providersWillBeDisposedExternally = providersWillBeDisposedExternally;

	m_filter = TXT("=====UNDEFINED=====");
	UpdateTagTree();
}

void wxTagTreeCtrl::UpdateTagTree(Bool force /*= false*/)
{
	String filter = ExtractFilter();
	filter.Trim();

	if (filter != m_filter || force)
	{
		m_filter = filter;

		// Begin update
		Freeze();
		DeleteRoot();

		// Create list root item
		wxTreeItemId root = AddRoot(TXT("Tags"), 0);
		wxTreeItemData *data = GetItemData(root);
		SetItemData(root, NULL);

		STagNode rootTagNode(TXT("Root"));
		TDynArray<CTagListProvider *>::iterator it;
		for (it = m_availableTagsProviders.Begin(); it != m_availableTagsProviders.End(); ++it)
		{
			(*it)->GetTags( rootTagNode, m_filter );
		}

		BuildTree(root, &rootTagNode);

		// Show tree structure
		ExpandAll(root);

		// End update
		Thaw();
	}
}

void wxTagTreeCtrl::BuildTree(wxTreeItemId parentNode, const STagNode *tagNode)
{
	Int32 imageIndex = tagNode->GetCount() > 0 ? 1 : 0;
	wxTreeItemId node = AppendItem( parentNode, tagNode->GetName().AsChar(), imageIndex, imageIndex );

	if (tagNode->GetCount() > 0)
	{
		// Put provider info only to leaf nodes
		CTagListProvider *provider = const_cast<CTagListProvider *>(tagNode->GetProvider());
		SetItemData(node, new wxTagListProviderTreeItemData(provider));

		// Put count info to leaf nodes
		SetItemText(node, 1, String::Printf(TXT("%i"), tagNode->GetCount()).AsChar());
	}
	const TDynArray<STagNode> &childNodes = tagNode->GetChildNodes();
	for (TDynArray<STagNode>::const_iterator it = childNodes.Begin(); it != childNodes.End(); ++it)
	{
		BuildTree(node, &( *it ));
	}
}

String wxTagTreeCtrl::ExtractFilter()
{
	Int32 caretPos = m_editCtrl ? m_editCtrl->GetInsertionPoint() : 0;
	String filter = m_editCtrl ? m_editCtrl->GetValue().wc_str() : TXT("");

	String leftString  = filter.LeftString(caretPos);
	if ( leftString.ContainsSubstring(TXT(";") ) )
	{
		leftString = leftString.StringAfter(TXT(";"), true);
	}
	filter = leftString;

	//WARN_EDITOR(TXT("Filter: %s"), filter.AsChar());

	return filter;
}
