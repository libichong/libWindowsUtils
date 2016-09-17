//+--------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       logfile.cpp
//
//  Contents:   Implementation of class to log results to a file.
//
//  Classes:    CLogFile
//
//  History:    07-15-1997   AnandhaG   Created
//
//---------------------------------------------------------------------

#include "stdafx.h"
#include "logfile.h"
#include "conv.h"

// Max file name length
#define   MAX_FILE_NAME_LEN 256
// Max # we wait for a locked file to be released ( MAX_LOCK_WAIT_ITER times 50ms)
#define   MAX_LOCK_WAIT_ITER  20
#define   MUTEX_NAME    _T("MutexForLoggingFile")

//+-------------------------------------------------------------------
//
//  Member:     CLogFile::CLogFile
//
//  Synopsis:   ctor
//
//  Arguments:  lpszStatusFileName and lpszResultFileNAme if these parameters were
//        NULL a new file name is generated.
//        Otherwise the filename passed is used.
//
//  History:    07-15-1997   AnandhaG   Created
//
//--------------------------------------------------------------------
CLogFile::CLogFile(LPTSTR lpszStatusFileName, LPTSTR lpszResultFileName)
{
  if (lpszStatusFileName == NULL && lpszResultFileName == NULL)
    GetFileName();

  m_hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

  if (NULL == m_hMutex) {
    m_hMutex = OpenMutex(NULL, FALSE, MUTEX_NAME);
    if (NULL == m_hMutex) {
      printf("Mutex could not be opened");
      exit(0);
    }
  }
}

//+-------------------------------------------------------------------
//
//  Member:     CLogFile::~CLogFile
//
//  Synopsis:   dtor
//
//  History:    07-15-1997   AnandhaG   Created
//
//  Notes:    The string m_szStatusFileName and m_szResultFileName must
//        be freed and the handle m_hStatusLogFile and
//        m_hResultLogFile must be closed.
//--------------------------------------------------------------------
CLogFile::~CLogFile()
{
  if (m_szStatusFileName)
    delete[] m_szStatusFileName;

  if (m_szResultFileName)
    delete[] m_szResultFileName;

  CloseHandle(m_hStatusLogFile);
  CloseHandle(m_hResultLogFile);
  CloseHandle(m_hMutex);
}

//+--------------------------------------------------------------------
//
//  Member:     CLogFile::GetFileName
//
//  Synopsis:   Generate a file name in the form of COMPUTERNAME(DATE).mmc
//        DATE will be of form MON_DAY_YEAR
//
//  Arguments:  None
//
//  Returns:    void
//
//  Modifies:   m_szStatusFileName, m_szResultFileName
//
//  History:    07-15-1997   AnandhaG   Created
//
//  Notes:      The dtor should free the space allocated to the file name
//
//---------------------------------------------------------------------
void CLogFile::GetFileName()
{
  TCHAR szCompName[MAX_COMPUTERNAME_LENGTH + 1];
  LPSTR szCompName1;
  TCHAR szTemp[MAX_FILE_NAME_LEN];
  DWORD dwCount = MAX_COMPUTERNAME_LENGTH;
  CTime t;

  // Get the computer name in the first part of szTemp
  if (!GetComputerName(szCompName, &dwCount)) {
    if (ERROR_BUFFER_OVERFLOW == GetLastError()){
      szCompName1 = getenv("COMPUTERNAME");
      CA2T lpszTemp (szCompName1);
      _tcscpy(szCompName, lpszTemp);
    }
  }

  t = CTime::GetCurrentTime();

  _stprintf(szTemp, _T("(%d_%d_%d)"),
          t.GetMonth(), t.GetDay(), t.GetYear()-1900);

  // Allocate space, a count of 5 is added for extension .sta and NULL char.
  m_szStatusFileName = new TCHAR[lstrlen(szCompName) + lstrlen(szTemp) + 5];
  _stprintf(m_szStatusFileName, _T("%s%s.sta"), szCompName, szTemp);

  // Allocate space, a count of 5 is added for extension .rst and NULL char.
  m_szResultFileName = new TCHAR[lstrlen(szCompName) + lstrlen(szTemp) + 5];
  _stprintf(m_szResultFileName, _T("%s%s.rst"), szCompName, szTemp);

  return;
}

