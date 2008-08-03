/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "include.h"
#include "GUIEPGGridContainer.h"
#include "GUIControlFactory.h"
#include "GUIListItem.h"
#include "GUIFontManager.h"

#define SHORTGAP     5
#define MINSPERBLOCK 5 /// would be nice to offer zooming of busy schedules

CGUIEPGGridContainer::CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, int scrollTime, int timeBlocks)
: CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  ControlType = GUICONTAINER_EPGGRID;
  m_blocksPerPage = timeBlocks;
  m_blockSize = 0;
  m_channelCursor = 0;
  m_blockCursor = 0;
  m_channelOffset = 0;
  m_blockOffset = 0;
  m_vertScrollOffset = 0;
  m_vertScrollSpeed = 0;
  m_vertScrollLastTime = 0;
  m_horzScrollOffset = 0;
  m_horzScrollSpeed = 0;
  m_horzScrollLastTime = 0;
  m_scrollTime = scrollTime ? scrollTime : 1;
  m_renderTime = 0;
  m_item.reset();
  m_lastItem.reset();
  m_lastChannel.reset();
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_channelWrapAround = false;
}

CGUIEPGGridContainer::~CGUIEPGGridContainer(void)
{
  Reset();
}

void CGUIEPGGridContainer::RenderItem(float posX, float posY, CGUIListItemPtr item, bool focused)
{
  if (!m_focusedLayout || !m_layout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_focusedLayout);
      item->SetFocusedLayout(layout);
    }
    if (item->GetFocusedLayout())
    {
      if (item != m_lastItem || !HasFocus())
      {
        item->GetFocusedLayout()->SetFocusedItem(0);
      }
      if (item != m_lastItem && HasFocus())
      {
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);      
        unsigned int subItem = 1;
        if (m_lastItem && m_lastItem->GetFocusedLayout())
          subItem = m_lastItem->GetFocusedLayout()->GetFocusedItem();
        item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
      }
      item->GetFocusedLayout()->Render(item.get(), m_dwParentID, m_renderTime);
    }
    m_lastItem = item;
  }
  else
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);  // focus is not set
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_layout);
      item->SetLayout(layout);
    }
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Render(item.get(), m_dwParentID, m_renderTime);
    else if (item->GetLayout())
      item->GetLayout()->Render(item.get(), m_dwParentID, m_renderTime);
  }
  g_graphicsContext.RestoreOrigin();
}

void CGUIEPGGridContainer::RenderChannel(float posX, float posY, CGUIListItemPtr item, bool focused)
{
  if (!m_focusedChannelLayout || !m_channelLayout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_focusedChannelLayout);
      item->SetFocusedLayout(layout);
    }
    if (item->GetFocusedLayout())
    {
      if (item != m_lastChannel || !HasFocus())
      {
        item->GetFocusedLayout()->SetFocusedItem(0);
      }
      if (item != m_lastChannel && HasFocus())
      {
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);      
        unsigned int subItem = 1;
        if (m_lastChannel && m_lastChannel->GetFocusedLayout())
          subItem = m_lastChannel->GetFocusedLayout()->GetFocusedItem();
        item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
      }
      item->GetFocusedLayout()->Render(item.get(), m_dwParentID, m_renderTime);
    }
    m_lastChannel = item;
  }
  else
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);  // focus is not set
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_channelLayout);
      item->SetLayout(layout);
    }
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Render(item.get(), m_dwParentID, m_renderTime);
    else if (item->GetLayout())
      item->GetLayout()->Render(item.get(), m_dwParentID, m_renderTime);
  }
  g_graphicsContext.RestoreOrigin();
}

