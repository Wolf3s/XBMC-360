#define NOMINMAX
#include "include.h"
#include "GUISpinControlEx.h"

CGUISpinControlEx::CGUISpinControlEx(int parentID, int controlID, float posX, float posY, float width, float height, float spinWidth, float spinHeight, const CLabelInfo& spinInfo, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus, const CTextureInfo& textureUp, const CTextureInfo& textureDown, const CTextureInfo& textureUpFocus, const CTextureInfo& textureDownFocus, const CLabelInfo& labelInfo, int iType)
	: CGUISpinControl(parentID, controlID, posX, posY, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, iType)
	, m_buttonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
{
	ControlType = GUICONTROL_SPINEX;
	m_spinPosX = 0;
}

CGUISpinControlEx::~CGUISpinControlEx(void)
{
}

void CGUISpinControlEx::PreAllocResources()
{
	CGUISpinControl::PreAllocResources();
	m_buttonControl.PreAllocResources();
}

void CGUISpinControlEx::AllocResources()
{
	// Correct alignment - We always align the spincontrol on the right
	m_label.GetLabelInfo().align = (m_label.GetLabelInfo().align & XBFONT_CENTER_Y) | XBFONT_RIGHT;
	CGUISpinControl::AllocResources();
	m_buttonControl.AllocResources();
	
	if (m_height == 0)
		m_height = GetSpinHeight();
}

void CGUISpinControlEx::FreeResources(bool immediately)
{
	CGUISpinControl::FreeResources(immediately);
	m_buttonControl.FreeResources(immediately);
}

void CGUISpinControlEx::DynamicResourceAlloc(bool bOnOff)
{
	CGUISpinControl::DynamicResourceAlloc(bOnOff);
	m_buttonControl.DynamicResourceAlloc(bOnOff);
}

void CGUISpinControlEx::Render()
{
	// Make sure the button has focus if it should have...
	m_buttonControl.SetFocus(HasFocus());
	m_buttonControl.SetPulseOnSelect(m_pulseOnSelect);
	m_buttonControl.SetEnabled(m_enabled);
	m_buttonControl.Render();
	
	if (m_bInvalidated)
		SetPosition(GetXPosition(), GetYPosition());

	CGUISpinControl::Render();
}

void CGUISpinControlEx::SetPosition(float posX, float posY)
{
	m_buttonControl.SetPosition(posX, posY);
	float spinPosX = posX + m_buttonControl.GetWidth() - GetSpinWidth() * 2 - (m_spinPosX ? m_spinPosX : m_buttonControl.GetLabelInfo().offsetX);
	float spinPosY = posY + (m_buttonControl.GetHeight() - GetSpinHeight()) * 0.5f;
	CGUISpinControl::SetPosition(spinPosX, spinPosY);
}

void CGUISpinControlEx::SetWidth(float width)
{
	m_buttonControl.SetWidth(width);
	SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::SetHeight(float height)
{
	m_buttonControl.SetHeight(height);
	SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::SetVisible(bool bVisible)
{
	m_buttonControl.SetVisible(bVisible);
	CGUISpinControl::SetVisible(bVisible);
}

void CGUISpinControlEx::UpdateColors()
{
	CGUISpinControl::UpdateColors();
	m_buttonControl.SetColorDiffuse(m_diffuseColor);
	m_buttonControl.UpdateColors();
}

void CGUISpinControlEx::SetEnabled(bool bEnable)
{
	m_buttonControl.SetEnabled(bEnable);
	CGUISpinControl::SetEnabled(bEnable);
}

const CStdString CGUISpinControlEx::GetCurrentLabel() const
{
	return CGUISpinControl::GetLabel();
}

CStdString CGUISpinControlEx::GetDescription() const
{
	CStdString strLabel;
	strLabel.Format("%s (%s)", m_buttonControl.GetDescription(), GetLabel());
	return strLabel;
}

void CGUISpinControlEx::SettingsCategorySetSpinTextColor(const CGUIInfoColor &color)
{
	m_label.GetLabelInfo().textColor = color;
	m_label.GetLabelInfo().focusedColor = color;
}

void CGUISpinControlEx::SetSpinPosition(float spinPosX)
{
	m_spinPosX = spinPosX;
	SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::RenderText(float posX, float width)
{
	const float spaceWidth = 10;
	
	// Check our limits from the button control
	float x = std::max(m_buttonControl.m_label.GetRenderRect().x2 + spaceWidth, posX);
	m_label.SetScrolling(HasFocus());

	CGUISpinControl::RenderText(x, width + posX - x);
}