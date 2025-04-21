#include "utils\log.h"
#include "MultiPathDirectory.h"
#include "Directory.h"
#include "utils\Util.h"
#include "utils\URIUtils.h"
#include "utils\StringUtils.h"
#include "URL.h"
#include "guilib\GUIWindowManager.h"
#include "guilib\dialogs\GUIDialogProgress.h"
#include "FileItem.h"
#include "utils\TimeUtils.h"

using namespace std;
using namespace XFILE;

//
// multipath://{path1}/{path2}/{path3}/.../{path-N}
//
// unlike the older virtualpath:// protocol, sub-folders are combined together into a new
// multipath:// style url.
//

CMultiPathDirectory::CMultiPathDirectory()
{
}

CMultiPathDirectory::~CMultiPathDirectory()
{
}

bool CMultiPathDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
	CLog::Log(LOGDEBUG,"CMultiPathDirectory::GetDirectory(%s)", strPath.c_str());

	vector<CStdString> vecPaths;

	if (!GetPaths(strPath, vecPaths))
		return false;

	DWORD progressTime = CTimeUtils::timeGetTime() + 3000L; // 3 seconds before showing progress bar
	CGUIDialogProgress* dlgProgress = NULL;
  
	unsigned int iFailures = 0;
	for (unsigned int i = 0; i < vecPaths.size(); ++i)
	{
		// Show the progress dialog if we have passed our time limit
		if (CTimeUtils::timeGetTime() > progressTime && !dlgProgress)
		{
			dlgProgress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
			if (dlgProgress)
			{
				dlgProgress->SetHeading(15310);
				dlgProgress->SetLine(0, 15311);
				dlgProgress->SetLine(1, "");
				dlgProgress->SetLine(2, "");
				dlgProgress->StartModal();
				dlgProgress->ShowProgressBar(true);
				dlgProgress->SetProgressMax((int)vecPaths.size()*2);
				dlgProgress->Progress();
			}
		}
		
		if (dlgProgress)
		{
			CURL url(vecPaths[i]);
			dlgProgress->SetLine(1, url.GetWithoutUserDetails());
			dlgProgress->SetProgressAdvance();
			dlgProgress->Progress();
		}

		CFileItemList tempItems;
		CLog::Log(LOGDEBUG,"Getting Directory (%s)", vecPaths[i].c_str());
		
		if (CDirectory::GetDirectory(vecPaths[i], tempItems, m_strFileMask, m_useFileDirectories, m_allowPrompting, m_cacheDirectory, m_extFileInfo))
			items.Append(tempItems);
		else
		{
			CLog::Log(LOGERROR,"Error Getting Directory (%s)", vecPaths[i].c_str());
			iFailures++;
		}

		if (dlgProgress)
		{
			dlgProgress->SetProgressAdvance();
			dlgProgress->Progress();
		}
	}

	if (dlgProgress)
		dlgProgress->Close();

	if (iFailures == vecPaths.size())
		return false;

	// Merge like-named folders into a sub multipath:// style url
	MergeItems(items);

	return true;
}

bool CMultiPathDirectory::Exists(const char* strPath)
{
	CLog::Log(LOGDEBUG,"Testing Existence (%s)", strPath);

	vector<CStdString> vecPaths;
	
	if (!GetPaths(strPath, vecPaths))
		return false;

	for (unsigned int i = 0; i < vecPaths.size(); ++i)
	{
		CLog::Log(LOGDEBUG,"Testing Existence (%s)", vecPaths[i].c_str());

		if (CDirectory::Exists(vecPaths[i]))
			return true;
	}
	return false;
}

bool CMultiPathDirectory::Remove(const char* strPath)
{
	vector<CStdString> vecPaths;
	
	if (!GetPaths(strPath, vecPaths))
		return false;

	bool success = false;
	for (unsigned int i = 0; i < vecPaths.size(); ++i)
	{
		if (CDirectory::Remove(vecPaths[i]))
			success = true;
	}
	return success;
}

CStdString CMultiPathDirectory::GetFirstPath(const CStdString &strPath)
{
	int pos = strPath.Find("/", 12);
	
	if (pos >= 0)
	{
		CStdString firstPath = strPath.Mid(12, pos - 12);
		CURL::Decode(firstPath);
		return firstPath;
	}
	return "";
}

bool CMultiPathDirectory::GetPaths(const CStdString& strPath, vector<CStdString>& vecPaths)
{
	vecPaths.empty();
	CStdString strPath1 = strPath;

	// Remove multipath:// from path and any trailing / (so that the last path doesn't get any more than it originally had)
	strPath1 = strPath1.Mid(12);
	URIUtils::RemoveSlashAtEnd(strPath1);

	// Split on "/"
	vector<CStdString> vecTemp;
	CStringUtils::SplitString(strPath1, "/", vecTemp);

	if (vecTemp.size() == 0)
		return false;

	// Check each item
	for (unsigned int i = 0; i < vecTemp.size(); i++)
	{
		CStdString tempPath = vecTemp[i];
		CURL::Decode(tempPath);
		vecPaths.push_back(tempPath);
	}
	return true;
}

