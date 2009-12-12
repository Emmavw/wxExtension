/******************************************************************************\
* File:          dir.cpp
* Purpose:       Implementation of wxExDirWithListView and wxExDirTool classes
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/config.h>
#include <wx/extension/report/dir.h>
#include <wx/extension/report/listitem.h>
#include <wx/extension/report/listview.h>

wxExDirTool::wxExDirTool(const wxExTool& tool,
  const wxString& fullpath, const wxString& filespec, int flags)
  : wxExDir(fullpath, filespec, flags)
  , m_Statistics()
  , m_Tool(tool)
{
}

void wxExDirTool::OnFile(const wxString& file)
{
  wxExTextFileWithListView report(file, m_Tool);
  report.RunTool();
  m_Statistics += report.GetStatistics();
}

wxExDirWithListView::wxExDirWithListView(wxExListViewFile* listview,
  const wxString& fullpath, const wxString& filespec, int flags)
  : wxExDir(fullpath, filespec, flags)
  , m_ListView(listview)
{
}

void wxExDirWithListView::OnDir(const wxString& dir)
{
  if (wxConfigBase::Get()->ReadBool(_("Add folders"), true))
  {
    wxExListItemWithFileName(m_ListView, dir, GetFileSpec()).Insert();
  }
}

void wxExDirWithListView::OnFile(const wxString& file)
{
  wxExListItemWithFileName(m_ListView, file, GetFileSpec()).Insert();
}
