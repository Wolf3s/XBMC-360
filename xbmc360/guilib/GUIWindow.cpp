#include "include.h"
#include "GUIWindow.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "TextureManager.h"
#include "GUISettings.h"
#include "GUIControlFactory.h"
#include "GUIControlGroup.h"
//#ifdef PRE_SKIN_VERSION_9_10_COMPATIBILITY
#include "GUIEditControl.h"
//#endif

#include "SkinInfo.h"
#include "GUIInfoManager.h"
#include "utils/SingleLock.h"
#include "utils/TimeUtils.h"
#include "ButtonTranslator.h"
#include "XMLUtils.h"

#include "GUITexture.h" //REMOVE ME

using namespace std;

CGUIWindow::CGUIWindow(int id, const CStdString &xmlFile)
{
	SetID(id);
	SetProperty("xmlfile", xmlFile);
	m_idRange = 1;
	m_saveLastControl = false;
	m_lastControlID = 0;
	m_bRelativeCoords = false;
	m_overlayState = OVERLAY_STATE_PARENT_WINDOW;// Use parent or previous window's state
	m_coordsRes = g_guiSettings.m_LookAndFeelResolution;
	m_isDialog = false;
	m_needsScaling = true;
	m_windowLoaded = false;
	m_loadOnDemand = true;
	m_renderOrder = 0;
	m_dynamicResourceAlloc = true;
	m_previousWindow = WINDOW_INVALID;
	m_animationsEnabled = true;
	m_manualRunActions = false;
	m_exclusiveMouseControl = 0;
	m_clearBackground = 0xff000000; // opaque black -> always clear
}

CGUIWindow::~CGUIWindow(void)
{
}

bool CGUIWindow::Load(const CStdString& strFileName, bool bContainsPath)
{
	if (m_windowLoaded)
		return true; // No point loading if it's already there

#ifdef _DEBUG
	int64_t start;
	start = CurrentHostCounter();
#endif
	RESOLUTION resToUse = INVALID;
	CLog::Log(LOGINFO, "Loading skin file: %s", strFileName.c_str());
	TiXmlDocument xmlDoc;
	
	// Find appropriate skin folder + resolution to load from
	CStdString strPath;
	CStdString strLowerPath;

	if (bContainsPath)
		strPath = strFileName;
	else
		strPath = g_SkinInfo.GetSkinPath(strFileName, &resToUse);

	if (!bContainsPath)
		m_coordsRes = HDTV_480p_16x9;//resToUse; // FIXME

	bool ret = LoadXML(strPath.c_str(), strLowerPath.c_str());

#ifdef _DEBUG
	int64_t end, freq;
	end = CurrentHostCounter();
	freq = CurrentHostFrequency();
	CLog::Log(LOGDEBUG,"Load %s: %.2fms", GetProperty("xmlfile").c_str(), 1000.f * (end - start) / freq);
#endif
	return ret;
}

bool CGUIWindow::LoadXML(const CStdString &strPath, const CStdString &strLowerPath)
{
	TiXmlDocument xmlDoc;
	if ( !xmlDoc.LoadFile(strPath) && !xmlDoc.LoadFile(CStdString(strPath).ToLower()) && !xmlDoc.LoadFile(strLowerPath))
	{
		CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
		SetID(WINDOW_INVALID);
		return false;
	}

	return Load(xmlDoc);
}

