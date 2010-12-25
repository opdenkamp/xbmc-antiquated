/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "TVDatabase.h"
#include "utils/GUIInfoManager.h"
#include "AdvancedSettings.h"
#include "utils/log.h"

#include "PVREpgInfoTag.h"
#include "PVRChannelGroups.h"
#include "PVRChannelGroup.h"

using namespace std;

using namespace dbiplus;

CTVDatabase::CTVDatabase(void)
{
  oneWriteSQLString = "";
  lastScanTime.SetValid(false);
}

CTVDatabase::~CTVDatabase(void)
{
}

bool CTVDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseTV);
}

bool CTVDatabase::OpenDS()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to open data source",
        __FUNCTION__);
    return false;
  }

  return true;
}

bool CTVDatabase::ExecuteQuery(const CStdString &strQuery, bool bOpenDS /* = true */)
{
  if (bOpenDS  && !OpenDS())
      return false;

  try
  {
    m_pDS->exec(strQuery.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to execute query '%s'",
        __FUNCTION__, strQuery.c_str());
    return false;
  }
}

CStdString CTVDatabase::GetSingleValue(const CStdString &strTable, const CStdString &strColumn, const CStdString &strWhereClause /* = CStdString() */, const CStdString &strOrderBy /* = CStdString() */)
{
  CStdString strReturn;

  CStdString strQueryBase = "SELECT %s FROM %s";
  if (!strWhereClause.IsEmpty())
    strQueryBase.AppendFormat(" WHERE %s", strWhereClause.c_str());
  if (!strOrderBy.IsEmpty())
    strQueryBase.AppendFormat(" ORDER BY %s", strOrderBy.c_str());

  CStdString strQuery = FormatSQL(strQueryBase,
      strColumn.c_str(), strTable.c_str());

  if (ExecuteQuery(strQuery))
  {
    if (m_pDS->num_rows() > 0)
      strReturn = m_pDS->fv(0).get_asString();
  }

  m_pDS->close();
  return strReturn;
}

bool CTVDatabase::DeleteValues(const CStdString &strTable, const CStdString &strWhereClause /* = CStdString() */)
{
  bool bReturn = true;

  CStdString strQueryBase = "DELETE FROM %s";
  if (!strWhereClause.IsEmpty())
    strQueryBase.AppendFormat(" WHERE %s", strWhereClause.c_str());

  CStdString strQuery = FormatSQL(strQueryBase, strTable.c_str());

  bReturn = ExecuteQuery(strQuery);

  m_pDS->close();

  return bReturn;
}

bool CTVDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "%s - creating tables", __FUNCTION__);

    CLog::Log(LOGDEBUG, "%s - creating table 'Clients'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE Clients (idClient integer primary key, Name text, GUID text)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'LastChannel'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE LastChannel (idChannel integer primary key, Number integer, Name text)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'LastEPGScan'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE LastEPGScan (idScan integer primary key, ScanTime datetime)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'GuideData'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE GuideData (idDatabaseBroadcast integer primary key, idUniqueBroadcast integer, idChannel integer, StartTime datetime, "
                "EndTime datetime, strTitle text, strPlotOutline text, strPlot text, GenreType integer, GenreSubType integer, "
                "firstAired datetime, parentalRating integer, starRating integer, notify integer, seriesNum text, "
                "episodeNum text, episodePart text, episodeName text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_GuideData on GuideData(idChannel, StartTime desc)\n"); /// pointless?

    CLog::Log(LOGDEBUG, "%s - creating table 'Channels'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE Channels (idChannel integer primary key, Name text, Number integer, ClientName text, "
                "ClientNumber integer, idClient integer, UniqueId integer, IconPath text, GroupID integer, countWatched integer, "
                "timeWatched integer, lastTimeWatched datetime, encryption integer, radio bool, hide bool, grabEpg bool, EpgGrabber text, "
                "lastGrabTime datetime, Virtual bool, strInputFormat text, strStreamURL text)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'ChannelLinkageMap'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE ChannelLinkageMap (idMapping integer primary key, idPortalChannel integer, idLinkedChannel integer)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'ChannelGroup'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE ChannelGroup (idGroup integer primary key, groupName text, sortOrder integer)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'RadioChannelGroup'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE RadioChannelGroup (idGroup integer primary key, groupName text, sortOrder integer)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'ChannelSettings'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE ChannelSettings ( idChannel integer primary key, Deinterlace integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer, Sharpness float, NoiseReduction float)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_ChannelSettings ON ChannelSettings (idChannel)\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create TV tables:%i", __FUNCTION__, (int)GetLastError());
    return false;
  }

  return true;
}

