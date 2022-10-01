#include "Settings.h"
#include "Application.h"
#include "utils\Log.h"
#include "GUISettings.h"
#include "guilib\XMLUtils.h"
#include "utils\URIUtils.h"
#include "AdvancedSettings.h"
#include "URL.h"
#include "filesystem\File.h"
#include "utils/Util.h"
#include "guilib/GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "LangInfo.h"

using namespace DIRECTORY;
using namespace XFILE;

class CSettings g_settings;
extern CStdString g_LoadErrorStr;

CSettings::CSettings()
{
}

CSettings::~CSettings()
{
}

//TODO Wolf3s: Some extensions are not working properly or not executing. need more tests and work.
void CSettings::LoadExtensions()
{
	m_strVideoExtensions = ".m4v|.3gp|.nsv|.ts|.ty|.strm|.pls|.rm|.rmvb|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.rar|.001|.wpl|.zip|.vdr|.dvr-ms";
	m_strAudioExtensions = ".nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.mod|.amf|.669|.dmf|.dsm|.far|.gdm|.imf|.it|.m15|.med|.okt|.s3m|.stm|.sfx|.ult|.uni|.xm|.sid|.ac3|.dts|.cue|.aif|.aiff|.wpl|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.rar|.wv|.nsf|.spc|.gym|.adplug|.adx|.dsp|.adp|.ymf|.ast|.afc|.hps|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd";
	m_strPictureExtensions = ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.cbr|.rar|.m3u";
}

void CSettings::Initialize()
{
	LoadExtensions();
	m_iSystemTimeTotalUp = 0;
	m_logFolder = "D:\\";
}

void CSettings::ClearSources()
{
	// Clear sources, then load xml file...
	m_vecProgramSources.clear();
	m_vecVideoSources.clear();
	m_vecMusicSources.clear();
	m_vecPictureSources.clear();
}