void CGUIEPGGridContainer::Render()
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout || !m_focusedChannelLayout || !m_channelLayout)
    return;

  m_vertScrollOffset += m_vertScrollSpeed * (m_renderTime - m_vertScrollLastTime);
  if ((m_vertScrollSpeed < 0 && m_vertScrollOffset < m_channelOffset * m_layout->Size(VERTICAL)) ||
    (m_vertScrollSpeed > 0 && m_vertScrollOffset > m_channelOffset * m_layout->Size(VERTICAL)))
  {
    m_vertScrollOffset = m_channelOffset * m_layout->Size(VERTICAL);
    m_vertScrollSpeed = 0;
  }
  m_vertScrollLastTime = m_renderTime;

  m_horzScrollOffset += m_horzScrollSpeed * (m_renderTime - m_horzScrollLastTime);
  if ((m_horzScrollSpeed < 0 && m_horzScrollOffset < m_blockOffset * m_blockSize) ||
    (m_horzScrollSpeed > 0 && m_horzScrollOffset > m_blockOffset * m_blockSize))
  {
    m_horzScrollOffset = m_blockOffset * m_blockSize;
    m_horzScrollSpeed = 0;
  }
  m_horzScrollLastTime = m_renderTime;
  
  int chanOffset = (int)(m_vertScrollOffset / m_layout->Size(VERTICAL));
  int blockOffset = (int)(m_horzScrollOffset / m_blockSize);



  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float vertDrawOffset = chanOffset * m_channelHeight - m_vertScrollOffset;
  float horzDrawOffset = blockOffset * m_blockSize - m_horzScrollOffset;

  float posY = m_posY;
  posY += vertDrawOffset;

  RenderItems(horzDrawOffset, posY, chanOffset, blockOffset);

  RenderChannels(posY, chanOffset);

  CGUIControl::Render();

  RenderDebug();
}
void CGUIEPGGridContainer::RenderItems(float horzDrawOffset, float posY, int chanOffset, int blockOffset)
{
  g_graphicsContext.SetClipRegion(m_gridPosX, m_posY, m_gridWidth, m_height);

  int channel = chanOffset;
  float focusedPosX = 0;
  float focusedPosY = 0;

  CGUIListItemPtr focusedItem;

  while (posY < m_posY + m_height && m_gridItems.size()) // FOR EACH ROW ///////////////
  {
    if (channel >= (int)m_gridItems.size())
      break;

    int block = blockOffset;
    float posX = m_gridPosX + horzDrawOffset;

    CGUIListItemPtr item;
    item = m_gridIndex[channel][block]; 
    if (item == m_gridIndex[channel][blockOffset-1] && blockOffset != 0)
    {
      /* first program starts before current view */
      int startBlock = blockOffset - 1;
      while (m_gridIndex[channel][startBlock] == item)
        startBlock--;

      block = startBlock + 1;
      int missingSection = blockOffset - block;
      posX -= missingSection * m_blockSize;
    }

    while (posX < m_gridPosX + m_gridWidth && m_gridItems[channel].size()) // FOR EACH ITEM ///////////////
    {
      item = m_gridIndex[channel][block];  

      bool focused = (channel == m_channelOffset + m_channelCursor) && (item == m_gridIndex[m_channelOffset + m_channelCursor][m_blockOffset + m_blockCursor]);
      // render our item
      if (focused)
      {
        focusedPosX = posX;
        focusedPosY = posY;
        focusedItem = item;
      }
      else
        RenderItem(posX, posY, item, focused);

      // increment our X position
      block += (int)(item->GetLayout()->Size(HORIZONTAL) / m_blockSize);
      posX += item->GetLayout()->Size(HORIZONTAL); // assumes focused & unfocused layouts have equal length
    }
    // increment our Y position
    channel++;
    posY += m_channelHeight;
  }

  // and render the focused item last (for overlapping purposes)
  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem, true);

  g_graphicsContext.RestoreClipRegion();
}
void CGUIEPGGridContainer::RenderChannels(float posY, int chanOffset)
{
  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_channelWidth, m_height);

  int channel = chanOffset;
  float focusedChannelPosY = 0;
  CGUIListItemPtr focusedChannel;
  CGUIListItemPtr item;

  while (posY < m_posY + m_height && m_channelItems.size())
  {
    if (channel >= (int)m_channelItems.size())
      break;

    item = m_channelItems[channel];
    bool focused = (channel == m_channelOffset + m_channelCursor);
    if (focused)
    {
      focusedChannelPosY = posY;
      focusedChannel = item;
    }
    else
      RenderChannel(m_posX, posY, item, focused);
    
    // increment our Y position
    channel++;
    posY += m_channelHeight;
  }

  /* finally, render the currently focused channel last*/
  if (focusedChannel)
    RenderChannel(m_posX, focusedChannelPosY, focusedChannel, true);

  g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::RenderDebug()
{
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  g_graphicsContext.SetScalingResolution(res, 0, 0, false);

  CStdStringW wszText;

  CStdStringW label = "";
  if (m_lastChannel)
    label = m_lastChannel->GetLabel();
  
  wszText.Format(L"Block: %u, BlockOffset: %u, Channel: %u, ChannelOffset: %u, ItemLabel: %s", m_blockCursor, m_blockOffset, m_channelCursor, m_channelOffset, label);

  float x = 0.05f * g_graphicsContext.GetWidth();
  float y = 0.95f * g_graphicsContext.GetHeight();
  CGUITextLayout::DrawOutlineText(g_fontManager.GetFont("font13"), x, y, 0xffffffff, 0xff000000, 2, wszText);
}