bool CGUIWindow::Load(TiXmlDocument &xmlDoc)
{
	TiXmlElement* pRootElement = xmlDoc.RootElement();
	if (strcmpi(pRootElement->Value(), "window"))
	{
		CLog::Log(LOGERROR, "file : XML file doesnt contain <window>");
		return false;
	}

	// Set the scaling resolution so that any control creation or initialisation can
	// be done with respect to the correct aspect ratio
	g_graphicsContext.SetScalingResolution(m_coordsRes, m_needsScaling);

	// Resolve any includes that may be present
	g_SkinInfo.ResolveIncludes(pRootElement);

	// Now load in the skin file
	SetDefaults();

	CGUIControlFactory::GetInfoColor(pRootElement, "backgroundcolor", m_clearBackground, GetID());
	CGUIControlFactory::GetActions(pRootElement, "onload", m_loadActions);
	CGUIControlFactory::GetActions(pRootElement, "onunload", m_unloadActions);
	CGUIControlFactory::GetHitRect(pRootElement, m_hitRect);
    
	TiXmlElement *pChild = pRootElement->FirstChildElement();
	while (pChild)
	{
		CStdString strValue = pChild->Value();
		if (strValue == "type" && pChild->FirstChild())
		{
			// If we have are a window type (ie not a dialog), and we have <type>dialog</type>
			// then make this window act like a dialog
			if (!IsDialog() && strcmpi(pChild->FirstChild()->Value(), "dialog") == 0)
				m_isDialog = true;
		}
		else if (strValue == "previouswindow" && pChild->FirstChild())
		{
			m_previousWindow = CButtonTranslator::TranslateWindow(pChild->FirstChild()->Value());
		}
		else if (strValue == "defaultcontrol" && pChild->FirstChild())
		{
			const char *always = pChild->Attribute("always");

			if (always && strcmpi(always, "true") == 0)
				m_saveLastControl = false;

			m_defaultControl = atoi(pChild->FirstChild()->Value());
		}
		else if (strValue == "visible" && pChild->FirstChild())
		{
			CGUIControlFactory::GetConditionalVisibility(pRootElement, m_visibleCondition);
		}
		else if (strValue == "animation" && pChild->FirstChild())
		{
			FRECT rect = { 0, 0, (float)g_settings.m_ResInfo[m_coordsRes].iWidth, (float)g_settings.m_ResInfo[m_coordsRes].iHeight };
			CAnimation anim;
			anim.Create(pChild, rect);
			m_animations.push_back(anim);
		}
		else if (strValue == "zorder" && pChild->FirstChild())
		{
			m_renderOrder = atoi(pChild->FirstChild()->Value());
		}
		else if (strValue == "coordinates")
		{
			// Resolve any includes within coordinates tag (such as multiple origin includes)
			g_SkinInfo.ResolveIncludes(pChild);
			TiXmlNode* pSystem = pChild->FirstChild("system");
			if (pSystem)
			{
				int iCoordinateSystem = atoi(pSystem->FirstChild()->Value());
				m_bRelativeCoords = (iCoordinateSystem == 1);
			}

			CGUIControlFactory::GetFloat(pChild, "posx", m_posX);
			CGUIControlFactory::GetFloat(pChild, "posy", m_posY);
			CGUIControlFactory::GetFloat(pChild, "left", m_posX);
			CGUIControlFactory::GetFloat(pChild, "top", m_posY);

			TiXmlElement *originElement = pChild->FirstChildElement("origin");
			while (originElement)
			{
				COrigin origin;
				g_SkinInfo.ResolveConstant(originElement->Attribute("x"), origin.x);
				g_SkinInfo.ResolveConstant(originElement->Attribute("y"), origin.y);
				
				if (originElement->FirstChild())
					origin.condition = g_infoManager.TranslateString(originElement->FirstChild()->Value());
				
				m_origins.push_back(origin);
				originElement = originElement->NextSiblingElement("origin");
			}
		}
		else if (strValue == "camera")
		{
			// Z is fixed
			g_SkinInfo.ResolveConstant(pChild->Attribute("x"), m_camera.x);
			g_SkinInfo.ResolveConstant(pChild->Attribute("y"), m_camera.y);
			m_hasCamera = true;
		}
		else if (strValue == "controls")
		{
			// Resolve any includes within controls tag (such as whole <control> includes)
			g_SkinInfo.ResolveIncludes(pChild);

			TiXmlElement *pControl = pChild->FirstChildElement();
			while (pControl)
			{
				if (strcmpi(pControl->Value(), "control") == 0)
				{
					LoadControl(pControl, NULL);
				}
				pControl = pControl->NextSiblingElement();
			}
		}
		else if (strValue == "allowoverlay")
		{
			bool overlay = false;
			if (XMLUtils::GetBoolean(pRootElement, "allowoverlay", overlay))
				m_overlayState = overlay ? OVERLAY_STATE_SHOWN : OVERLAY_STATE_HIDDEN;
		}

		pChild = pChild->NextSiblingElement();
	}
	LoadAdditionalTags(pRootElement);

	m_windowLoaded = true;
	OnWindowLoaded();
	return true;
}

