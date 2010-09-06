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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#if !defined(_LINUX) && !defined(HAS_GL) && defined(HAS_DS_PLAYER)

#include "WinBaseRenderer.h"
#include "../DSPlayer/DSUtil/SmartPtr.h"
#include "CriticalSection.h"

class CBaseTexture;
class IPaintCallback;

class CWinDsRenderer : public CWinBaseRenderer
{
public:
  CWinDsRenderer();
  ~CWinDsRenderer();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot();
  void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height){};

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false) { return 0; };
  virtual void         ReleaseImage(int source, bool preserve = false) {};
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y) { return 0; };
  virtual void         FlipPage(int source) {};
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual bool         IsConfigured() { return m_bConfigured; }
  
  virtual void         RegisterCallback(IPaintCallback *callback);
  virtual void         UnregisterCallback();
  virtual inline void  OnAfterPresent();

  virtual bool         Supports(ERENDERFEATURE feature);
  virtual bool         Supports(EINTERLACEMETHOD method);
  virtual bool         Supports(ESCALINGMETHOD method);

  virtual void AutoCrop(bool bCrop);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
protected:
  virtual void Render(DWORD flags);
  
  bool m_bConfigured;
  bool m_bIsEvr;

  //dsplayer
  void RenderDShowBuffer(DWORD flags);

  Com::SmartPtr<IDirect3DTexture9> m_D3DVideoTexture;
  Com::SmartPtr<IDirect3DSurface9> m_D3DMemorySurface;
  IPaintCallback* m_paintCallback;
  // clear colour for "black" bars
  DWORD m_clearColour;
  unsigned int m_flags;
  Com::SmartRect m_pScreenSize;
};



class CDsPixelShaderRenderer : public CWinDsRenderer
{
public:
  CDsPixelShaderRenderer(bool isevr);
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);

protected:
  virtual void Render(DWORD flags);
};



#else
#include "LinuxRenderer.h"
#endif