void CGUIEPGGridContainer::UpdateChannels(VECFILEITEMS &channels)
{
  UpdateLayout();
  if (!m_channelItems.empty())
    m_channelItems.clear();

  VECFILEITEMS::iterator channel = channels.begin();
  for ( ; channel != channels.end(); channel++)
  {
    CGUIListItemPtr item = *channel;
    long test = item.use_count();
    item->SetLayout(m_channelLayout);
    item->SetFocusedLayout(m_focusedChannelLayout);
    item->SetInvalid();
    m_channelItems.push_back(item);
  }
}

void CGUIEPGGridContainer::UpdateItems(EPGGrid &gridData, const CDateTime &start, const CDateTime &end)
{
  if (!m_gridItems.empty())
    m_gridItems.clear();

  float posX = m_posX + m_channelWidth;
  float posY = m_posY;

  CDateTime gridStart, gridEnd;
  CDateTimeSpan gridDuration, blockDuration, pageDuration;

  gridDuration = end - start;
  gridStart = start;
  pageDuration.SetDateTimeSpan(0, 0, MINSPERBLOCK*m_blocksPerPage, 0);
  gridEnd = start + pageDuration;
  m_gridStart = gridStart;
  m_gridEnd = gridEnd;

  m_blocks = (gridDuration.GetDays()*24*60 + gridDuration.GetHours()*60 + gridDuration.GetMinutes()) / MINSPERBLOCK;
  blockDuration.SetDateTimeSpan(0, 0, MINSPERBLOCK, 0);

  DWORD tick(timeGetTime());
  iEPGRow itY = gridData.begin();
  
  /** FOR EACH CHANNEL **********************************************************************/
  for (int row = 0 ; itY != gridData.end(); itY++, row++)
  {
    CDateTime gridCursor = gridStart; //reset cursor for new channel
    iEPGItem program = itY->begin();

    /** FOR EACH BLOCK **********************************************************************/
    for (int block = 0; block < m_blocks; block++)
    {
      while (program != itY->end() && (*program)->GetEPGInfoTag()->m_endTime < gridCursor)
        program++;

      // we are either at the end of the programs, or have a program ending after this time
      if (program == itY->end())
      {
        m_gridIndex[row][block].reset(); // no program here
      }
      else
      {
        // we have a program ending after this time, so check whether it starts before this
        if ((*program)->GetEPGInfoTag()->m_startTime <= gridCursor)
        {
          m_gridIndex[row][block] = *program;
        }
        else
        {
          m_gridIndex[row][block].reset();
        }
      }
      gridCursor += blockDuration;
    }
    
    /** FOR EACH BLOCK **********************************************************************/
    int itemSize = 1; // size of the programme in blocks
    std::vector< CGUIListItemPtr > items; // this channel's items
    for (int block = 0; block < m_blocks; block++)
    {
      if (m_gridIndex[row][block] != m_gridIndex[row][block+1])
      {
        GenerateItemLayout(row, itemSize, block);
        items.push_back(m_gridIndex[row][block]);
        itemSize = 1;
      }
      else
      {
        itemSize++;
      }
    }

    m_gridItems.push_back(items);  // store this channel's gridItems
  }
  /******************************************* END ******************************************/

  if (m_gridItems.size() < 1)
  {
    CLog::Log(LOGDEBUG, "%s No data found", __FUNCTION__);
    return; 
  }

  CLog::Log(LOGDEBUG, "%s completed successfully in %u ms", __FUNCTION__, timeGetTime()-tick);

  m_channels = (int)m_gridItems.size();
  m_item = GetItem(m_channelCursor);
  m_blockCursor = GetBlock(m_item, m_channelCursor);

  SetInvalid(); 
}

