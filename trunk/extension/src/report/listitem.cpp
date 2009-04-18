/******************************************************************************\
* File:          listitem.cpp
* Purpose:       Implementation of class 'exListItemWithFileName'
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2008 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/filetool/listitem.h>
#include <wx/filetool/textfile.h>
#include <wx/filetool/util.h>

// Do not give an error if columns do not exist.
// E.g. the LIST_PROCESS has none of the file columns.
exListItemWithFileName::exListItemWithFileName(exListView* lv, const int itemnumber)
  : exListItem(lv, itemnumber)
  , m_Statistics(
      (!GetColumnText(_("File Name"), false).empty() ? 
          GetColumnText(_("In Folder"), false) + wxFileName::GetPathSeparator() +
          GetColumnText(_("File Name"), false) : wxString(wxEmptyString))
      )
  , m_FileSpec(GetColumnText(_("Type"), false))
{
}

exListItemWithFileName::exListItemWithFileName(
  exListView* listview,
  const wxString& fullpath,
  const wxString& filespec)
  : exListItem(listview, -1)
  , m_Statistics(fullpath)
  , m_FileSpec(filespec)
{
}

void exListItemWithFileName::Insert(long index)
{
  SetId(index == -1 ? GetListView()->GetItemCount(): index);
  const long col = GetListView()->FindColumn(_("File Name"), false);
  const wxString filename = (
    m_Statistics.FileExists() || m_Statistics.DirExists() ? 
      m_Statistics.GetFullName(): 
      m_Statistics.GetFullPath());

  if (col == 0)
  {
    SetColumn(col); // do not combine this with next statement in SetColumnText!!
    SetText(filename);
  }

  GetListView()->InsertItem(*this);
  GetListView()->UpdateStatusBar();

  SetImage(m_Statistics.GetIcon());

  Update();

  if (col > 0)
  {
    SetColumnText(col, filename);
  }
}

bool exListItemWithFileName::Run(const exTool& tool, exListViewFile* listview)
{
  exFrame::StatusText(m_Statistics.GetFullPath());

  if (m_Statistics.FileExists())
  {
    exTextFileWithReport file(m_Statistics);

    if (file.RunTool(tool))
    {
      if (tool.IsRCSType())
      {
        Update();
      }

      m_Statistics += file.GetStatistics();

      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    exDirWithReport dir(listview, m_Statistics.GetFullPath(), m_FileSpec);

    if (dir.RunTool(tool))
    {
      m_Statistics += dir.GetStatistics();

      // Here we show the counts of individual folders on the top level.
      if (tool.IsCountType() && GetListView()->GetSelectedItemCount() > 1)
      {
        m_Statistics.Log(tool);
      }

      return true;
    }
    else
    {
      return false;
    }
  }
}

void exListItemWithFileName::Update()
{
  // Update readonly state in listview item data.
  // SetData does not work, as list items are constructed/destructed a lot.
  GetListView()->SetItemData(GetId(), m_Statistics.GetStat().IsReadOnly());

  const int fontstyle = (m_Statistics.GetStat().IsReadOnly() ? wxFONTSTYLE_ITALIC: wxFONTSTYLE_NORMAL);

  wxFont font(
    exApp::GetConfig(_("List Font") + "/Size", 10),
    wxFONTFAMILY_DEFAULT,
    fontstyle,
    wxFONTWEIGHT_NORMAL,
    false,
    exApp::GetConfig(_("List Font") + "/Name", "courier new"));

  SetFont(font);
  GetListView()->SetItem(*this);

  if (m_Statistics.GetStat().IsLink())
  {
    // Do something if this is a link. Currently nothing is done.
  }

  if (m_Statistics.FileExists() ||
      wxFileName::DirExists(m_Statistics.GetFullPath()))
  {
    const unsigned long size = m_Statistics.GetStat().st_size; // to prevent warning
    SetColumnText(_("Type"),
      (wxFileName::DirExists(m_Statistics.GetFullPath()) ?
         m_FileSpec:
         m_Statistics.GetExt()));
    SetColumnText(_("In Folder"), m_Statistics.GetPath());
    SetColumnText(_("Size"),
      (!wxFileName::DirExists(m_Statistics.GetFullPath()) ?
         (wxString::Format("%lu", size)):
          wxString(wxEmptyString)));
    SetColumnText(_("Modified"), m_Statistics.GetStat().GetModificationTime());
  }
}

void exListItemWithFileName::UpdateRevisionList(const exRCS& rcs)
{
  SetColumnText(_("Revision"), rcs.GetRevisionNumber());
  SetColumnText(_("Date"), rcs.GetRevisionTime().Format());
  SetColumnText(_("Initials"), rcs.GetUser());
  SetColumnText(_("Revision Comment"), rcs.GetDescription());
}
