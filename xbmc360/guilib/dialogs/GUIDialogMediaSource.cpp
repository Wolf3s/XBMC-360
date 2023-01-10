//#include "system.h" // TODO
#include "guilib\dialogs\GUIDialogMediaSource.h"
#include "GUIDialogKeyboard.h"
#include "guilib\dialogs\GUIDialogFileBrowser.h"
//#include "settings\GUIDialogContentSettings.h"// TODO
#include "guilib\windows\GUIWindowVideoFiles.h"
#include "guilib\GUIWindowManager.h"
#include "utils\Util.h"
#include "utils\URIUtils.h"
//#include "FileSystem\PluginDirectory.h"// TODO
#include "guilib\dialogs\GUIDialogYesNo.h"
#include "FileSystem\File.h"
#include "FileItem.h"
#include "guilib\LocalizeStrings.h"

using namespace std;
using namespace XFILE;

#define CONTROL_HEADING          2
#define CONTROL_PATH            10
#define CONTROL_PATH_BROWSE     11
#define CONTROL_NAME            12
#define CONTROL_PATH_ADD        13
#define CONTROL_PATH_REMOVE     14
#define CONTROL_OK              18
#define CONTROL_CANCEL          19
#define CONTROL_CONTENT         20

CGUIDialogMediaSource::CGUIDialogMediaSource(void)
	: CGUIDialog(WINDOW_DIALOG_MEDIA_SOURCE, "DialogMediaSource.xml")
{
	m_paths =  new CFileItemList;
}

CGUIDialogMediaSource::~CGUIDialogMediaSource()
{
	delete m_paths;
}

bool CGUIDialogMediaSource::OnBack(int actionID)
{
	m_confirmed = false;
	return CGUIDialog::OnBack(actionID);
}

bool CGUIDialogMediaSource::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_CLICKED:
		{
			int iControl = message.GetSenderId();
			int iAction = message.GetParam1();
#ifdef _HAS_MOUSE
			if (iControl == CONTROL_PATH && (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK))
#else
			if (iControl == CONTROL_PATH && iAction == ACTION_SELECT_ITEM)
#endif
				OnPath(GetSelectedItem());
			else if (iControl == CONTROL_PATH_BROWSE)
				OnPathBrowse(GetSelectedItem());
			else if (iControl == CONTROL_PATH_ADD)
				OnPathAdd();
			else if (iControl == CONTROL_PATH_REMOVE)
				OnPathRemove(GetSelectedItem());
			else if (iControl == CONTROL_NAME)
			{
				OnEditChanged(iControl, m_name);
				UpdateButtons();
			}
			else if (iControl == CONTROL_OK)
				OnOK();
			else if (iControl == CONTROL_CANCEL)
				OnCancel();
			else if (iControl == CONTROL_CONTENT)
			{
				CMediaSource share;
				share.FromNameAndPaths("video", m_name, GetPaths());

//				CGUIDialogContentSettings::ShowForDirectory(share.strPath,m_info,m_settings,m_bRunScan); // TODO
			}
			return true;
		}
		break;
		case GUI_MSG_WINDOW_INIT:
		{
			m_confirmed = false;
			m_bRunScan = false;
			m_bNameChanged = false;
//			m_settings.parent_name = false; // TODO
//			m_settings.recurse = 0; // TODO
			UpdateButtons();
		}
		break;
		case GUI_MSG_SETFOCUS:
		if (message.GetControlId() == CONTROL_PATH_BROWSE ||
			message.GetControlId() == CONTROL_PATH_ADD ||
			message.GetControlId() == CONTROL_PATH_REMOVE)
		{
				HighlightItem(GetSelectedItem());
		}
		else
			HighlightItem(-1);
		break;
	}
	return CGUIDialog::OnMessage(message);
}