bool CGUIEPGGridContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
    { // use base class implementation

      return CGUIControl::OnAction(action);
    }
    break;

  default:
    if (action.wID)
    { 
      return true /*OnClick(action.wID)*/;
    }
  }
  return false;
}

bool CGUIEPGGridContainer::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUIEPGGridContainer::OnUp()
{
  if (m_channelCursor > 0)
  {
    SetChannel(m_channelCursor - 1);
  }
  else if (m_channelCursor == 0 && m_channelOffset)
  {
    ScrollToChannelOffset(m_channelOffset - 1);
    SetChannel(0);
  }
  else if (m_channelWrapAround)
  {
    int offset = m_channels - m_channelsPerPage;
    if (offset < 0) offset = 0;
    SetChannel(m_channels - offset - 1);
    ScrollToChannelOffset(offset);
  }
  else
    CGUIControl::OnUp();
}

void CGUIEPGGridContainer::OnDown()
{
  if (m_channelOffset + m_channelCursor + 1 < m_channels)
  {
    if (m_channelCursor + 1 < m_channelsPerPage)
    {
      SetChannel(m_channelCursor + 1);
    }
    else
    {
      ScrollToChannelOffset(m_channelOffset + 1);
      SetChannel(m_channelsPerPage - 1);
    }
  }
  else if(m_channelWrapAround)
  {
    SetChannel(0);
    ScrollToChannelOffset(0);
  }
  else
    CGUIControl::OnDown();
}

void CGUIEPGGridContainer::OnLeft()
{
  if (m_item != m_gridIndex[m_channelCursor + m_channelOffset][m_blockOffset])
  {
    // this is not first item on page
    m_item = GetPrevItem(m_channelCursor);
    m_blockCursor = GetBlock(m_item, m_channelCursor);
  }
  else if (m_blockCursor == 0 && m_blockOffset)
  {
    // we're at the left edge and offset
    int itemSize = GetItemSize(m_item);
    int block = GetRealBlock(m_item, m_channelCursor);

    if (block < m_blockOffset) /* current item begins before current offset, keep selected */
    {
      if (itemSize > m_blocksPerPage) /* current item is longer than one page, scroll one page left */
      {
        m_blockOffset < m_blocksPerPage ? block = 0 : block = m_blockOffset - m_blocksPerPage; // number blocks left < m_blocksPerPAge
        ScrollToBlockOffset(block);
        SetBlock(0);
      }
      else /* current item is shorter than one page, scroll left to start of item */
      {
        ScrollToBlockOffset(block); // -1?
        SetBlock(0); // align cursor to left edge
      }
    }
    else /* current item starts on this page's edge, select the previous item */
    {
      m_item = GetPrevItem(m_channelCursor);
      itemSize = GetItemSize(m_item);
      int block = GetRealBlock(m_item, m_channelCursor);
      if (itemSize > m_blocksPerPage) // previous item is longer than one page, scroll left to last page of item */
      {
        ScrollToBlockOffset(m_blockOffset - m_blocksPerPage); // left one whole page
        //SetBlock(m_blocksPerPage -1 ); // helps navigation by setting cursor to far right edge
        SetBlock(0); // align cursor to left edge
      }
      else /* previous item is shorter than one page, scroll left to start of item */
      {
        ScrollToBlockOffset(m_blockOffset - itemSize);
        SetBlock(0); //should be zero
      }
    }
  }
  /*else if (m_channelWrapAround) ///
  {
    int offset = m_blocks - m_blocksPerPage;
    if (offset < 0)
      offset = 0;
    ScrollToBlockOffset(offset);
  }*/
  else
    CGUIControl::OnLeft();
  /* call CGUIWindowEPG::OnLeft(); to load previous range of dates*/
}

