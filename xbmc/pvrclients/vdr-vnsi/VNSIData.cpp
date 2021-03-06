/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include "VNSIData.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "vdrcommand.h"
#include "recordings.h"
#include "tools.h"

#define CMD_LOCK cMutexLock CmdLock((cMutex*)&m_Mutex)

cVNSIData::cVNSIData()
{
}

cVNSIData::~cVNSIData()
{
  Close();
}

bool cVNSIData::Open(CStdString hostname, int port, long timeout)
{
  if(!m_session.Open(hostname, port, timeout))
    return false;

  SetDescription("VNSI Data Listener");
  Start();
  SetClientConnected(true);
  return true;
}

void cVNSIData::Close()
{
  Cancel(1);
  m_session.Abort();
  m_session.Close();
}

bool cVNSIData::CheckConnection()
{
  return true;
}

cResponsePacket* cVNSIData::ReadResult(cRequestPacket* vrp)
{
  m_Mutex.Lock();

  SMessage &message(m_queue[vrp->getSerial()]);
  message.event = new cCondWait();
  message.pkt   = NULL;

  m_Mutex.Unlock();

  if(!m_session.SendMessage(vrp))
  {
    m_queue.erase(vrp->getSerial());
    return NULL;
  }

  message.event->Wait(2000);

  m_Mutex.Lock();

  cResponsePacket* vresp = message.pkt;
  delete message.event;

  m_queue.erase(vrp->getSerial());

  m_Mutex.Unlock();

  return vresp;
}

bool cVNSIData::GetTime(time_t *localTime, int *gmtOffset)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_GETTIME))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTime - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTime - Can't get response packed");
    return false;
  }

  uint32_t vdrTime       = vresp->extract_U32();
  int32_t  vdrTimeOffset = vresp->extract_S32();

  *localTime = vdrTime;
  *gmtOffset = vdrTimeOffset;

  delete vresp;
  return true;
}

bool cVNSIData::GetDriveSpace(long long *total, long long *used)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_RECORDINGS_DISKSIZE))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetDriveSpace - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetDriveSpace - Can't get response packed");
    return false;
  }

  uint32_t totalspace    = vresp->extract_U32();
  uint32_t freespace     = vresp->extract_U32();
  /* vresp->extract_U32(); percent not used */

  *total = totalspace;
  *used  = (totalspace - freespace);

  /* Convert from kBytes to Bytes */
  *total *= 1024;
  *used  *= 1024;

  delete vresp;
  return true;
}

bool cVNSIData::SupportChannelScan()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_SCAN_SUPPORTED))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::SupportChannelScan - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::SupportChannelScan - Can't get response packed");
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VDR_RET_OK ? true : false;
}

bool cVNSIData::EnableStatusInterface(bool onOff)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_ENABLESTATUSINTERFACE)) return false;
  if (!vrp.add_U8(onOff)) return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::EnableStatusInterface - Can't get response packed");
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VDR_RET_OK ? true : false;
}

bool cVNSIData::EnableOSDInterface(bool onOff)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_ENABLEOSDINTERFACE)) return false;
  if (!vrp.add_U8(onOff)) return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::EnableStatusInterface - Can't get response packed");
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VDR_RET_OK ? true : false;
}

int cVNSIData::GetGroupsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELS_GROUPSCOUNT))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetGroupsCount - Can't init cRequestPacket");
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetGroupsCount - Can't get response packed");
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

int cVNSIData::GetChannelsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsCount - Can't init cRequestPacket");
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsCount - Can't get response packed");
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

bool cVNSIData::GetGroupsList(PVRHANDLE handle, bool radio)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELS_GETGROUPS))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetGroupsList - Can't init cRequestPacket");
    return false;
  }
  if (!vrp.add_U32(radio))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetGroupsList - Can't add parameter to cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetGroupsList - Can't get response packed");
    return false;
  }

  while (!vresp->end())
  {
    uint32_t    index = vresp->extract_U32();
    uint32_t    count = vresp->extract_U32();
    const char *name  = vresp->extract_String();

//    LOGDBG("Have added a group to list. %lu %lu %s", index, count, name);
    delete name;
  }

  delete vresp;
  return true;
}

