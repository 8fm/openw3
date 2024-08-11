#include "build.h"
#include "shlwapi.h"	// DLLVERSIONINFO

#define IDM_POPUP_CLEAR			110
#define IDM_TOOLBAR_ERROR		131
#define IDM_TOOLBAR_WARNING		132
#define IDM_TOOLBAR_MESSAGE		133
#define IDM_TOOLBAR_SPAM		135
#define IDM_TOOLBAR_SEPARATOR	134
#define IDM_TOOLBAR_STAYONTOP	137
#define IDM_TOOLBAR_ONBYDEFAULT	138
#define IDC_LISTVIEW			500
#define IDC_CHANNELVIEW			501

static Bool SDoNotRefresh = false;

CLogOutput* GLogOutput = NULL;

const Int32 CLogOutput::CHANNEL_CHECKLIST_WIDTH	= 150;
const Int32 CLogOutput::CHANNEL_CHECKBOX_WIDTH	= 140;
const Int32 CLogOutput::CHANNEL_CHECKBOX_HEIGHT	= 16;
const Uint32 MAX_LOG_LINE = 100000;

const CName CLogOutput::CHANNEL_ALL = CName(TXT("All"));//CNAME( All );

CLogOutput::SLogMessage::SLogMessage()
	: type( Red::System::Log::P_Error )
	, channelIndex( 0 )
	, hash( Red::CNameHash() )
{
	message[ 0 ] = TXT( '\0' );  
	tickStr[ 0 ] = TXT( '\0' );
	timeStr[ 0 ] = TXT( '\0' );
}

CLogOutput::SLogMessage::SLogMessage(const Char* msg, Red::System::Log::EPriority t, const CName& chnNm, const Uint32& chnIdx, const Char* tck, const Char* time )
	: type( t )
	, channelName( chnNm )
	, channelIndex( chnIdx )
{
	Red::System::StringCopy( message, msg, ARRAY_COUNT( message ) );
	Red::System::StringCopy( tickStr, tck, ARRAY_COUNT( tickStr ) );
	Red::System::StringCopy( timeStr, time, ARRAY_COUNT( timeStr ) );
	CalcHash();
}

void CLogOutput::SLogMessage::CalcHash()
{
	// Generate a unique hash based on the combination of all the parameters that comprise this log message
	Char buffer[ Red::System::Log::MAX_LINE_LENGTH + 256 ];
	Int32 length = Red::System::SNPrintF
	(
		buffer,
		Red::System::Log::MAX_LINE_LENGTH,
		TXT( "%s%u%i%s%s" ),
		message,
		channelIndex,
		type,
		tickStr,
		timeStr
	);

	if( length == -1 )
	{
		length = Red::System::Log::MAX_LINE_LENGTH - 1;
	}

	const Uint32 maxLength = Red::System::Log::MAX_LINE_LENGTH + 256;
	AnsiChar cbuf[ maxLength ];
	Red::System::StringConvert( cbuf, buffer, maxLength );

	hash = Red::CNameHash::Hash( cbuf );
}

CLogOutput::CMessageQueue::CMessageQueue()
{
	m_messages.Reserve( 20 );
}

void CLogOutput::CMessageQueue::AddEntry( const SLogMessage& msg )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	
	m_messages.PushBack( msg );
}

Uint32 CLogOutput::CMessageQueue::Process( CLogOutput& logOutput, TDynArray<SLogMessage>& outList )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

	const Uint32 entryCount = outList.Size();

	// Merge cached buffer with the main one
	for (Uint32 i = 0; i < m_messages.Size(); ++i)
	{
		SLogMessage &logMsg = m_messages[ i ];

		// HACK: Give the logMsg a channel on the log thread for now...
		logMsg.channelIndex = logOutput.AddChannel( logMsg.channelName );

		const Char * token = Red::System::StringSearch( logMsg.message, TXT('\n') );
		if( token != nullptr )
		{
			const Char * previous = logMsg.message;
			while( token != nullptr )
			{
				Char message[ 256 ];
				Red::System::StringCopy( message, previous, token - previous + 1 );		
				outList.PushBack( SLogMessage( message, logMsg.type, logMsg.channelName, logMsg.channelIndex, logMsg.tickStr, logMsg.timeStr) );
				previous = token + 1;
				token = Red::System::StringSearch( token + 1, TXT('\n') );
			}
		}
		else
		{
			outList.PushBack( logMsg );
		}
	}
	
	m_messages.Clear();

	const Uint32 entryAdded = outList.Size() - entryCount;

	return entryAdded;
}

Bool CLogOutput::SLogMessage::operator==( const SLogMessage& log ) const
{
	return hash == log.hash;
}

CLogOutput::CLogOutput( Uint32 size )
	: Red::Threads::CThread( "CLogOutput" )
	, Red::System::Log::OutputDevice()
	, m_hInstance(GetModuleHandle(0))
	, m_hImageList(NULL)
	, m_hFont(NULL)
	, m_hWindow(NULL)
	, m_hToolbar(NULL)
	, m_hListView(NULL)
	, m_hTooltip(NULL)
	, m_hChannelList( NULL )
	, m_toolInfoSize(GetSizeOfToolInfoStructure())
	, m_lastCoveredItem(-1)
	, m_stayOnTop(false)
	, m_channelsOnByDefault(false)
	, m_saveConfig( false )
	, m_requestExit( false )
	, m_ready( false )
	, m_numLogMessages( 0 )
	, m_size( Min( size, MAX_LOG_LINE ) )
{
	SetUnsafeToCallOnCrash();
	m_logs.Reserve( m_size );
	RED_WARNING( size <= MAX_LOG_LINE, "Log Buffer is too big. Maximum allowed number of line is: %d", MAX_LOG_LINE );
}