void CGUIEPGGridContainer::OnRight()
{
  if ( m_item != m_gridIndex[m_channelCursor + m_channelOffset][m_blocksPerPage + m_blockOffset - 1] )
  {
    // this is not last item on page
    m_item = GetNextItem(m_channelCursor);
    m_blockCursor = GetBlock(m_item, m_channelCursor);
  }
  else if ((m_blockOffset != m_blocks - m_blocksPerPage) && m_blocks > m_blocksPerPage)
  {
    // at right edge, more than one page and not at maximum offset
    int itemSize = GetItemSize(m_item);
    int block = GetRealBlock(m_item, m_channelCursor);

    if (itemSize > m_blocksPerPage - m_blockCursor) // current item extends into next page, keep selected
    {
      if (itemSize > m_blocksPerPage) // current item is longer than one page, scroll one page right
      {
        if (m_blockOffset && m_blockOffset + m_blocksPerPage > m_blocks)
          block = m_blocks - m_blocksPerPage;
        else
          block = m_blockOffset + m_blocksPerPage;
        ScrollToBlockOffset(block);
        SetBlock(0);
      }
      else // current item is shorter than one page, scroll so end of item sits on end of grid
      {
        ScrollToBlockOffset(block + itemSize - m_blocksPerPage);
        SetBlock(GetBlock(m_item, m_channelCursor)); /// change to middle block of item?
      }
    }
    else // current item finishes on this page's edge, select the next item
    {
      m_item = GetNextItem(m_channelCursor);
      itemSize = GetItemSize(m_item);
      if (itemSize > m_blocksPerPage) // next item is longer than one page, scroll to first page of this item
      {
        ScrollToBlockOffset(m_blockOffset + m_blocksPerPage);
        SetBlock(0);
      }
      else // next item is shorter than one page, scroll so end of item sits on end of grid
      {
        ScrollToBlockOffset(m_blockOffset + itemSize);
        SetBlock(m_blocksPerPage - itemSize); /// change to middle block of item?
      }
    }
  }
  else
    CGUIControl::OnRight();
  /// call parent handler CGUIWindowEPG::OnRight(); to load next range of dates
}

void CGUIEPGGridContainer::SetChannel(int channel)
{
  if (m_blockCursor + m_blockOffset == 0 || m_blockOffset + m_blockCursor + GetItemSize(m_item) == m_blocks)
  {
    m_item = GetItem(channel);
    m_blockCursor = GetBlock(m_item, channel);
    m_channelCursor = channel;
    return;
  }

  /* basic checks failed, need to correctly identify nearest item */
  m_item = GetClosestItem(channel);
  m_channelCursor = channel;
  m_blockCursor = GetBlock(m_item, m_channelCursor);
}

void CGUIEPGGridContainer::SetBlock(int block)
{
  m_blockCursor = block; 
  m_item = GetItem(m_channelCursor);
}

int CGUIEPGGridContainer::GetSelectedItem() const
{
  return 0;
}

CGUIListItemPtr CGUIEPGGridContainer::GetClosestItem(const int &channel)
{
  CGUIListItemPtr closest = GetItem(channel);
  int block = GetBlock(closest, channel);
  int left;   // num blocks to start of previous item
  int right;  // num blocks to start of next item

  if (block == m_blockCursor)
    return closest; // item & m_item start together

  if (block + GetItemSize(closest) == m_blockCursor + GetItemSize(m_item))
    return closest; // closest item ends when current does

  if (block > m_blockCursor)  // item starts after m_item 
  {
    left = m_blockCursor - GetBlock(closest, channel); 
    right = block - m_blockCursor;
  }
  else
  {
    left  = m_blockCursor - block;
    right = GetBlock(GetNextItem(channel), channel) - m_blockCursor;
  }


  if (right <= SHORTGAP && right <= left && m_blockCursor + right < m_blocksPerPage) 
    return m_gridIndex[channel + m_channelOffset][m_blockCursor + right + m_blockOffset];
  else
    return m_gridIndex[channel + m_channelOffset][m_blockCursor - left  + m_blockOffset];
}


int CGUIEPGGridContainer::GetItemSize(CGUIListItemPtr item)
{
  return (int)(item->GetLayout()->Size(HORIZONTAL) / m_blockSize);
}

