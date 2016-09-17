//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       logfile.h
//
//  Contents:   Class to log test status on to a File
//
//  Classes:    CLogFile
//
//  History:    07-15-1997   AnandhaG   Created
//
//--------------------------------------------------------------------

#ifndef __LOGFILE_H__
#define __LOGFILE_H__

//+-------------------------------------------------------------------
//
//  Class:      CLogFile
//
//  Purpose:    Creates file and logs status/results to this file
//        The Status file logs all the output while Result file
//        logs only the failures.
//
//  History:    07-15-1997   AnandhaG   Created
//
//--------------------------------------------------------------------

class CLogFile {
//  friend class CSnapin;
//  friend class CComponentDataImpl;

private:
  LPTSTR  m_szStatusFileName;
  LPTSTR  m_szResultFileName;
  HANDLE  m_hStatusLogFile;
  HANDLE  m_hResultLogFile;
  HANDLE  m_hMutex;

private:
  void GetFileName();

public:
  CLogFile(LPTSTR lpszStatusFileName = NULL, LPTSTR lpszResultFileName = NULL);
  virtual ~CLogFile();

  BOOL OpenFile();
  BOOL WriteFile(LPTSTR lpszStringToWrite, BOOL bIsStatusLogFile);
};

#endif // __LOGFILE_H__