bool cVNSIData::GetChannelsList(PVRHANDLE handle, bool radio)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELS_GETCHANNELS))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsList - Can't init cRequestPacket");
    return false;
  }
  if (!vrp.add_U32(radio))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsList - Can't add parameter to cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsList - Can't get response packed");
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(tag));

    tag.number        = vresp->extract_U32();
    tag.name          = vresp->extract_String();
    tag.callsign      = tag.name;
    tag.uid           = vresp->extract_U32();
    tag.bouquet       = vresp->extract_U32();
    tag.encryption    = vresp->extract_U32();
    uint32_t vtype    = vresp->extract_U32();
    tag.radio         = radio;
    tag.input_format  = "";
    tag.stream_url    = "";

    PVR->TransferChannelEntry(handle, &tag);
    delete tag.name;
  }

  delete vresp;
  return true;
}

bool cVNSIData::GetEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_EPG_GETFORCHANNEL))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetEPGForChannel - Can't init cRequestPacket");
    return false;
  }
  if (!vrp.add_U32(channel.number) || !vrp.add_U32(start) || !vrp.add_U32(end - start))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetEPGForChannel - Can't add parameter to cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetEPGForChannel - Can't get response packed");
    return false;
  }

  while (!vresp->end())
  {
    PVR_PROGINFO tag;
    memset(&tag, 0 , sizeof(tag));

    tag.channum         = channel.number;
    tag.uid             = vresp->extract_U32();
    tag.starttime       = vresp->extract_U32();
    tag.endtime         = tag.starttime + vresp->extract_U32();
    uint32_t content    = vresp->extract_U32();
    tag.genre_type      = content & 0xF0;
    tag.genre_sub_type  = content & 0x0F;
    tag.parental_rating = vresp->extract_U32();
    tag.title           = vresp->extract_String();
    tag.subtitle        = vresp->extract_String();
    tag.description     = vresp->extract_String();

    PVR->TransferEpgEntry(handle, &tag);
    delete tag.title;
    delete tag.subtitle;
    delete tag.description;
  }

  delete vresp;
  return true;
}


/** OPCODE's 60 - 69: VNSI network functions for timer access */

int cVNSIData::GetTimersCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersCount - Can't init cRequestPacket");
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersCount - Can't get response packed");
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cVNSIData::GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_GET))           return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timernumber))          return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode != VDR_RET_OK)
  {
    delete vresp;
    if (returnCode == VDR_RET_DATAUNKNOWN)
      return PVR_ERROR_NOT_POSSIBLE;
    else if (returnCode == VDR_RET_ERROR)
      return PVR_ERROR_SERVER_ERROR;
  }

  tag.index       = vresp->extract_U32();
  tag.active      = vresp->extract_U32();
  uint32_t recording      = vresp->extract_U32();
  uint32_t pending        = vresp->extract_U32();
  tag.priority    = vresp->extract_U32();
  tag.lifetime    = vresp->extract_U32();
  tag.channelNum  = vresp->extract_U32();
  tag.starttime   = vresp->extract_U32();
  tag.endtime     = vresp->extract_U32();
  tag.firstday    = vresp->extract_U32();
  tag.repeatflags = vresp->extract_U32();
  tag.repeat      = tag.repeatflags == 0 ? false : true;
  tag.title       = vresp->extract_String();
  tag.directory   = "";

  delete tag.title;
  delete vresp;
  return PVR_ERROR_NO_ERROR;
}

bool cVNSIData::GetTimersList(PVRHANDLE handle)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_GETLIST))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersList - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersList - Can't get response packed");
    return false;
  }

  uint32_t numTimers = vresp->extract_U32();
  if (numTimers > 0)
  {
    while (!vresp->end())
    {
      PVR_TIMERINFO tag;
      tag.index       = vresp->extract_U32();
      tag.active      = vresp->extract_U32();
      uint32_t recording      = vresp->extract_U32();
      uint32_t pending        = vresp->extract_U32();
      tag.priority    = vresp->extract_U32();
      tag.lifetime    = vresp->extract_U32();
      tag.channelNum  = vresp->extract_U32();
      tag.starttime   = vresp->extract_U32();
      tag.endtime     = vresp->extract_U32();
      tag.firstday    = vresp->extract_U32();
      tag.repeatflags = vresp->extract_U32();
      tag.repeat      = tag.repeatflags == 0 ? false : true;
      tag.title       = vresp->extract_String();
      tag.directory   = "";

      PVR->TransferTimerEntry(handle, &tag);
      delete tag.title;
    }
  }
  delete vresp;
  return true;
}