///*************** could store this value as a CGUIListItem property **********************/
int CGUIEPGGridContainer::GetBlock(const CGUIListItemPtr &item, const int &channel)
{
  int block = 0;
  while (m_gridIndex[channel + m_channelOffset][block + m_blockOffset] != item && block + m_blockOffset < m_blocks)
    block++;
  return block;
}

int CGUIEPGGridContainer::GetRealBlock(const CGUIListItemPtr &item, const int &channel)
{
  int block = 0;
  while (m_gridIndex[channel + m_channelOffset][block] != item && block < m_blocks)
    block++;
  return block;
}
/******************************************************************************************/

void CGUIEPGGridContainer::GenerateItemLayout(int row, int itemSize, int block)
{
  CGUIListItemLayout *pItemLayout = new CGUIListItemLayout(*m_layout);
  CGUIListItemLayout *pItemFocusedLayout = new CGUIListItemLayout(*m_focusedLayout);
  pItemLayout->SetWidth(itemSize*m_blockSize);
  pItemFocusedLayout->SetWidth(itemSize*m_blockSize);

  if (!m_gridIndex[row][block])
  {
   CFileItemPtr unknown(new CFileItem("Unknown"));
   for (int i = block ; i > block - itemSize; i--)
     m_gridIndex[row][i] = unknown;
  }
  
  m_gridIndex[row][block]->SetFocusedLayout(pItemFocusedLayout);
  m_gridIndex[row][block]->SetLayout(pItemLayout);
  //m_lastItem = m_gridIndex[row][block];
  //m_lastChannel = m_channelItems[row];
}

/// store numerical position in m_gridData[channel] as a property of CGUIListItem
CGUIListItemPtr CGUIEPGGridContainer::GetNextItem(const int &channel)
{
  int i = m_blockCursor;
  while (m_gridIndex[channel + m_channelOffset][i + m_blockOffset] == m_gridIndex[channel + m_channelOffset][m_blockCursor + m_blockOffset] && i < m_blocksPerPage)
    i++;
  return m_gridIndex[channel + m_channelOffset][i + m_blockOffset];
}

CGUIListItemPtr CGUIEPGGridContainer::GetPrevItem(const int &channel)
{
  return m_gridIndex[channel + m_channelOffset][m_blockCursor + m_blockOffset - 1];
}

CGUIListItemPtr CGUIEPGGridContainer::GetItem(const int &channel)
{
  return m_gridIndex[channel + m_channelOffset][m_blockCursor + m_blockOffset];
}

void CGUIEPGGridContainer::SetFocus(bool bOnOff)
{
  if (bOnOff != HasFocus())
  {
    SetInvalid();
    /*m_lastItem.reset();
    m_lastChannel.reset();*/
  }
  CGUIControl::SetFocus(bOnOff);
}


void CGUIEPGGridContainer::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
  m_wasReset = false;
}

void CGUIEPGGridContainer::ScrollToChannelOffset(int offset)
{
  float size = m_layout->Size(VERTICAL);
  int range = m_channelsPerPage / 4;
  if (range <= 0) range = 1;
  if (offset * size < m_vertScrollOffset &&  m_vertScrollOffset - offset * size > size * range)
  { // scrolling up, and we're jumping more than 0.5 of a screen
    m_vertScrollOffset = (offset + range) * size;
  }
  if (offset * size > m_vertScrollOffset && offset * size - m_vertScrollOffset > size * range)
  { // scrolling down, and we're jumping more than 0.5 of a screen
    m_vertScrollOffset = (offset - range) * size;
  }
  m_vertScrollSpeed = (offset * size - m_vertScrollOffset) / m_scrollTime;
  m_channelOffset = offset;
}

void CGUIEPGGridContainer::ScrollToBlockOffset(int offset)
{
  float size = m_blockSize;
  int range = m_blocksPerPage / 1;
  if (range <= 0) range = 1;
  if (offset * size < m_horzScrollOffset &&  m_horzScrollOffset - offset * size > size * range)
  { // scrolling left, and we're jumping more than 0.5 of a screen
    m_horzScrollOffset = (offset + range) * size;
  }
  if (offset * size > m_horzScrollOffset && offset * size - m_horzScrollOffset > size * range)
  { // scrolling right, and we're jumping more than 0.5 of a screen
    m_horzScrollOffset = (offset - range) * size;
  }
  m_horzScrollSpeed = (offset * size - m_horzScrollOffset) / m_scrollTime;
  m_blockOffset = offset;
}

