#pragma once

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

#include "GUIDialog.h"

#include <queue>

#define TOAST_DISPLAY_TIME   5000L  // default 5 seconds
#define TOAST_MESSAGE_TIME   1000L  // minimal message time 1 second

class CGUIDialogKaiToast: public CGUIDialog
{
public:
  CGUIDialogKaiToast(void);
  virtual ~CGUIDialogKaiToast(void);

  struct Notification
  {
    CStdString caption;
    CStdString description;
    CStdString imagefile;
    unsigned int displayTime;
    bool withSound;
  };

  enum eMessageType { mtStatus = 0, mtInfo, mtWarning, mtError };

  void QueueNotification(eMessageType eType, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME, bool withSound = true);
  void QueueNotification(const CStdString& aCaption, const CStdString& aDescription);
  void QueueNotification(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME, bool withSound = true);
  bool DoWork();

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void Render();
  void ResetTimer();

protected:

  unsigned int m_timer;

  unsigned int m_toastDisplayTime;

  CStdString m_defaultIcon;

  typedef std::queue<Notification> TOASTQUEUE;
  TOASTQUEUE m_notifications;
  CCriticalSection m_critical;
};