CLogOutput::~CLogOutput()
{
	
}

void CLogOutput::RequestExit()
{
	if ( ! m_ready.GetValue() )
	{
		return;
	}

	m_requestExit.SetValue( true );
}

void CLogOutput::ThreadFunc()
{
	CreateLogWindow();
	m_ready.SetValue( true );

	// Begin message loop
	MSG msg;
	BOOL ret;
	while ( ( ret = ::GetMessage(&msg, NULL, 0, 0) ) )
	{
		if ( ret == -1 ) break;
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

		if( m_saveConfig.Exchange( false ) )
		{
			DoSaveOptionsToConfig();
		}

		if ( m_requestExit.Exchange( false ) )
		{
			m_ready.SetValue( false );
			DestroyLogWindow();
		}
	}
}

void CLogOutput::CreateLogWindow()
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= &CLogOutput::WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= m_hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= TXT("CLogOutput");
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	// Perform application initialization
	m_hWindow = ::CreateWindow(TXT("CLogOutput"), TXT("Log Window"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInstance, this );
	if (!m_hWindow)
	{
		throw TXT("Creation of Log Window failed.");
	}

	m_hFont = ::CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Consolas"));

	CreateToolbar();
	CreateListView();

	m_hChannelList = ::CreateWindow(WC_LISTVIEW, NULL, WS_BORDER|WS_CHILD|WS_VISIBLE|LVS_LIST, 0, 0, 1, 1, m_hWindow, (HMENU)IDC_CHANNELVIEW, m_hInstance, NULL);
	ListView_SetExtendedListViewStyle(m_hChannelList, LVS_EX_CHECKBOXES);

	AddChannel( CHANNEL_ALL );

	::SetTimer(m_hWindow, NULL, 500, NULL);

	::UpdateWindow(m_hWindow);

	LoadOptionsFromConfig();

	OnResize();

	HWND insertAfter = m_stayOnTop ? HWND_TOPMOST : HWND_TOP;
	::SetWindowPos(m_hWindow, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void CLogOutput::DestroyLogWindow()
{
	DoSaveOptionsToConfig();
	//::ShowWindow(m_hWindow, SW_HIDE);

	::DeleteObject(m_hImageList);
	::DeleteObject(m_hFont);

	// Destroy the window
	::DestroyWindow( m_hWindow );
}

void CLogOutput::Write( const Red::System::Log::Message& message )
{
	if ( ! m_ready.GetValue() )
	{
		return;
	}

	// Get image index
	SLogMessage logMsg;
	
	Red::System::StringCopy( logMsg.message, message.text, ARRAY_COUNT( logMsg.message ) );

	// Possibly creates a new CName so use the text and not the hash
	logMsg.channelName = CName( message.channelText ); // for now, find the channel when processing the msg queue on the log thread

	Red::System::SNPrintF( logMsg.tickStr, ARRAY_COUNT( logMsg.tickStr ), TXT( "%I64u" ), message.tick );
	Red::System::SNPrintF
	(
		logMsg.timeStr,
		ARRAY_COUNT( logMsg.timeStr ),
		TXT( "%u:%u:%u.%u" ),
		message.dateTime.GetHour(),
		message.dateTime.GetMinute(),
		message.dateTime.GetSecond(),
		message.dateTime.GetMilliSeconds()
	);

	logMsg.type = message.priority;
	logMsg.CalcHash();
	
	m_messageQueue.AddEntry( logMsg );
}

void CLogOutput::SaveOptionsToConfig()
{
	if ( ! m_ready.GetValue() )
	{
		return;
	}

	m_saveConfig.SetValue( true );
}

void CLogOutput::DoSaveOptionsToConfig()
{
	RECT rect;
	::GetWindowRect(m_hWindow, &rect);
	BOOL isVisible = ::IsWindowVisible(m_hWindow);

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	if ( rect.left > -2000 && rect.top > -2000 )
	{
		// Serialize window's properties
		config.Write(TXT( "/Frames/Output2/Left" ), (Int32)rect.left);
		config.Write(TXT( "/Frames/Output2/Top" ), (Int32)rect.top);
		config.Write(TXT( "/Frames/Output2/Width" ), (Int32)(rect.right - rect.left));
		config.Write(TXT( "/Frames/Output2/Height" ), (Int32)(rect.bottom - rect.top));
		config.Write(TXT( "/Frames/Output2/Visible" ), (Int32)isVisible);
	}

	config.Write(TXT( "/Frames/Output2/StayOnTop" ), (Int32)m_stayOnTop);

	// Serialize the width of each column of ListView control
	config.Write(TXT( "/Frames/Output2/column_Id" ), ListView_GetColumnWidth(m_hListView, COLUMN_ID));
	config.Write(TXT( "/Frames/Output2/column_Channel" ), ListView_GetColumnWidth(m_hListView, COLUMN_CHANNEL));
	config.Write(TXT( "/Frames/Output2/column_Tick" ), ListView_GetColumnWidth(m_hListView, COLUMN_TICK));
	config.Write(TXT( "/Frames/Output2/column_Time" ), ListView_GetColumnWidth(m_hListView, COLUMN_TIME));
	config.Write(TXT( "/Frames/Output2/column_Message" ), ListView_GetColumnWidth(m_hListView, COLUMN_MESSAGE));

	String channels;
	if ( false == m_channels.Empty() )
	{
		for ( Uint32 i = 0; i < m_channels.Size(); ++i )
		{
			if ( m_channels[ i ].m_active )
			{
				channels.Append( m_channels[ i ].m_text.AsChar(), m_channels[ i ].m_text.GetLength() ).Append( L';' );
			}
		}

		config.Write( TXT("/Frames/Output2/channels"), channels ); 
	}

}

void CLogOutput::LoadOptionsFromConfig()
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	// Deserialize window's properties
	Int32 left = (Int32)config.Read(TXT( "/Frames/Output2/Left" ), 10);
	Int32 top = (Int32)config.Read(TXT( "/Frames/Output2/Top" ), 10);
	Int32 width = (Int32)config.Read(TXT( "/Frames/Output2/Width" ), 640);
	Int32 height = (Int32)config.Read(TXT( "/Frames/Output2/Height" ), 690);
	BOOL isVisible = (BOOL)config.Read(TXT( "/Frames/Output2/Visible" ), FALSE);
	m_stayOnTop = config.Read(TXT( "/Frames/Output2/StayOnTop" ), 0) ? true : false;

	if ( left < -2000 || top < -2000 )
	{
		left = 10;
		top = 10;
		width = 640;
		height = 690;
	}

	MoveWindow(m_hWindow, left, top, width, height, FALSE);
	ShowWindow(m_hWindow, isVisible ? SW_SHOWNORMAL : SW_HIDE);

	SendMessage(m_hToolbar, TB_CHECKBUTTON, (WPARAM)IDM_TOOLBAR_STAYONTOP, m_stayOnTop ? TRUE : FALSE);

	// Deserialize the width of each column of ListView control
	ListView_SetColumnWidth(m_hListView, COLUMN_ID, config.Read(TXT( "/Frames/Output2/column_Id" ), 25));
	ListView_SetColumnWidth(m_hListView, COLUMN_CHANNEL, config.Read(TXT( "/Frames/Output2/column_Channel" ), 70));
	ListView_SetColumnWidth(m_hListView, COLUMN_TICK, config.Read(TXT( "/Frames/Output2/column_Tick" ), 70));
	ListView_SetColumnWidth(m_hListView, COLUMN_TIME, config.Read(TXT( "/Frames/Output2/column_Time" ), 70));
	ListView_SetColumnWidth(m_hListView, COLUMN_MESSAGE, config.Read(TXT( "/Frames/Output2/column_Message" ), 500));

	const Bool channelsOnByDefault = m_channelsOnByDefault;
	m_channelsOnByDefault = true;
	String channels = config.Read( TXT("/Frames/Output2/channels"), String::EMPTY );
	CTokenizer tokenizer( channels, TXT(";") );
	for ( Uint32 i = 0; i < tokenizer.GetNumTokens(); ++i )
	{
		String token = tokenizer.GetToken( i );
		if ( token.Empty() )
		{
			continue;
		}

		AddChannel( CName( token ) ); 
	}
	m_channelsOnByDefault = channelsOnByDefault;
}

Uint32 CLogOutput::AddChannel(const CName& channel)
{
	if ( m_channels.PushBackUnique( channel ) )
	{
		static LVITEM lvIData[2048]; // this needs to be static because the pointer must be valid when the LVM_INSERTITEM message is processed
		
		int index = m_channels.Size() - 1;
		if ( index >= 1024 )
		{
			::MessageBoxA( NULL, "Too many channels. You cannot have more than 1024 channels (what were you thinking???)", "Too many channels", MB_ICONERROR|MB_OK );
			return 0;
		}

		m_channels[ index ].m_active = m_channelsOnByDefault;

		LVITEM* lvI = lvIData + index;
		ZeroMemory( lvI, sizeof(LVITEM) );
		lvI->pszText   = (LPWSTR)m_channels[ index ].m_text.AsChar();
		lvI->mask      = LVIF_TEXT | LVIF_STATE;
		lvI->stateMask = 0;
		lvI->iSubItem  = 0;
		lvI->state     = 0;
		lvI->iItem     = index;
		PostMessage( m_hChannelList, LVM_INSERTITEM, 0, (LPARAM)lvI );

		lvI = lvIData + ( index + 1024 );
		ZeroMemory( lvI, sizeof(LVITEM) );
		lvI->stateMask = LVIS_STATEIMAGEMASK;
		lvI->state     = INDEXTOSTATEIMAGEMASK( ( m_channels[ index ].m_active )? 2 : 1 );
		PostMessage( m_hChannelList, LVM_SETITEMSTATE, index, (LPARAM)lvI );

		return m_channels.Size() - 1;
	}
	else
	{
		for( Uint32 i = 0; i < m_channels.Size(); ++i )
		{
			if( m_channels[ i ].m_name == channel )
			{
				return i;
			}
		}
	}

	return 0;
}

void CLogOutput::MergeBuffers()
{
	Uint32 entriesAdded = m_messageQueue.Process( *this, m_logs ); // HACK to add channels on the log thread. Should refactor this more...
	
	if( entriesAdded )
	{
		if( m_logs.Size() > m_size )
		{
			const Uint32 entryToErase = m_logs.Size() - m_size;
			m_logs.Erase( m_logs.Begin(), m_logs.Begin() + entryToErase );
		}

		Uint32 startIndex = entriesAdded < m_logs.Size() ? m_logs.Size() - entriesAdded : 0;

		// Refresh list view
		Refresh( startIndex );
	}
}

void CLogOutput::Refresh(Uint32 startIndex)
{
	// Clear list view
	if (startIndex == 0)
	{
		LockWindowUpdate( m_hListView );
		ListView_DeleteAllItems(m_hListView);
	}

	Bool priorityDisabled[ Red::System::Log::P_Count ];

	priorityDisabled[ Red::System::Log::P_Error ]		= SendMessage(m_hToolbar, TB_ISBUTTONCHECKED, (WPARAM)IDM_TOOLBAR_ERROR, 0) == 0;
	priorityDisabled[ Red::System::Log::P_Warning ]		= SendMessage(m_hToolbar, TB_ISBUTTONCHECKED, (WPARAM)IDM_TOOLBAR_WARNING, 0) == 0;
	priorityDisabled[ Red::System::Log::P_Information ]	= SendMessage(m_hToolbar, TB_ISBUTTONCHECKED, (WPARAM)IDM_TOOLBAR_MESSAGE, 0) == 0;
	priorityDisabled[ Red::System::Log::P_Spam ]		= SendMessage(m_hToolbar, TB_ISBUTTONCHECKED, (WPARAM)IDM_TOOLBAR_SPAM, 0) == 0;
	
	Int32 repeatCount = 1;
	const Uint32 initialItemCount = ListView_GetItemCount(m_hListView);
	Uint32 index = initialItemCount;	

	for ( Uint32 i = startIndex; i < m_logs.Size(); ++i )
	{
		SLogMessage &logMsg = m_logs[i];

		Int32 numberOfChannels = m_channels.Size();
		
		if( numberOfChannels > 0 )
		{
			ASSERT( m_channels[ 0 ].m_name == CHANNEL_ALL );

			// If not all active process single
			if( !m_channels[ 0 ].m_active && !m_channels[ logMsg.channelIndex ].m_active )
			{
				continue;
			}
		}

		ASSERT( logMsg.type < Red::System::Log::P_Count );

		if( priorityDisabled[ logMsg.type ] )
		{
			continue;
		}

		if (i + 1 < m_logs.Size())
		{
			if (m_logs[i + 1] == logMsg)
			{
				++repeatCount;
				continue;
			}
		}

		String repeatMessage;
		const Char * displayedMessage = logMsg.message;

		if (repeatCount > 1)
		{
			repeatMessage = String::Printf(TXT("%s   {x %d}"), logMsg.message, repeatCount);
			repeatCount = 1;
			displayedMessage = repeatMessage.AsChar();
		}

		LVITEM itemId;
		itemId.mask = LVIF_IMAGE;
		itemId.iItem = index;
		itemId.iSubItem = COLUMN_ID;
		itemId.iImage = logMsg.type;
		itemId.iIndent = 1;
		ListView_InsertItem(m_hListView, &itemId);

		LVITEM itemChn;
		itemChn.mask = LVIF_TEXT;
		itemChn.iItem = index;
		itemChn.iSubItem = COLUMN_CHANNEL;
		itemChn.pszText = const_cast<Char *>( m_channels[ logMsg.channelIndex ].m_text.AsChar() );
		itemChn.cchTextMax = m_channels[ logMsg.channelIndex ].m_text.GetLength();
		ListView_SetItem(m_hListView, &itemChn);

		Char* tickStr = logMsg.tickStr;		
		LVITEM itemTick;
		itemTick.mask = LVIF_TEXT;
		itemTick.iItem = index;
		itemTick.iSubItem = COLUMN_TICK;
		itemTick.pszText = tickStr;
		itemTick.cchTextMax = Red::System::StringLength( tickStr );
		ListView_SetItem(m_hListView, &itemTick);

		Char* engineTimeStr = logMsg.timeStr;
		LVITEM itemTime;
		itemTime.mask = LVIF_TEXT;
		itemTime.iItem = index;
		itemTime.iSubItem = COLUMN_TIME;
		itemTime.pszText = engineTimeStr;
		itemTime.cchTextMax = Red::System::StringLength( engineTimeStr );
		ListView_SetItem(m_hListView, &itemTime);

		LVITEM itemMsg;
		itemMsg.mask = LVIF_TEXT;
		itemMsg.iItem = index;
		itemMsg.iSubItem = COLUMN_MESSAGE;
		itemMsg.pszText = const_cast<Char *>( displayedMessage );
		itemMsg.cchTextMax = Red::System::StringLength( displayedMessage );
		ListView_SetItem(m_hListView, &itemMsg);
		++index;
	}

	Int32 selectionMark = ListView_GetSelectionMark(m_hListView);
	ASSERT(index == ListView_GetItemCount(m_hListView));
	if (selectionMark == initialItemCount - 1)
	{
		ListView_SetItemState(m_hListView, initialItemCount - 1, 0, LVIS_SELECTED | LVIS_FOCUSED);
		selectionMark = ListView_SetSelectionMark(m_hListView, -1);
	}

	if (selectionMark == -1)
	{
		ListView_EnsureVisible(m_hListView, index - 1, FALSE);
	}

	LockWindowUpdate( NULL );
}

void CLogOutput::CreateImageList()
{
	if (m_hImageList == NULL)
	{
		// Create image list
		HBITMAP imghandle = (HBITMAP)LoadImage( NULL, TXT( "log_icons16.bmp" ), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );

		if( imghandle )
		{
			m_hImageList = ImageList_Create( 16, 16, ILC_COLOR32, 6, 0 );

			ImageList_Add( m_hImageList, imghandle, NULL );

			DeleteObject( imghandle );
		}
	}
}

void CLogOutput::CreateToolbar()
{
	const String Errors					= TXT( "Errors" );
	const String Warnings				= TXT( "Warnings" );
	const String Messages				= TXT( "Information" );
	const String Spam					= TXT( "Spam" );
	const String StayOnTop				= TXT( "Stay on top" );
	const String ChannelsOnByDefault	= TXT( "Channels on by default" );
	const int BitmapSize = 16;

	CreateImageList();

	// Register Toolbar and make it available to the application
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_BAR_CLASSES; 
	InitCommonControlsEx(&icc); 

	m_hToolbar = CreateWindowEx(NULL, TOOLBARCLASSNAME, 0, WS_CHILD | WS_VISIBLE | TBSTYLE_LIST, 0, 0, 0, 0, m_hWindow, NULL, m_hInstance, NULL); 
	if (!m_hToolbar)
	{
	   throw L"Unable to create toolbar control.";
	}

	// Set font
	SendMessage(m_hToolbar, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)TRUE);

	// Set images list
	SendMessage(m_hToolbar, TB_SETIMAGELIST, 0, (LPARAM)m_hImageList);
	SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(BitmapSize, BitmapSize));

	Int32 i = 0;
	TBBUTTON buttons[ Red::System::Log::P_Count + 3 ]; 
	ZeroMemory(buttons, sizeof(buttons));

	// Button Errors
	buttons[i].iBitmap = Red::System::Log::P_Error;
	buttons[i].idCommand = IDM_TOOLBAR_ERROR;
	buttons[i].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED;
	buttons[i].fsStyle = TBSTYLE_BUTTON | TBSTYLE_CHECK;
	buttons[i++].iString = (INT_PTR)Errors.AsChar();

	// Button Warnings
	buttons[i].iBitmap = Red::System::Log::P_Warning;
	buttons[i].idCommand = IDM_TOOLBAR_WARNING;
	buttons[i].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED;
	buttons[i].fsStyle = TBSTYLE_BUTTON | TBSTYLE_CHECK;
	buttons[i++].iString = (INT_PTR)Warnings.AsChar();

	// Information button
	buttons[i].iBitmap = Red::System::Log::P_Information;
	buttons[i].idCommand = IDM_TOOLBAR_MESSAGE;
	buttons[i].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED;
	buttons[i].fsStyle = TBSTYLE_BUTTON | TBSTYLE_CHECK;
	buttons[i++].iString = (INT_PTR)Messages.AsChar();

	// Spam button
	buttons[i].iBitmap = Red::System::Log::P_Spam;
	buttons[i].idCommand = IDM_TOOLBAR_SPAM;
	buttons[i].fsState = TBSTATE_ENABLED;
	buttons[i].fsStyle = TBSTYLE_BUTTON | TBSTYLE_CHECK;
	buttons[i++].iString = (INT_PTR)Spam.AsChar();

	// Separator
	buttons[i].iBitmap = -1;
	buttons[i].idCommand = IDM_TOOLBAR_SEPARATOR;
	buttons[i].fsState = TBSTATE_INDETERMINATE;
	buttons[i].fsStyle = TBSTYLE_SEP;
	buttons[i++].iString = -1;

	// Stay on top
	buttons[i].iBitmap = 4;
	buttons[i].idCommand = IDM_TOOLBAR_STAYONTOP;
	buttons[i].fsState = TBSTATE_ENABLED;
	buttons[i].iString = (INT_PTR)StayOnTop.AsChar();
	buttons[i++].fsStyle = TBSTYLE_CHECK | BTNS_AUTOSIZE;

	// Channels On By Default
	buttons[i].iBitmap = 5;
	buttons[i].idCommand = IDM_TOOLBAR_ONBYDEFAULT;
	buttons[i].fsState = TBSTATE_ENABLED | ( m_channelsOnByDefault ? TBSTATE_CHECKED : 0 );
	buttons[i].iString = (INT_PTR)ChannelsOnByDefault.AsChar();
	buttons[i++].fsStyle = TBSTYLE_CHECK | BTNS_AUTOSIZE;

	// Create buttons
	SendMessage(m_hToolbar, TB_ADDBUTTONS, (WPARAM)(sizeof(buttons) / sizeof(TBBUTTON)), (LPARAM)buttons);

	// Set width of toolbar items
	TBBUTTONINFO tbbi;
	ZeroMemory(&tbbi, sizeof(tbbi));
	tbbi.cbSize = sizeof(tbbi);
	tbbi.dwMask = TBIF_SIZE;

	HDC hDC = GetDC(m_hListView);
	SelectObject(hDC, m_hFont);
	SIZE size;

	// Button Errors
	GetTextExtentPoint32(hDC, Errors.AsChar(), (Int32)Errors.GetLength(), &size);
	tbbi.cx = 5 + BitmapSize + 3 + size.cx + 5;
	SendMessage(m_hToolbar, TB_SETBUTTONINFO, IDM_TOOLBAR_ERROR, (LPARAM)&tbbi);

	// Button Warnings
	GetTextExtentPoint32(hDC, Warnings.AsChar(), (Int32)Warnings.GetLength(), &size);
	tbbi.cx = 5 + BitmapSize + 3 + size.cx + 5;
	SendMessage(m_hToolbar, TB_SETBUTTONINFO, IDM_TOOLBAR_WARNING, (LPARAM)&tbbi);

	// Button Messages
	GetTextExtentPoint32(hDC, Messages.AsChar(), (Int32)Messages.GetLength(), &size);
	tbbi.cx = 5 + BitmapSize + 3 + size.cx + 5;
	SendMessage(m_hToolbar, TB_SETBUTTONINFO, IDM_TOOLBAR_MESSAGE, (LPARAM)&tbbi);

	// Button Spam
	GetTextExtentPoint32(hDC, Spam.AsChar(), (Int32)Spam.GetLength(), &size);
	tbbi.cx = 5 + BitmapSize + 3 + size.cx + 5;
	SendMessage(m_hToolbar, TB_SETBUTTONINFO, IDM_TOOLBAR_SPAM, (LPARAM)&tbbi);

	ReleaseDC(m_hListView, hDC);
}