void CGUIWindow::LoadControl(TiXmlElement* pControl, CGUIControlGroup *pGroup)
{
	// Get control type
	CGUIControlFactory factory;

	FRECT rect = { 0, 0, (float)g_settings.m_ResInfo[m_coordsRes].iWidth, (float)g_settings.m_ResInfo[m_coordsRes].iHeight };
	if (pGroup)
	{
		rect.left = pGroup->GetXPosition();
		rect.top = pGroup->GetYPosition();
		rect.right = rect.left + pGroup->GetWidth();
		rect.bottom = rect.top + pGroup->GetHeight();
	}

	CGUIControl* pGUIControl = factory.Create(GetID(), rect, pControl);
	if (pGUIControl)
	{
		float maxX = pGUIControl->GetXPosition() + pGUIControl->GetWidth();
		if (maxX > m_width)
		{
			m_width = maxX;
		}

		float maxY = pGUIControl->GetYPosition() + pGUIControl->GetHeight();
		if (maxY > m_height)
		{
			m_height = maxY;
		}
		
		// If we are in a group, add to the group, else add to our window
		if (pGroup)
			pGroup->AddControl(pGUIControl);
		else
			AddControl(pGUIControl);
		
		// If the new control is a group, then add it's controls
		if (pGUIControl->IsGroup())
		{
			TiXmlElement *pSubControl = pControl->FirstChildElement("control");
			while (pSubControl)
			{
				LoadControl(pSubControl, (CGUIControlGroup *)pGUIControl);
				pSubControl = pSubControl->NextSiblingElement("control");
			}
		}
	}
}

void CGUIWindow::OnWindowLoaded()
{
	DynamicResourceAlloc(true);
}

void CGUIWindow::CenterWindow()
{
	if (m_bRelativeCoords)
	{
		m_posX = (g_settings.m_ResInfo[m_coordsRes].iWidth - GetWidth()) / 2;
		m_posY = (g_settings.m_ResInfo[m_coordsRes].iHeight - GetHeight()) / 2;
	}
}

void CGUIWindow::Render()
{
	// If we're rendering from a different thread, then we should wait for the main
	// app thread to finish AllocResources(), as dynamic resources (images in particular)
	// will try and be allocated from 2 different threads, which causes nasty things
	// to occur.
	if (!m_bAllocated) return;

	g_graphicsContext.SetRenderingResolution(m_coordsRes, m_needsScaling);

	m_renderTime = CTimeUtils::GetFrameTime();

	// render our window animation - returns false if it needs to stop rendering
	if (!RenderAnimation(m_renderTime))
		return;

	if (m_hasCamera)
		g_graphicsContext.SetCameraPosition(m_camera);

	CGUIControlGroup::Render();
}

void CGUIWindow::Close(bool forceClose)
{
	CLog::Log(LOGERROR,"%s - should never be called on the base class!", __FUNCTION__);
}

bool CGUIWindow::OnAction(const CAction &action)
{
#ifdef _HAS_MOUSE
	if (action.IsMouse())
		return OnMouseAction(action);
#endif

	CGUIControl *focusedControl = GetFocusedControl();
	if (focusedControl)
	{
		if (focusedControl->OnAction(action))
			return true;
	}
	else
	{
		// No control has focus?
		// Set focus to the default control then
		CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_defaultControl);
		OnMessage(msg);
	}

	// Default implementations
	if (action.GetID() == ACTION_NAV_BACK || action.GetID() == ACTION_PREVIOUS_MENU)
		return OnBack(action.GetID());

	return false;
}

CPoint CGUIWindow::GetPosition() const
{
	for (unsigned int i = 0; i < m_origins.size(); i++)
	{
		// No condition implies true
		if (!m_origins[i].condition || g_infoManager.GetBool(m_origins[i].condition, GetID()))
		{
			// Found origin
			return CPoint(m_origins[i].x, m_origins[i].y);
		}
	}
	return CGUIControlGroup::GetPosition();
}

