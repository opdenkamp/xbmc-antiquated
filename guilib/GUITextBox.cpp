#include "include.h"
#include "GUITextBox.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/StringUtils.h"
#include "GUILabelControl.h"
#include "../xbmc/utils/GUIInfoManager.h"

#define CONTROL_LIST  0
#define CONTROL_UPDOWN 9998
CGUITextBox::CGUITextBox(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                         float spinWidth, float spinHeight,
                         const CImage& textureUp, const CImage& textureDown,
                         const CImage& textureUpFocus, const CImage& textureDownFocus,
                         const CLabelInfo& spinInfo, float spinX, float spinY,
                         const CLabelInfo& labelInfo, int scrollTime)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_upDown(dwControlId, CONTROL_UPDOWN, 0, 0, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_INT)
{
  m_upDown.SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  m_offset = 0;
  m_scrollOffset = 0;
  m_label = labelInfo;
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  m_spinPosX = spinX;
  m_spinPosY = spinY;
  m_upDown.SetShowRange(true); // show the range by default
  ControlType = GUICONTROL_TEXTBOX;
  m_pageControl = 0;
  m_singleInfo = 0;
  m_renderTime = 0;
  m_lastRenderTime = 0;
  m_scrollTime = scrollTime;
  m_autoScrollCondition = 0;
  m_autoScrollTime = 0;
  m_autoScrollDelay = 3000;
  m_autoScrollDelayTime = 0;
}

CGUITextBox::~CGUITextBox(void)
{
}

void CGUITextBox::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
  // if not visible, we reset the autoscroll timer and positioning
  if (!IsVisible() && m_autoScrollTime)
  {
    m_autoScrollDelayTime = 0;
    m_lastRenderTime = 0;
    m_offset = 0;
    m_scrollOffset = 0;
    m_scrollSpeed = 0;
  }
}

void CGUITextBox::Render()
{
  CStdString renderLabel;
	if (m_singleInfo)
		renderLabel = g_infoManager.GetLabel(m_singleInfo, m_dwParentID);
	else
    renderLabel = g_infoManager.GetMultiInfo(m_multiInfo, m_dwParentID);

  // need to check the last one
  if (m_renderLabel != renderLabel)
  { // different, so reset to the top of the textbox and invalidate.  The page control will update itself below
    m_renderLabel = renderLabel;
    m_offset = 0;
    m_scrollOffset = 0;
    m_bInvalidated = true;
    m_autoScrollDelayTime = 0;
  }

  if (m_bInvalidated)
  { 
    // first correct any sizing we need to do
    float fWidth, fHeight;
    m_label.font->GetTextExtent( L"y", &fWidth, &fHeight);
    m_itemHeight = fHeight;
    float fTotalHeight = m_height;
    if (!m_pageControl)
      fTotalHeight -=  m_upDown.GetHeight() + 5;
    m_itemsPerPage = (unsigned int)(fTotalHeight / fHeight);

    // we have all the sizing correct so do any wordwrapping
    CStdStringW utf16Text;
    g_charsetConverter.utf8ToUTF16(m_renderLabel, utf16Text);
    CGUILabelControl::WrapText(utf16Text, m_label.font, m_width, m_lines);

    // disable all second label information
    m_lines2.clear();
    UpdatePageControl();
  }

  // update our auto-scrolling as necessary
  if (m_autoScrollTime && m_lines.size() > m_itemsPerPage)
  {
    if (!m_autoScrollCondition || g_infoManager.GetBool(m_autoScrollCondition, m_dwParentID))
    {
      if (m_lastRenderTime)
        m_autoScrollDelayTime += m_renderTime - m_lastRenderTime;
      if (m_autoScrollDelayTime > (unsigned int)m_autoScrollDelay && m_scrollSpeed == 0)
      { // delay is finished - start scrolling
        if (m_offset < (int)m_lines.size() - m_itemsPerPage)
          ScrollToOffset(m_offset + 1, true);
      }
    }
    else if (m_autoScrollCondition)
      m_autoScrollDelayTime = 0;    // conditional is false, so reset the scroll delay.
  }

  // update our scroll position as necessary
  if (m_lastRenderTime)
    m_scrollOffset += m_scrollSpeed * (m_renderTime - m_lastRenderTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_itemHeight) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_itemHeight))
  {
    m_scrollOffset = m_offset * m_itemHeight;
    m_scrollSpeed = 0;
  }
  m_lastRenderTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_itemHeight);

  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float posX = m_posX;
  float posY = m_posY + offset * m_itemHeight - m_scrollOffset;

  // alignment correction
  DWORD align = m_label.align;
  if (align & XBFONT_CENTER_X)
    posX += m_width * 0.5f;
  if (align & XBFONT_RIGHT)
    posX += m_width;

  if (m_label.font)
  {
    m_label.font->Begin();
    int current = offset;
    while (posY < m_posY + m_height && current < (int)m_lines.size())
    {
      float maxWidth = m_width;
      // render item
      if (current < (int)m_lines2.size())
      {
        float fTextWidth, fTextHeight;
        m_label.font->GetTextExtent( m_lines2[current].c_str(), &fTextWidth, &fTextHeight);
        maxWidth -= fTextWidth;
        m_label.font->DrawTextWidth(posX + maxWidth, posY + 2, m_label.textColor, m_label.shadowColor, m_lines2[current].c_str(), fTextWidth);
      }
      if (current == (int)m_lines.size() - 1)
        align &= ~XBFONT_JUSTIFIED; // last line shouldn't be justified
      m_label.font->DrawText(posX, posY + 2, m_label.textColor, m_label.shadowColor, m_lines[current].c_str(), align, maxWidth);
      posY += m_itemHeight;
      current++;
    }
    m_label.font->End();
  }

  g_graphicsContext.RestoreClipRegion();

  if (!m_pageControl)
  {
    m_upDown.SetPosition(m_posX + m_spinPosX, m_posY + m_spinPosY);
    m_upDown.Render();
  }
  else
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, offset);
    SendWindowMessage(msg);
  }
  CGUIControl::Render();
}

