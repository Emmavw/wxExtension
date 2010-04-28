////////////////////////////////////////////////////////////////////////////////
// Name:      stat.cpp
// Purpose:   Implementation of wxExStat class.
// Author:    Anton van Wezenbeek
// Created:   2010-04-28
// RCS-ID:    $Id$
// Copyright: (c) 2010 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/stat.h>

wxExStat::wxExStat(const wxString& fullpath) 
{
  Sync(fullpath);
}

bool wxExStat::Sync() 
{
#ifdef __UNIX__
    m_IsOk = (::stat(m_FullPath.c_str(), this) != -1);
#else
    m_IsOk = (stat(m_FullPath.c_str(), this) != -1);
#endif
  return m_IsOk;
}

bool wxExStat::Sync(const wxString& fullpath) 
{
  m_FullPath = fullpath;
  return Sync();
}