// OnMouseAction - Called by OnAction()
#ifdef _HAS_MOUSE
bool CGUIWindow::OnMouseAction(const CAction &action)
{
	g_graphicsContext.SetScalingResolution(m_coordsRes, m_needsScaling);
	CPoint mousePoint(action.GetAmount(0), action.GetAmount(1));
	g_graphicsContext.InvertFinalCoords(mousePoint.x, mousePoint.y);

	// Create the mouse event
	CMouseEvent event(action.GetID(), action.GetHoldTime(), action.GetAmount(2), action.GetAmount(3));
	if (m_exclusiveMouseControl)
	{
		CGUIControl *child = (CGUIControl *)GetControl(m_exclusiveMouseControl);
		if (child)
		{
			CPoint renderPos = child->GetRenderPosition() - CPoint(child->GetXPosition(), child->GetYPosition());
			return child->OnMouseEvent(mousePoint - renderPos, event);
		}
	}

	UnfocusFromPoint(mousePoint);

	return SendMouseEvent(mousePoint, event);
}
#endif

#ifdef _HAS_MOUSE
bool CGUIWindow::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
	if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
	{
		// No control found to absorb this click - go to previous menu
		return OnAction(CAction(ACTION_PREVIOUS_MENU));
	}
	return false;
}
#endif

// Called on window open.
//  * Restores the control state(s)
//  * Sets initial visibility of controls
//  * Queue WindowOpen animation
//  * Set overlay state
// Override this function and do any window-specific initialisation such
// as filling control contents and setting control focus before
// calling the base method
void CGUIWindow::OnInitWindow()
{
	// Set our rendered state
	m_hasRendered = false;
	ResetAnimations();  // We need to reset our animations as those windows that don't dynamically allocate
                        // need their anims reset. An alternative solution is turning off all non-dynamic
                        // allocation (which in some respects may be nicer, but it kills hdd spindown and the like)

	// Set our initial control visibility before restoring control state and
	// focusing the default control, and again afterward to make sure that
	// any controls that depend on the state of the focused control (and or on
	// control states) are active.
	SetInitialVisibility();
	RestoreControlStates();
	SetInitialVisibility();
	QueueAnimation(ANIM_TYPE_WINDOW_OPEN);
	g_windowManager.ShowOverlay(m_overlayState);
  
	if (!m_manualRunActions)
	{
		RunLoadActions();
	}
}

//===============================================================================================================================================================================================

void CGUIWindow::OnEditChanged(int id, CStdString &text)
{
	CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), id);
	OnMessage(msg);
	text = msg.GetLabel();
}