bool CGUITextBox::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    OnPageUp();
    m_autoScrollDelayTime = 0;
    return true;
    break;

  case ACTION_PAGE_DOWN:
    OnPageDown();
    m_autoScrollDelayTime = 0;
    return true;
    break;

  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
    return CGUIControl::OnAction(action);
    break;

  default:
    return m_upDown.OnAction(action);
  }
}

bool CGUITextBox::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId() == CONTROL_UPDOWN)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_autoScrollDelayTime = 0;
        if (m_upDown.GetValue() >= 1)
          ScrollToOffset((m_upDown.GetValue() - 1) * m_itemsPerPage);
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL_BIND)
    { // send parameter is a link to a vector of CGUIListItem's
      vector<CGUIListItem> *items = (vector<CGUIListItem> *)message.GetLPVOID();
      if (items)
      {
        m_offset = 0;
        m_scrollOffset = 0;
        m_autoScrollDelayTime = 0;
        m_lines.clear();
        m_lines2.clear();
        for (unsigned int i = 0; i < items->size(); ++i)
        {
          CStdStringW utf16Label;
          CGUIListItem &item = items->at(i);
          g_charsetConverter.utf8ToUTF16(item.GetLabel(), utf16Label);
          m_lines.push_back(utf16Label);
          g_charsetConverter.utf8ToUTF16(item.GetLabel2(), utf16Label);
          m_lines2.push_back(utf16Label);
        }
        UpdatePageControl();
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_offset = 0;
      m_scrollOffset = 0;
      m_autoScrollDelayTime = 0;
      m_lines.clear();
      m_lines2.clear();
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);

      SetLabel( message.GetLabel() );
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_offset = 0;
      m_scrollOffset = 0;
      m_autoScrollDelayTime = 0;
      m_lines.clear();
      m_lines2.clear();
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
        SendWindowMessage(msg);
      }
    }

    if (message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_upDown.SetFocus(true);
      m_autoScrollDelayTime = 0;
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS)
    {
      m_upDown.SetFocus(false);
    }

    if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page
        m_autoScrollDelayTime = 0;
        ScrollToOffset(message.GetParam1());
        return true;
      }
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUITextBox::PreAllocResources()
{
  if (!m_label.font) return;
  CGUIControl::PreAllocResources();
  m_upDown.PreAllocResources();
}

void CGUITextBox::AllocResources()
{
  if (!m_label.font) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();

  SetHeight(m_height);
}

void CGUITextBox::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
}

void CGUITextBox::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_upDown.DynamicResourceAlloc(bOnOff);
}

void CGUITextBox::OnRight()
{
  if (!m_pageControl && !m_upDown.IsFocusedOnUp())
    m_upDown.OnRight();
  else
    CGUIControl::OnRight();
}

void CGUITextBox::OnLeft()
{
  if (!m_pageControl && m_upDown.IsFocusedOnUp())
    m_upDown.OnLeft();
  else
    CGUIControl::OnLeft();
}

void CGUITextBox::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    ScrollToOffset((m_upDown.GetValue() - 1) * m_itemsPerPage);
  }
  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, m_offset);
    SendWindowMessage(msg);
  }
}

void CGUITextBox::OnPageDown()
{
  int iPages = m_lines.size() / m_itemsPerPage;
  if (m_lines.size() % m_itemsPerPage) iPages++;

  int iPage = m_upDown.GetValue();
  if (iPage + 1 <= iPages)
  {
    iPage++;
    m_upDown.SetValue(iPage);
    ScrollToOffset((m_upDown.GetValue() - 1) * m_itemsPerPage);
  }
  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, m_offset);
    SendWindowMessage(msg);
  }
}