bool CSettings::Load()
{
  // load settings file...
  bool bXboxMediaCenter360 = false;

  char szDevicePath[1024];
  CStdString strMnt = _P(GetProfileUserDataFolder());
  if (strMnt.Left(2).Equals("D:"))
  {
    CUtil::GetHomePath(strMnt);
    strMnt += _P(GetProfileUserDataFolder()).substr(2);
  }
#ifdef WIP
  CIoSupport::GetPartition(strMnt.c_str()[0], szDevicePath);
  strcat(szDevicePath,strMnt.c_str()+2);
  CIoSupport::RemapDriveLetter('P', szDevicePath);
#endif
  CSpecialProtocol::SetProfilePath(GetProfileUserDataFolder());
  CLog::Log(LOGNOTICE, "loading %s", GetSettingsFile().c_str());
  if (!LoadSettings(GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to load %s, creating new %s with default values", GetSettingsFile().c_str(), GetSettingsFile().c_str());
    Save();
    if (!(bXboxMediaCenter360 = Reset()))
      return false;
  }

  // clear sources, then load xml file...
  ClearSources();
  CStdString strXMLFile = GetSourcesFile();
  CLog::Log(LOGNOTICE, "%s", strXMLFile.c_str());
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  if ( xmlDoc.LoadFile( strXMLFile ) )
  {
    pRootElement = xmlDoc.RootElement();
    CStdString strValue;
    if (pRootElement)
      strValue = pRootElement->Value();
    if ( strValue != "sources")
      CLog::Log(LOGERROR, "%s sources.xml file does not contain <sources>", __FUNCTION__);
  }
  else if (CFile::Exists(strXMLFile))
    CLog::Log(LOGERROR, "%s Error loading %s: Line %d, %s", __FUNCTION__, strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());

  // look for external sources file
  TiXmlNode *pInclude = pRootElement ? pRootElement->FirstChild("remote") : NULL;
  if (pInclude)
  {
    CStdString strRemoteFile = pInclude->FirstChild()->Value();
    if (!strRemoteFile.IsEmpty())
    {
      CLog::Log(LOGDEBUG, "Found <remote> tag");
      CLog::Log(LOGDEBUG, "Attempting to retrieve remote file: %s", strRemoteFile.c_str());
      // sometimes we have to wait for the network
      if (!g_application.getNetwork().IsAvailable(true) && CFile::Exists(strRemoteFile))
      {
        if ( xmlDoc.LoadFile(strRemoteFile) )
        {
          pRootElement = xmlDoc.RootElement();
          CStdString strValue;
          if (pRootElement)
            strValue = pRootElement->Value();
          if ( strValue != "sources")
            CLog::Log(LOGERROR, "%s remote_sources.xml file does not contain <sources>", __FUNCTION__);
        }
        else
          CLog::Log(LOGERROR, "%s unable to load file: %s, Line %d, %s", __FUNCTION__, strRemoteFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
      }
      else
        CLog::Log(LOGNOTICE, "Could not retrieve remote file, defaulting to local sources");
    }
  }


  if (pRootElement)
  { // parse sources...
#ifdef WIP
    GetSources(pRootElement, "programs", m_programSources, m_defaultProgramSource);
    GetSources(pRootElement, "pictures", m_pictureSources, m_defaultPictureSource);
    GetSources(pRootElement, "files", m_fileSources, m_defaultFileSource);
    GetSources(pRootElement, "music", m_musicSources, m_defaultMusicSource);
    GetSources(pRootElement, "video", m_videoSources, m_defaultVideoSource);
#else
	GetSources(pRootElement, "programs", m_vecProgramSources);
	GetSources(pRootElement, "videos", m_vecVideoSources);
	GetSources(pRootElement, "music", m_vecMusicSources);
	GetSources(pRootElement, "pictures", m_vecPictureSources);
#endif
  }

  bXboxMediaCenter360 = true;

#ifdef WIP
  LoadRSSFeeds();
  LoadUserFolderLayout();
#endif
  return true;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile)
{

  // load the xml file
  TiXmlDocument xmlDoc;

  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    g_LoadErrorStr.Format("%s, Line %d\n%s", strSettingsFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "settings") != 0)
  {
    g_LoadErrorStr.Format("%s\nDoesn't contain <settings>", strSettingsFile.c_str());
    return false;
  }
#ifdef WIP
  // mymusic settings
  TiXmlElement *pElement = pRootElement->FirstChildElement("mymusic");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_bMyMusicPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_bMyMusicPlaylistShuffle);
    }
    // if the user happened to reboot in the middle of the scan we save this state
    pChild = pElement->FirstChildElement("scanning");
    if (pChild)
    {
      XMLUtils::GetBoolean(pChild, "isscanning", m_bMyMusicIsScanning);
    }
    GetInteger(pElement, "startwindow", m_iMyMusicStartWindow, WINDOW_MUSIC_FILES, WINDOW_MUSIC_FILES, WINDOW_MUSIC_NAV); //501; view songs
    XMLUtils::GetBoolean(pElement, "songinfoinvis", m_bMyMusicSongInfoInVis);
    XMLUtils::GetBoolean(pElement, "songthumbinvis", m_bMyMusicSongThumbInVis);
    GetPath(pElement, "defaultlibview", m_defaultMusicLibSource);
  }
  // myvideos settings
  pElement = pRootElement->FirstChildElement("myvideos");
  if (pElement)
  {
    GetInteger(pElement, "startwindow", m_iVideoStartWindow, WINDOW_VIDEO_FILES, WINDOW_VIDEO_FILES, WINDOW_VIDEO_NAV);
    GetInteger(pElement, "stackvideomode", m_iMyVideoStack, STACK_NONE, STACK_NONE, STACK_SIMPLE);

    GetPath(pElement, "defaultlibview", m_defaultVideoLibSource);
    GetInteger(pElement, "watchmode", m_iMyVideoWatchMode, VIDEO_SHOW_ALL, VIDEO_SHOW_ALL, VIDEO_SHOW_WATCHED);
    XMLUtils::GetBoolean(pElement, "flatten", m_bMyVideoNavFlatten);

    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    { // playlist
      XMLUtils::GetBoolean(pChild, "repeat", m_bMyVideoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_bMyVideoPlaylistShuffle);
    }
  }

  pElement = pRootElement->FirstChildElement("viewstates");
  if (pElement)
  {
    GetViewState(pElement, "musicnavartists", m_viewStateMusicNavArtists);
    GetViewState(pElement, "musicnavalbums", m_viewStateMusicNavAlbums);
    GetViewState(pElement, "musicnavsongs", m_viewStateMusicNavSongs);
    GetViewState(pElement, "musicshoutcast", m_viewStateMusicShoutcast);
    GetViewState(pElement, "musiclastfm", m_viewStateMusicLastFM);
    GetViewState(pElement, "videonavactors", m_viewStateVideoNavActors);
    GetViewState(pElement, "videonavyears", m_viewStateVideoNavYears);
    GetViewState(pElement, "videonavgenres", m_viewStateVideoNavGenres);
    GetViewState(pElement, "videonavtitles", m_viewStateVideoNavTitles);
    GetViewState(pElement, "videonavepisodes", m_viewStateVideoNavEpisodes, SORT_METHOD_EPISODE);
    GetViewState(pElement, "videonavtvshows", m_viewStateVideoNavTvShows);
    GetViewState(pElement, "videonavseasons", m_viewStateVideoNavSeasons);
    GetViewState(pElement, "videonavmusicvideos", m_viewStateVideoNavMusicVideos);

    GetViewState(pElement, "programs", m_viewStatePrograms, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
    GetViewState(pElement, "pictures", m_viewStatePictures, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
    GetViewState(pElement, "videofiles", m_viewStateVideoFiles, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
    GetViewState(pElement, "musicfiles", m_viewStateMusicFiles, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
  }

  // general settings
  pElement = pRootElement->FirstChildElement("general");
  if (pElement)
  {
    GetInteger(pElement, "systemtotaluptime", m_iSystemTimeTotalUp, 0, 0, INT_MAX);
    GetInteger(pElement, "httpapibroadcastlevel", m_HttpApiBroadcastLevel, 0, 0,5);
    GetInteger(pElement, "httpapibroadcastport", m_HttpApiBroadcastPort, 8278, 1, 65535);
  }

  pElement = pRootElement->FirstChildElement("defaultvideosettings");
  if (pElement)
  {
    int interlaceMethod;
    GetInteger(pElement, "interlacemethod", interlaceMethod, VS_INTERLACEMETHOD_NONE, VS_INTERLACEMETHOD_NONE, VS_INTERLACEMETHOD_DEINTERLACE);
    m_defaultVideoSettings.m_InterlaceMethod = (EINTERLACEMETHOD)interlaceMethod;

    GetFloat(pElement, "filmgrain", m_defaultVideoSettings.m_FilmGrain, 0, 0, 10);
    GetInteger(pElement, "viewmode", m_defaultVideoSettings.m_ViewMode, VIEW_MODE_NORMAL, VIEW_MODE_NORMAL, VIEW_MODE_CUSTOM);
    GetFloat(pElement, "zoomamount", m_defaultVideoSettings.m_CustomZoomAmount, 1.0f, 0.5f, 2.0f);
    GetFloat(pElement, "pixelratio", m_defaultVideoSettings.m_CustomPixelRatio, 1.0f, 0.5f, 2.0f);
    GetFloat(pElement, "volumeamplification", m_defaultVideoSettings.m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MAXIMUM * 0.01f);
    GetFloat(pElement, "noisereduction", m_defaultVideoSettings.m_NoiseReduction, 0.0f, 0.0f, 1.0f);
    XMLUtils::GetBoolean(pElement, "postprocess", m_defaultVideoSettings.m_PostProcess);
    GetFloat(pElement, "sharpness", m_defaultVideoSettings.m_Sharpness, 0.0f, -1.0f, 1.0f);
    XMLUtils::GetBoolean(pElement, "outputtoallspeakers", m_defaultVideoSettings.m_OutputToAllSpeakers);
    XMLUtils::GetBoolean(pElement, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
    GetFloat(pElement, "brightness", m_defaultVideoSettings.m_Brightness, 50, 0, 100);
    GetFloat(pElement, "contrast", m_defaultVideoSettings.m_Contrast, 50, 0, 100);
    GetFloat(pElement, "gamma", m_defaultVideoSettings.m_Gamma, 20, 0, 100);
    GetFloat(pElement, "audiodelay", m_defaultVideoSettings.m_AudioDelay, 0.0f, -10.0f, 10.0f);
    GetFloat(pElement, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay, 0.0f, -10.0f, 10.0f);

    m_defaultVideoSettings.m_SubtitleCached = false;
  }
  // audio settings
  pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    GetInteger(pElement, "volumelevel", m_nVolumeLevel, VOLUME_MAXIMUM, VOLUME_MINIMUM, VOLUME_MAXIMUM);
    GetInteger(pElement, "dynamicrangecompression", m_dynamicRangeCompressionLevel, VOLUME_DRC_MINIMUM, VOLUME_DRC_MINIMUM, VOLUME_DRC_MAXIMUM);
    for (int i = 0; i < 4; i++)
    {
      CStdString setting;
      setting.Format("karaoke%i", i);
#ifndef HAS_XBOX_AUDIO
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)
#endif
      GetFloat(pElement, setting + "energy", m_karaokeVoiceMask[i].energy, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "pitch", m_karaokeVoiceMask[i].pitch, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "whisper", m_karaokeVoiceMask[i].whisper, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "robotic", m_karaokeVoiceMask[i].robotic, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
    }
  }

  LoadCalibration(pRootElement, strSettingsFile);
  g_guiSettings.LoadXML(pRootElement);
  LoadSkinSettings(pRootElement);

#ifdef WIP
  // Configure the PlayerCoreFactory
  LoadPlayerCoreFactorySettings("special://xbmc/system/playercorefactory.xml", true);
  LoadPlayerCoreFactorySettings(GetUserDataItem("playercorefactory.xml"), false);
#endif

  // Advanced settings
  g_advancedSettings.Load();

  // Default players?
  CLog::Log(LOGNOTICE, "Default Video Player: %s", GetDefaultVideoPlayerName().c_str());
  CLog::Log(LOGNOTICE, "Default Audio Player: %s", GetDefaultAudioPlayerName().c_str());

  // setup logging...
  if (g_guiSettings.GetBool("debug.showloginfo"))
  {
    g_advancedSettings.m_logLevel = std::max(g_advancedSettings.m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    CLog::SetLogLevel(g_advancedSettings.m_logLevel);
    CLog::Log(LOGNOTICE, "Enabled debug logging due to GUI setting (%d)", g_advancedSettings.m_logLevel);
  }
  
  // Override settings with avpack settings
  if ( GetCurrentProfile().useAvpackSettings())
  {
    CLog::Log(LOGNOTICE, "Per AV pack settings are on");
    LoadAvpackXML();
  }
  else
    CLog::Log(LOGNOTICE, "Per AV pack settings are off");
#else

//	(pRootElement, "loglevel", g_stSettings.m_iLogLevel, LOGWARNING, LOGDEBUG, LOGNONE); //TODO

	// General settings
	TiXmlElement *pElement = pRootElement->FirstChildElement("general");
	if (pElement)
	{
		GetInteger(pElement, "systemtotaluptime", m_iSystemTimeTotalUp, 0, 0, INT_MAX);
	}

	g_guiSettings.LoadXML(pRootElement);	

	// Advanced settings
	g_advancedSettings.Load();

	return true;
#endif
  return true;
}

CStdString CSettings::GetSettingsFile() const
{
  CStdString settings;
  if (m_currentProfile == 0)
    settings = "special://masterprofile/guisettings.xml";
  else
    settings = "special://profile/guisettings.xml";
  return settings;
}

void CSettings::Save() const
{
	if(g_application.IsStopping())
	{
		// Don't save settings when we're busy stopping the application
		// a lot of screens try to save settings on deinit and deinit is called
		// for every screen when the application is stopping.
		return;
	}
	if (!SaveSettings(GetSettingsFile()))
	{
		CLog::Log(LOGERROR, "Unable to save settings to %s", GetSettingsFile().c_str());
	}
}

bool CSettings::Reset()
{
  CLog::Log(LOGINFO, "Resetting settings");
  CFile::Delete(GetSettingsFile());
  Save();
  return LoadSettings(GetSettingsFile());
}



CStdString CSettings::GetUserDataItem(const CStdString& strFile) const
{
  CStdString folder;
  folder = "special://profile/"+strFile;
  //check if item exists in the profile
  //(either for folder or for a file (depending on slashAtEnd of strFile)
  //otherwise return path to masterprofile
  if ( (URIUtils::HasSlashAtEnd(folder) && !CDirectory::Exists(folder)) || !CFile::Exists(folder))
    folder = "special://masterprofile/"+strFile;
  return folder;
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile) const
{
	TiXmlDocument xmlDoc;
	TiXmlElement xmlRootElement("settings");

	TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
	if(!pRoot) return false;

	// Write our tags one by one, just a big list for now (can be flashed up later)

	// General settings
	TiXmlElement generalNode("general");
	TiXmlNode *pNode = pRoot->InsertEndChild(generalNode);
	if(!pNode) return false;
	XMLUtils::SetInt(pNode, "systemtotaluptime", m_iSystemTimeTotalUp);

	g_guiSettings.SaveXML(pRoot);

	return xmlDoc.SaveFile(strSettingsFile);
}

bool CSettings::AddShare(const CStdString &type, const CMediaSource &share)
{
	VECSOURCES *pShares = GetSourcesFromType(type);
	if(!pShares) return false;

	// Translate dir and add to our current shares
	CStdString strPath1 = share.strPath;
	strPath1.ToUpper();
	
	if(strPath1.IsEmpty())
	{
		CLog::Log(LOGERROR, "Unable to add empty path");
		return false;
	}

	CMediaSource shareToAdd = share;
	pShares->push_back(shareToAdd);

	return SaveSources();
}

bool CSettings::LoadProfile(unsigned int index)
{
  unsigned int oldProfile = m_currentProfile;
  m_currentProfile = index;
  bool bSourcesXML=true;
  CStdString strOldSkin = g_guiSettings.GetString("lookandfeel.skin");
  CStdString strOldFont = g_guiSettings.GetString("lookandfeel.font");
  CStdString strOldTheme = g_guiSettings.GetString("lookandfeel.skintheme");
  CStdString strOldColors = g_guiSettings.GetString("lookandfeel.skincolors");

  if (Load())
  {
    CreateProfileFolders();
#ifdef WIP
    // initialize our charset converter
    g_charsetConverter.reset();
#endif
    // Load the langinfo to have user charset <-> utf-8 conversion
    CStdString strLanguage = g_guiSettings.GetString("locale.language");
    strLanguage[0] = toupper(strLanguage[0]);

    CStdString strLangInfoPath;
    strLangInfoPath.Format("special://xbmc-360/language/%s/langinfo.xml", strLanguage.c_str());
    CLog::Log(LOGINFO, "load language info file: %s", strLangInfoPath);
	g_langInfo.Load(strLangInfoPath);
#ifdef _XBOX
    CStdString strKeyboardLayoutConfigurationPath;
    strKeyboardLayoutConfigurationPath.Format("special://xbmc-360/language/%s/keyboardmap.xml", strLanguage.c_str());
    CLog::Log(LOGINFO, "load keyboard layout configuration info file: %s", strKeyboardLayoutConfigurationPath.c_str());
#ifdef WIP
    g_keyboardLayoutConfiguration.Load(strKeyboardLayoutConfigurationPath);
#endif
#endif
#ifdef WIP
    CButtonTranslator::GetInstance().Load();
#endif
    g_localizeStrings.Load("special://xbmc-360/language/", strLanguage);

    g_infoManager.ResetCache();
    g_infoManager.ResetLibraryBools();

    // always reload the skin - we need it for the new language strings
    g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));
#ifdef WIP
    if (m_currentProfile != 0)
    {
      TiXmlDocument doc;
      if (doc.LoadFile(URIUtils::AddFileToFolder(GetUserDataFolder(),"guisettings.xml")))
        g_guiSettings.LoadMasterLock(doc.RootElement());
	}
   
#ifdef HAS_XBOX_HARDWARE //TODO: Add cooler class
    if (g_guiSettings.GetBool("system.autotemperature"))
    {
      CLog::Log(LOGNOTICE, "start fancontroller");
      CFanController::Instance()->Start(g_guiSettings.GetInt("system.targettemperature"), g_guiSettings.GetInt("system.minfanspeed"));
    }
    else if (g_guiSettings.GetBool("system.fanspeedcontrol"))
    {
      CLog::Log(LOGNOTICE, "setting fanspeed");
      CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("system.fanspeed"));
    }
    g_application.StartLEDControl(false);
#endif
#endif
    // to set labels - shares are reloaded
#ifdef WIP    
	CDetectDVDMedia::UpdateState();
#endif    
	// init windows
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WINDOW_RESET);

    g_windowManager.SendMessage(msg);
#ifdef WIP    
	CUtil::DeleteMusicDatabaseDirectoryCache();
    CUtil::DeleteVideoDatabaseDirectoryCache();
#endif
    return true;
  }

  m_currentProfile = oldProfile;

  return false;
}