bool CTVDatabase::UpdateOldVersion(int iVersion)
{
//  BeginTransaction();
//
//  try
//  {
//
//  }
//  catch (...)
//  {
//    CLog::Log(LOGERROR, "Error attempting to update the database version!");
//    RollbackTransaction();
//    return false;
//  }
//  CommitTransaction();
  return true;
}

CDateTime CTVDatabase::GetLastEPGScanTime()
{
  if (lastScanTime.IsValid())
    return lastScanTime;

  CStdString strValue = GetSingleValue("LastEPGScan", "ScanTime");

  if (strValue.IsEmpty())
    return -1;

  CDateTime lastTime;
  lastTime.SetFromDBDateTime(strValue);
  lastScanTime = lastTime;

  return lastTime;
}

bool CTVDatabase::UpdateLastEPGScan(const CDateTime lastScan)
{
  CLog::Log(LOGDEBUG, "%s - updating last scan time to '%s'",
      __FUNCTION__, lastScan.GetAsDBDateTime().c_str());
  lastScanTime = lastScan;

  bool bReturn = true;
  CStdString strQuery = FormatSQL("REPLACE INTO LastEPGScan (ScanTime) VALUES ('%s')\n",
      lastScan.GetAsDBDateTime().c_str());

  bReturn = ExecuteQuery(strQuery);
  m_pDS->close();

  return bReturn;
}

int CTVDatabase::GetLastChannel()
{
  CStdString strValue = GetSingleValue("LastChannel", "idChannel");

  if (strValue.IsEmpty())
    return -1;

  return atoi(strValue.c_str());
}

bool CTVDatabase::UpdateLastChannel(const CPVRChannel &info)
{
  if (info.ChannelID() < 0)
  {
    /* invalid channel */
    return false;
  }

  bool bReturn = true;
  CStdString strQuery = FormatSQL("REPLACE INTO LastChannel (idChannel, Number, Name) VALUES ('%i', '%i', '%s')\n",
      info.ChannelID(), info.ChannelNumber(), info.ChannelName().c_str());

  bReturn = ExecuteQuery(strQuery);
  m_pDS->close();

  return bReturn;
}

bool CTVDatabase::EraseClients()
{
  return DeleteValues("Clients") &&
      DeleteValues("LastChannel") &&
      DeleteValues("LastEPGScan");
}

long CTVDatabase::AddClient(const CStdString &client, const CStdString &guid)
{
  long iReturn = -1;
  CStdString strQuery = FormatSQL("INSERT INTO Clients (idClient, Name, GUID) VALUES (NULL, '%s', '%s')\n",
      client.c_str(), guid.c_str());

  if (ExecuteQuery(strQuery))
  {
    iReturn = (long) m_pDS->lastinsertid();
  }
  m_pDS->close();

  return iReturn;
}

long CTVDatabase::GetClientId(const CStdString& guid)
{
  CStdString strWhereClause = FormatSQL("GUID like '%s'", guid.c_str());
  CStdString strValue = GetSingleValue("Clients", "idClient", strWhereClause);

  if (strValue.IsEmpty())
    return -1;

  return atoi(strValue.c_str());
}

bool CTVDatabase::EraseEPG()
{
  return DeleteValues("GuideData");
}

bool CTVDatabase::EraseEPGForChannel(long channelID, CDateTime after)
{
  CStdString strWhereClause;
  if (after == NULL)
    strWhereClause = FormatSQL("GuideData.idChannel = '%u'", channelID);
  else
    strWhereClause = FormatSQL("GuideData.idChannel = '%u' AND GuideData.EndTime > '%s'", channelID, after.GetAsDBDateTime().c_str());

  return DeleteValues("GuideData", strWhereClause);
}

bool CTVDatabase::EraseOldEPG()
{
  CDateTime yesterday = CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0);
  CStdString strWhereClause = FormatSQL("EndTime < '%s'", yesterday.GetAsDBDateTime().c_str());

  return DeleteValues("GuideData", strWhereClause);
}