// Show CGUIDialogMediaSource dialog and prompt for a new media source
// return True if the media source is added, false otherwise
bool CGUIDialogMediaSource::ShowAndAddMediaSource(const CStdString &type)
{
	CGUIDialogMediaSource *dialog = (CGUIDialogMediaSource *)g_windowManager.GetWindow(WINDOW_DIALOG_MEDIA_SOURCE);
	if (!dialog) return false;
	
	dialog->Initialize();
	dialog->SetShare(CMediaSource());
	dialog->SetTypeOfMedia(type);
	dialog->DoModal();
	bool confirmed(dialog->IsConfirmed());
	
	if (confirmed)
	{
		// Yay, add this share
		CMediaSource share;
		unsigned int i,j=2;
		bool bConfirmed=false;
		VECSOURCES* pShares = g_settings.GetSourcesFromType(type);
		CStdString strName = dialog->m_name;
		
		while (!bConfirmed)
		{
			for (i = 0; i < pShares->size(); ++i)
			{
				if ((*pShares)[i].strName.Equals(strName))
					break;
			}
			
			if (i < pShares->size()) // Found a match - Try next
				strName.Format("%s (%i)",dialog->m_name,j++);
			else
				bConfirmed = true;
		}
		
		share.FromNameAndPaths(type, strName, dialog->GetPaths());
		
		if (dialog->m_paths->Size() > 0)
		{
			share.m_strThumbnailImage = dialog->m_paths->Get(0)->GetThumbnailImage();
		}
		
		g_settings.AddShare(type, share);

		if (type == "video")
		{
/*			if (dialog->m_bRunScan)
				CGUIWindowVideoBase::OnScan(share.strPath,dialog->m_info,dialog->m_settings); // TODO
*/		}
	}
	dialog->m_paths->Clear();
	return confirmed;
}

bool CGUIDialogMediaSource::ShowAndEditMediaSource(const CStdString &type, const CStdString&share)
{
	VECSOURCES* pShares = NULL;

	if (pShares)
	{
		for (unsigned int i=0;i<pShares->size();++i)
		{
			if ((*pShares)[i].strName.Equals(share))
				return ShowAndEditMediaSource(type,(*pShares)[i]);
		}
	}
	return false;
}

bool CGUIDialogMediaSource::ShowAndEditMediaSource(const CStdString &type, const CMediaSource &share)
{
	CStdString strOldName = share.strName;
	CGUIDialogMediaSource *dialog = (CGUIDialogMediaSource *)g_windowManager.GetWindow(WINDOW_DIALOG_MEDIA_SOURCE);
	if (!dialog) return false;

	dialog->Initialize();
	dialog->SetShare(share);
	dialog->SetTypeOfMedia(type, true);
	dialog->DoModal();
	bool confirmed(dialog->IsConfirmed());
	
	if (confirmed)
	{
		// Yay, add this share
		unsigned int i,j=2;
		bool bConfirmed=false;
		VECSOURCES* pShares = g_settings.GetSourcesFromType(type);
		CStdString strName = dialog->m_name;
		
		while (!bConfirmed)
		{
			for (i=0;i<pShares->size();++i)
			{
				if ((*pShares)[i].strName.Equals(strName))
					break;
			}
			
			if (i < pShares->size() && (*pShares)[i].strName != strOldName) // Found a match -  try next
				strName.Format("%s (%i)",dialog->m_name,j++);
			else
				bConfirmed = true;
		}

		CMediaSource newShare;
		newShare.FromNameAndPaths(type, strName, dialog->GetPaths());
		g_settings.UpdateShare(type, strOldName, newShare);
	}
	dialog->m_paths->Clear();
	return confirmed;
}

void CGUIDialogMediaSource::OnPathBrowse(int item)
{
	if (item < 0 || item > m_paths->Size()) return;

	// Browse is called.  Open the filebrowser dialog.
	// Ignore current path is best at this stage??
	CStdString path;
	bool allowNetworkShares(m_type != "programs");
	VECSOURCES extraShares;

	if (m_name != CUtil::GetTitleFromPath(m_paths->Get(item)->GetPath()))
		m_bNameChanged=true;

	if (m_type == "music")
	{
		// Add the music playlist location
		CMediaSource share1;

		share1.strPath = "smb://";
		share1.strName = g_localizeStrings.Get(20171);
		extraShares.push_back(share1);
	}
	else if (m_type == "video")
	{
		CMediaSource share1;

		share1.strPath = "smb://";
		share1.strName = g_localizeStrings.Get(20171);
		extraShares.push_back(share1);
	}
	else if (m_type == "pictures")
	{
		CMediaSource share1;
		share1.m_ignore = true;

		share1.strPath = "smb://";
		share1.strName = g_localizeStrings.Get(20171);
		extraShares.push_back(share1);
	}
	else if (m_type == "programs")
	{
		// TODO
	}

	if (CGUIDialogFileBrowser::ShowAndGetSource(path, allowNetworkShares, extraShares.size()==0?NULL:&extraShares))
	{
		if (item < m_paths->Size()) // If the skin does funky things, m_paths may have been cleared
			m_paths->Get(item)->SetPath(path);
		
		if (!m_bNameChanged || m_name.IsEmpty())
		{
			CURL url(path);
			m_name = url.GetWithoutUserDetails();
			URIUtils::RemoveSlashAtEnd(m_name);
			m_name = CUtil::GetTitleFromPath(m_name);
		}
		UpdateButtons();
	}
}