CStdString CSettings::GetProfileUserDataFolder() const
{
  CStdString folder;
  if (m_currentProfile == 0)
    return GetUserDataFolder();

  URIUtils::AddFileToFolder(GetUserDataFolder(), GetCurrentProfile().getDirectory(), folder);

  return folder;
}

CStdString CSettings::GetUserDataFolder() const
{
  return GetMasterProfile().getDirectory();
}

CStdString CSettings::GetThumbnailsFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails", folder);

  return folder;
}

CStdString CSettings::GetMusicThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Music", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Music", folder);

  return folder;
}

CStdString CSettings::GetMusicArtistThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Music/Artists", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Music/Artists", folder);

  return folder;
}

CStdString CSettings::GetVideoThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Video", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Video", folder);

  return folder;
}

CStdString CSettings::GetBookmarksThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Video/Bookmarks", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Video/Bookmarks", folder);

  return folder;
}

CStdString CSettings::GetPicturesThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Pictures", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Pictures", folder);

  return folder;
}

CStdString CSettings::GetProgramsThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Programs", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Programs", folder);

  return folder;
}

CStdString CSettings::GetGameSaveThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/GameSaves", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/GameSaves", folder);

  return folder;
}

CStdString CSettings::GetProfilesThumbFolder() const
{
  CStdString folder;
  URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Profiles", folder);

  return folder;
}