void CGUIEPGGridContainer::ValidateOffset()
{
  if (!m_layout) 
    return;
  if (m_channelOffset > m_channels - m_channelsPerPage)
  {
    m_channelOffset = m_channels - m_channelsPerPage;
    m_vertScrollOffset = m_channelOffset * m_layout->Size(VERTICAL);
  }
  if (m_channelOffset < 0)
  {
    m_channelOffset = 0;
    m_vertScrollOffset = 0;
  }
  if (m_blockOffset > m_blocks - m_blocksPerPage)
  {
    m_blockOffset = m_blocks - m_blocksPerPage;
    m_horzScrollOffset = m_blockOffset * m_blockSize;
  }
  if (m_blockOffset < 0)
  {
    m_blockOffset = 0;
    m_horzScrollOffset = 0;
  }
}
void CGUIEPGGridContainer::LoadLayout(TiXmlElement *layout)
{
  /* layouts for the channel column */
  TiXmlElement *itemElement = layout->FirstChildElement("channellayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, false);
    m_channelLayout = new CGUIListItemLayout(itemLayout);
    itemElement = itemElement->NextSiblingElement("channellayout");
  }
  itemElement = layout->FirstChildElement("focusedchannellayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, true);
    m_focusedChannelLayout = new CGUIListItemLayout(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedchannellayout");
  }

  /* layouts for the grid items */
  itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, false);
    m_layout = new CGUIListItemLayout(itemLayout);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, true);
    m_focusedLayout = new CGUIListItemLayout(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
}

void CGUIEPGGridContainer::UpdateLayout(bool updateAllItems)
{
  // if container is invalid, either new data has arrived, or m_blockSize has changed
  //  need to run UpdateItems rather than CalculateLayout?
  if (updateAllItems)
  { // free memory of items
    for (iChannels itC = m_gridItems.begin(); itC != m_gridItems.end(); itC++)
    {
      for (iShows itS = itC->begin(); itS != itC->end(); itS++)
      {
        (*itS)->FreeMemory();
      }
    }
  }
  // and recalculate the layout
  CalculateLayout();
}

void CGUIEPGGridContainer::CalculateLayout()
{
  assert(m_focusedLayout && m_layout && m_focusedChannelLayout && m_channelLayout);

  if (!m_focusedLayout || !m_layout || !m_focusedChannelLayout || !m_channelLayout)
  {
    UpdateLayout(true);
    return;
  }

  m_channelHeight = m_channelLayout->Size(VERTICAL);
  m_channelWidth = m_channelLayout->Size(HORIZONTAL);
  m_gridPosX = m_posX + m_channelWidth;
  m_gridWidth = m_width - m_channelWidth;
  m_channelsPerPage = (int)((m_height - m_channelHeight) / m_channelHeight) + 1;
  m_blockSize = m_gridWidth / m_blocksPerPage;

  // ensure that the scroll offsets are a multiple of our sizes
  m_vertScrollOffset = m_channelOffset * m_layout->Size(VERTICAL);
  m_horzScrollOffset = m_blockOffset * m_blockSize;
}

CStdString CGUIEPGGridContainer::GetDescription() const
{
  CStdString strLabel;
  unsigned item = GetSelectedItem();
  //if (item >= 0 && item < GetNumItems())
  //{
  //  /*CGUIListItem pItem = m_gridItems[0][item];*/
  //  //strLabel = pItem.GetLabel(); // get ptr
  //}
  return strLabel;
}

void CGUIEPGGridContainer::Reset()
{
  m_wasReset = true;
  for (iChannels itC = m_gridItems.begin(); itC != m_gridItems.end(); itC++)
  {
    for (iShows itS = itC->begin(); itS != itC->end(); itS++)
    {
      (*itS)->FreeMemory();
    }
  }
  m_lastItem.reset();
  m_lastChannel.reset();
}