void CGUITextBox::SetLabel(const string &strText)
{
  g_infoManager.ParseLabel(strText, m_multiInfo);
  m_bInvalidated = true;
}

void CGUITextBox::UpdatePageControl()
{
  // and update our page control
  int iPages = m_lines.size() / m_itemsPerPage;
  if (m_lines.size() % m_itemsPerPage || !iPages) iPages++;
  m_upDown.SetRange(1, iPages);
  m_upDown.SetValue(1);
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
    SendWindowMessage(msg);
  }
}

bool CGUITextBox::HitTest(const CPoint &point) const
{
  if (m_upDown.HitTest(point)) return true;
  return CGUIControl::HitTest(point);
}

bool CGUITextBox::CanFocus() const
{
  if (m_pageControl) return false;
  return CGUIControl::CanFocus();
}

bool CGUITextBox::OnMouseOver(const CPoint &point)
{
  if (m_upDown.HitTest(point))
    m_upDown.OnMouseOver(point);
  return CGUIControl::OnMouseOver(point);
}

bool CGUITextBox::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (m_upDown.HitTest(point))
    return m_upDown.OnMouseClick(dwButton, point);
  return false;
}

bool CGUITextBox::OnMouseWheel(char wheel, const CPoint &point)
{
  if (m_upDown.HitTest(point))
  {
    return m_upDown.OnMouseWheel(wheel, point);
  }
  else
  { // increase or decrease our offset by the appropriate amount.
    int offset = m_offset - wheel;
    // check that we are within the correct bounds.
    if (offset + m_itemsPerPage > (int)m_lines.size())
      offset = (m_lines.size() >= m_itemsPerPage) ? m_lines.size() - m_itemsPerPage : 0;
    ScrollToOffset(offset);
    // update the page control...
    int iPage = offset / m_itemsPerPage + 1;
    // last page??
    if (offset + m_itemsPerPage == m_lines.size())
      iPage = m_upDown.GetMaximum();
    m_upDown.SetValue(iPage);
    m_autoScrollDelayTime = 0;
  }
  return true;
}

void CGUITextBox::SetPosition(float posX, float posY)
{
  // offset our spin control by the appropriate amount
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition();
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition();
  CGUIControl::SetPosition(posX, posY);
  m_upDown.SetPosition(GetXPosition() + spinOffsetX, GetYPosition() + spinOffsetY);
}

void CGUITextBox::SetWidth(float width)
{
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition() - GetWidth();
  CGUIControl::SetWidth(width);
  m_upDown.SetPosition(GetXPosition() + GetWidth() + spinOffsetX, m_upDown.GetYPosition());
}

void CGUITextBox::SetHeight(float height)
{
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition() - GetHeight();
  CGUIControl::SetHeight(height);
  m_upDown.SetPosition(m_upDown.GetXPosition(), GetYPosition() + GetHeight() + spinOffsetY);
}

void CGUITextBox::SetPulseOnSelect(bool pulse)
{
  m_upDown.SetPulseOnSelect(pulse);
  CGUIControl::SetPulseOnSelect(pulse);
}

void CGUITextBox::SetNavigation(DWORD up, DWORD down, DWORD left, DWORD right)
{
  CGUIControl::SetNavigation(up, down, left, right);
  m_upDown.SetNavigation(up, down, left, right);
}

void CGUITextBox::SetPageControl(DWORD pageControl)
{
  m_pageControl = pageControl;
}

void CGUITextBox::SetInfo(int singleInfo)
{
  m_singleInfo = singleInfo;
}

void CGUITextBox::SetColorDiffuse(D3DCOLOR color)
{
  CGUIControl::SetColorDiffuse(color);
  m_upDown.SetColorDiffuse(color);
}

void CGUITextBox::ScrollToOffset(int offset, bool autoScroll)
{
  float size = m_itemHeight;
  m_scrollOffset = m_offset * m_itemHeight;
  int timeToScroll = autoScroll ? m_autoScrollTime : m_scrollTime;
  m_scrollSpeed = (offset * m_itemHeight - m_scrollOffset) / timeToScroll;
  m_offset = offset;
}

void CGUITextBox::SetAutoScrolling(const TiXmlNode *node)
{
  if (!node) return;
  const TiXmlElement *scroll = node->FirstChildElement("autoscroll");
  if (scroll)
  {
    scroll->Attribute("delay", &m_autoScrollDelay);
    scroll->Attribute("time", &m_autoScrollTime);
    if (scroll->FirstChild())
      m_autoScrollCondition = g_infoManager.TranslateString(scroll->FirstChild()->ValueStr());
  }
}