CStdString CSettings::GetSkinFolder() const
{
  CStdString folder;

  // Get the Current Skin Path
  return GetSkinFolder(g_guiSettings.GetString("lookandfeel.skin"));
}

CStdString CSettings::GetSkinFolder(const CStdString &skinName) const
{
  CStdString folder;

  // Get the Current Skin Path
  URIUtils::AddFileToFolder("special://home/skin/", skinName, folder);
  if ( ! CDirectory::Exists(folder) )
    URIUtils::AddFileToFolder("special://xbmc-360/skin/", skinName, folder);

  return folder;
}

CStdString CSettings::GetDatabaseFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Database", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Database", folder);

  return folder;
}

CStdString CSettings::GetSourcesFile() const
{
  CStdString folder;
  if (GetCurrentProfile().hasSources())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(),"sources.xml",folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(),"sources.xml",folder);
  return folder;
}



void CSettings::CreateProfileFolders()
{
  CDirectory::Create(GetDatabaseFolder());
#ifdef WIP
  CDirectory::Create(GetCDDBFolder());
#endif
  // Thumbnails/
  CDirectory::Create(GetThumbnailsFolder());
  CDirectory::Create(GetMusicThumbFolder());
  CDirectory::Create(GetMusicArtistThumbFolder());
#ifdef WIP  
  CDirectory::Create(GetLastFMThumbFolder());
#endif
  CDirectory::Create(GetVideoThumbFolder());
#ifdef WIP  
  CDirectory::Create(GetVideoFanartFolder());
  CDirectory::Create(GetMusicFanartFolder());
#endif  
  CDirectory::Create(GetBookmarksThumbFolder());
  CDirectory::Create(GetProgramsThumbFolder());
  CDirectory::Create(GetPicturesThumbFolder());
  CDirectory::Create(GetGameSaveThumbFolder());

  CLog::Log(LOGINFO, "thumbnails folder: %s", GetThumbnailsFolder().c_str());
  for (unsigned int hex=0; hex < 16; hex++)
  {
    CStdString strHex;
    strHex.Format("%x",hex);
    CDirectory::Create(URIUtils::AddFileToFolder(GetPicturesThumbFolder(), strHex));
    CDirectory::Create(URIUtils::AddFileToFolder(GetMusicThumbFolder(), strHex));
    CDirectory::Create(URIUtils::AddFileToFolder(GetVideoThumbFolder(), strHex));
    CDirectory::Create(URIUtils::AddFileToFolder(GetProgramsThumbFolder(), strHex));
 
  }
   CDirectory::Create("special://profile/visualisations");

}