void CGUIWindow::ChangeButtonToEdit(int id, bool singleLabel /* = false*/)
{
//#ifdef PRE_SKIN_VERSION_9_10_COMPATIBILITY
	CGUIControl *name = (CGUIControl *)GetControl(id);
	if (name && name->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
	{
		// Change it to an edit control
		CGUIEditControl *edit = new CGUIEditControl(*(const CGUIButtonControl *)name);
		if (edit)
		{
			if (singleLabel)
				edit->SetLabel("");
			InsertControl(edit, name);
			RemoveControl(name);
			name->FreeResources();
			delete name;
		}
	}
//#endif
}

void CGUIWindow::RunLoadActions()
{
  m_loadActions.Execute(GetID(), GetParentID());
}

bool CGUIWindow::Initialize()
{
	if (!g_windowManager.Initialized())
		return false; // Can't load if we have no skin yet
	
	return Load(GetProperty("xmlfile"));
}

void CGUIWindow::DynamicResourceAlloc(bool bOnOff)
{
	m_dynamicResourceAlloc = bOnOff;

}


bool CGUIWindow::CheckAnimation(ANIMATION_TYPE animType)
{
  // special cases first
  if (animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    if (!m_bAllocated || !m_hasRendered) // can't render an animation if we aren't allocated or haven't rendered
      return false;
    // make sure we update our visibility prior to queuing the window close anim
    for (unsigned int i = 0; i < m_children.size(); i++)
      m_children[i]->UpdateVisibility();
  }
  return true;
}

bool CGUIWindow::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_INIT:
		{
			CLog::Log(LOGDEBUG, "------ Window Init (%s) ------", GetProperty("xmlfile").c_str());
			if (m_dynamicResourceAlloc || !m_bAllocated) AllocResources();
			OnInitWindow();
			return true;
		}
		break;

		case GUI_MSG_WINDOW_DEINIT:
		{
			CLog::Log(LOGDEBUG, "------ Window Deinit (%s) ------", GetProperty("xmlfile").c_str());
			OnDeinitWindow(message.GetParam1());
			// Now free the window
			if (m_dynamicResourceAlloc) FreeResources();
			return true;
		}
		break;
/*
		case GUI_MSG_CLICKED: /?TODO
		{
			// A specific control was clicked
			CLICK_EVENT clickEvent = m_mapClickEvents[ message.GetSenderId() ];

			// Determine if there are any handlers for this event
			if (clickEvent.HasAHandler())
			{
				// Fire the message to all handlers
				clickEvent.Fire(message);
			}
			break;
		}
*/
/*
		case GUI_MSG_SELCHANGED: //TODO
		{
			// A selection within a specific control has changed
			SELECTED_EVENT selectedEvent = m_mapSelectedEvents[ message.GetSenderId() ];

			// Determine if there are any handlers for this event
			if (selectedEvent.HasAHandler())
			{
				// Fire the message to all handlers
				selectedEvent.Fire(message);
			}
		   break;
		}
*/
		case GUI_MSG_FOCUSED:
		{
			// A control has been focused
			if (HasID(message.GetSenderId()))
			{
				m_focusedControl = message.GetControlId();
				return true;
			}
			break;
		}

		case GUI_MSG_LOSTFOCUS:
		{
			// Nothing to do at the window level when we lose focus
			return true;
		}

		case GUI_MSG_MOVE:
		{
			if (HasID(message.GetSenderId()))
				return OnMove(message.GetControlId(), message.GetParam1());
			break;
		}

		case GUI_MSG_SETFOCUS:
		{
			CLog::Log(LOGDEBUG,"set focus to control:%i window:%i (%i)\n", message.GetControlId(),message.GetSenderId(), GetID());
			if ( message.GetControlId() )
			{
				// First unfocus the current control
				CGUIControl *control = GetFocusedControl();
				if (control)
				{
					CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS, GetID(), control->GetID(), message.GetControlId());
					control->OnMessage(msgLostFocus);
				}

				// Get the control to focus
				CGUIControl* pFocusedControl = GetFirstFocusableControl(message.GetControlId());
				if (!pFocusedControl) pFocusedControl = (CGUIControl *)GetControl(message.GetControlId());

				// and focus it
				if (pFocusedControl)
					return pFocusedControl->OnMessage(message);
			}
			return true;
		}
		break;

#ifdef _HAS_MOUSE
		case GUI_MSG_EXCLUSIVE_MOUSE:
		{
			m_exclusiveMouseControl = message.GetSenderId();
			return true;
		}
		break;
#endif
/*		case GUI_MSG_NOTIFY_ALL: //TODO
		{
			// Only process those notifications that come from this window, or those intended for every window
			if (HasID(message.GetSenderId()) || !message.GetSenderId())
			{
				if (message.GetParam1() == GUI_MSG_PAGE_CHANGE ||
					message.GetParam1() == GUI_MSG_REFRESH_THUMBS ||
					message.GetParam1() == GUI_MSG_REFRESH_LIST)
				{
					// Alter the message accordingly, and send to all controls
					for (iControls it = m_children.begin(); it != m_children.end(); ++it)
					{
						CGUIControl *control = *it;
						CGUIMessage msg(message.GetParam1(), message.GetControlId(), control->GetID(), message.GetParam2());
						control->OnMessage(msg);
					}
				}
			}
		}
		break;
*/	}

	return SendControlMessage(message);
}

// Returns true if the control group with id groupID has controlID as
// its focused control
bool CGUIWindow::ControlGroupHasFocus(int groupID, int controlID)
{
	// 1.  Run through and get control with groupID (assume unique)
	// 2.  Get it's selected item.
	CGUIControl *group = GetFirstFocusableControl(groupID);
	if (!group) group = (CGUIControl *)GetControl(groupID);

	if (group && group->IsGroup())
	{
		if (controlID == 0)
		{
			// Just want to know if the group is focused
			return group->HasFocus();
		}
		else
		{
			CGUIMessage message(GUI_MSG_ITEM_SELECTED, GetID(), group->GetID());
			group->OnMessage(message);
			return (controlID == (int) message.GetParam1());
		}
	}
	return false;
}