void CLogOutput::CreateListView()
{
	CreateImageList();

	// Register ListView and make it available to the application
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_LISTVIEW_CLASSES; 
	InitCommonControlsEx(&icc); 

	m_hListView = CreateWindowEx(NULL, WC_LISTVIEW, TXT("Logger"), LVS_REPORT | WS_CHILD | WS_VISIBLE, 0, 0, 300, 200, m_hWindow, (HMENU)IDC_LISTVIEW, m_hInstance, NULL);
	if (!m_hListView)
	{
		throw TXT("Unable to create list view control.");
	}

	// Set font
	SendMessage(m_hListView, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)TRUE);
	SendMessage(m_hListView, LVM_SETTEXTCOLOR, 0, 0x00cccccc);
	SendMessage(m_hListView, LVM_SETTEXTBKCOLOR, 0, 0);

	ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT /*| LVS_EX_GRIDLINES*/);
	ListView_SetImageList(m_hListView, m_hImageList, LVSIL_SMALL);

	// Create columns
	LVCOLUMN columnType;
	String columnTypeName(TXT("Id"));
	columnType.mask = LVCF_TEXT | LVCF_WIDTH;
	columnType.cx = 26;
	columnType.pszText = const_cast<Char *>(columnTypeName.AsChar());
	columnType.cchTextMax = columnTypeName.GetLength();
	ListView_InsertColumn(m_hListView, COLUMN_ID, &columnType); 

	LVCOLUMN columnChannel;
	String columnChannelName(TXT("Channel"));
	columnChannel.mask = LVCF_TEXT | LVCF_WIDTH;
	columnChannel.cx = 70;
	columnChannel.pszText = const_cast<Char *>(columnChannelName.AsChar());
	columnChannel.cchTextMax = columnChannelName.GetLength();
	ListView_InsertColumn(m_hListView, COLUMN_CHANNEL, &columnChannel); 

	LVCOLUMN columnTick;
	String columnTickName(TXT("Tick"));
	columnTick.mask = LVCF_TEXT | LVCF_WIDTH;
	columnTick.cx = 70;
	columnTick.pszText = const_cast<Char *>(columnTickName.AsChar());
	columnTick.cchTextMax = columnTickName.GetLength();
	ListView_InsertColumn(m_hListView, COLUMN_TICK, &columnTick);

	LVCOLUMN columnTime;
	String columnTimeName(TXT("Time"));
	columnTime.mask = LVCF_TEXT | LVCF_WIDTH;
	columnTime.cx = 70;
	columnTime.pszText = const_cast<Char *>(columnTimeName.AsChar());
	columnTime.cchTextMax = columnTimeName.GetLength();
	ListView_InsertColumn(m_hListView, COLUMN_TIME, &columnTime);

	LVCOLUMN columnMessage;
	String columnMessageName(TXT("Message"));
	columnMessage.mask = LVCF_TEXT | LVCF_WIDTH;
	columnMessage.cx = 1500;
	columnMessage.pszText = const_cast<Char *>(columnMessageName.AsChar());
	columnMessage.cchTextMax = columnMessageName.GetLength();
	ListView_InsertColumn(m_hListView, COLUMN_MESSAGE, &columnMessage); 
}