PVR_ERROR cVNSIData::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_ADD))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::AddTimer - Can't init cRequestPacket");
    return PVR_ERROR_UNKOWN;
  }
  if (!vrp.add_U32(timerinfo.active))     return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.priority))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.lifetime))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.channelNum)) return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.starttime))  return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.endtime))    return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeat ? timerinfo.firstday : 0))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeatflags))return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(timerinfo.title))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp->noResponse())
  {
    delete vresp;
    XBMC->Log(LOG_ERROR, "cVNSIData::AddTimer - Can't get response packed");
    return PVR_ERROR_UNKOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == VDR_RET_DATALOCKED)
    return PVR_ERROR_ALREADY_PRESENT;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_DELETE))
    return PVR_ERROR_UNKOWN;

  if (!vrp.add_U32(timerinfo.index))
    return PVR_ERROR_UNKOWN;

  if (!vrp.add_U32(force))
    return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if (returnCode == VDR_RET_DATALOCKED)
    return PVR_ERROR_NOT_DELETED;
  if (returnCode == VDR_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  PVR_TIMERINFO timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.index, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.title = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR cVNSIData::UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_UPDATE))        return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.index))      return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.active))     return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.priority))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.lifetime))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.channelNum)) return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.starttime))  return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.endtime))    return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeat ? timerinfo.firstday : 0))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeatflags))return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(timerinfo.title))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == VDR_RET_DATAUNKNOWN)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

