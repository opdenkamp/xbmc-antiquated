/*
* know bugs:
* - when opening a server for the first time with ip adres and the second time
*   with server name, access to the server is denied.
* - when browsing entire network, user can't go back one step
*   share = smb://, user selects a workgroup, user selects a server.
*   doing ".." will go back to smb:// (entire network) and not to workgroup list.
*
* debugging is set to a max of 10 for release builds (see local.h)
*/

#include "../stdafx.h"
#include "smbdirectory.h"
#include "../util.h"
#include "directorycache.h"
#include "localizeStrings.h"
#include "../GUIPassword.h"


CSMBDirectory::CSMBDirectory(void)
{
} 

CSMBDirectory::~CSMBDirectory(void)
{
}

bool CSMBDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]
  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath);

  /* samba isn't thread safe with old interface, always lock */
  CSingleLock lock(smb);

  smb.Init();

  /* we need an url to do proper escaping */
  CURL url(strPath);

  //Separate roots for the authentication and the containing items to allow browsing to work correctly
  CStdString strRoot = strPath, strAuth = smb.URLEncode(url);
  if (!CUtil::HasSlashAtEnd(strRoot)) strRoot += "/";
  if (!CUtil::HasSlashAtEnd(strAuth)) strAuth += "/";

  int fd = OpenDir(strAuth);
  if (fd < 0)
    return false;

  struct smbc_dirent* dirEnt;
  CStdString strFile;

  dirEnt = smbc_readdir(fd);

  while (dirEnt)
  {
    // We use UTF-8 internally, as does SMB
    strFile = dirEnt->name;

    if (!strFile.Equals(".") && !strFile.Equals("..") && !strFile.Right(1).Equals("$"))
    {
     unsigned __int64 iSize = 0;
      bool bIsDir = true;
      __int64 lTimeDate = 0;

      // doing stat on one of these types of shares leaves an open session
      // so just skip them and only stat real dirs / files.
      if ( dirEnt->smbc_type != SMBC_IPC_SHARE &&
           dirEnt->smbc_type != SMBC_FILE_SHARE &&
           dirEnt->smbc_type != SMBC_PRINTER_SHARE &&
           dirEnt->smbc_type != SMBC_COMMS_SHARE &&
           dirEnt->smbc_type != SMBC_WORKGROUP &&
           dirEnt->smbc_type != SMBC_SERVER)
      {
        struct __stat64 info = {0};

        //Make sure we use the authenticated path wich contains any default username
        CStdString strFullName = strAuth + smb.URLEncode(strFile);

        if( smbc_stat(strFullName.c_str(), &info) == 0 )
        {
          bIsDir = (info.st_mode & S_IFDIR) ? true : false;
          lTimeDate = info.st_mtime;
          iSize = info.st_size;
        }
        else
          CLog::Log(LOGERROR, __FUNCTION__" - Failed to stat file %s", strFullName.c_str());

      }

      FILETIME fileTime, localTime;
      LONGLONG ll = Int32x32To64(lTimeDate & 0xffffffff, 10000000) + 116444736000000000;
      fileTime.dwLowDateTime = (DWORD) (ll & 0xffffffff);
      fileTime.dwHighDateTime = (DWORD)(ll >> 32);
      FileTimeToLocalFileTime(&fileTime, &localTime);

      if (bIsDir)
      {
        CFileItem *pItem = new CFileItem(strFile);
        pItem->m_strPath = strRoot;

        // needed for network / workgroup browsing
        // skip if root has already a valid domain and type is not a server
        if ((strRoot.find(';') == -1) &&
            (dirEnt->smbc_type == SMBC_SERVER))
          /*&& (strRoot.find('@') == -1))*/ //Removed to allow browsing even if a user is specified
        {
          // length > 6, which means a workgroup name is specified and we need to
          // remove it. Domain without user is not allowed
          int strLength = strRoot.length();
          if (strLength > 6)
          {
            if (CUtil::HasSlashAtEnd(strRoot))
              pItem->m_strPath = "smb://";
          }
        }
        pItem->m_strPath += dirEnt->name;
        if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';
        pItem->m_bIsFolder = true;
        FileTimeToSystemTime(&localTime, &pItem->m_stTime);
        vecCacheItems.Add(pItem);
        items.Add(new CFileItem(*pItem));
      }
      else
      {
        CFileItem *pItem = new CFileItem(strFile);
        pItem->m_strPath = strRoot + dirEnt->name;
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = iSize;
        FileTimeToSystemTime(&localTime, &pItem->m_stTime);

        vecCacheItems.Add(pItem);
        if (IsAllowed(dirEnt->name)) items.Add(new CFileItem(*pItem));
      }
    }
    dirEnt = smbc_readdir(fd);
  }

  smbc_closedir(fd);
  smb.PurgeEx(CURL(strAuth));

  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath, vecCacheItems);

  return true;
}

int CSMBDirectory::Open(const CStdString& strPath)
{
  smb.Init();
  CStdString strAuth = strPath;

  return OpenDir(strAuth);
}

