#pragma once

const Uint32 MAX_LOG_MESSAGE_LENGTH = 256;

class CLogOutput : public Red::Threads::CThread, public Red::System::Log::OutputDevice, public ISavableToConfig
{
public:
	CLogOutput( Uint32 size );
	virtual ~CLogOutput();
	void RequestExit();

	virtual void ThreadFunc() override;

	void Show() { if ( ! m_ready.GetValue() ) return; ::ShowWindowAsync(m_hWindow, SW_SHOW); }
	void Hide() { if ( ! m_ready.GetValue() ) return; ::ShowWindowAsync(m_hWindow, SW_HIDE); }
	void Minimize() { if ( ! m_ready.GetValue() ) return; if (::IsWindowVisible(m_hWindow)) ::ShowWindowAsync(m_hWindow, SW_SHOWMINIMIZED); }
	void ShowNormal() { if ( ! m_ready.GetValue() ) return; if (::IsWindowVisible(m_hWindow)) ::ShowWindowAsync(m_hWindow, SW_SHOWNORMAL); }

	virtual void Write( const Red::System::Log::Message& message );

	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;

private:

	void CreateLogWindow();
	void DestroyLogWindow();

	Uint32 AddChannel(const CName &channel);
	void MergeBuffers();
	void Refresh(Uint32 startIndex=0);
	void CreateImageList();
	void CreateToolbar();
	void CreateListView();	
	HWND CreateTooltip(HWND hParent);
	void OnClear();
	void OnResize();
	void OnListViewMouseMove();
	void OnCopyToClipboard();
	Int32 GetSizeOfToolInfoStructure();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void DoSaveOptionsToConfig();

	struct SLogMessage
	{
		SLogMessage();

		SLogMessage(const Char* msg, Red::System::Log::EPriority t, const CName& chnNm, const Uint32& chnIdx, const Char* tck, const Char* time );

		void CalcHash();
		
		Bool operator==( const SLogMessage& log ) const;
		
		Char message[ MAX_LOG_MESSAGE_LENGTH ];
		Red::System::Log::EPriority type;
		CName  channelName;
		Uint32 channelIndex;
		Red::CNameHash hash;
		Char tickStr[ 16 ];
		Char timeStr[ 16 ];
	};

	struct SChannel
	{
		SChannel() : m_active( true ) {}
		SChannel( CName name ) : m_name( name ), m_text( name.AsString() ), m_active( true ) {}

		CName m_name;
		String m_text;
		Bool m_active;

		Bool operator==( const SChannel& other ) const { return m_name == other.m_name; }
	};


	class CMessageQueue
	{
	public:
		CMessageQueue();

		void AddEntry( const SLogMessage& msg );
		Uint32 Process( CLogOutput& logOutput, TDynArray<SLogMessage>& outList );
		void Clear() { Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex ); m_messages.Clear(); }

	private:
		
		Red::Threads::CMutex m_mutex;
		TDynArray< SLogMessage > m_messages;
	};

	static const Int32 COLUMN_ID		= 0;
	static const Int32 COLUMN_CHANNEL	= 1;
	static const Int32 COLUMN_TICK	= 2;
	static const Int32 COLUMN_TIME	= 3;
	static const Int32 COLUMN_MESSAGE	= 4;

	static const Int32 CHANNEL_CHECKLIST_WIDTH;
	static const Int32 CHANNEL_CHECKBOX_WIDTH;
	static const Int32 CHANNEL_CHECKBOX_HEIGHT;

	static const CName CHANNEL_ALL;

	HINSTANCE m_hInstance;
	HIMAGELIST m_hImageList;
	HFONT m_hFont;
	HWND m_hWindow;
	HWND m_hToolbar;
	HWND m_hListView;
	HWND m_hTooltip;
	HWND m_hChannelList;

	Char m_buffer[4096];

	CMessageQueue m_messageQueue;

	TDynArray<SLogMessage> m_logs;
	TDynArray< SChannel > m_channels;
	
	const Int32 m_toolInfoSize;
	Int32 m_lastCoveredItem;
	Bool m_stayOnTop;
	Bool m_channelsOnByDefault;

	Red::Threads::CAtomic< Bool > m_saveConfig;
	Red::Threads::CAtomic< Bool > m_ready;
	Red::Threads::CAtomic< Bool > m_requestExit;

	Uint32 m_numLogMessages;

	Uint32 m_size;
};

extern CLogOutput* GLogOutput;