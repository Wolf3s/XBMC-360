#ifndef H_CSETTINGS
#define H_CSETTINGS

#include "utils\StdString.h"
#include "guilib\XMLUtils.h"
#include "MediaManager.h"
#include "VideoSettings.h"

#define DEFAULT_SKIN "Project Mayhem III"
#define SETTINGS_FILE "D:\\settings.xml"
#define SOURCES_FILE "D:\\sources.xml" //TODO: GetUserDataFolder()

class CSettings
{
public:
	CSettings();
	virtual ~CSettings();

	void Initialize();
	void LoadExtensions();
	bool LoadSettings(const CStdString& strSettingsFile);
	bool Load();
	bool SaveSettings(const CStdString& strSettingsFile) const;
	void Save() const;
	void ClearSources();
	bool AddShare(const CStdString &type, const CMediaSource &share);
	VECSOURCES *GetSourcesFromType(const CStdString &type);
	bool DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath, bool virtualSource = false);

	CStdString GetVideoExtensions() { return m_strVideoExtensions; };
	CStdString GetAudioExtensions() { return m_strAudioExtensions; };
	CStdString GetPictureExtensions() { return m_strPictureExtensions; };

	int m_iSystemTimeTotalUp; // Uptime in minutes!

	CVideoSettings m_currentVideoSettings;
	CStdString m_logFolder;
protected:
	void GetSources(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSOURCES& items);
	bool GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share);
	bool SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares);
	bool SaveSources();

	void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);

	VECSOURCES m_vecProgramSources;
	VECSOURCES m_vecVideoSources;
	VECSOURCES m_vecMusicSources;
	VECSOURCES m_vecPictureSources;

	CStdString m_strVideoExtensions;
	CStdString m_strAudioExtensions;
	CStdString m_strPictureExtensions;
};

extern class CSettings g_settings;

#endif //H_CSETTINGS