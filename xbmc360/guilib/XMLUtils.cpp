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

#include "XMLUtils.h"

bool XMLUtils::GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue)
{
	const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag );
	if (!pElement) return false;
		const char* encoded = pElement->Attribute("urlencoded");
	const TiXmlNode* pNode = pElement->FirstChild();
	if (pNode != NULL)
	{
		strStringValue = pNode->Value();
//		if (encoded && stricmp(encoded,"yes") == 0) FIXME
//			CURL::Decode(strStringValue);
		return true;
	}
	strStringValue.Empty();
	return false;
}

bool XMLUtils::GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue)
{
#ifdef HAVE_TIXML1
	const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
#elif HAVE_TIXML2
	const TiXmlNode* pNode = pRootNode->FirstChildElement(strTag);
#endif
	if(!pNode || !pNode->FirstChild())
		return false;

	dwDWORDValue = atol(pNode->FirstChild()->Value());
	return true;
}

bool XMLUtils::GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue)
{
#ifdef HAVE_TIXML1
	const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
#elif HAVE_TIXML2
	const TiXmlNode* pNode = pRootNode->FirstChildElement(strTag);
#endif
	if(!pNode || !pNode->FirstChild())
		return false;

	iIntValue = atoi(pNode->FirstChild()->Value());
  
	return true;
}

bool XMLUtils::GetInt(const TiXmlNode* pRootNode, const char* strTag, int &value, const int min, const int max)
{
	if (GetInt(pRootNode, strTag, value))
	{
		if (value < min) value = min;
		if (value > max) value = max;
		return true;
	}
	return false;
}

bool XMLUtils::GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue)
{
#ifdef HAVE_TIXML1
	const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
#elif HAVE_TIXML2
	const TiXmlNode* pNode = pRootNode->FirstChildElement(strTag);
#endif
	if (!pNode || !pNode->FirstChild()) return false;
	sscanf(pNode->FirstChild()->Value(), "%x", &dwHexValue );
	return true;
}

bool XMLUtils::GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue)
{
#ifdef HAVE_TIXML1
	const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
#elif HAVE_TIXML2
	const TiXmlNode* pNode = pRootNode->FirstChildElement(strTag );
#endif
	if (!pNode || !pNode->FirstChild()) return false;
	CStdString strEnabled = pNode->FirstChild()->Value();
	strEnabled.ToLower();
	if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" || strEnabled == "false" || strEnabled == "0" )
		bBoolValue = false;
	else
	{
		bBoolValue = true;
		if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" && strEnabled != "true")
			return false; // invalid bool switch - it's probably some other string.
	}
	return true;
}

bool XMLUtils::GetDouble(const TiXmlNode *root, const char *tag, double &value)
{
	const TiXmlNode* node = root->FirstChild(tag);
	if (!node || !node->FirstChild()) return false;
	value = atof(node->FirstChild()->Value());
	return true;
}

bool XMLUtils::GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value)
{
	const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
	if (!pNode || !pNode->FirstChild()) return false;
	value = (float)atof(pNode->FirstChild()->Value());
	return true;
}

// Returns true if the encoding of the document is other then UTF-8.
// param strEncoding Returns the encoding of the document. Empty if UTF-8
bool XMLUtils::GetEncoding(const TiXmlDocument* pDoc, CStdString& strEncoding)
{
	const TiXmlNode* pNode=NULL;
	while ((pNode=pDoc->IterateChildren(pNode)) && pNode->Type()!=TiXmlNode::DECLARATION) {}
	if (!pNode) return false;
	const TiXmlDeclaration* pDecl=pNode->ToDeclaration();
	if (!pDecl) return false;
	strEncoding=pDecl->Encoding();
	if (strEncoding.Equals("UTF-8") || strEncoding.Equals("UTF8")) strEncoding.Empty();
	strEncoding.MakeUpper();

	return !strEncoding.IsEmpty(); // Other encoding then UTF8?
}

void XMLUtils::SetString(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue)
{
#ifdef HAVE_TIXML1
	TiXmlElement newElement(strTag);
	TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
#elif HAVE_TIXML2
	TiXmlDocument doc;
	auto *newElement = doc.NewElement(strTag);
	TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
#endif
	if(pNewNode)
	{
#ifdef HAVE_TIXML1
		TiXmlText value(strValue);
		pNewNode->InsertEndChild(value);
#elif HAVE_TIXML2
		auto *value = doc.NewText(strValue.c_str()); //new doc.NewText(strValue.c_str());
		pNewNode->InsertEndChild(value);
#endif
	}
}

void XMLUtils::SetInt(TiXmlNode* pRootNode, const char *strTag, int value)
{
	CStdString strValue;
	strValue.Format("%i", value);
	SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetFloat(TiXmlNode* pRootNode, const char *strTag, float value)
{
	CStdString strValue;
	strValue.Format("%f", value);
	SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetHex(TiXmlNode* pRootNode, const char *strTag, uint32_t value)
{
	CStdString strValue;
	strValue.Format("%x", value);
	SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value)
{
	SetString(pRootNode, strTag, value ? "true" : "false");
}

void XMLUtils::SetPath(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue)
{
#ifdef HAVE_TIXML1
	TiXmlElement newElement(strTag);
	TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
#elif HAVE_TIXML2
	TiXmlDocument doc;
	auto *newElement = doc.NewElement(strTag);
	TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
#endif
	if(pNewNode)
	{
#ifdef HAVE_TIXML1
		TiXmlText value(strValue);
		pNewNode->InsertEndChild(value);
#elif HAVE_TIXML2
		auto *value = doc.NewText(strValue.c_str());
		pNewNode->InsertEndChild(value);
#endif	
	}
}