bool CMultiPathDirectory::HasPath(const CStdString& strPath, const CStdString& strPathToFind)
{
	// Remove multipath:// from path and any trailing / (so that the last path doesn't get any more than it originally had)
	CStdString strPath1 = strPath.Mid(12);
	URIUtils::RemoveSlashAtEnd(strPath1);

	// Split on "/"
	vector<CStdString> vecTemp;
	CStringUtils::SplitString(strPath1, "/", vecTemp);
	
	if (vecTemp.size() == 0)
		return false;

	// Check each item
	for (unsigned int i = 0; i < vecTemp.size(); i++)
	{
		CStdString tempPath = vecTemp[i];
		CURL::Decode(tempPath);
		
		if(tempPath == strPathToFind)
			return true;
	}
	return false;
}

CStdString CMultiPathDirectory::ConstructMultiPath(const CFileItemList& items, const vector<int> &stack)
{
	// We replace all instances of comma's with double comma's, then separate
	// the paths using " , "
	
	CLog::Log(LOGDEBUG, "Building multipath");
	CStdString newPath = "multipath://";
	
	CLog::Log(LOGDEBUG, "-- adding path: %s", newPath.c_str());
	
	for (unsigned int i = 0; i < stack.size(); ++i)
		AddToMultiPath(newPath, items[stack[i]]->GetPath());

	CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
	return newPath;
}

void CMultiPathDirectory::AddToMultiPath(CStdString& strMultiPath, const CStdString& strPath)
{
	CStdString strPath1 = strPath;
	URIUtils::AddSlashAtEnd(strMultiPath);
	
	CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
	CURL::Encode(strPath1);  
	strMultiPath += strPath1;
	strMultiPath += "/";
}

CStdString CMultiPathDirectory::ConstructMultiPath(const vector<CStdString> &vecPaths)
{
	// We replace all instances of comma's with double comma's, then separate
	// the paths using " , "
	
	CLog::Log(LOGDEBUG, "Building multipath");
	CStdString newPath = "multipath://";
	
	CLog::Log(LOGDEBUG, "-- adding path: %s", newPath.c_str());

	for (unsigned int i = 0; i < vecPaths.size(); ++i)
		AddToMultiPath(newPath, vecPaths[i]);
	
	CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
	return newPath;
}

void CMultiPathDirectory::MergeItems(CFileItemList &items)
{
	CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = %i", (int)items.Size());
	DWORD dwTime=GetTickCount();
	
	if (items.Size() == 0)
		return;
	
	// Sort items by label
	// folders are before files in this sort method
	items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
	int i = 0;

	// If first item in the sorted list is a file, just abort
	if (!items.Get(i)->m_bIsFolder)
		return;

	while (i + 1 < items.Size())
	{
		// There are no more folders left, so exit the loop
		CFileItemPtr pItem1 = items.Get(i);

		if (!pItem1->m_bIsFolder)
			break;

		vector<int> stack;
		stack.push_back(i);
		CLog::Log(LOGDEBUG,"Testing path: [%03i] %s", i, pItem1->GetPath().c_str());

		int j = i + 1;
		do
		{
			CFileItemPtr pItem2 = items.Get(j);

			if (!pItem2->GetLabel().Equals(pItem1->GetLabel()))
				break;

			// Ignore any filefolders which may coincidently have
			// the same label as a true folder
			if (!pItem2->IsFileFolder())
			{
				stack.push_back(j);
				CLog::Log(LOGDEBUG,"  Adding path: [%03i] %s", j, pItem2->GetPath().c_str());
			}
			j++;
		}
		while (j < items.Size());

		// Do we have anything to combine?
		if (stack.size() > 1)
		{
			// We have a multipath so remove the items and add the new item
			CStdString newPath = ConstructMultiPath(items, stack);
			
			for (unsigned int k = stack.size() - 1; k > 0; --k)
				items.Remove(stack[k]);
			
			pItem1->SetPath(newPath);
			CLog::Log(LOGDEBUG,"  New path: %s", pItem1->GetPath().c_str());
		}

		i++;
	}
	CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = %i,  took %ld ms", items.Size(), GetTickCount()-dwTime);
}

bool CMultiPathDirectory::SupportsWriteFileOperations(const CStdString &strPath)
{
	vector<CStdString> paths;
	GetPaths(strPath, paths);
	
// TODO: Just writing for now, will address soon
/*
	for (unsigned int i = 0; i < paths.size(); ++i)
		if (CUtil::SupportsWriteFileOperations(paths[i]))
			return true;
*/	
	return false;
}