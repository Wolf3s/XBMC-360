#include "LocalizeStrings.h"
#include "..\utils\Log.h"
#ifdef HAVE_TIXML1
#include "tinyxml\tinyxml.h"
#elif HAVE_TIXML2
#include "tinyxml2\tinyxml2.h"
using namespace tinyxml2;
#include "XMLUtils.h"
#endif
CLocalizeStrings g_localizeStrings;

extern CStdString g_LoadErrorStr;

using namespace std;

CLocalizeStrings::CLocalizeStrings(void)
{}

CLocalizeStrings::~CLocalizeStrings(void)
{}

bool CLocalizeStrings::Load(const CStdString& strFileName)
{
	m_vecStrings.erase(m_vecStrings.begin(), m_vecStrings.end());
    
	TiXmlDocument xmlDoc;
    if (!xmlDoc.LoadFile(strFileName.c_str()))
    {
#ifdef HAVE_TIXML1
		CLog::Log(LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
	    g_LoadErrorStr.Format("%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());

#elif HAVE_TIXML2
		CLog::Log(LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorStr(), xmlDoc.ErrorStr());
		g_LoadErrorStr.Format("%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorLineNum(), xmlDoc.ErrorStr());
#endif
		return false;
    }

	TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (strValue != CStdString("strings"))
    {
		CLog::Log(LOGERROR, "%s Doesn't contain <strings>", strFileName.c_str());
		g_LoadErrorStr.Format("%s\nDoesnt start with <strings>", strFileName.c_str());
		return false;
    }

	const TiXmlNode *pChild = pRootElement->FirstChild();
	while (pChild)
    {
		CStdString strValue = pChild->Value();
		if (strValue == "string")
		{
#ifdef HAVE_TIXML1
			const TiXmlNode *pChildID = pChild->FirstChild("id");
			const TiXmlNode *pChildText = pChild->FirstChild("value");
#elif HAVE_TIXML2
			const TiXmlNode *pChildID = pChild->FirstChildElement("id");
			const TiXmlNode *pChildText = pChild->FirstChildElement("value");

#endif
			DWORD dwID = atoi(pChildID->FirstChild()->Value());
			CStdString utf8String;
			
			if (!pChildText->NoChildren())
			{
//				if (strEncoding.IsEmpty()) // Is language file utf8?
					utf8String=pChildText->FirstChild()->Value();
//				else
//					g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8String); //TODO
			}

			if (!utf8String.IsEmpty())
				m_vecStrings[dwID] = utf8String;
		}
		pChild = pChild->NextSibling();
	}

	return true;
}

static string szEmptyString = "";

const string& CLocalizeStrings::Get(DWORD dwCode) const
{
	ivecStrings i;
	i = m_vecStrings.find(dwCode);
	
	if (i == m_vecStrings.end())
	{
		return szEmptyString;
	}
	
	return i->second;
}

void CLocalizeStrings::Clear()
{
	m_vecStrings.erase(m_vecStrings.begin(), m_vecStrings.end());
}