//+--------------------------------------------------------------------
//
//  Member:     CLogFile::OpenFile
//
//  Synopsis:   Open the file if it exists else create a new one.
//
//  Arguments:  None
//
//  Returns:    HANDLE of the opened/created file
//
//  Modifies:   None
//
//  History:    07-15-1997   AnandhaG   Created
//
//  Notes:      None
//
//---------------------------------------------------------------------
BOOL CLogFile::OpenFile()
{
  // Open or Create the file m_szFileName
  m_hStatusLogFile = ::CreateFile(m_szStatusFileName, GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_ALWAYS,
                      0, NULL);

  if (INVALID_HANDLE_VALUE == m_hStatusLogFile) {
    printf("Invalid Status Handle while opening the file\n");
    exit(0);
  }

  ::SetFilePointer(m_hStatusLogFile, 0, 0, FILE_END);

  m_hResultLogFile = ::CreateFile(m_szResultFileName, GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_ALWAYS,
                      0, NULL);

  if (INVALID_HANDLE_VALUE == m_hResultLogFile) {
    printf("Invalid Result Handle while opening the file\n");
    exit(0);
  }

  ::SetFilePointer(m_hResultLogFile, 0, 0, FILE_END);

  return TRUE;
}

//+--------------------------------------------------------------------
//
//  Member:     CLogFile::WriteFile
//
//  Synopsis:   This method locks the file and then writes
//
//  Arguments:  [lpszStringToWrite]  - The string that is to be written.
//
//  Returns:    BOOL indicating success or failure.
//
//  Modifies:   None
//
//  History:    07-15-1997   AnandhaG   Created
//
//  Notes:      None
//
//---------------------------------------------------------------------
BOOL CLogFile::WriteFile(LPTSTR lpszStringToWrite, BOOL bIsStatusLogFile)
{
  DWORD dwFileSize;
  DWORD dwLockRegionSize;
  DWORD dwStringSize;
  DWORD dwWritten = 0;
  DWORD dwError;
  BOOL  fLocked;
  DWORD dwRetVal;
  INT   nLockCount = 0;
  HANDLE  hLogFile;

  hLogFile = bIsStatusLogFile ? m_hStatusLogFile : m_hResultLogFile;

  dwStringSize = lstrlen(lpszStringToWrite);
  dwLockRegionSize = dwStringSize;

  while (TRUE) {
    dwRetVal = WaitForSingleObject(m_hMutex, INFINITE);

    if (WAIT_OBJECT_0 == dwRetVal) {
      dwFileSize = ::GetFileSize(hLogFile, NULL);
      if (0xFFFFFFFF == dwFileSize) {
        ReleaseMutex(m_hMutex);
        exit(0);
      }

      fLocked = ::LockFile(hLogFile, dwFileSize, 0, dwLockRegionSize, 0);
      if (fLocked) {
        ::SetFilePointer(hLogFile, 0, 0, FILE_END);
        CT2A lpszTemp (lpszStringToWrite);
        ::WriteFile(hLogFile, (LPCVOID) lpszTemp, dwStringSize, &dwWritten, NULL);
        fLocked = ::UnlockFile(hLogFile, dwFileSize, 0, dwLockRegionSize, 0);
        ReleaseMutex(m_hMutex);
        break;
      } // if (fLocked)
      else {
        dwError = GetLastError();
        printf("How can I aquire mutex and could not lock file: Error Code is: %d", dwError);
        ReleaseMutex(m_hMutex);
        break;
      }
    }
    else {
      printf("Could not wait!!!");
    } // if (WAIT_OBJECT_0 == dwRetVal)
  } // While

  return fLocked;
}