static CProfile emptyProfile;

const CProfile &CSettings::GetMasterProfile() const
{
  if (GetNumProfiles())
    return m_vecProfiles[0];
  CLog::Log(LOGERROR, "%s - master profile requested while none exists", __FUNCTION__);
  return emptyProfile;
}

const CProfile &CSettings::GetCurrentProfile() const
{
  if (m_currentProfile < m_vecProfiles.size())
    return m_vecProfiles[m_currentProfile];
  CLog::Log(LOGERROR, "%s - last profile index (%u) is outside the valid range (%u)", __FUNCTION__, m_currentProfile, m_vecProfiles.size());
  return emptyProfile;
}

void CSettings::UpdateCurrentProfileDate()
{
  if (m_currentProfile < m_vecProfiles.size())
    m_vecProfiles[m_currentProfile].setDate();
}

const CProfile *CSettings::GetProfile(unsigned int index) const
{
  if (index < GetNumProfiles())
    return &m_vecProfiles[index];
  return NULL;
}

CProfile *CSettings::GetProfile(unsigned int index)
{
  if (index < GetNumProfiles())
    return &m_vecProfiles[index];
  return NULL;
}

unsigned int CSettings::GetNumProfiles() const
{
  return m_vecProfiles.size();
}

int CSettings::GetProfileIndex(const CStdString &name) const
{
  for (unsigned int i = 0; i < m_vecProfiles.size(); i++)
    if (m_vecProfiles[i].getName().Equals(name))
      return i;
  return -1;
}