void CGUIDialogMediaSource::OnPath(int item)
{
	if (item < 0 || item > m_paths->Size()) return;

	if (m_name != CUtil::GetTitleFromPath(m_paths->Get(item)->GetPath()))
		m_bNameChanged=true;

	CStdString path(m_paths->Get(item)->GetPath());
	CGUIDialogKeyboard::ShowAndGetInput(path, g_localizeStrings.Get(1021), false);
	URIUtils::AddSlashAtEnd(path);
	m_paths->Get(item)->SetPath(path);

	if (!m_bNameChanged || m_name.IsEmpty())
	{
		CURL url(m_paths->Get(item)->GetPath());
		m_name = url.GetWithoutUserDetails();
		URIUtils::RemoveSlashAtEnd(m_name);
		m_name = CUtil::GetTitleFromPath(m_name);
	}
	UpdateButtons();
}

void CGUIDialogMediaSource::OnOK() // TODO
{
	// Verify the path by doing a GetDirectory.
	CFileItemList items;

	CMediaSource share;
	share.FromNameAndPaths(m_type, m_name, GetPaths());

	// Hack: Need to temporarily add the share, then get path, then remove share
	VECSOURCES *shares = g_settings.GetSourcesFromType(m_type);
	if (shares)
		shares->push_back(share);
	
//	if (share.strPath.Left(9).Equals("plugin://") || CDirectory::GetDirectory(share.strPath, items, "", false, true) || CGUIDialogYesNo::ShowAndGetInput(1001,1025,1003,1004)) // TODO
	{
/*		if (share.strPath.Left(9).Equals("plugin://"))
		{
			CStdString strPath=share.strPath;
			strPath.Replace("plugin://","special://home/plugins/");
			CFileItem item(strPath,true);
			item.SetCachedProgramThumb();
			
			if (!item.HasThumbnail())
				item.SetUserProgramThumb();
			
			if (!item.HasThumbnail())
			{
				item.SetPath(URIUtils::AddFileToFolder(strPath,"default.py"));
				item.m_bIsFolder = false;
				item.SetCachedProgramThumb();
				
				if (!item.HasThumbnail())
					item.SetUserProgramThumb();
			}
			
			if (item.HasThumbnail() && m_paths->Size())
			{
				CFileItem item2(share.strPath,true);
				XFILE::CFile::Cache(item.GetThumbnailImage(),item2.GetCachedProgramThumb());
				m_paths->Get(0)->SetThumbnailImage(item2.GetCachedProgramThumb());
			}
		}
*/		m_confirmed = true;
		Close();
	}

	// Special handling of multipath:// shares.
	// * GetScraperForPath takes the first path of the multipath:// element to fetch needed scraper and scan settings.
	// * SetScraperForPath loops through all elements and adds the appropriate settings for each path.
	if (URIUtils::IsMultiPath(share.strPath))
	{
/*		CVideoDatabase database; // TODO
		database.Open();
		database.GetScraperForPath(share.strPath, m_info, m_settings);
		database.SetScraperForPath(share.strPath, m_info, m_settings);
		database.Close();
*/	}

	// and remove the share again
	if (shares)
		shares->erase(--shares->end());
}

void CGUIDialogMediaSource::OnCancel()
{
	m_confirmed = false;
	Close();
}

