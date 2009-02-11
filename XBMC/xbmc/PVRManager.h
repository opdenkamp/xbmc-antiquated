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

#include "utils/Thread.h"
#include "pvrclients/PVRClient.h"
#include "pvrclients/PVRClientTypes.h"
#include "utils/GUIInfoManager.h"
#include "TVDatabase.h"

#include <vector>

typedef std::vector< std::pair< DWORD, int > > PVRSCHEDULES;


class CPVRManager : /*public IPVRClientCallback
                  ,*/ private CThread 
{
public:

  CPVRManager();
  ~CPVRManager();

  void Start();
  void Stop();

  /* Manager access */
  static void RemoveInstance();
  static void ReleaseInstance();
  static CPVRManager* GetInstance();
  

  /* Event Handling */
  //void OnClientMessage(DWORD clientID, PVR_EVENT clientEvent, const std::string& data);
  void FillChannelData(DWORD clientID, PVR_PROGINFO* data, int count);

  /* Thread handling */
  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

  // info manager
  const char* TranslateInfo(DWORD dwInfo);
  bool  IsConnected();
  static bool HasTimer()  { return m_hasTimer;  };
  static bool IsRecording()   { return m_isRecording; };
  static bool HasRecordings() { return m_hasRecordings; };


  // called from TV Guide window
  CEPG* GetEPG() { return m_EPG; };
//  PVRSCHEDULES GetScheduled();
////   PVRSCHEDULES GetTimers();
//  PVRSCHEDULES GetConflicting();

protected:
  void SyncInfo(); // synchronize InfoManager related stuff

  bool LoadClients();
  bool CheckClientConnections();

  CURL GetConnString(DWORD clientID);
  void GetClientProperties(); // call GetClientProperties(DWORD clientID) for each client connected
  void GetClientProperties(DWORD clientID); // request the PVR_SERVERPROPS struct from each client

  void UpdateChannelsList(); // call UpdateChannelsList(DWORD clientID) for each client connected
  void UpdateChannelsList(DWORD clientID); // update the list of channels for the client specified

  void UpdateChannelData(); // call UpdateChannelData(DWORD clientID) for each client connected
  void UpdateChannelData(DWORD clientID); // update the guide data for the client specified

  void GetTimers(); // call GetTimers(DWORD clientID) for each client connected, active or otherwise
  int  GetTimers(DWORD clientID); // update the list of timers for the client specified, active or otherwise

  void GetRecordings(); // call GetRecordings(DWORD clientID) for each client connected
  int  GetRecordings(DWORD clientID); // update the list of active & completed recordings for the client specified

  CStdString PrintStatus(RecStatus status); // convert a RecStatus into a human readable string
  CStdString PrintStatusDescription(RecStatus status); // convert a RecStatus into a more verbose human readable string

private:
  static CPVRManager* m_instance;

  std::map< DWORD, CPVRClient* >     m_clients; // pointer to each enabled client's interface
  std::map< DWORD, PVR_SERVERPROPS > m_clientProps; // store the properties of each client locally

  PVRSCHEDULES m_timers; // list of all timers (including custom & manual)
  PVRSCHEDULES m_scheduledRecordings; // what will actually be recorded
  PVRSCHEDULES m_conflictingSchedules; // what is conflicting

  CCriticalSection m_critSection;
  bool m_running;
  bool m_bRefreshSettings;

  unsigned m_numRecordings;
  unsigned m_numUpcomingSchedules;
  unsigned m_numTimers;

  static bool m_isRecording;
  static bool m_hasRecordings;
  static bool m_hasTimer;
  static bool m_hasTimers;

  CStdString  m_nextRecordingDateTime;
  CStdString  m_nextRecordingClient;
  CStdString  m_nextRecordingTitle;
  CStdString  m_nowRecordingDateTime;
  CStdString  m_nowRecordingClient;
  CStdString  m_nowRecordingTitle;

  CEPG       *m_EPG;
  CTVDatabase m_database;
};