bool CTVDatabase::UpdateEPGEntry(const CPVREpgInfoTag &info, bool oneWrite, bool firstWrite, bool lastWrite)
{
  try
  {
    if (!OpenDS())
      return false;

    if (NULL == m_pDS2.get()) return false;

    if (!oneWrite && firstWrite)
      m_pDS2->insert();

    if (info.UniqueBroadcastID() > 0)
    {
      CStdString SQL = FormatSQL("select idDatabaseBroadcast from GuideData WHERE (GuideData.idUniqueBroadcast = '%u' OR GuideData.StartTime = '%s') AND GuideData.idChannel = '%u'", info.UniqueBroadcastID(), info.Start().GetAsDBDateTime().c_str(), info.ChannelTag()->ChannelID());
      m_pDS->query(SQL.c_str());

      if (m_pDS->num_rows() > 0)
      {
        int id = m_pDS->fv("idDatabaseBroadcast").get_asInt();
        m_pDS->close();
        // update the item
        SQL = FormatSQL("update GuideData set idChannel=%i, StartTime='%s', EndTime='%s', strTitle='%s', strPlotOutline='%s', "
                        "strPlot='%s', GenreType=%i, GenreSubType=%i, firstAired='%s', parentalRating=%i, starRating=%i, "
                        "notify=%i, seriesNum='%s', episodeNum='%s', episodePart='%s', episodeName='%s', "
                        "idUniqueBroadcast=%i WHERE idDatabaseBroadcast=%i",
                        info.ChannelTag()->ChannelID(), info.Start().GetAsDBDateTime().c_str(), info.End().GetAsDBDateTime().c_str(),
                        info.Title().c_str(), info.PlotOutline().c_str(), info.Plot().c_str(), info.GenreType(), info.GenreSubType(),
                        info.FirstAired().GetAsDBDateTime().c_str(), info.ParentalRating(), info.StarRating(), info.Notify(),
                        info.SeriesNum().c_str(), info.EpisodeNum().c_str(), info.EpisodePart().c_str(), info.EpisodeName().c_str(),
                        info.UniqueBroadcastID(), id);

        if (oneWrite)
          m_pDS->exec(SQL.c_str());
        else
          m_pDS2->add_insert_sql(SQL);

        return true;
      }
      else   // add the items
      {
        m_pDS->close();
        SQL = FormatSQL("insert into GuideData (idDatabaseBroadcast, idChannel, StartTime, "
                        "EndTime, strTitle, strPlotOutline, strPlot, GenreType, GenreSubType, "
                        "firstAired, parentalRating, starRating, notify, seriesNum, "
                        "episodeNum, episodePart, episodeName, idUniqueBroadcast) "
                        "values (NULL,'%i','%s','%s','%s','%s','%s','%i','%i','%s','%i','%i','%i','%s','%s','%s','%s','%i')\n",
                        info.ChannelTag()->ChannelID(), info.Start().GetAsDBDateTime().c_str(), info.End().GetAsDBDateTime().c_str(),
                        info.Title().c_str(), info.PlotOutline().c_str(), info.Plot().c_str(), info.GenreType(), info.GenreSubType(),
                        info.FirstAired().GetAsDBDateTime().c_str(), info.ParentalRating(), info.StarRating(), info.Notify(),
                        info.SeriesNum().c_str(), info.EpisodeNum().c_str(), info.EpisodePart().c_str(), info.EpisodeName().c_str(),
                        info.UniqueBroadcastID());

        if (oneWrite)
        {
          m_pDS->exec(SQL.c_str());
        }
        else
        {
          if (firstWrite)
            m_pDS2->insert();

          m_pDS2->add_insert_sql(SQL);
        }

        if (GetEPGDataEnd(info.ChannelTag()->ChannelID()) > info.End())
        {
          CLog::Log(LOGNOTICE, "TV-Database: erasing epg data due to event change on channel %s", info.ChannelTag()->ChannelName().c_str());
          EraseEPGForChannel(info.ChannelTag()->ChannelID(), info.End());
        }
      }
    }

    if (!oneWrite && lastWrite)
    {
      m_pDS2->post();
      m_pDS2->clear_insert_sql();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s on Channel %s) failed", __FUNCTION__, info.Title().c_str(), info.ChannelTag()->ChannelName().c_str());
  }
  return false;
}

bool CTVDatabase::RemoveEPGEntry(const CPVREpgInfoTag &info)
{
  CStdString strWhereClause = FormatSQL("GuideData.idUniqueBroadcast = '%u'", info.UniqueBroadcastID() );
  return DeleteValues("GuideData", strWhereClause);
}

