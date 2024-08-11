// prefs.h

namespace LoopbackAPI {

class CPrefs {
public:
    IMMDevice *m_pMMDevice;
    HMMIO m_hFile;
    bool m_bInt16;
    PWAVEFORMATEX m_pwfx;
    LPWSTR m_szFilename;

    // set hr to S_FALSE to abort but return success
	CPrefs( LPCWSTR outputFileName, size_t fileNameSize );
    ~CPrefs();

};

}