void CSettings::AddProfile(const CProfile &profile)
{
  m_vecProfiles.push_back(profile);
}

void CSettings::LoadMasterForLogin()
{
  // save the previous user
  m_lastUsedProfile = m_currentProfile;
  if (m_currentProfile != 0)
    LoadProfile(0);
}

VECSOURCES *CSettings::GetSourcesFromType(const CStdString &type)
{
	CStdString strType(type);

	CStringUtils::MakeLowercase(strType);

	if(strType == "programs" || strType == "myprograms")
		return &m_vecProgramSources;
	else if(strType== "videos")
		return &m_vecVideoSources;
	else if(strType == "music")
		return &m_vecMusicSources;
	else if(strType == "pictures")
		return &m_vecPictureSources;

	return NULL;
}

bool CSettings::DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath, bool virtualSource)
{
	VECSOURCES *pShares = GetSourcesFromType(strType);
	if(!pShares) return false;

	bool found(false);

	for(IVECSOURCES it = pShares->begin(); it != pShares->end(); it++)
	{
		if((*it).strName == strName && (*it).strPath == strPath)
		{
			CLog::Log(LOGDEBUG,"Found source, removing!");
			pShares->erase(it);
			found = true;
			break;
		}
	}

	if(virtualSource)
		return found;

	return SaveSources();
}