int cVNSIData::GetRecordingsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_RECORDINGS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsCount - Can't init cRequestPacket");
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsCount - Can't get response packed");
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cVNSIData::GetRecordingsList(PVRHANDLE handle)
{
  m_recIndex = 1;
  if (g_bUseRecordingsDir && g_szRecordingsDir != "")
  {
    ScanVideoDir(handle, g_szRecordingsDir.c_str(), g_szRecordingsDir.c_str());
  }
  else
  {
    bool haveCheckedLocalAccess = true /*false*/;
    m_RecordsPaths.clear();

    cRequestPacket vrp;
    if (!vrp.init(VDR_RECORDINGS_GETLIST))
    {
      XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsList - Can't init cRequestPacket");
      return PVR_ERROR_UNKOWN;
    }

    cResponsePacket* vresp = ReadResult(&vrp);
    if (!vresp)
    {
      XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsList - Can't get response packed");
      return PVR_ERROR_UNKOWN;
    }

    char* videodir = vresp->extract_String();
    m_videodir = videodir;
    delete[] videodir;

    while (!vresp->end())
    {
      CStdString title;

      PVR_RECORDINGINFO tag;
      tag.index           = m_recIndex++;
      tag.recording_time  = vresp->extract_U32();
      tag.duration        = vresp->extract_U32();
      tag.priority        = vresp->extract_U32();
      tag.lifetime        = vresp->extract_U32();
      tag.channel_name    = vresp->extract_String();
      char* name = vresp->extract_String();
      title = name;
      tag.subtitle        = vresp->extract_String();
      tag.description     = vresp->extract_String();

      char* fileName  = vresp->extract_String();

      /* Save the given path name for later to translate the
         index numbers to the path name. */
      m_RecordsPaths.push_back(fileName);

      /* Cleanup now the path name and remove VDR's 2 top
         directories and the strip the base path */
      CStdString path = fileName+m_videodir.size()+1;
      path = path.substr(0, path.find_last_of("/\\"));

      size_t found = path.find_last_of("/\\");
      if (found != CStdString::npos)
      {
        /* If no title is present use recording dir name
           as title */
        if (title.IsEmpty())
          title = path.substr(found+1);
        path = path.substr(0, found);
      }
      else
      {
        /* If no title is present use recording dir name
           as title */
        if (title.IsEmpty())
          title = path;
        path = "";
      }

      tag.title           = title.c_str();
      tag.directory       = path.c_str();
      tag.stream_url      = "";

      /* Check if we can open the first given recording dir on
         local filesystem, if yes, scan the files itself and
         ignore VNSI server for access them */
      if (!haveCheckedLocalAccess)
      {
        XBMC->Log(LOG_NOTICE, "Trying to open '%s'", fileName);
        DIR *vdrrecdir = opendir(fileName);
        if (vdrrecdir)
        {
          closedir(vdrrecdir);
          delete[] tag.channel_name;
          delete[] tag.subtitle;
          delete[] tag.description;
          delete[] name;
          delete[] fileName;
          delete vresp;

          XBMC->Log(LOG_NOTICE, "Found recordings on local disk, ignoring VNSI Server and scanning directories byself");

          ScanVideoDir(handle, m_videodir.c_str(), m_videodir.c_str());
          return PVR_ERROR_NO_ERROR;
        }

        haveCheckedLocalAccess = true;
      }

      PVR->TransferRecordingEntry(handle, &tag);

      delete[] tag.channel_name;
      delete[] tag.subtitle;
      delete[] tag.description;
      delete[] name;
      delete[] fileName;
    }

    delete vresp;
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::DeleteRecording(CStdString path)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_RECORDINGS_DELETE))
    return PVR_ERROR_UNKOWN;

  if (!vrp.add_String(path))
    return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if (returnCode == VDR_RET_DATALOCKED)
    return PVR_ERROR_NOT_DELETED;
  if (returnCode == VDR_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

void cVNSIData::Action()
{
  uint32_t channelID;
  uint32_t requestID;
  uint32_t userDataLength;
  uint8_t* userData;

  uint32_t timeNow;
  uint32_t lastKAsent = 0;
  uint32_t lastKArecv = time(NULL);
  bool readSuccess;

  cResponsePacket* vresp;

  while (Running())
  {
    timeNow = time(NULL);

    readSuccess = readData((uint8_t*)&channelID, sizeof(uint32_t));  // 2s timeout atm
    if (!readSuccess && !IsClientConnected())
      return; // return to stop this thread

    // Error or timeout.
    if (!lastKAsent) // have not sent a KA
    {
      if (lastKArecv < (timeNow - 5))
      {
        DEVDBG("cVNSIData::Action() - Sending KA packet");
        if (!sendKA(timeNow))
        {
          XBMC->Log(LOG_ERROR, "cVNSIData::Action() - Could not send KA, calling connectionDied");
          SetClientConnected(false);
          return;
        }
        lastKAsent = timeNow;
      }
    }
    else
    {
      if (lastKAsent <= (timeNow - 10))
      {
        XBMC->Log(LOG_ERROR, "cVNSIData::Action() - lastKA over 10s ago, calling connectionDied");
        SetClientConnected(false);
        return;
      }
    }

    if (!readSuccess) continue; // no data was read but the connection is ok.

    // Data was read
    channelID = ntohl(channelID);
    if (channelID == CHANNEL_REQUEST_RESPONSE)
    {
      if (!readData((uint8_t*)&requestID, sizeof(uint32_t))) break;
      requestID = ntohl(requestID);
      if (!readData((uint8_t*)&userDataLength, sizeof(uint32_t))) break;
      userDataLength = ntohl(userDataLength);
      if (userDataLength > 5000000) break; // how big can these packets get?
      userData = NULL;
      if (userDataLength > 0)
      {
        userData = (uint8_t*)malloc(userDataLength);
        if (!userData) break;
        if (!readData(userData, userDataLength)) break;
      }

      vresp = new cResponsePacket();
      vresp->setResponse(requestID, userData, userDataLength);
      DEVDBG("cVNSIData::Action() - Rxd a response packet, requestID=%lu, len=%lu", requestID, userDataLength);

      CMD_LOCK;
      SMessages::iterator it = m_queue.find(requestID);
      if (it != m_queue.end())
      {
        it->second.pkt = vresp;
        it->second.event->Signal();
      }
      else
      {
        delete vresp;
      }
    }
    else if (channelID == CHANNEL_STATUS)
    {
      if (!readData((uint8_t*)&requestID, sizeof(uint32_t))) break;
      requestID = ntohl(requestID);
      if (!readData((uint8_t*)&userDataLength, sizeof(uint32_t))) break;
      userDataLength = ntohl(userDataLength);
      if (userDataLength > 5000000) break; // how big can these packets get?
      userData = NULL;
      if (userDataLength > 0)
      {
        userData = (uint8_t*)malloc(userDataLength);
        if (!userData) break;
        if (!readData(userData, userDataLength)) break;
      }

      if (requestID == VDR_STATUS_MESSAGE)
      {
        uint32_t type = ntohl(*(uint32_t*)&userData[0]);
        int length = strlen((char*)&userData[4]);
        char* str = new char[length + 1];
        strcpy(str, (char*)&userData[4]);

        CStdString text = str;
        if (g_bCharsetConv)
          XBMC->UnknownToUTF8(text);

        if (type == 2)
          XBMC->QueueNotification(QUEUE_ERROR, text.c_str());
        if (type == 1)
          XBMC->QueueNotification(QUEUE_WARNING, text.c_str());
        else
          XBMC->QueueNotification(QUEUE_INFO, text.c_str());

        delete[] str;
      }
      else if (requestID == VDR_STATUS_RECORDING)
      {
        uint32_t device = ntohl(*(uint32_t*)&userData[0]);
        uint32_t on     = ntohl(*(uint32_t*)&userData[4]);

        int length = strlen((char*)&userData[8]);
        char* str = NULL;
        if (length > 1)
        {
          str = new char[length + 1];
          strcpy(str, (char*)&userData[8]);
        }

        int length2 = strlen((char*)&userData[8+length]);
        char* str2 = NULL;
        if (length2 > 1)
        {
          str2 = new char[length2 + 1];
          strcpy(str2, (char*)&userData[8+length]);
        }

        PVR->Recording(str, str2, on);
        PVR->TriggerTimerUpdate();

        delete[] str;
        delete[] str2;
      }
      else if (requestID == VDR_STATUS_TIMERCHANGE)
      {
        uint32_t status = ntohl(*(uint32_t*)&userData[0]);
        int length = strlen((char*)&userData[4]);
        char* str = new char[length + 1];
        strcpy(str, (char*)&userData[4]);
        delete[] str;
        PVR->TriggerTimerUpdate();
      }

      if (userData)
        free(userData);
    }
    else if (channelID == CHANNEL_KEEPALIVE)
    {
      uint32_t KAreply = 0;
      if (!readData((uint8_t*)&KAreply, sizeof(uint32_t))) break;
      KAreply = (uint32_t)ntohl(KAreply);
      if (KAreply == lastKAsent) // successful KA response
      {
        lastKAsent = 0;
        lastKArecv = KAreply;
        DEVDBG("cVNSIData::Action() - Rxd correct KA reply");
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "cVNSIData::Action() - Rxd a response packet on channel %lu !!", channelID);
      break;
    }
  }
}

bool cVNSIData::sendKA(uint32_t timeStamp)
{
  char buffer[8];
  *(uint32_t*)&buffer[0] = htonl(CHANNEL_KEEPALIVE);
  *(uint32_t*)&buffer[4] = htonl(timeStamp);
  if ((uint32_t)m_session.sendData(buffer, 8) != 8) return false;
  return true;
}

bool cVNSIData::readData(uint8_t* buffer, int totalBytes, int TimeOut)
{
  int ret = m_session.readData(buffer, totalBytes, TimeOut);
  if (ret == 1)
    return true;
  else if (ret == 0)
    return false;

  SetClientConnected(false);
  return false;
}

CStdString cVNSIData::GetRecordingPath(int index)
{
  if (index <= 0 || index > m_RecordsPaths.size())
    return "";

  return m_RecordsPaths[index-1];
}

#define MAX_LINK_LEVEL  6
void cVNSIData::ScanVideoDir(PVRHANDLE handle, const char *DirName, const char *DirBase, bool Deleted, int LinkLevel)
{
  cReadDir d(DirName);
  struct dirent *e;
  while ((e = d.Next()) != NULL)
  {
    if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
    {
      char *buffer = strdup(AddDirectory(DirName, e->d_name));
      struct stat st;
      if (stat(buffer, &st) == 0)
      {
        int Link = 0;
        if (S_ISLNK(st.st_mode))
        {
          if (LinkLevel > MAX_LINK_LEVEL)
          {
            XBMC->Log(LOG_ERROR, "max link level exceeded - not scanning %s", buffer);
            continue;
          }
          Link = 1;
          char *old = buffer;
          buffer = ReadLink(old);
          free(old);
          if (!buffer)
            continue;
          if (stat(buffer, &st) != 0)
          {
            free(buffer);
            continue;
          }
        }
        if (S_ISDIR(st.st_mode))
        {
          if (endswith(buffer, Deleted ? DELEXT : RECEXT))
          {
            cRecording recording(buffer, DirBase);

            PVR_RECORDINGINFO tag;
            tag.index           = m_recIndex++;
            tag.channel_name    = recording.ChannelName();
            tag.lifetime        = recording.Lifetime();
            tag.priority        = recording.Priority();
            tag.recording_time  = recording.StartTime();
            tag.duration        = recording.Duration();
            tag.subtitle        = recording.ShortText();
            tag.description     = recording.Description();
            tag.title           = recording.Title();
            tag.directory       = recording.Directory();
            tag.stream_url      = recording.StreamURL();

            PVR->TransferRecordingEntry(handle, &tag);
          }
          else
            ScanVideoDir(handle, buffer, DirBase, Deleted, LinkLevel + Link);
        }
      }
      free(buffer);
    }
  }
}
