#ifndef BUTTONTRANSLATOR_H
#define BUTTONTRANSLATOR_H

#include "utils\Stdafx.h"
#include "utils\StdString.h"
#ifdef HAVE_TIXML1
#include "guilib\tinyxml\tinyxml.h"
#elif HAVE_TIXML2
#include "guilib\tinyxml2\tinyxml2.h"
using namespace tinyxml2;
#include "guilib\XMLUtils.h"
#endif
#include "guilib\Key.h"
#include <map>

struct CButtonAction
{
	int iID;
	CStdString strID; // needed for "XBMC.ActivateWindow()" type actions
};

// class to map from buttons to actions
class CButtonTranslator
{
public:
	CButtonTranslator();
	virtual ~CButtonTranslator();

	bool Load();
	WORD TranslateWindowString(const char *szWindow);
	bool TranslateActionString(const char *szAction, int &iAction);
	void GetAction(WORD wWindow, const CKey &key, CAction &action);
	void Clear();

private:
	typedef std::multimap<WORD, CButtonAction> buttonMap; // Our button map to fill in
	std::map<WORD, buttonMap> translatorMap; // Mapping of windows to button maps

	void MapWindowActions(TiXmlNode *pWindow, WORD wWindowID);
	void MapAction(WORD wButtonCode, const char *szAction, buttonMap &map);
	WORD TranslateGamepadString(const char *szButton);
	WORD GetActionCode(WORD wWindow, const CKey &key, CStdString &strAction);
};

extern CButtonTranslator g_buttonTranslator;

#endif //BUTTONTRANSLATOR_H