void CSettings::GetSources(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSOURCES& items)
{
	CLog::Log(LOGDEBUG, "Parsing <%s> tag", strTagName.c_str());

	items.clear();
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());

	if(pChild)
	{
		pChild = pChild->FirstChild();
		while(pChild > 0)
		{
			CStdString strValue = pChild->Value();
			if(strValue == "source")
			{
				CMediaSource share;
				if(GetSource(strTagName, pChild, share))
				{
					items.push_back(share);
				}
				else
				{
					CLog::Log(LOGERROR, "Missing or invalid <name> and/or <path> in source");
				}
			}
			pChild = pChild->NextSibling();
		}
	}
	else
	{
		CLog::Log(LOGDEBUG, "  <%s> tag is missing or sources.xml is malformed", strTagName.c_str());
	}
}

bool CSettings::SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares)
{
	TiXmlElement sectionElement(section);
	TiXmlNode *sectionNode = root->InsertEndChild(sectionElement);
	
	if(sectionNode)
	{
		for(unsigned int i = 0; i < shares.size(); i++)
		{
			const CMediaSource &share = shares[i];
				
			TiXmlElement source("source");

			XMLUtils::SetString(&source, "name", share.strName);

			for(unsigned int i = 0; i < share.vecPaths.size(); i++)
				XMLUtils::SetPath(&source, "path", share.vecPaths[i]);

			if(!share.m_strThumbnailImage.IsEmpty())
				XMLUtils::SetPath(&source, "thumbnail", share.m_strThumbnailImage);

			sectionNode->InsertEndChild(source);
		}
	}
	return true;
}

