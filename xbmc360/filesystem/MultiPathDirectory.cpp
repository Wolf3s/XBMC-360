#include "MultiPathDirectory.h"
#include "..\utils\log.h"
#include "..\utils\StringUtils.h"
#include "Directory.h"

using namespace DIRECTORY;

//
// Multipath://{path1} , {path2} , {path3} , ... , {path-N}
//
// Unlike the older virtualpath:// protocol, sub-folders are combined together into a new
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

	if(!GetPaths(strPath, vecPaths))
		return false;

/*
	DWORD progressTime = timeGetTime() + 3000L; // Three seconds before showing progress bar
	CGUIDialogProgress* dlgProgress = NULL;
*/
	int iFailures = 0;
	for(int i = 0; i < (int)vecPaths.size(); ++i)
	{
		// Show the progress dialog if we have passed our time limit
/*		if(timeGetTime() > progressTime && !dlgProgress)
		{
			dlgProgress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
			if(dlgProgress)
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

		if(dlgProgress)
		{
			CURL url(vecPaths[i]);
			CStdString strStripped;
			url.GetURLWithoutUserDetails(strStripped);
			dlgProgress->SetLine(1, strStripped);
			dlgProgress->SetProgressAdvance();
			dlgProgress->Progress();
		}
*/
		CFileItemList tempItems;
		CLog::Log(LOGDEBUG,"Getting Directory (%s)", vecPaths[i].c_str());
		
		if(CDirectory::GetDirectory(vecPaths[i], tempItems, m_strFileMask))
			items.Append(tempItems);
		else
		{
			CLog::Log(LOGERROR,"Error Getting Directory (%s)", vecPaths[i].c_str());
			iFailures++;
		}
/*
		if(dlgProgress)
		{
			dlgProgress->SetProgressAdvance();
			dlgProgress->Progress();
		}
*/	}

//	if(dlgProgress)
//		dlgProgress->Close();

	if(iFailures == vecPaths.size())
		return false;

	// Merge like-named folders into a sub multipath:// style url
	MergeItems(items);

	return true;
}

bool CMultiPathDirectory::GetPaths(const CStdString& strPath, vector<CStdString>& vecPaths)
{
	vecPaths.empty();
	CStdString strPath1 = strPath;

	// Remove multipath:// from path
	strPath1 = strPath1.Mid(12);

	// Split on " , "
	vector<CStdString> vecTemp;
	CStringUtils::SplitString(strPath1, " , ", vecTemp);

	if(vecTemp.size() == 0)
		return false;

	// Check each item
	for(int i = 0; i < (int)vecTemp.size(); i++)
	{
		CStdString tempPath = vecTemp[i];

		// Replace double comma's with single ones.
		tempPath.Replace(",,", ",");
		vecPaths.push_back(tempPath);
	}
	
	if(vecPaths.size() == 0)
		return false;

	return true;
}

CStdString CMultiPathDirectory::GetFirstPath(const CStdString &strPath)
{
	int pos = strPath.Find(" , ", 12);

	if(pos >= 0)
	{
		CStdString firstPath = strPath.Mid(12, pos - 12);
		firstPath.Replace(",,",",");
		return firstPath;
	}
	return "";
}

void CMultiPathDirectory::MergeItems(CFileItemList &items)
{
	CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = %i", (int)items.Size());
	DWORD dwTime=GetTickCount();

	// Sort items by label
	// Folders are before files in this sort method
//	items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC); //TODO
	int i = 0;

	// If first item in the sorted list is a file, just abort
	if(!items.Get(i)->m_bIsFolder)
		return;

	while(i + 1 < items.Size())
	{
		// There are no more folders left, so exit the loop
		CFileItem* pItem1 = items.Get(i);

		if(!pItem1->m_bIsFolder)
			break;

		vector<int> stack;
		stack.push_back(i);
		CLog::Log(LOGDEBUG,"Testing path: [%03i] %s", i, pItem1->m_strPath.c_str());

		int j = i + 1;
		
		do
		{
			CFileItem* pItem2 = items.Get(j);

			if(!pItem2->GetLabel().Equals(pItem1->GetLabel()))
				break;

			// Ignore any filefolders which may coincidently have
			// the same label as a true folder
			if(!pItem2->IsFileFolder())
			{
				stack.push_back(j);
				CLog::Log(LOGDEBUG,"  Adding path: [%03i] %s", j, pItem2->m_strPath.c_str());
			}
			j++;
		} while(j < items.Size());

		// Do we have anything to combine?
		if(stack.size() > 1)
		{
			// We have a multipath so remove the items and add the new item
			CStdString newPath = ConstructMultiPath(items, stack);
			for(unsigned int k = stack.size() - 1; k > 0; --k)
				items.Remove(stack[k]);
			
			pItem1->m_strPath = newPath;
			CLog::Log(LOGDEBUG,"  New path: %s", pItem1->m_strPath.c_str());
		}
		i++;
	}
	CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = %i,  took %ld ms", items.Size(), GetTickCount()-dwTime);
}

CStdString CMultiPathDirectory::ConstructMultiPath(const vector<CStdString> &vecPaths)
{
	// We replace all instances of comma's with double comma's, then separate the paths using " , "
	CLog::Log(LOGDEBUG, "Building multipath");
	CStdString newPath = "multipath://";
	CStdString strPath = vecPaths[0];

	CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
	strPath.Replace(",", ",,");
	newPath += strPath;

	for(unsigned int i = 1; i < vecPaths.size(); ++i)
	{
		newPath += " , ";
		strPath = vecPaths[i];
		
		CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
		strPath.Replace(",", ",,");
		newPath += strPath;
	}
	CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
	return newPath;
}

CStdString CMultiPathDirectory::ConstructMultiPath(const CFileItemList& items, const vector<int> &stack)
{
	// We replace all instances of comma's with double comma's, then separate the paths using " , "
	CLog::Log(LOGDEBUG, "Building multipath");
	CStdString newPath = "multipath://";
	CStdString strPath = items[stack[0]]->m_strPath;

	CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
	strPath.Replace(",", ",,");
	newPath += strPath;

	for(unsigned int i = 1; i < stack.size(); ++i)
	{
		newPath += " , ";
		strPath = items[stack[i]]->m_strPath;
		
		CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
		strPath.Replace(",", ",,");
		newPath += strPath;
	}
	CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
	
	return newPath;
}