bool CTVDatabase::RemoveEPGEntries(unsigned int channelID, const CDateTime &start, const CDateTime &end)
{
  CStdString strWhereClause;
  if (channelID < 0)
    strWhereClause = FormatSQL("StartTime < '%s' AND EndTime > '%s'",
                     start.GetAsDBDateTime().c_str(), end.GetAsDBDateTime().c_str());
  else
    strWhereClause = FormatSQL("idChannel = '%u' AND StartTime < '%s' AND EndTime > '%s'",
                     channelID, start.GetAsDBDateTime().c_str(), end.GetAsDBDateTime().c_str());

  return DeleteValues("GuideData", strWhereClause);
}

bool CTVDatabase::GetEPGForChannel(CPVREpg *epg, const CDateTime &start, const CDateTime &end)
{
  if (!OpenDS())
    return false;

  try
  {
    CPVRChannel *channel = epg->Channel();
    if (!channel)
      return false;

    CStdString SQL=FormatSQL("select * from GuideData WHERE GuideData.idChannel = '%u' AND GuideData.EndTime > '%s' "
                             "AND GuideData.StartTime < '%s' ORDER BY GuideData.StartTime;", channel->ChannelID(), start.GetAsDBDateTime().c_str(), end.GetAsDBDateTime().c_str());
    m_pDS->query( SQL.c_str() );

    if (m_pDS->num_rows() <= 0)
      return false;

    while (!m_pDS->eof())
    {
      PVR_PROGINFO broadcast;
      CDateTime startTime, endTime;
      time_t startTime_t, endTime_t;
      broadcast.uid             = m_pDS->fv("idUniqueBroadcast").get_asInt();
      broadcast.title           = m_pDS->fv("strTitle").get_asString().c_str();
      broadcast.subtitle        = m_pDS->fv("strPlotOutline").get_asString().c_str();
      broadcast.description     = m_pDS->fv("strPlot").get_asString().c_str();
      broadcast.genre_type      = m_pDS->fv("GenreType").get_asInt();
      broadcast.genre_sub_type  = m_pDS->fv("GenreSubType").get_asInt();
      broadcast.parental_rating = m_pDS->fv("parentalRating").get_asInt();
      startTime.SetFromDBDateTime(m_pDS->fv("StartTime").get_asString());
      endTime.SetFromDBDateTime(m_pDS->fv("EndTime").get_asString());
      startTime.GetAsTime(startTime_t);
      endTime.GetAsTime(endTime_t);
      broadcast.starttime = startTime_t;
      broadcast.endtime = endTime_t;

      CPVREpg *epg = channel->GetEPG();
      if (!epg)
        continue;

      epg->UpdateEntry(&broadcast, false);

      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return false;
}

CDateTime CTVDatabase::GetEPGDataStart(int channelID)
{
  CDateTime firstProgramme;
  CStdString strWhereClause;

  if (channelID != -1)
    strWhereClause = FormatSQL("idChannel = '%u'", channelID);

  CStdString strReturn = GetSingleValue("GuideData", "StartTime", strWhereClause, "StartTime ASC");
  if (!strReturn.IsEmpty())
    firstProgramme.SetFromDBDateTime(strReturn);

  m_pDS->close();

  if (!firstProgramme.IsValid())
    return CDateTime::GetCurrentDateTime();
  else
    return firstProgramme;
}

CDateTime CTVDatabase::GetEPGDataEnd(int channelID)
{
  CDateTime lastProgramme;
  CStdString strWhereClause;

  if (channelID != -1)
    strWhereClause = FormatSQL("idChannel = '%u'", channelID);

  CStdString strReturn = GetSingleValue("GuideData", "EndTime", strWhereClause, "EndTime DESC");
  if (!strReturn.IsEmpty())
    lastProgramme.SetFromDBDateTime(strReturn);

  m_pDS->close();

  if (!lastProgramme.IsValid())
    return CDateTime::GetCurrentDateTime();
  else
    return lastProgramme;
}

bool CTVDatabase::EraseChannels()
{
  return DeleteValues("Channels");
}

bool CTVDatabase::EraseClientChannels(long clientID)
{
  CStdString strWhereClause = FormatSQL("Channels.idClient = '%u'", clientID);
  return DeleteValues("Channels", strWhereClause);
}

long CTVDatabase::AddDBChannel(const CPVRChannel &info, bool oneWrite, bool firstWrite, bool lastWrite)
{
  try
  {
    if (info.ClientID() < 0) return -1;
    if (!OpenDS())
      return -1;

    long channelId = info.ChannelID();
    if (channelId < 0)
    {
      CStdString SQL = FormatSQL("insert into Channels (idChannel, idClient, Number, Name, ClientName, "
                                 "ClientNumber, UniqueId, IconPath, GroupID, encryption, radio, "
                                 "grabEpg, EpgGrabber, hide, Virtual, strInputFormat, strStreamURL) "
                                 "values (NULL, '%i', '%i', '%s', '%s', '%i', '%i', '%s', '%i', '%i', '%i', '%i', '%s', '%i', '%i', '%s', '%s')\n",
                                 info.ClientID(), info.ChannelNumber(), info.ChannelName().c_str(), info.ClientChannelName().c_str(),
                                 info.ClientChannelNumber(), info.UniqueID(), info.m_strIconPath.c_str(), info.m_iChannelGroupId,
                                 info.EncryptionSystem(), info.m_bIsRadio, info.m_bEPGEnabled, info.m_strEPGScraper.c_str(),
                                 info.m_bIsHidden, info.m_bIsVirtual, info.m_strInputFormat.c_str(), info.m_strStreamURL.c_str());

      if (oneWrite)
      {
        m_pDS->exec(SQL.c_str());
        channelId = (long)m_pDS->lastinsertid();
      }
      else
      {
        if (firstWrite)
          m_pDS->insert();

        m_pDS->add_insert_sql(SQL);
        if (lastWrite)
          m_pDS->post();
      }
    }
    return channelId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.m_strChannelName.c_str());
  }

  return -1;
}

bool CTVDatabase::RemoveDBChannel(const CPVRChannel &info)
{
  if (info.ChannelID() < 0 || info.ClientID() < 0)
    return false;

  CStdString strWhereClause = FormatSQL("Channels.idChannel = '%u' AND Channels.idClient = '%u'",
      info.ChannelID(), info.ClientID());
  return DeleteValues("Channels", strWhereClause);
}

long CTVDatabase::UpdateDBChannel(const CPVRChannel &info)
{
  long iChannelID = info.ChannelID();

  CStdString strQuery;
  if (iChannelID > 0)
  {
    strQuery = FormatSQL("UPDATE Channels SET idClient=%i, Number=%i, Name='%s', ClientName='%s',"
        "ClientNumber=%i, UniqueId=%i, IconPath='%s', GroupID=%i, encryption=%i, radio=%i,"
        "hide=%i, grabEpg=%i, EpgGrabber='%s', Virtual=%i, strInputFormat='%s', strStreamURL='%s' where idChannel=%i",
        info.ClientID(), info.ChannelNumber(), info.ChannelName().c_str(), info.ClientChannelName().c_str(),
        info.ClientChannelNumber(), info.UniqueID(), info.IconPath().c_str(), info.GroupID(),
        info.EncryptionSystem(), info.IsRadio(), info.IsHidden(), info.EPGEnabled(), info.EPGScraper().c_str(),
        info.IsVirtual(), info.InputFormat().c_str(), info.StreamURL().c_str(), iChannelID);
  }
  else
  {
    strQuery = FormatSQL("INSERT INTO Channels (idChannel, idClient, Number, Name, ClientName, "
        "ClientNumber, UniqueId, IconPath, GroupID, encryption, radio, "
        "grabEpg, EpgGrabber, hide, Virtual, strInputFormat, strStreamURL) "
        "values (NULL, '%i', '%i', '%s', '%s', '%i', '%i', '%s', '%i', '%i', '%i', '%i', '%s', '%i', '%i', '%s', '%s')\n",
        info.ClientID(), info.ChannelNumber(), info.ChannelName().c_str(), info.ClientChannelName().c_str(),
        info.ClientChannelNumber(), info.UniqueID(), info.IconPath().c_str(), info.GroupID(),
        info.EncryptionSystem(), info.IsRadio(), info.EPGEnabled(), info.EPGScraper().c_str(),
        info.IsHidden(), info.IsVirtual(), info.InputFormat().c_str(), info.StreamURL().c_str());
  }

  if (ExecuteQuery(strQuery) && iChannelID > 0)
  {
    iChannelID = (long) m_pDS->lastinsertid();
  }

  m_pDS->close();

  return iChannelID;
}

bool CTVDatabase::HasChannel(const CPVRChannel &info)
{
  bool bReturn = false;
  CStdString strQuery = FormatSQL("SELECT COUNT(*) FROM Channels WHERE Name = '%s' AND ClientNumber = '%i'",
      info.m_strChannelName.c_str(), info.m_iClientChannelNumber);

  if (ExecuteQuery(strQuery))
  {
    bReturn = (m_pDS->fv(0).get_asInt() > 0);
    m_pDS->close();
  }

  return bReturn;
}

int CTVDatabase::GetDBNumChannels(bool radio)
{
  int iReturn = false;
  CStdString strQuery = FormatSQL("SELECT COUNT(*) FROM Channels WHERE radio = %u", radio);

  if (ExecuteQuery(strQuery))
  {
    iReturn = m_pDS->fv(0).get_asInt();
    m_pDS->close();
  }

  return iReturn;
}

int CTVDatabase::GetNumHiddenChannels()
{
  int iReturn = -1;
  CStdString strQuery = FormatSQL("SELECT COUNT(*) FROM Channels hide = %u", true);

  if (ExecuteQuery(strQuery))
  {
    iReturn = m_pDS->fv(0).get_asInt();
    m_pDS->close();
  }

  return iReturn;
}

bool CTVDatabase::GetDBChannelList(CPVRChannels &results, bool radio)
{
  if (!OpenDS())
    return false;

  try
  {
    CStdString SQL=FormatSQL("SELECT * FROM Channels WHERE radio = %u ORDER BY Number", radio);

    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      CPVRChannel *channel = new CPVRChannel();

      channel->m_iClientId               = m_pDS->fv("idClient").get_asInt();
      channel->m_iDatabaseId             = m_pDS->fv("idChannel").get_asInt();
      channel->m_iChannelNumber          = m_pDS->fv("Number").get_asInt();
      channel->m_strChannelName          = m_pDS->fv("Name").get_asString();
      channel->m_strClientChannelName    = m_pDS->fv("ClientName").get_asString();
      channel->m_iClientChannelNumber    = m_pDS->fv("ClientNumber").get_asInt();
      channel->m_iUniqueId               = m_pDS->fv("UniqueId").get_asInt();
      channel->m_strIconPath             = m_pDS->fv("IconPath").get_asString();
      channel->m_iChannelGroupId         = m_pDS->fv("GroupID").get_asInt();
      channel->m_iClientEncryptionSystem = m_pDS->fv("encryption").get_asInt();
      channel->m_bIsRadio                = m_pDS->fv("radio").get_asBool();
      channel->m_bIsHidden               = m_pDS->fv("hide").get_asBool();
      channel->m_bEPGEnabled             = m_pDS->fv("grabEpg").get_asBool();
      channel->m_strEPGScraper           = m_pDS->fv("EpgGrabber").get_asString();
      channel->m_strInputFormat          = m_pDS->fv("strInputFormat").get_asString();
      channel->m_strStreamURL            = m_pDS->fv("strStreamURL").get_asString();
      channel->m_bIsVirtual              = m_pDS->fv("Virtual").get_asBool();
      channel->UpdatePath();

      results.push_back(channel);
      m_pDS->next();
    }

    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }

  return false;
}

