/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef H_CXMLUTILS
#define H_CXMLUTILS

#include "../utils/StdString.h"

#ifdef HAVE_TIXML2
#include "tinyxml2\tinyxml2.h"
using namespace tinyxml2;
typedef XMLDocument TiXmlDocument;
typedef XMLNode TiXmlNode;
typedef XMLElement TiXmlElement;
typedef XMLText TiXmlText;
#elif HAVE_TIXML1
class TiXmlNode;
class TiXmlDocument;
#endif

class XMLUtils
{
public:
	static bool GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue);
	static bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);
	static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue);
	static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue, const int min, const int max);
	static bool GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue);
	static bool GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue);

	static void SetString(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue);
	static void SetInt(TiXmlNode* pRootNode, const char *strTag, int value);
	static void SetFloat(TiXmlNode* pRootNode, const char *strTag, float value);
	static void SetHex(TiXmlNode* pRootNode, const char *strTag, uint32_t value);
	static void SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value);
	static void SetPath(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue);
};

#endif // H_CXMLUTILS