void CGUIDialogMediaSource::UpdateButtons()
{
	if (!m_paths->Size()) // Sanity
		return;
  
	CONTROL_ENABLE_ON_CONDITION(CONTROL_OK, !m_paths->Get(0)->GetPath().IsEmpty() && !m_name.IsEmpty());
	CONTROL_ENABLE_ON_CONDITION(CONTROL_PATH_REMOVE, m_paths->Size() > 1);
	
	// Name
	SET_CONTROL_LABEL2(CONTROL_NAME, m_name);
	SendMessage(GUI_MSG_SET_TYPE, CONTROL_NAME, 0, 1022);

	int currentItem = GetSelectedItem();
	SendMessage(GUI_MSG_LABEL_RESET, CONTROL_PATH);
	
	for (int i = 0; i < m_paths->Size(); i++)
	{
		CFileItemPtr item = m_paths->Get(i);
		CStdString path;
		CURL url(item->GetPath());
		path = url.GetWithoutUserDetails();

		if (path.IsEmpty()) path = "<"+g_localizeStrings.Get(231)+">"; // <None>
			item->SetLabel(path);
	}
	
	CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PATH, 0, 0, m_paths);
	OnMessage(msg);
	SendMessage(GUI_MSG_ITEM_SELECT, CONTROL_PATH, currentItem);

	if (m_type.Equals("video"))
	{
		SET_CONTROL_VISIBLE(CONTROL_CONTENT);
		CONTROL_ENABLE_ON_CONDITION(CONTROL_CONTENT, !m_paths->Get(0)->GetPath().IsEmpty() && !m_name.IsEmpty());
	}
	else
	{
		SET_CONTROL_HIDDEN(CONTROL_CONTENT);
	}
}

void CGUIDialogMediaSource::SetShare(const CMediaSource &share)
{
	m_paths->Clear();
	
	for (unsigned int i = 0; i < share.vecPaths.size(); i++)
	{
		CFileItemPtr item(new CFileItem(share.vecPaths[i], true));
		m_paths->Add(item);
	}
	
	if (0 == share.vecPaths.size())
	{
		CFileItemPtr item(new CFileItem("", true));
		m_paths->Add(item);
	}
	
	m_name = share.strName;
	UpdateButtons();
}

void CGUIDialogMediaSource::SetTypeOfMedia(const CStdString &type, bool editNotAdd)
{
	m_type = type;
	int typeStringID = -1;
	
	if (type == "music")
		typeStringID = 249; // "Music"
	else if (type == "video")
		typeStringID = 291;  // "Video"
	else if (type == "programs")
		typeStringID = 350;  // "Programs"
	else if (type == "pictures")
		typeStringID = 1213;  // "Pictures"
	else // if (type == "files");
		typeStringID = 744;  // "Files"
	
	CStdString format;
	format.Format(g_localizeStrings.Get(editNotAdd ? 1028 : 1020).c_str(), g_localizeStrings.Get(typeStringID).c_str());
	SET_CONTROL_LABEL(CONTROL_HEADING, format);
}

void CGUIDialogMediaSource::OnWindowLoaded()
{
	CGUIDialog::OnWindowLoaded();
	ChangeButtonToEdit(CONTROL_NAME, true); // True for single label
}

int CGUIDialogMediaSource::GetSelectedItem()
{
	CGUIMessage message(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PATH);
	OnMessage(message);
	
	int value = message.GetParam1();
	if (value < 0 || value > m_paths->Size()) return 0;
	
	return value;
}

void CGUIDialogMediaSource::HighlightItem(int item)
{
	for (int i = 0; i < m_paths->Size(); i++)
		m_paths->Get(i)->Select(false);
	
	if (item >= 0 && item < m_paths->Size())
		m_paths->Get(item)->Select(true);
	
	CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_PATH, item);
	OnMessage(msg);
}

void CGUIDialogMediaSource::OnPathRemove(int item)
{
	m_paths->Remove(item);
	UpdateButtons();
	
	if (item >= m_paths->Size())
		HighlightItem(m_paths->Size() - 1);
	else
		HighlightItem(item);
	
	if (m_paths->Size() <= 1)
	{
		SET_CONTROL_FOCUS(CONTROL_PATH_ADD, 0);
	}
}

void CGUIDialogMediaSource::OnPathAdd()
{
	// Add a new item and select it as well
	CFileItemPtr item(new CFileItem("", true));
	m_paths->Add(item);
	UpdateButtons();
	HighlightItem(m_paths->Size() - 1);
}

vector<CStdString> CGUIDialogMediaSource::GetPaths()
{
	vector<CStdString> paths;
	
	for (int i = 0; i < m_paths->Size(); i++)
		if (!m_paths->Get(i)->GetPath().IsEmpty())
			paths.push_back(m_paths->Get(i)->GetPath());
	
	return paths;
}