/// \brief Checks authentication against SAMBA share and prompts for username and password if needed
/// \param strAuth The SMB style path
/// \return SMB file descriptor
int CSMBDirectory::OpenDir(CStdString& strAuth)
{
  int fd = -1;
  int nt_error;
  
  CURL urlIn(strAuth);

  CStdString strPath;
  CStdString strShare = urlIn.GetShareName();	// it's only the server\share we're interested in authenticating

  int iTryAutomatic = 0;
  IMAPPASSWORDS it = g_passwordManager.m_mapSMBPasswordCache.find(strShare);

  if( strShare.IsEmpty() ) 
  { 
    //Reset username/password
    //this is cause when we navigate backwords usernames and passwords
    //will be kept, and can cause problems
    urlIn.SetUserName("");
    urlIn.SetPassword("");

    //Only allow automatic authentication on workgroups/computers
    if( !urlIn.GetHostName().IsEmpty() )
      iTryAutomatic = 2;
  }
  else if(it != g_passwordManager.m_mapSMBPasswordCache.end())
  {
    // if share found in cache use it to supply username and password
    CURL url(it->second);		// map value contains the full url of the originally authenticated share. map key is just the share
    CStdString strPassword = url.GetPassWord();
    CStdString strUserName = url.GetUserName();
    urlIn.SetPassword(strPassword);
    urlIn.SetUserName(strUserName);
  }
  else if( urlIn.GetUserName().IsEmpty() )
  { 
    //No username specified, try to authenticate using default password or anonomously
    if( g_stSettings.m_strSambaDefaultUserName.length() > 0 )
      iTryAutomatic = 2;
    else
      iTryAutomatic = 1;
  }

  
  urlIn.GetURL(strPath);

  // for a finite number of attempts use the following instead of the while loop:
  // for(int i = 0; i < 3, fd < 0; i++)
  while (fd < 0)
  {
    
    if(iTryAutomatic)
    { 
      //Try using default username/password if available
      if ( iTryAutomatic == 2 )
      {
        urlIn.SetUserName(g_stSettings.m_strSambaDefaultUserName);
        urlIn.SetPassword(g_stSettings.m_strSambaDefaultPassword);
      }
      
      //Try anonomously
      if( iTryAutomatic == 1 )
      {
        urlIn.SetUserName("");
        urlIn.SetPassword("");
      }

      urlIn.GetURL(strPath);
      iTryAutomatic--;
    }

    // remove the / or \ at the end. the samba library does not strip them off
    // don't do this for smb:// !!
    CStdString s = strPath;
    int len = s.length();
    if (len > 1 && s.at(len - 2) != '/' &&
        (s.at(len - 1) == '/' || s.at(len - 1) == '\\'))
    {
      s.erase(len - 1, 1);
    }
    
    { CSingleLock lock(smb);
      fd = smbc_opendir(s.c_str());
    }
    
    if (fd < 0)
    {
      int error = errno;
      if (error == ENODEV) nt_error = NT_STATUS_INVALID_COMPUTER_NAME;
      else if (error == ENETUNREACH) nt_error = NT_STATUS_INVALID_COMPUTER_NAME;
      else nt_error = map_nt_error_from_unix(error);

      // if we have an 'invalid handle' error we don't display the error
      // because most of the time this means there is no cdrom in the server's
      // cdrom drive.
      if (nt_error == 0xc0000008)
        break;

      // NOTE: be sure to warn in XML file about Windows account lock outs when too many attempts
      // if the error is access denied, prompt for a valid user name and password
      if (nt_error == 0xc0000022)
      {
        //if there is more automatic tries left, just continue
        if( iTryAutomatic ) 
          continue;

        if (m_allowPrompting)
        {
          g_passwordManager.SetSMBShare(strPath);
          g_passwordManager.GetSMBShareUserPassword();  // Do this bit via a threadmessage?
          if (g_passwordManager.IsCanceled())
          	break;
          strPath = g_passwordManager.GetSMBShare();
        }
        else
          break;
      }
      else
      {
        CStdString cError;
        if (nt_error == 0xc0000034)
          cError.Format(g_localizeStrings.Get(770).c_str(),nt_error);
        else
          cError = get_friendly_nt_error_msg(nt_error);
        
        if (m_allowPrompting)
        {
          CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          pDialog->SetHeading(257);
          pDialog->SetLine(0, cError);
          pDialog->SetLine(1, "");
          pDialog->SetLine(2, "");

          ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
          g_applicationMessenger.SendMessage(tMsg, false);
        }
        break;
      }
    }
  }

  if (fd < 0)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'",
              strPath.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
  }
  else if (strPath != strAuth && !strShare.IsEmpty()) // we succeeded so, if path was changed, return the correct one and cache it
  {
    g_passwordManager.m_mapSMBPasswordCache[strShare] = strPath;
    strAuth = strPath;
  }

  return fd;
}

bool CSMBDirectory::Create(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  int result = smbc_mkdir(strFileName.c_str(), 0);
  return (result == 0 || EEXIST == result);
}

bool CSMBDirectory::Remove(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  int result = smbc_rmdir(strFileName.c_str());
  return (result == 0);
}

bool CSMBDirectory::Exists(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  SMB_STRUCT_STAT info;
  if (smbc_stat(strFileName.c_str(), &info) != 0)
    return false;

  return (info.st_mode & S_IFDIR) ? true : false;
}