HWND CLogOutput::CreateTooltip(HWND hParent)
{
	// Register Tootip
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_WIN95_CLASSES; 
	InitCommonControlsEx(&icc);

	HWND hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hParent, NULL, m_hInstance, NULL); 

	SetWindowPos(hTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	TOOLINFO ti;
	ti.cbSize = m_toolInfoSize; 
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; 
	ti.hwnd = m_hWindow; 
	ti.hinst = NULL; 
	ti.uId = (UINT_PTR)hParent; 
	ti.lpszText = L"";
	GetClientRect(hParent, &ti.rect); 
	SendMessage(hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
	SendMessage(hTooltip, TTM_SETMAXTIPWIDTH, 0, 500);

	return hTooltip;
}

void CLogOutput::OnClear()
{
	// Restore channels
	SendMessage( m_hChannelList, LVM_DELETEALLITEMS, 0, 0 );

	m_channels.Clear();
	AddChannel( CHANNEL_ALL );

	m_logs.Clear();
	m_messageQueue.Clear();
	Refresh();
}

void CLogOutput::OnResize()
{
	RECT clientRect;
	::GetClientRect(m_hWindow, &clientRect);
	RECT toolbarRect;
	::GetClientRect(m_hToolbar, &toolbarRect);

	const int toolbarHeight = toolbarRect.bottom - toolbarRect.top + 1;
	::MoveWindow(m_hToolbar, 0, 0, clientRect.right - clientRect.left, toolbarHeight, true);
	::MoveWindow(m_hListView, 0, toolbarHeight, clientRect.right - clientRect.left - CHANNEL_CHECKLIST_WIDTH, (clientRect.bottom - clientRect.top) - toolbarHeight, true);
	::MoveWindow(m_hChannelList, clientRect.right - CHANNEL_CHECKLIST_WIDTH, toolbarHeight, CHANNEL_CHECKLIST_WIDTH, clientRect.bottom - clientRect.top - toolbarHeight, true);
}

void CLogOutput::OnListViewMouseMove()
{
	POINT pos;
	::GetCursorPos(&pos);
	::ScreenToClient(m_hListView, &pos);

	LVHITTESTINFO info;
	ZeroMemory(&info, sizeof(LVHITTESTINFO));
	info.pt = pos;
	ListView_SubItemHitTest(m_hListView, &info);

	if (info.flags & LVHT_ONITEM)
	{
		if (info.iSubItem == COLUMN_MESSAGE)
		{
			ListView_GetItemText(m_hListView, info.iItem, info.iSubItem, m_buffer, sizeof(m_buffer) / sizeof(Char));

			SIZE size = { 0, 0 };
			HDC hDC = GetDC(m_hListView);
			HFONT hFont = (HFONT)SendMessage(m_hListView, WM_GETFONT, 0, 0);
			SelectObject(hDC, hFont);
			BOOL ret = GetTextExtentPoint32(hDC, m_buffer, Red::System::StringLength(m_buffer), &size);
			ReleaseDC(m_hListView, hDC);
			int textWidth = size.cx; //ListView_GetStringWidth(m_hListView, s_buffer);
			int columnWidth = ListView_GetColumnWidth(m_hListView, COLUMN_MESSAGE);

			if (textWidth > columnWidth)
			{
				if (m_hTooltip == NULL)
					m_hTooltip = CreateTooltip(m_hListView);

				SendMessage(m_hTooltip, TTM_ACTIVATE, (WPARAM)TRUE, 0L);
				if (m_lastCoveredItem != info.iItem)
				{
					TOOLINFO ti;
					ti.cbSize = m_toolInfoSize; 
					ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; 
					ti.hwnd = m_hWindow; 
					ti.hinst = NULL; 
					ti.uId = (UINT_PTR)m_hListView; 
					ti.lpszText = m_buffer;
					ListView_GetSubItemRect(m_hListView, info.iItem, info.iSubItem, LVIR_BOUNDS, &ti.rect);
					SendMessage(m_hTooltip, TTM_SETTOOLINFO, 0, (LPARAM) &ti);
					SendMessage(m_hTooltip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
					m_lastCoveredItem = info.iItem;
				}

				GetCursorPos(&pos);
				SendMessage(m_hTooltip, TTM_TRACKPOSITION, 0, (LPARAM) MAKELONG(pos.x, pos.y));
				return;
			}
		}
	}
	SendMessage(m_hTooltip, TTM_ACTIVATE, (WPARAM)FALSE, 0L);
	m_lastCoveredItem = -1;
}

void CLogOutput::OnCopyToClipboard()
{
	// Open the clipboard
	if (!OpenClipboard(m_hWindow)) 
		return;

	// Empty the clipboard
	EmptyClipboard(); 

	String copyText;
	const Int32 messageColumn = COLUMN_MESSAGE;
	Int32 selectedCount = ListView_GetSelectedCount(m_hListView);
	int itemIndex = -1;
	while(selectedCount--)
	{
		itemIndex = ListView_GetNextItem(m_hListView, itemIndex, LVNI_ALL | LVNI_SELECTED);
		if (itemIndex == -1)
			break;

		ListView_GetItemText(m_hListView, itemIndex, messageColumn, m_buffer, sizeof(m_buffer) / sizeof(Char));
		if (!copyText.Empty())
			copyText += TXT("\r\n");
		copyText += m_buffer;
	}
	ASSERT(selectedCount == -1);
	
	// Allocate a global memory object for the text.
	HGLOBAL hGlobalCopy = GlobalAlloc(GMEM_MOVEABLE, (copyText.GetLength() + 1) * sizeof(Char)); 
	if (hGlobalCopy != NULL) 
	{ 
		// Lock the handle and copy the text to the buffer. 
		LPTSTR lpStrCopy = (LPTSTR)GlobalLock(hGlobalCopy); 
		Red::System::MemoryCopy(lpStrCopy, copyText.AsChar(), copyText.GetLength() * sizeof(Char)); 
		lpStrCopy[copyText.GetLength()] = (Char)0;    // null character 
		GlobalUnlock(hGlobalCopy); 

		// Put data to the clipboard
		SetClipboardData(CF_UNICODETEXT, hGlobalCopy);
	} 

	// Close the clipboard. 
	CloseClipboard();
}

/*
Bool CLogOutput::OnButtonClicked( HWND hButton )
{
	Int32 s = m_channels.Size();
	for( Int32 c=0; c<s; c++ )
	{
		if( m_channels[c].m_hCheckBox == hButton )
		{
			Bool newVal;
			if( SendMessage( hButton, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
			{
				newVal= true;
			}
			else
			{
				newVal = false;
			}

			if( m_channels[c].m_name == CHANNEL_ALL )
			{
				for( Int32 j=0; j<s; j++ )
				{
					m_channels[j].m_active = newVal;
					if( j!=c )
					{						
						WPARAM checked = newVal ? BST_CHECKED : BST_UNCHECKED;
						SendMessage( m_channels[j].m_hCheckBox, BM_SETCHECK, checked, 0 ); 
					}
				}
				Refresh();
			}
			else
			{
				if( m_channels[c].m_active != newVal )
				{
					m_channels[c].m_active = newVal;

					if( newVal )
					{
						Bool allActive = true;
						for( Int32 j=0; j<s; j++ )
						{							
							if( m_channels[j].m_name != CHANNEL_ALL )
							{
								if( !m_channels[j].m_active )
								{
									allActive = false;
									break;
								}								
							}
						}

						if( allActive )
						{
							// Check 'All'
							ASSERT( m_channels[0].m_name == CHANNEL_ALL );
							SendMessage( m_channels[0].m_hCheckBox, BM_SETCHECK, BST_CHECKED, 0 ); 
							m_channels[0].m_active = true;
						}
					}
					else
					{
						// Uncheck 'All'
						ASSERT( m_channels[0].m_name == CHANNEL_ALL );
						SendMessage( m_channels[0].m_hCheckBox, BM_SETCHECK, BST_UNCHECKED, 0 ); 
						m_channels[0].m_active = false;
					}					

					Refresh();		
				}
			}

			return true;;
		}
	}

	return false;
}
*/

int CLogOutput::GetSizeOfToolInfoStructure()
{
#ifndef MAKEVERSION 
	#define MAKEVERSION(_h,_l) ( ((((WORD)(_h)) << 8) & 0xFF00) | (((WORD)(_l)) & 0xFF) )
#endif

	// Get the version of the common controls DLL
	ULONG cmoCtlVersion = 0;
	HMODULE dll = GetModuleHandle(_T("comctl32.dll"));
	if (dll)
	{
		DLLGETVERSIONPROC DllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(dll, "DllGetVersion");
		if (DllGetVersion)
		{
			DLLVERSIONINFO vinfo;
			Red::System::MemorySet(&vinfo, 0, sizeof(vinfo));
			vinfo.cbSize = sizeof(vinfo);
			DllGetVersion(&vinfo);
			cmoCtlVersion = MAKEVERSION(vinfo.dwMajorVersion, vinfo.dwMinorVersion);
		}
	}

	if (cmoCtlVersion >= MAKEVERSION(6,0))
	{
		// common controls version 6 (WinXP with visual styles)
		return sizeof(TOOLINFO);
	}
	else
	{
		// Win2000 or XP without visual styles
#ifdef UNICODE
		return TTTOOLINFOW_V2_SIZE;
#else
		return TTTOOLINFOA_V2_SIZE;
#endif
	}
}

LRESULT CALLBACK CLogOutput::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	CLogOutput* s_instance = (CLogOutput*)::GetWindowLongPtr( hWnd, GWLP_USERDATA );

	switch (message)
	{
	case WM_CREATE:
		{
			LPCREATESTRUCT data = (LPCREATESTRUCT)lParam;
			::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)data->lpCreateParams );
		}
		break;

	case WM_DESTROY:
		{
			::SetWindowLongPtr( hWnd, GWLP_USERDATA, NULL );
			::PostQuitMessage(0);
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_POPUP_CLEAR:
			if ( s_instance )
			{
				s_instance->OnClear();
			}
			break;

		case IDM_TOOLBAR_MESSAGE:
		case IDM_TOOLBAR_WARNING:
		case IDM_TOOLBAR_ERROR:
		case IDM_TOOLBAR_SPAM:
			if ( s_instance )
			{
				s_instance->Refresh();
			}
			break;

		case IDM_TOOLBAR_STAYONTOP:
			{
				if ( s_instance )
				{
					s_instance->m_stayOnTop = (SendMessage(s_instance->m_hToolbar, TB_ISBUTTONCHECKED, (WPARAM)IDM_TOOLBAR_STAYONTOP, 0) != 0);
					HWND insertAfter = s_instance->m_stayOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;				
					SetWindowPos(hWnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
				}
			}
			break;

		case IDM_TOOLBAR_ONBYDEFAULT:
			if ( s_instance )
			{
				s_instance->m_channelsOnByDefault = !s_instance->m_channelsOnByDefault;
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_NOTIFY:
		if (LOWORD(wParam) == IDC_LISTVIEW)
		{
			if (((LPNMHDR)lParam)->code == LVN_KEYDOWN)
			{
				if (((LPNMLVKEYDOWN)lParam)->wVKey == L'C' && (::GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0)
				{
					if ( s_instance )
					{
						s_instance->OnCopyToClipboard();
					}
				}
			}
		}
		else if (LOWORD(wParam) == IDC_CHANNELVIEW)
		{
			if (((LPNMHDR)lParam)->code == LVN_ITEMCHANGED &&
				(((LPNMLISTVIEW)lParam)->uChanged & LVIF_STATE))
			{
				if ( s_instance && !SDoNotRefresh )
				{
					int index = ((LPNMLISTVIEW)lParam)->iItem;
					Bool prevState = s_instance->m_channels[index].m_active;
/* these are here because there seems to be a bug in Microsoft's headers
 * causing warnings (which are treated in this project as errors) to be
 * issued about BOOL<->UINT conversion when using the
 * ListView_GetCheckState macro */
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4800 )
					s_instance->m_channels[index].m_active = ListView_GetCheckState( s_instance->m_hChannelList, index );
					if ( index == 0 )
					{
						SDoNotRefresh = true;
						for ( Uint32 i = 1; i  <s_instance->m_channels.Size(); ++i )
						{
							ListView_SetCheckState( s_instance->m_hChannelList, i, s_instance->m_channels[0].m_active );
							s_instance->m_channels[ i ].m_active = s_instance->m_channels[0].m_active;
						}
						SDoNotRefresh = false;

						s_instance->Refresh();
					}
RED_WARNING_POP()
					else if ( s_instance->m_channels[index].m_active != prevState )
					{
						if( !s_instance->m_channels[index].m_active )
						{
							SDoNotRefresh = true;
							ListView_SetCheckState( s_instance->m_hChannelList, 0, false );
							s_instance->m_channels[0].m_active = false;
							SDoNotRefresh = false;
						}

						s_instance->Refresh();
					}
				}
			}
		}
		break;

	case WM_SETCURSOR:
		if ( s_instance )
		{
			if (wParam == (DWORD)s_instance->m_hListView)
			{
				if (HIWORD(lParam) == WM_MOUSEMOVE && LOWORD(lParam) == 1)
				{
					s_instance->OnListViewMouseMove();
				}
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_TIMER:
		if ( s_instance )
		{
			s_instance->MergeBuffers();
		}
		break;

	case WM_SIZE:
		if ( s_instance )
		{
			s_instance->OnResize();
		}
		break;

	case WM_CONTEXTMENU:
		{
			int xpos = LOWORD(lParam);
			int ypos = HIWORD(lParam);
			HMENU hPopupMenu = CreatePopupMenu();
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, IDM_POPUP_CLEAR, L"Clear");
			TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, xpos, ypos, 0, hWnd, NULL);
		}
		break;
		
	case WM_CLOSE:
		if ( s_instance )
		{
			::ShowWindow(s_instance->m_hWindow, SW_HIDE);
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