bool CTVDatabase::EraseChannelGroups()
{
  return DeleteValues("ChannelGroup");
}

long CTVDatabase::AddChannelGroup(const CStdString &groupName, int sortOrder)
{
  try
  {
    if (groupName == "")      return -1;
    if (!OpenDS())
      return false;

    long groupId;
    groupId = GetChannelGroupId(groupName);
    if (groupId < 0)
    {
      CStdString SQL=FormatSQL("insert into ChannelGroup (idGroup, groupName, sortOrder) values (NULL, '%s', '%i')", groupName.c_str(), sortOrder);
      m_pDS->exec(SQL.c_str());
      groupId = (long)m_pDS->lastinsertid();
    }

    return groupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, groupName.c_str());
  }
  return -1;
}

bool CTVDatabase::DeleteChannelGroup(int GroupId)
{
  if (GroupId < 0)
    return false;

  CStdString strWhereClause = FormatSQL("ChannelGroup.idGroup = '%u'", GroupId);
  return DeleteValues("ChannelGroup", strWhereClause);
}

bool CTVDatabase::GetChannelGroupList(CPVRChannelGroups &results)
{
  if (!OpenDS())
    return false;

  try
  {
    CStdString SQL = FormatSQL("select * from ChannelGroup ORDER BY ChannelGroup.sortOrder");
    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      CPVRChannelGroup data;

      data.SetGroupID(m_pDS->fv("idGroup").get_asInt());
      data.SetGroupName(m_pDS->fv("groupName").get_asString());
      data.SetSortOrder(m_pDS->fv("sortOrder").get_asInt());

      results.push_back(data);
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return false;
}

bool CTVDatabase::SetChannelGroupName(unsigned int GroupId, const CStdString &newname)
{
  if (!OpenDS())
    return false;

  try
  {
    if (GroupId < 0)   // no match found, update required
    {
      return false;
    }

    CStdString SQL;

    SQL=FormatSQL("select * from ChannelGroup WHERE ChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update ChannelGroup set groupName='%s' WHERE idGroup=%i", newname.c_str(), GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
    return false;
  }
}

bool CTVDatabase::SetChannelGroupSortOrder(unsigned int GroupId, int sortOrder)
{
  if (!OpenDS())
    return false;

  try
  {
    if (GroupId < 0)   // no match found, update required
      return false;

    CStdString SQL;

    SQL=FormatSQL("select * from ChannelGroup WHERE ChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update ChannelGroup set sortOrder='%i' WHERE idGroup=%i", sortOrder, GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
  }
  return -1;
}

long CTVDatabase::GetChannelGroupId(const CStdString &groupname)
{
  CStdString strWhereClause = FormatSQL("groupName LIKE '%s'", groupname.c_str());
  CStdString strReturn = GetSingleValue("ChannelGroup", "idGroup", strWhereClause);

  m_pDS->close();

  return atoi(strReturn);
}

bool CTVDatabase::EraseRadioChannelGroups()
{
  return DeleteValues("RadioChannelGroup");
}

long CTVDatabase::AddRadioChannelGroup(const CStdString &groupName, int sortOrder)
{
  try
  {
    if (groupName == "")      return -1;
    if (!OpenDS())
      return -1;

    long groupId;
    groupId = GetRadioChannelGroupId(groupName);
    if (groupId < 0)
    {
      CStdString SQL=FormatSQL("insert into RadioChannelGroup (idGroup, groupName, sortOrder) values (NULL, '%s', '%i')", groupName.c_str(), sortOrder);
      m_pDS->exec(SQL.c_str());
      groupId = (long)m_pDS->lastinsertid();
    }

    return groupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, groupName.c_str());
  }
  return -1;
}

bool CTVDatabase::DeleteRadioChannelGroup(unsigned int GroupId)
{
  if (!OpenDS())
    return false;

  try
  {
    if (GroupId < 0)   // no match found, update required
      return false;

    CStdString strSQL=FormatSQL("delete from RadioChannelGroup WHERE RadioChannelGroup.idGroup = '%u'", GroupId);

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
    return false;
  }
}

bool CTVDatabase::GetRadioChannelGroupList(CPVRChannelGroups &results)
{
  if (!OpenDS())
    return false;

  try
  {
    CStdString SQL = FormatSQL("select * from RadioChannelGroup ORDER BY RadioChannelGroup.sortOrder");
    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      CPVRChannelGroup data;

      data.SetGroupID(m_pDS->fv("idGroup").get_asInt());
      data.SetGroupName(m_pDS->fv("groupName").get_asString());
      data.SetSortOrder(m_pDS->fv("sortOrder").get_asInt());

      results.push_back(data);
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return false;
}

bool CTVDatabase::SetRadioChannelGroupName(unsigned int GroupId, const CStdString &newname)
{
  if (!OpenDS())
    return false;

  try
  {
    if (GroupId < 0)   // no match found, update required
    {
      return false;
    }

    CStdString SQL;

    SQL=FormatSQL("select * from RadioChannelGroup WHERE RadioChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update RadioChannelGroup set groupName='%s' WHERE idGroup=%i", newname.c_str(), GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
    return false;
  }
}

bool CTVDatabase::SetRadioChannelGroupSortOrder(unsigned int GroupId, int sortOrder)
{
  if (!OpenDS())
    return false;

  try
  {
    if (GroupId < 0)   // no match found, update required
      return false;

    CStdString SQL;

    SQL=FormatSQL("select * from RadioChannelGroup WHERE RadioChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update RadioChannelGroup set sortOrder='%i' WHERE idGroup=%i", sortOrder, GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
  }
  return -1;
}

long CTVDatabase::GetRadioChannelGroupId(const CStdString &groupname)
{
  CStdString strWhereClause = FormatSQL("groupName LIKE '%s'", groupname.c_str());
  CStdString strReturn = GetSingleValue("RadioChannelGroup", "idGroup", strWhereClause);

  m_pDS->close();

  return atoi(strReturn);
}

bool CTVDatabase::EraseChannelSettings()
{
  return DeleteValues("ChannelSettings");
}

bool CTVDatabase::GetChannelSettings(unsigned int channelID, CVideoSettings &settings)
{
  if (!OpenDS())
    return false;

  try
  {
    if (channelID < 0) return false;

    CStdString strSQL=FormatSQL("select * from ChannelSettings where idChannel like '%u'", channelID);

    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      settings.m_AudioDelay           = m_pDS->fv("AudioDelay").get_asFloat();
      settings.m_AudioStream          = m_pDS->fv("AudioStream").get_asInt();
      settings.m_Brightness           = m_pDS->fv("Brightness").get_asFloat();
      settings.m_Contrast             = m_pDS->fv("Contrast").get_asFloat();
      settings.m_CustomPixelRatio     = m_pDS->fv("PixelRatio").get_asFloat();
      settings.m_NoiseReduction       = m_pDS->fv("NoiseReduction").get_asFloat();
      settings.m_Sharpness            = m_pDS->fv("Sharpness").get_asFloat();
      settings.m_CustomZoomAmount     = m_pDS->fv("ZoomAmount").get_asFloat();
      settings.m_Gamma                = m_pDS->fv("Gamma").get_asFloat();
      settings.m_SubtitleDelay        = m_pDS->fv("SubtitleDelay").get_asFloat();
      settings.m_SubtitleOn           = m_pDS->fv("SubtitlesOn").get_asBool();
      settings.m_SubtitleStream       = m_pDS->fv("SubtitleStream").get_asInt();
      settings.m_ViewMode             = m_pDS->fv("ViewMode").get_asInt();
      settings.m_Crop                 = m_pDS->fv("Crop").get_asBool();
      settings.m_CropLeft             = m_pDS->fv("CropLeft").get_asInt();
      settings.m_CropRight            = m_pDS->fv("CropRight").get_asInt();
      settings.m_CropTop              = m_pDS->fv("CropTop").get_asInt();
      settings.m_CropBottom           = m_pDS->fv("CropBottom").get_asInt();
      settings.m_InterlaceMethod      = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInt();
      settings.m_VolumeAmplification  = m_pDS->fv("VolumeAmplification").get_asFloat();
      settings.m_OutputToAllSpeakers  = m_pDS->fv("OutputToAllSpeakers").get_asBool();
      settings.m_SubtitleCached       = false;
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CTVDatabase::SetChannelSettings(unsigned int channelID, const CVideoSettings &settings)
{
  if (!OpenDS())
    return false;

  try
  {
    if (channelID < 0)
    {
      // no match found, update required
      return false;
    }
    CStdString strSQL;
    strSQL.Format("select * from ChannelSettings where idChannel=%i", channelID);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL=FormatSQL("update ChannelSettings set Deinterlace=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%f,Contrast=%f,Gamma=%f,"
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,Sharpness=%f,NoiseReduction=%f,",
                       settings.m_InterlaceMethod, settings.m_ViewMode, settings.m_CustomZoomAmount, settings.m_CustomPixelRatio,
                       settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay, settings.m_SubtitleOn,
                       settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay,
                       settings.m_OutputToAllSpeakers,settings.m_Sharpness,settings.m_NoiseReduction);
      CStdString strSQL2;
      strSQL2=FormatSQL("Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idChannel=%i\n", settings.m_Crop, settings.m_CropLeft, settings.m_CropRight, settings.m_CropTop, settings.m_CropBottom, channelID);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return true;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL=FormatSQL("insert into ChannelSettings ( idChannel,Deinterlace,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "VolumeAmplification,AudioDelay,OutputToAllSpeakers,Crop,CropLeft,CropRight,CropTop,CropBottom,Sharpness,NoiseReduction)"
                       " values (%i,%i,%i,%f,%f,%i,%i,%f,%i,%f,%f,%f,%f,%f,",
                       channelID, settings.m_InterlaceMethod, settings.m_ViewMode, settings.m_CustomZoomAmount, settings.m_CustomPixelRatio,
                       settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay, settings.m_SubtitleOn,
                       settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay);
      CStdString strSQL2;
      strSQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%f,%f)\n",  settings.m_OutputToAllSpeakers, settings.m_Crop, settings.m_CropLeft, settings.m_CropRight,
                    settings.m_CropTop, settings.m_CropBottom, settings.m_Sharpness, settings.m_NoiseReduction);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (ID=%i) failed", __FUNCTION__, channelID);
    return false;
  }
}
