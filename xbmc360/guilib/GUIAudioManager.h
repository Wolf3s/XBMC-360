#ifndef H_CGUIAUDIOMANAGER
#define H_CGUIAUDIOMANAGER

#include "..\utils\SingleLock.h"
#include "..\utils\StdString.h"
#ifdef HAVE_TIXML1
#include "tinyxml\tinyxml.h"
#elif HAVE_TIXML2
#include "tinyxml2\tinyxml2.h"
using namespace tinyxml2;
#include "XMLUtils.h"
#endif
#include <map>
#include <vector>

// Forward definitions
class CAction;
class CGUISound;

enum WINDOW_SOUND { SOUND_INIT = 0, SOUND_DEINIT };

class CGUIAudioManager
{
	class CWindowSounds
	{
		public:
		CStdString strInitFile;
		CStdString strDeInitFile;
	};

public:
	CGUIAudioManager();
	~CGUIAudioManager();

	void Initialize();
	bool Load();
	void PlayActionSound(const CAction& action);
	void PlayWindowSound(int id, WINDOW_SOUND event);
	void Cleanup();

private:
	bool LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier, CStdString& strFile);

	typedef std::map<int, CStdString> actionSoundMap;
	actionSoundMap      m_actionSoundMap;
	std::vector<CGUISound*>	m_vecActionSounds;

	typedef std::map<int, CWindowSounds> windowSoundMap;
	windowSoundMap      m_windowSoundMap;
	std::vector<CGUISound*>	m_vecWindowSounds;

	CCriticalSection    m_cs;
};

extern CGUIAudioManager g_audioManager;

#endif //H_CGUIAUDIOMANAGER