// Called on window close.
//  * Executes the window close animation(s)
//  * Saves control state(s)
// Override this function and call the base class before doing any dynamic memory freeing
void CGUIWindow::OnDeinitWindow(int nextWindowID)
{
	if (!m_manualRunActions)
		RunUnloadActions();
  
	if (nextWindowID != WINDOW_FULLSCREEN_VIDEO)
	{
		// Dialog animations are handled in Close() rather than here
		if (HasAnimation(ANIM_TYPE_WINDOW_CLOSE) && !IsDialog())
		{
			// Perform the window out effect
			QueueAnimation(ANIM_TYPE_WINDOW_CLOSE);
			
			while (IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
			{
				g_windowManager.ProcessRenderLoop(true);
			}
		}
	}
	SaveControlStates();
}

bool CGUIWindow::RenderAnimation(unsigned int time)
{
	g_graphicsContext.ResetWindowTransform();

	if (m_animationsEnabled)
		CGUIControlGroup::Animate(time);
	else
		m_transform.Reset();

	return true;
}

void CGUIWindow::ResetControlStates()
{
	m_lastControlID = 0;
	m_focusedControl = 0;
	m_controlStates.clear();
}

bool CGUIWindow::OnBack(int actionID)
{
	g_windowManager.PreviousWindow();
	return true;
}

void CGUIWindow::ClearBackground()
{
	DWORD color = m_clearBackground;
	if (color)
		g_graphicsContext.Clear(color);
}

void CGUIWindow::AllocResources(bool forceLoad /*= FALSE */)
{
	CSingleLock lock(g_graphicsContext);

#ifdef _DEBUG
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
#endif
	
	// Load skin xml fil
	CStdString xmlFile = GetProperty("xmlfile");
	bool bHasPath=false;

	if (xmlFile.Find("\\") > -1 || xmlFile.Find("/") > -1 )
		bHasPath = true;
	if (xmlFile.size() && (forceLoad || m_loadOnDemand || !m_windowLoaded))
		Load(xmlFile,bHasPath);

	LARGE_INTEGER slend;
	QueryPerformanceCounter(&slend);

	// and now allocate resources
//	g_TextureManager.StartPreLoad();
	CGUIControlGroup::PreAllocResources();
//	g_TextureManager.EndPreLoad();

	LARGE_INTEGER plend;
	QueryPerformanceCounter(&plend);

	CGUIControlGroup::AllocResources();

//	g_TextureManager.FlushPreLoad();

#ifdef _DEBUG
	LARGE_INTEGER end, freq;
	QueryPerformanceCounter(&end);
	QueryPerformanceFrequency(&freq);
	CLog::Log(LOGDEBUG,"Alloc resources: %.2fms (%.2f ms skin load, %.2f ms preload)", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, 
				1000.f * (slend.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (plend.QuadPart - slend.QuadPart) / freq.QuadPart);
#endif
	m_bAllocated = true;
}

void CGUIWindow::FreeResources(bool forceUnload /*= FALSE */)
{
	m_bAllocated = false;
	CGUIControlGroup::FreeResources();
	//g_TextureManager.Dump();
	// Unload the skin
	if (m_loadOnDemand || forceUnload) ClearAll();
}

void CGUIWindow::ClearAll()
{
	OnWindowUnload();
	CGUIControlGroup::ClearAll();
	m_windowLoaded = false;
	m_dynamicResourceAlloc = true;
}

void CGUIWindow::SetInitialVisibility()
{
	// Reset our info manager caches
	g_infoManager.ResetCache();
	CGUIControlGroup::SetInitialVisibility();
}

bool CGUIWindow::IsActive() const
{
	return g_windowManager.IsWindowActive(GetID());
}

bool CGUIWindow::IsAnimating(ANIMATION_TYPE animType)
{
	if (!m_animationsEnabled)
		return false;

	return CGUIControlGroup::IsAnimating(animType);
}

void CGUIWindow::DisableAnimations()
{
	m_animationsEnabled = false;
}

bool CGUIWindow::OnMove(int fromControl, int moveAction)
{
	const CGUIControl *control = GetFirstFocusableControl(fromControl);
	if(!control) control = GetControl(fromControl);
	if(!control)
	{
		// No current control??
		CLog::Log(LOGERROR, "Unable to find control %i in window %i", fromControl, GetID());
		return false;
	}

	vector<int> moveHistory;
	int nextControl = fromControl;
	
	int iCtrlID = control->GetID();

	while(control)
	{
		// Grab the next control direction
		moveHistory.push_back(nextControl);
		nextControl = control->GetNextControl(moveAction);

		// Check our history - if the nextControl is in it, we can't focus it
		for(unsigned int i = 0; i < moveHistory.size(); i++)
		{
			if(nextControl == moveHistory[i])
				return false; // No control to focus so do nothing
		}
		
		control = GetFirstFocusableControl(nextControl);
		if(control)
			break; // Found a focusable control

		control = GetControl(nextControl); // Grab the next control and try again
	}

	if(!control)
		return false; // no control to focus
	
	// If we get here we have our new control so focus it (and unfocus the current control)
	CGUIMessage msg(GUI_MSG_SETFOCUS, iCtrlID, nextControl, 0);
	OnMessage(msg);
	return true;
}

void CGUIWindow::SetDefaults()
{
	m_renderOrder = 0;
	m_saveLastControl = true;
	m_defaultControl = 0;
	m_bRelativeCoords = false;
	m_posX = m_posY = m_width = m_height = 0;
	m_overlayState = OVERLAY_STATE_PARENT_WINDOW;   // Use parent or previous window's state
	m_visibleCondition = 0;
	m_previousWindow = WINDOW_INVALID;
	m_animations.clear();
	m_origins.clear();
	m_hasCamera = false;
	m_animationsEnabled = true;
//	m_hitRect.SetRect(0, 0, (float)g_settings.m_ResInfo[m_coordsRes].iWidth, (float)g_settings.m_ResInfo[m_coordsRes].iHeight); //TODO
	m_clearBackground = 0xff000000; // opaque black -> clear
}

void CGUIWindow::SetProperty(const CStdString &key, const CStdString &value)
{
	m_mapProperties[key] = value;
}

void CGUIWindow::SetProperty(const CStdString &key, const char *value)
{
	m_mapProperties[key] = value;
}

void CGUIWindow::SetProperty(const CStdString &key, int value)
{
	CStdString strVal;
	strVal.Format("%d", value);
	SetProperty(key, strVal);
}

void CGUIWindow::SetProperty(const CStdString &key, bool value)
{
	SetProperty(key, value ? "1" : "0");
}

void CGUIWindow::SetProperty(const CStdString &key, double value)
{
	CStdString strVal;
	strVal.Format("%f", value);
	SetProperty(key, strVal);
}

CStdString CGUIWindow::GetProperty(const CStdString &key) const
{
	std::map<CStdString, CStdString, icompare>::const_iterator iter = m_mapProperties.find(key);

	if(iter == m_mapProperties.end())
		return "";

	return iter->second;
}

int CGUIWindow::GetPropertyInt(const CStdString &key) const
{
	return atoi(GetProperty(key).c_str());
}

bool CGUIWindow::GetPropertyBool(const CStdString &key) const
{
	return GetProperty(key) == "1";
}

double CGUIWindow::GetPropertyDouble(const CStdString &key) const
{
	return atof(GetProperty(key).c_str());
}

void CGUIWindow::ClearProperties()
{
	m_mapProperties.clear();
}

#ifdef _DEBUG
void CGUIWindow::DumpTextureUse()
{
	CLog::Log(LOGDEBUG, "%s for window %u", __FUNCTION__, GetID());
//	CGUIControlGroup::DumpTextureUse(); //TODO
}
#endif

void CGUIWindow::SaveControlStates()
{
	if (m_saveLastControl)
		m_lastControlID = GetFocusedControlID();
}

void CGUIWindow::RestoreControlStates()
{
	if (m_saveLastControl)
	{
		// set focus to our saved control
		int focusControl = m_lastControlID ? m_lastControlID : m_defaultControl;
		
		CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), focusControl, 0);
		OnMessage(msg);
	}
	else
	{
		// Set the default control focus
		CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_defaultControl, 0);
		OnMessage(msg);
	}
}

bool CGUIWindow::SendMessage(int message, int id, int param1 /* = 0*/, int param2 /* = 0*/)
{
	CGUIMessage msg(message, GetID(), id, param1, param2);
	return OnMessage(msg);
}

void CGUIWindow::RunUnloadActions()
{
	m_unloadActions.Execute(GetID(), GetParentID());
}