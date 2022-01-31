#ifndef CUTIL_H
#define CUTIL_H

#include "StdString.h"
#include "stdafx.h"
#include "..\MediaManager.h"

class CUtil
{
public:
	static bool IsBuiltIn(const CStdString& execString);
	static void SplitExecFunction(const CStdString &execString, CStdString &strFunction, CStdString &strParam);
	static const CStdString GetFileName(const CStdString& strFileNameAndPath);
	static CStdString GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder = false);
	static int ExecBuiltIn(const CStdString& execString);
	static bool GetParentPath(const CStdString& strPath, CStdString& strParent);
	static bool HasSlashAtEnd(const CStdString& strFile);
	static void AddSlashAtEnd(CStdString& strFolder);
	static const CStdString GetExtension(const CStdString& strFileName);
	static void GetExtension(const CStdString& strFile, CStdString& strExtension);
	static void RemoveSlashAtEnd(CStdString& strFolder);
	static bool IsLocalDrive(const CStdString& strPath, bool bFullPath = false);
	static bool FileExists(CStdString strFullPath) { return (GetFileAttributes(strFullPath.c_str()) != 0xFFFFFFFF); } // TODO : Remove once filesystem is more sorted out
	static int GetMatchingShare(const CStdString& strPath, VECSOURCES& vecShares, bool& bIsBookmarkName);
	static void ForceForwardSlashes(CStdString& strPath);
	static void Stat64ToStat(struct _stat *result, struct __stat64 *stat);
};

#endif //CUTIL_H