bool CSettings::SaveSources()
{
	TiXmlDocument doc;
	TiXmlElement xmlRootElement("sources");
	TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);

	if(!pRoot) return false;

	// Ok, now run through and save each sources section
	SetSources(pRoot, "programs", m_vecProgramSources);
	SetSources(pRoot, "videos", m_vecVideoSources);
	SetSources(pRoot, "music", m_vecMusicSources);
	SetSources(pRoot, "pictures", m_vecPictureSources);

	return doc.SaveFile(GetUserDataFolder());
}

bool CSettings::GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share)
{
	CLog::Log(LOGDEBUG,"---- SOURCE START ----");
	const TiXmlNode *pNodeName = source->FirstChild("name");

	CStdString strName;
	if(pNodeName && pNodeName->FirstChild())
	{
		strName = pNodeName->FirstChild()->Value();
		CLog::Log(LOGDEBUG,"Found name: %s", strName.c_str());
	}

	// Get multiple paths
	vector<CStdString> vecPaths;
	const TiXmlElement *pPathName = source->FirstChildElement("path");
	
	while(pPathName)
	{
		if(pPathName->FirstChild())
		{
			CStdString strPath = pPathName->FirstChild()->Value();

			if(!strPath.IsEmpty())
			{
				CLog::Log(LOGDEBUG,"-> Translated to path: %s", strPath.c_str());
			}
			else
			{
				CLog::Log(LOGERROR,"-> Skipping invalid token: %s", strPath.c_str());
				pPathName = pPathName->NextSiblingElement("path");
				continue;
			}
			URIUtils::AddSlashAtEnd(strPath);
			vecPaths.push_back(strPath);
		}
		pPathName = pPathName->NextSiblingElement("path");
	}

	const TiXmlNode *pThumbnailNode = source->FirstChild("thumbnail");

	if(!strName.IsEmpty() && vecPaths.size() > 0)
	{
		vector<CStdString> verifiedPaths;

		// Disallowed for files, or theres only a single path in the vector
		if(vecPaths.size() == 1)
			verifiedPaths.push_back(vecPaths[0]);
		else // Multiple paths?
		{
			// Validate the paths
			for(int j = 0; j < (int)vecPaths.size(); ++j)
			{
				CURL url(vecPaths[j]);
				bool bIsInvalid = false;

				// For my programs
				if(category.Equals("programs") || category.Equals("myprograms"))
				{
					// Only allow HD and plugins
					if(url.IsLocal())
						verifiedPaths.push_back(vecPaths[j]);
					else
						bIsInvalid = true;
				}		
				else
					verifiedPaths.push_back(vecPaths[j]); // For others allow everything (if the user does something silly, we can't stop them)

			// Error message
			if(bIsInvalid)
				CLog::Log(LOGERROR,"Invalid path type (%s) for multipath source", vecPaths[j].c_str());
			}

			// No valid paths? skip to next source
			if(verifiedPaths.size() == 0)
			{
				CLog::Log(LOGERROR,"Missing or invalid <name> and/or <path> in source");
				return false;
			}
		}

		share.FromNameAndPaths(category, strName, verifiedPaths);

		if(pThumbnailNode)
		{
			if(pThumbnailNode->FirstChild())
				share.m_strThumbnailImage = pThumbnailNode->FirstChild()->Value();
		}
		return true;
	}
	return false;
}

void CSettings::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if(pChild)
	{
		iValue = atoi( pChild->FirstChild()->Value() );
		if((iValue < iMin) || (iValue > iMax)) iValue = iDefault;
	}
	else
		iValue = iDefault;

	CLog::Log(LOGDEBUG, "  %s: %d", strTagName.c_str(), iValue);
}