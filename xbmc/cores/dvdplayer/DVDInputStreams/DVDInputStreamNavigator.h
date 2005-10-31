
#pragma once

#include "DVDInputStream.h"
#include "..\IDVDPlayer.h"
#include <string>
#include "..\DVDOverlay.h"

#include "DllDvdNav.h"

#define DVD_VIDEO_BLOCKSIZE         DVD_VIDEO_LB_LEN // 2048 bytes

#define NAVRESULT_OK                0x00000001
#define NAVRESULT_SKIPPED_STILL     0x00000002
#define NAVRESULT_STILL_NOT_SKIPPED 0x00000004

#define LIBDVDNAV_BUTTON_NORMAL 0
#define LIBDVDNAV_BUTTON_CLICKED 1

class CDVDDemuxSPU;
class CSPUInfo;
class CDVDOverlayPicture;

struct dvdnav_s;

class DVDNavResult
{
public:
  DVDNavResult(){};
  DVDNavResult(void* p, int t) { pData = p; type = t; };
  void* pData;
  int type;
};

class CDVDInputStreamNavigator : public CDVDInputStream
{
public:
  CDVDInputStreamNavigator(IDVDPlayer* player);
  virtual ~CDVDInputStreamNavigator();

  virtual bool Open(const char* strFile);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual int GetBlockSize() { return DVDSTREAM_BLOCK_SIZE_DVD; }
  
  void ActivateButton();
  void SelectButton(int iButton);
  void SkipStill();
  void SkipWait();
  void OnUp();
  void OnDown();
  void OnLeft();
  void OnRight();
  void OnMenu();
  void OnBack();
  void OnNext();
  void OnPrevious();

  int GetCurrentButton();
  
  bool GetCurrentButtonInfo(CDVDOverlaySpu* pOverlayPicture, CDVDDemuxSPU* pSPU, int iButtonType /* 0 = selection, 1 = action (clicked)*/);
  
  bool IsInMenu();

  int GetActiveSubtitleStream();
  std::string GetSubtitleStreamLanguage(int iId);
  int GetSubTitleStreamCount();
  bool SetActiveSubtitleStream(int iId);
  int GetMpegSubtitleStream(int iId);
  void EnableSubtitleStream(bool bEnable);

  int GetActiveAudioStream();
  std::string GetAudioStreamLanguage(int iId);
  int GetAudioStreamCount();
  bool SetActiveAudioStream(int iId);
  int GetMpegAudioStream(int iId);

  
  
  int GetNrOfTitles();
  int GetNrOfParts(int iTitle);
  bool PlayTitle(int iTitle);
  bool PlayPart(int iTitle, int iPart);

  int GetTotalTime(); // the total time in milli seconds
  int GetTime(); // the current position in milli seconds

  float GetVideoAspectRatio();

  bool Seek(int iTimeInMsec); //seek within current pg(c)

protected:

  int ProcessBlock(BYTE* buffer, int* read);

  int GetTotalButtons();
  void CheckButtons();
  
  void Lock()   { EnterCriticalSection(&m_critSection); }
  void Unlock() { LeaveCriticalSection(&m_critSection); }
  
  DllDvdNav m_dll;
  bool m_bDiscardHop;
  bool m_bCheckButtons;
  bool m_bStopped;

  int m_iTotalTime;
  int m_iTime;
  
  struct dvdnav_s* m_dvdnav;
  
  IDVDPlayer* m_pDVDPlayer;
  
  BYTE m_tempbuffer[DVD_VIDEO_BLOCKSIZE];
  CRITICAL_SECTION m_critSection;
};