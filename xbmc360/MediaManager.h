#ifndef CMEDIAMANAGER_H
#define CMEDIAMANAGER_H

#include "utils\StdString.h"

class CMediaSource
{
public:
	enum eSourceType
	{
		SOURCE_TYPE_UNKNOWN      = 0,
		SOURCE_TYPE_LOCAL        = 1,
		SOURCE_TYPE_DVD          = 2,
	};
	CMediaSource()
	{
		m_iDriveType = SOURCE_TYPE_UNKNOWN;
	};
	virtual ~CMediaSource()
	{
	};

	void FromNameAndPaths(const CStdString &category, const CStdString &name, const std::vector<CStdString> &paths);

	CStdString strName; // Name of the share, can be choosen freely
	CStdString strPath; // Path of the share, eg. iso9660:// or F:
	CStdString m_strThumbnailImage;
	std::vector<CStdString> vecPaths;
	eSourceType m_iDriveType;
};

typedef std::vector<CMediaSource> VECSOURCES;
typedef std::vector<CMediaSource>::iterator IVECSOURCES;

//======================================================
#include "Settings.h" // for VECSOURCES
#include "URL.h"

class CNetworkLocation
{
public:
	CNetworkLocation() { id = 0; };
	int id;
	CStdString path;
};

class CMediaManager
{
public:
	CMediaManager();

	bool LoadSources();
	bool SaveSources();

	void GetLocalDrives(VECSOURCES &localDrives, bool includeQ = true);
	void GetNetworkLocations(VECSOURCES &locations);

	bool AddNetworkLocation(const CStdString &path);
	bool HasLocation(const CStdString& path) const;
	bool RemoveLocation(const CStdString& path);
	bool SetLocationPath(const CStdString& oldPath, const CStdString& newPath);

protected:
	std::vector<CNetworkLocation> m_locations;
};

extern class CMediaManager g_mediaManager;

#endif //CMEDIAMANAGER_H