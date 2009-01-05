/******************************************************************************\
* File:          listview.cpp
* Purpose:       Implementation of class 'ftListView' and support classes
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2008 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/tokenzr.h>
#include <wx/extension/svn.h>
#include <wx/filetool/filetool.h>
#include <wx/filetool/process.h>

#if wxUSE_DRAG_AND_DROP
class ftListViewDropTarget : public wxFileDropTarget
{
public:
  ftListViewDropTarget(ftListView* owner) {m_Owner = owner;}
private:
  virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
  ftListView* m_Owner;
};
#endif

#ifdef __WXMSW__
class ftRBSFile : public exFile
{
public:
  ftRBSFile(ftListView* listview);
  void GenerateDialog();
private:
  void Body(
    const wxString& filename,
    const wxString& source,
    const wxString& destination);
  void Footer();
  void GenerateTransmit(const wxString& text);
  void GenerateWaitFor(const wxString& text);
  void Header();
  bool Substitute(
    wxString& text,
    const wxString& pattern,
    const wxString& new_pattern,
    const bool is_required);
  ftListView* m_Owner;
  wxString m_Prompt;
};
#endif

class ftToolThread : public wxThread
{
public:
  ftToolThread(const exTool& tool, ftListView* list) 
    : wxThread(wxTHREAD_JOINABLE)
    , m_ListView(list)
    , m_Tool(tool) {}
protected:
  virtual ExitCode Entry()
  {
    int i = -1;

    exFileNameStatistics stats(m_ListView->GetFileName().GetName());

    while (!TestDestroy())
    {
      if (m_ListView->GetSelectedItemCount() == 0)
      {
        i = i + 1;
        if (i >= m_ListView->GetItemCount()) break;
      }
      else
      {
        i = m_ListView->GetNextSelected(i);
        if (i == -1) break;
      }

      ftListItem item(m_ListView, i);
      item.Run(m_Tool, m_ListView);

      stats += item.GetStatistics();

      Yield();

      Sleep(20);
    }

    stats.Log();

    return NULL;
  };
private:
  ftListView* m_ListView;
  const exTool& m_Tool;
};

const int ID_LISTVIEW = 100;

BEGIN_EVENT_TABLE(ftListView, exListView)
  EVT_IDLE(ftListView::OnIdle)
  EVT_LIST_ITEM_ACTIVATED(ID_LISTVIEW, ftListView::OnList)
  EVT_LIST_ITEM_SELECTED(ID_LISTVIEW, ftListView::OnList)
  EVT_MENU_RANGE(wxID_CUT, wxID_PROPERTIES, ftListView::OnCommand)
  EVT_MENU_RANGE(ID_LIST_LOWEST, ID_LIST_HIGHEST, ftListView::OnCommand)
  EVT_MENU_RANGE(ID_TOOL_LOWEST, ID_TOOL_HIGHEST, ftListView::OnCommand)
  EVT_LEFT_DOWN(ftListView::OnMouse)
  EVT_RIGHT_DOWN(ftListView::OnMouse)
  EVT_TIMER(-1, ftListView::OnTimer)
END_EVENT_TABLE()

ftProcess* ftListView::m_Process = NULL;

ftListView::ftListView(wxWindow* parent,
  ftListType type,
  long menu_flags,
  const exLexer* lexer,
  const wxPoint& pos,
  const wxSize& size,
  long style,
  const wxValidator& validator)
  : exListView(parent, ID_LISTVIEW, pos, size, style, validator, IMAGE_FILE_ICON)
  , exFile()
  , m_Thread(NULL)
  , m_ContentsChanged(false)
  , m_ItemUpdated(false)
  , m_ItemNumber(0)
  , m_MenuFlags(menu_flags)
  , m_Type(type)
{
  Initialize(lexer);
}

ftListView::ftListView(wxWindow* parent,
  const wxString& file,
  const wxString& wildcard,
  long menu_flags,
  const wxPoint& pos,
  const wxSize& size,
  long style,
  const wxValidator& validator)
  : exListView(parent, ID_LISTVIEW, pos, size, style, validator, IMAGE_FILE_ICON)
  , exFile()
  , m_Thread(NULL)
  , m_ContentsChanged(false)
  , m_ItemUpdated(false)
  , m_ItemNumber(0)
  , m_MenuFlags(menu_flags)
  , m_Type(LIST_PROJECT)
{
  Initialize(NULL);
  SetWildcard(wildcard);
  FileOpen(file);
}

int ftListView::AddItems()
{
  std::vector<exConfigItem> v;
  v.push_back(exConfigItem(_("Add what"), CONFIG_COMBOBOX, wxEmptyString, true));
  v.push_back(exConfigItem(_("In folder"), CONFIG_COMBOBOXDIR, wxEmptyString, true));
  v.push_back(exConfigItem(_("Add files"), CONFIG_CHECKBOX));
  v.push_back(exConfigItem(_("Add folders"), CONFIG_CHECKBOX));

  if (exConfigDialog(NULL,
    v,
    _("Add Files")).ShowModal() == wxID_CANCEL)
  {
    return 0;
  }

  ftDir dir(
    this,
    exApp::GetConfig(_("In folder")),
    exApp::GetConfig(_("Add what")));

  int flags = 0;
  if (exApp::GetConfigBool(_("Add files"))) flags |= wxDIR_FILES;
  if (exApp::GetConfigBool(_("Add folders"))) flags |= wxDIR_DIRS;

  const int retValue = dir.FindFiles(flags);

  if (retValue > 0)
  {
    m_ContentsChanged = true;
  }

  if (exApp::GetConfigBool("List/SortSync") && m_Type == LIST_PROJECT)
  {
    SortColumn(exApp::GetConfig(
      "List/SortColumn",
      FindColumn(_("Modified"))), SORT_KEEP);
  }

  const wxString text = _("Added") + wxString::Format(" %d ", retValue) + _("file(s)");

  exFrame::StatusText(text);

  return retValue;
}

void ftListView::AfterSorting()
{
  // Only if we are not sort syncing, set contents changed.
  if (!exApp::GetConfigBool("List/SortSync"))
  {
    m_ContentsChanged = true;
  }
}

void ftListView::BuildPopupMenu(exMenu& menu)
{
  bool add = false;
  bool exists = true;
  bool is_folder = false;
  bool is_make = false;
  bool read_only = false;

  if (GetSelectedItemCount() == 1)
  {
    ftListItem item(this, GetFirstSelected());

    is_folder = wxDirExists(item.GetFileName().GetFullPath());
    exists = item.GetFileName().FileExists() || is_folder;
    read_only = item.GetFileName().GetStat().IsReadOnly();
    is_make = item.GetFileName().GetLexer().GetScintillaLexer() == "makefile";
  }

  if (GetSelectedItemCount() >= 1)
  {
    if (exists)
    {
      menu.Append(ID_LIST_OPEN_ITEM, _("&Open"), wxART_FILE_OPEN);

      if (is_make)
      {
        menu.Append(ID_LIST_RUN_MAKE, _("&Make"));
      }
    }

    if (GetSelectedItemCount() > 1)
    {
      menu.Append(ID_LIST_COMPARE, _("C&ompare") + "\tCtrl+N");
    }
    else
    {
      if (m_Type != LIST_PROJECT && 
          !exApp::GetConfigBool("SVN") &&
          m_Frame != NULL && exists && !is_folder)
      {
        ftListView* list = m_Frame->Activate(LIST_PROJECT);
        if (list != NULL && list->GetSelectedItemCount() == 1)
        {
          ftListItem thislist(this, GetFirstSelected());
          const wxString current_file = thislist.GetFileName().GetFullPath();

          ftListItem otherlist(list, list->GetFirstSelected());
          const wxString with_file = otherlist.GetFileName().GetFullPath();

          if (current_file != with_file)
            menu.Append(ID_LIST_COMPARE,
              _("&Compare With") + " " + exGetEndOfWord(with_file));
        }
      }
    }

    if (exists && !is_folder)
    {
      if (!exApp::GetConfigBool("SVN"))
      {
        menu.Append(ID_LIST_COMPARELAST, _("&Compare Recent Version") + "\tCtrl+M");
        menu.Append(ID_LIST_VERSIONLIST, _("&Version List"));
      }
      else if (GetSelectedItemCount() == 1)
      {
        wxMenu* svnmenu = new wxMenu;
        svnmenu->Append(ID_LIST_SVN_DIFF, exEllipsed(_("&Diff")));
        svnmenu->Append(ID_LIST_SVN_LOG, exEllipsed(_("&Log")));
        svnmenu->Append(ID_LIST_SVN_CAT, exEllipsed(_("&Cat")));
        menu.Append(-1, "&SVN", wxEmptyString, wxITEM_NORMAL, svnmenu);
      }
    }

#ifdef __WXMSW__
    if (exists && !is_folder && (m_MenuFlags & FT_LISTVIEW_RBS))
    {
      menu.AppendSeparator();
      menu.Append(ID_LIST_SEND_ITEM, exEllipsed(_("&Build RBS File")));
      menu.AppendSeparator();
    }
#endif
  }
  else
  {
    if (m_Type == LIST_PROJECT)
    {
      if (!m_FileName.IsOk() ||
          !m_FileName.FileExists() ||
          (m_FileName.FileExists() && !m_FileName.GetStat().IsReadOnly()))
      {
        add = true;
      }
    }
  }

  // The ID_TOOL_REPORT_FIND and REPLACE only work if there is a frame, so if not do not add them.
  // Also, finding in the LIST_FIND and REPLACE would result in recursive calls.
  if (m_Frame != NULL && exists &&
      m_Type != LIST_FIND && m_Type != LIST_REPLACE)
  {
    if (GetItemCount() > 0 && (
        m_MenuFlags & FT_LISTVIEW_REPORT_FIND))
    {
      menu.Append(ID_TOOL_REPORT_FIND, exEllipsed(GetFindInCaption(ID_TOOL_REPORT_FIND)));

      if (!read_only)
      {
        menu.Append(ID_TOOL_REPORT_REPLACE, exEllipsed(GetFindInCaption(ID_TOOL_REPORT_REPLACE)));
      }

      menu.AppendSeparator();
    }
  }

  if (add)
  {
    menu.Append(ID_LIST_ADD_ITEM, exEllipsed(_("&Add Files")));
    menu.AppendSeparator();
  }

  exListView::BuildPopupMenu(menu);

  if (m_Frame != NULL && GetItemCount() > 0 && exists &&
     (m_MenuFlags & FT_LISTVIEW_TOOL))
  {
    menu.AppendSeparator();
    menu.AppendTools(ID_LIST_TOOL_MENU);
  }
}

void ftListView::CleanUp()
{
  wxDELETE(m_Process);
}

void ftListView::DeleteDoubles()
{
  wxDateTime mtime((time_t)0);
  wxString name;
  const int itemcount = GetItemCount();

  for (int i = itemcount - 1; i >= 0; i--)
  {
    ftListItem item(this, i);

    // Delete this element if it has the same mtime
    // and the same name as the previous one.
    if (mtime == item.GetFileName().GetStat().st_mtime &&
        name == item.GetColumnText(_("File Name")))
      DeleteItem(i);
    else
    {
      mtime = item.GetFileName().GetStat().st_mtime;
      name = item.GetColumnText(_("File Name"));
    }
  }

  if (itemcount != GetItemCount())
  {
    ItemsUpdate();
    m_ContentsChanged = true;
  }
}

bool ftListView::FileNew(const exFileName& filename)
{
  if (!exFile::FileNew(filename))
  {
    return false;
  }

  EditClearAll();
  return true;
}

bool ftListView::FileOpen(const exFileName& filename)
{
  if (!exFile::FileOpen(filename))
  {
    return false;
  }

  EditClearAll();

  wxString* buffer = Read();
  if (buffer == NULL) return false;

  wxStringTokenizer tkz(*buffer, wxTextFile::GetEOL());
  delete buffer;

  while (tkz.HasMoreTokens())
  {
    ItemFromText(tkz.GetNextToken());
  }

  wxFile::Close();

  if (m_Frame != NULL)
  {
    m_Frame->SetRecentProject(filename.GetFullPath());
  }

  m_ContentsChanged = false; // override behaviour from ItemFromText

  if (exApp::GetConfigBool("List/SortSync"))
    SortColumn(exApp::GetConfig("List/SortColumn", FindColumn(_("Modified"))), SORT_KEEP);
  else
    SortColumnReset();

  return true;
}

bool ftListView::FileSave()
{
  if (m_FileName.GetName().empty())
  {
    return FileSaveAs();
  }

  if (!wxFile::Open(m_FileName.GetFullPath(), wxFile::write))
  {
    return false;
  }

  for (int i = 0; i < GetItemCount(); i++)
  {
    Write(ItemToText(i) + wxTextFile::GetEOL());
  }

  exFile::FileSave();

  m_ContentsChanged = false;

  return true;
}

const wxString ftListView::GetFindInCaption(int id)
{
  const wxString prefix =
    (id == ID_TOOL_REPORT_REPLACE ?
       _("Replace In"):
       _("Find In")) + " ";

  if (GetSelectedItemCount() == 1)
  {
    exListItem item(this, GetFirstSelected());

    // The File Name is better than using 0, as it can be another column as well.
    return prefix + item.GetColumnText(_("File Name"));
  }
  else if (GetSelectedItemCount() > 1)
  {
    return prefix + _("Selection");
  }
  else if (!m_FileName.GetName().empty())
  {
    return prefix + m_FileName.GetName();
  }
  else
  {
    return prefix + GetName();
  }
}

int ftListView::GetTypeTool(const exTool& tool)
{
  switch (tool.GetId())
  {
    case ID_TOOL_REPORT_COUNT: return LIST_COUNT; break;
    case ID_TOOL_REPORT_FIND: return LIST_FIND; break;
    case ID_TOOL_REPORT_HEADER:  return LIST_HEADER; break;
    case ID_TOOL_REPORT_KEYWORD: return LIST_KEYWORD; break;
    case ID_TOOL_REPORT_REPLACE: return LIST_REPLACE; break;
    case ID_TOOL_REPORT_REVISION: return LIST_REVISION; break;
    case ID_TOOL_REPORT_SQL: return LIST_SQL; break;
    case ID_TOOL_REPORT_VERSION: return LIST_VERSION; break;
    default: wxLogError(FILE_INFO("Unhandled id: %d"), tool.GetId()); return LIST_PROJECT;
  }
}

const wxString ftListView::GetTypeDescription(ftListType type)
{
  wxString value;

  switch (type)
  {
  case LIST_COUNT: value = _("File Count"); break;
  case LIST_FIND: value = _("Find Results"); break;
  case LIST_HEADER: value = _("Headers"); break;
  case LIST_HISTORY: value = _("History"); break;
  case LIST_KEYWORD: value = _("Keywords"); break;
  case LIST_PROCESS: value = _("Process Output"); break;
  case LIST_PROJECT: value = _("Project"); break;
  case LIST_REPLACE: value = _("Replace Results"); break;
  case LIST_REVISION: value = _("Revisions"); break;
  case LIST_SQL: value = _("SQL Queries"); break;
  case LIST_VERSION: value = _("Version List"); break;
  default: wxLogError(FILE_INFO("Unhandled list type: %d"), type);
  }

  return value;
}

void ftListView::Initialize(const exLexer* lexer)
{
  SetName(GetTypeDescription());

  if (m_Type == LIST_KEYWORD)
  {
    if (lexer == NULL)
    {
      wxLogError(FILE_INFO("Lexer should be specified for a keyword list"));
      return;
    }

    SetName(GetName() + " " + lexer->GetScintillaLexer());
  }

  wxWindow* window = wxTheApp->GetTopWindow();
  m_Frame = wxDynamicCast(window, ftFrame);

#ifndef __WXMSW__
  // Under Linux this should be done before adding any columns, under MSW it does not matter!
  SetSingleStyle(wxLC_REPORT);
#else
  // Set initial style depending on type. 
  // Might be improved.
  SetSingleStyle((m_Type == LIST_PROJECT || m_Type == LIST_HISTORY ?
    exApp::GetConfig("List/Style", wxLC_REPORT) : 
    wxLC_REPORT));
#endif

  if (m_Type != LIST_PROCESS)
  {
    InsertColumn(_("File Name"), exColumn::COL_STRING);
  }

  switch (m_Type)
  {
  case LIST_COUNT:
    // See ftTextFile::Report, the order in which columns are set should be the same there.
    InsertColumn(_("Lines"));
    InsertColumn(_("Lines Of Code"));
    InsertColumn(_("Empty Lines"));
    InsertColumn(_("Words Of Code"));
    InsertColumn(_("Comments"));
    InsertColumn(_("Comment Size"));
  break;
  case LIST_FIND:
  case LIST_REPLACE:
    InsertColumn(_("Line"), exColumn::COL_STRING, 400);
    InsertColumn(_("Match"), exColumn::COL_STRING);
    InsertColumn(_("Line No"));
  break;
  case LIST_HEADER:
    InsertColumn(_("Header Description"), exColumn::COL_STRING, 400);
  break;
  case LIST_KEYWORD:
    for (
      std::set<wxString>::const_iterator it = lexer->GetKeywords().begin();
      it != lexer->GetKeywords().end();
      ++it)
    {
      InsertColumn(*it);
    }

    InsertColumn(_("Keywords"));
  break;
  case LIST_PROCESS:
    InsertColumn(_("Line"), exColumn::COL_STRING, 500);
    InsertColumn(_("Line No"));
    InsertColumn(_("File Name"), exColumn::COL_STRING);
  break;
  case LIST_REVISION:
    InsertColumn(_("Revision Comment"), exColumn::COL_STRING, 400);
    InsertColumn(_("Date"), exColumn::COL_DATE);
    InsertColumn(_("Initials"), exColumn::COL_STRING);
    InsertColumn(_("Line No"));
    InsertColumn(_("Revision"), exColumn::COL_STRING);
  break;
  case LIST_SQL:
    InsertColumn(_("Run Time"), exColumn::COL_DATE);
    InsertColumn(_("Query"), exColumn::COL_STRING, 400);
    InsertColumn(_("Line No"));
  break;
  default: break; // to prevent warnings
  }

  if (m_Type == LIST_REPLACE)
  {
    InsertColumn(_("Replaced"));
  }

  InsertColumn(_("Modified"), exColumn::COL_DATE);
  InsertColumn(_("In Folder"), exColumn::COL_STRING, 175);
  InsertColumn(_("Type"), exColumn::COL_STRING);
  InsertColumn(_("Size"));

  if (m_Type == LIST_HISTORY && m_Frame != NULL)
  {
    m_Frame->UseFileHistoryList(this);
  }

#if wxUSE_DRAG_AND_DROP
  SetDropTarget(new ftListViewDropTarget(this));
#endif

  // TODO: Because we use our own accel, the accels from exListView
  // are copied to, otherwise they are not inherited!
  const int accels = 9;
  wxAcceleratorEntry entries[accels];
  entries[0].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_DELETE);
  entries[1].Set(wxACCEL_CTRL, WXK_INSERT, wxID_COPY);
  entries[2].Set(wxACCEL_SHIFT, WXK_INSERT, wxID_PASTE);
  entries[3].Set(wxACCEL_SHIFT, WXK_DELETE, wxID_CUT);
  entries[4].Set(wxACCEL_NORMAL, WXK_F3, ID_LIST_FIND_NEXT);
  entries[5].Set(wxACCEL_NORMAL, WXK_F4, ID_LIST_FIND_PREVIOUS);
  entries[6].Set(wxACCEL_NORMAL, WXK_F5, ID_LIST_FIND);
  entries[7].Set(wxACCEL_CTRL, 'N', ID_LIST_COMPARE);
  entries[8].Set(wxACCEL_CTRL, 'M', ID_LIST_COMPARELAST);

  wxAcceleratorTable accel(accels, entries);
  SetAcceleratorTable(accel);
}

bool ftListView::ItemOpenFile(int item_number)
{
  if (item_number < 0) return false;

  ftListItem item(this, item_number);

  if (wxFileName::DirExists(item.GetFileName().GetFullPath()))
  {
    wxTextEntryDialog dlg(this,
      _("Input") + ":",
      _("Folder Type"),
      item.GetColumnText(_("Type")));

    if (dlg.ShowModal() == wxID_CANCEL)
    {
      return false;
    }
    else
    {
      item.SetColumnText(_("Type"), dlg.GetValue());
      return true;
    }
  }
  else if (item.GetFileName().FileExists() && m_Frame != NULL)
  {
    const wxString line_number_str = item.GetColumnText(_("Line No"), false);
    const int line_number = (!line_number_str.empty() ? atoi(line_number_str.c_str()): 0);
    const wxString match = 
      (m_Type == LIST_REPLACE ? 
         item.GetColumnText(_("Replaced")): 
         item.GetColumnText(_("Match"), false));

    const bool retValue = m_Frame->OpenFile(
      item.GetFileName().GetFullPath(), 
      line_number, match);

    SetFocus();
    return retValue;
  }
  else
  {
    return false;
  }
}

void ftListView::ItemsUpdate()
{
  for (int i = 0; i < GetItemCount(); i++)
  {
    ftListItem item(this, i);
    item.Update();
  }
}

void ftListView::ItemFromText(const wxString& text)
{
  if (text.empty())
  {
    wxLogError(FILE_INFO("Text is empty"));
    return;
  }

  wxStringTokenizer tkz(text, GetFieldSeparator());
  if (tkz.HasMoreTokens())
  {
    const wxString value = tkz.GetNextToken();
    wxFileName fn(value);

    if (fn.FileExists())
    {
      ftListItem item(this, value);
      item.Insert();

      // And try to set the rest of the columns (that are not already set by inserting).
      int col = 1;
      while (tkz.HasMoreTokens() && col < GetColumnCount() - 1)
      {
        const wxString value = tkz.GetNextToken();

        if (col != FindColumn(_("Type")) &&
            col != FindColumn(_("In Folder")) &&
            col != FindColumn(_("Size")) &&
            col != FindColumn(_("Modified")))
        {
          item.SetColumnText(col, value);
        }

        col++;
      }
    }
    else
    {
      // Now we need only the first column (containing findfiles). If more
      // columns are present, these are ignored.
      const wxString findfiles =
        (tkz.HasMoreTokens() ? tkz.GetNextToken(): tkz.GetString());

      ftListItem dir(this, value, findfiles);
      dir.Insert();
    }
  }
  else
  {
    ftListItem item(this, text);
    item.Insert();
  }

  m_ContentsChanged = true;
}

const wxString ftListView::ItemToText(int item_number)
{
  ftListItem item(this, item_number);

  wxString text =
    (item.GetFileName().FileExists() ? item.GetFileName().GetFullPath(): item.GetFileName().GetFullName());

  if (wxFileName::DirExists(item.GetFileName().GetFullPath()))
  {
    text += GetFieldSeparator() + item.GetColumnText(_("Type"));
  }

  if (GetType() != LIST_PROJECT)
  {
    text += GetFieldSeparator() + exListView::ItemToText(item_number);
  }

  return text;
}

void ftListView::OnCommand(wxCommandEvent& event)
{
  if (event.GetId() > ID_TOOL_LOWEST && event.GetId() < ID_TOOL_HIGHEST)
  {
    RunItems(event.GetId());
    return;
  }

  switch (event.GetId())
  {
  // These are added to disable changing this listview if it is read-only,
  // or if there is no file involved.
  case wxID_CLEAR:
  case wxID_CUT:
  case wxID_DELETE:
  case wxID_PASTE:
    if (m_FileName.GetStat().IsOk())
    {
      if (!m_FileName.GetStat().IsReadOnly())
      {
        event.Skip();
        m_ContentsChanged = true;
      }
    }
    else
    {
      event.Skip();
      m_ContentsChanged = false;
    }
  break;

  case ID_LIST_ADD_ITEM: AddItems(); break;

  case ID_LIST_SVN_CAT:
    exSVN(SVN_CAT).Show(ftListItem(this, GetNextSelected(-1)).GetFileName().GetFullPath());
  break;
  case ID_LIST_SVN_DIFF:
    exSVN(SVN_DIFF).Show(ftListItem(this, GetNextSelected(-1)).GetFileName().GetFullPath());
  break;
  case ID_LIST_SVN_LOG:
    exSVN(SVN_LOG).Show(ftListItem(this, GetNextSelected(-1)).GetFileName().GetFullPath());
  break;

  case ID_LIST_COMPARE:
  case ID_LIST_COMPARELAST:
  case ID_LIST_VERSIONLIST:
  {
    if (m_Frame == NULL) return;

    bool first = true;
    wxString file1,file2;
    int i = -1;
    bool found = false;

    ftListView* list = NULL;

    if (event.GetId() == ID_LIST_VERSIONLIST)
    {
      if ((list = m_Frame->Activate(LIST_VERSION)) == NULL) 
      {
        wxLogError("Version list is not activated");
        return;
      }
    }

    while ((i = GetNextSelected(i)) != -1)
    {
      ftListItem li(this, i);
      const wxFileName* filename = &li.GetFileName();
      if (wxFileName::DirExists(filename->GetFullPath())) continue;
      switch (event.GetId())
      {
        case ID_LIST_COMPARE:
        {
          if (GetSelectedItemCount() == 1)
          {
            list = m_Frame->Activate(LIST_PROJECT);
            if (list == NULL) return;
            int main_selected = list->GetFirstSelected();
            ftCompareFile(ftListItem(list, main_selected).GetFileName(), *filename);
          }
          else
          {
            if (first)
            {
              first = false;
              file1 = filename->GetFullPath();
            }
            else
            {
              first = true;
              file2 = filename->GetFullPath();
            }
            if (first) ftCompareFile(wxFileName(file1), wxFileName(file2));
          }
        }
        break;
        case ID_LIST_COMPARELAST:
        {
          wxFileName lastfile;
          if (ftFindOtherFileName(*filename, NULL, &lastfile))
          {
            ftCompareFile(*filename, lastfile);
          }
        }
        break;
        case ID_LIST_VERSIONLIST:
          if (ftFindOtherFileName(*filename, list, NULL))
          {
            found = true;
          }
        break;
      }
    }

    if (event.GetId() == ID_LIST_VERSIONLIST && found)
    {
      list->SortColumn(list->FindColumn(_("Modified")), SORT_DESCENDING);
      list->DeleteDoubles();
    }
  }
  break;

  case ID_LIST_OPEN_ITEM:
  {
    int i = -1;
    while ((i = GetNextSelected(i)) != -1)
    {
      if (!ItemOpenFile(i)) break;
    }
  }
  break;

  case ID_LIST_RUN_MAKE:
  {
    const ftListItem item(this, GetNextSelected(-1));

    wxSetWorkingDirectory(item.GetFileName().GetPath());

    ProcessRun(
      exApp::GetConfig(_("Make")) + wxString(" ") +
      exApp::GetConfig("MakeSwitch", "-f") + wxString(" ") +
      item.GetFileName().GetFullPath());
  }
  break;

#ifdef __WXMSW__
  case ID_LIST_SEND_ITEM:
    ftRBSFile(this).GenerateDialog();
    break;
#endif

  default: event.Skip();
  }

  UpdateStatusBar();
}

void ftListView::OnIdle(wxIdleEvent& event)
{
  event.Skip();

  if (
    !IsShown() ||
     IsOpened() || 
     GetItemCount() == 0)
  {
    return;
  }

  if (m_ItemNumber < GetItemCount())
  {
    ftListItem item(this, m_ItemNumber);

    if ( item.GetFileName().FileExists() &&
        (item.GetFileName().GetStat().GetModificationTime() != item.GetColumnText(_("Modified")) ||
        (size_t)item.GetFileName().GetStat().IsReadOnly() != GetItemData(m_ItemNumber))
        )
    {
      item.Update();
      m_ItemUpdated = true;
      exStatusText(item.GetFileName(), STAT_SYNC | STAT_FULLPATH);
    }

    m_ItemNumber++;
  }
  else
  {
    m_ItemNumber = 0;

    if (m_ItemUpdated)
    {
      if (exApp::GetConfigBool("List/SortSync") && m_Type == LIST_PROJECT)
      {
        SortColumn(exApp::GetConfig(
          "List/SortColumn",
          FindColumn(_("Modified"))), SORT_KEEP);
      }

      m_ItemUpdated = false;
    }
  }

  m_FileName.GetStat().Update(m_FileName.GetFullPath());

  if (m_FileName.GetStat().IsOk())
  {
    if (m_FileName.GetStat().st_mtime != GetStat().st_mtime)
    {
      if (FileOpen(m_FileName))
      {
        exStatusText(m_FileName, STAT_SYNC | STAT_FULLPATH);
      }
    }
  }
}

void ftListView::OnList(wxListEvent& event)
{
  if (event.GetEventType() == wxEVT_COMMAND_LIST_ITEM_ACTIVATED)
  {
    ItemOpenFile(event.GetIndex());
  }
  else if (event.GetEventType() == wxEVT_COMMAND_LIST_ITEM_SELECTED)
  {
    if (GetSelectedItemCount() == 1)
    {
      ftListItem item(this, event.GetIndex());

      // The LIST_PROCESS is treated specially, since there will oexen be
      // entries that do not exist. We do not want a message in these cases.
      if ((m_Type != LIST_PROCESS) ||
          (m_Type == LIST_PROCESS && item.GetFileName().FileExists()))
      {
        exStatusText(item.GetFileName(), STAT_FULLPATH);
      }
    }

    event.Skip();
  }
}

void ftListView::OnMouse(wxMouseEvent& event)
{
  if (event.LeftDown())
  {
    event.Skip();
    int flags = wxLIST_HITTEST_ONITEM;
    const long index = HitTest(wxPoint(event.GetX(), event.GetY()), flags);
    if (index < 0)
    {
      if (m_FileName.FileExists())
      {
        exStatusText(m_FileName);
      }
    }
  }
  else if (event.RightDown())
  {
    long style;

    if (m_Type == LIST_PROJECT)
    {
      // This contains the CAN_PASTE flag, only for these types.
      style = exMenu::MENU_DEFAULT;
    }
    else
    {
      style = 0;
    }

    if (m_FileName.FileExists() && m_FileName.GetStat().IsReadOnly()) style |= exMenu::MENU_IS_READ_ONLY;
    if (GetSelectedItemCount() > 0) style |= exMenu::MENU_IS_SELECTED;
    if (GetItemCount() == 0) style |= exMenu::MENU_IS_EMPTY;
    if (GetSelectedItemCount() == 0 && GetItemCount() > 0) style |= exMenu::MENU_ALLOW_CLEAR;
    exMenu menu(style);

    BuildPopupMenu(menu);
    PopupMenu(&menu);
  }

  UpdateStatusBar();
}

const wxString ftListView::PrintHeader()
{
  if (m_FileName.FileExists())
  {
    return wxString::Format(_("File: %s Modified: %s Printed: %s"),
      m_FileName.GetFullPath().c_str(),
      m_FileName.GetStat().GetModificationTime().c_str(),
      wxDateTime::Now().Format().c_str());
  }
  else
  {
    return GetTypeDescription() + " " + wxDateTime::Now().Format();
  }
}

bool ftListView::ProcessIsRunning()
{
  return m_Process != NULL && m_Process->IsRunning();
}

void ftListView::ProcessRun(const wxString& command)
{
  if (m_Process != NULL)
  {
    wxLogError("Process not null");
    return;
  }

  // This is a static method, we cannot use m_Frame here.
  wxWindow* window = wxTheApp->GetTopWindow();
  ftFrame* frame= wxDynamicCast(window, ftFrame);
  if (frame == NULL) return;

  ftListView* listview = frame->Activate(LIST_PROCESS);
  if (listview == NULL) return;

  if ((m_Process = new ftProcess(listview, command)) != NULL)
  {
    if (!m_Process->Run())
    {
      wxDELETE(m_Process);
    }
  }
}

void ftListView::ProcessStop()
{
  if (m_Process != NULL) 
  {
    m_Process->Stop();
  }
}

void ftListView::ProcessTerminated()
{
  wxBell();
  wxDELETE(m_Process);
}

void ftListView::RunItems(const exTool& tool)
{
  if ((tool.GetId() == ID_TOOL_REPORT_COUNT && m_Type == LIST_COUNT) ||
      (tool.GetId() == ID_TOOL_REPORT_HEADER && m_Type == LIST_HEADER) ||
      (tool.GetId() == ID_TOOL_REPORT_KEYWORD && m_Type == LIST_KEYWORD) ||
      (tool.GetId() == ID_TOOL_REPORT_SQL && m_Type == LIST_SQL)
     )
  {
    return;
  }

  if (m_Thread != NULL && m_Thread->IsRunning())
  {
    if (wxMessageBox(_("Already running, stop") + "?",
      _("Confirm"),
     wxOK | wxCANCEL | wxICON_QUESTION) == wxCANCEL)
    {
      return;
    }

    m_Thread->Delete();
  }

  if (tool.IsFindType())
  {
    if (m_Frame != NULL)
    {
      ftSTC* stc = m_Frame->GetCurrentSTC();

      if (stc != NULL)
      {
        stc->GetSearchText();
      }
    }

    std::vector<exConfigItem> v;
    v.push_back(exConfigItem(_("Find what"), CONFIG_COMBOBOX, wxEmptyString, true));
    if (tool.GetId() == ID_TOOL_REPORT_REPLACE) v.push_back(exConfigItem(_("Replace with"), CONFIG_COMBOBOX));
    v.push_back(exConfigItem(_("Match whole word"), CONFIG_CHECKBOX));
    v.push_back(exConfigItem(_("Match case"), CONFIG_CHECKBOX));

    if (exConfigDialog(NULL,
      v,
      GetFindInCaption(tool.GetId())).ShowModal() == wxID_CANCEL)
    {
      return;
    }

    exApp::GetConfig()->GetFindReplaceData()->Update();
    ftFindLog(tool.GetId() == ID_TOOL_REPORT_REPLACE);
  }

  if (!ftTextFile::SetupTool(tool))
  {
    return;
  }

  m_Thread = new ftToolThread(tool, this);
  m_Thread->Create();
  m_Thread->Run();
}

#if wxUSE_DRAG_AND_DROP
bool ftListViewDropTarget::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
  size_t nFiles = filenames.GetCount();

  for (size_t n = 0; n < nFiles; n++)
  {
    m_Owner->ItemFromText(filenames[n]);
  }

  if (exApp::GetConfigBool("List/SortSync"))
  {
    m_Owner->SortColumn(
      exApp::GetConfig("List/SortColumn",
      m_Owner->FindColumn(_("Modified"))), SORT_KEEP);
  }

  return true;
}
#endif

#ifdef __WXMSW__
ftRBSFile::ftRBSFile(ftListView* listview)
  : exFile()
  , m_Owner(listview)
  , m_Prompt(exApp::GetConfig("RBS/Prompt", ">"))
{
}

void ftRBSFile::Body(
  const wxString& filename,
  const wxString& source,
  const wxString& destination)
{
  GenerateTransmit("SET DEF [" + destination + "]");
  GenerateTransmit("; *** Sending: " + filename + " ***");
  GenerateWaitFor(m_Prompt);
  GenerateTransmit("KERMIT RECEIVE");
  Write(".KermitSendFile \"" + source + wxFILE_SEP_PATH + filename + "\",\"" + filename + ("\",rcASCII\n"));

  GenerateTransmit(wxEmptyString);
  GenerateWaitFor(m_Prompt);
  GenerateTransmit("; *** Done: " + filename + " ***");
}

void ftRBSFile::Footer()
{
  Write("End With\n");
  Write("End Sub\n");
}

void ftRBSFile::GenerateDialog()
{
  std::vector<exConfigItem> v;
  v.push_back(exConfigItem(_("RBS File"), CONFIG_FILEPICKERCTRL, wxEmptyString, true));
  v.push_back(exConfigItem(_("RBS Pattern"), CONFIG_DIRPICKERCTRL));
  exConfigDialog dlg(NULL, v, _("Build RBS File"));
  if (dlg.ShowModal() == wxID_CANCEL) return;

  const wxString script = exApp::GetConfig(_("RBS File"));

  if (!Open(script, wxFile::write))
  {
    return;
  }

  wxBusyCursor wait;

  Header();

  const wxString rsx_pattern = exApp::GetConfig(_("RBS Pattern")) + wxFILE_SEP_PATH;
  int i = -1;
  while ((i = m_Owner->GetNextSelected(i)) != -1)
  {
    ftListItem li(m_Owner, i);
    const wxFileName* filename = &li.GetFileName();
    if (!wxFileName::DirExists(filename->GetFullPath()))
    {
      const wxString source = filename->GetPath();
      wxString destination = source, pattern, with;
      if (source.find(rsx_pattern) != wxString::npos)
      {
        pattern = rsx_pattern;
        with = exApp::GetConfig("RBS/With");
      }
      else
      {
        wxLogError("Cannot find: %s inside: %s", rsx_pattern.c_str(), source.c_str());
        return;
      }

      if (!Substitute(destination, pattern, with, true)) return;
      Substitute(destination, wxFILE_SEP_PATH, ",", false);
      Body(filename->GetFullName(), source, destination);
    }
  }

  Footer();
  Close();

  exApp::Log(_("RBS File") + ": " + script + " " + _("generated"));
}


void ftRBSFile::GenerateTransmit(const wxString& text)
{
  if (text.empty()) Write(".Transmit Chr$(13)\n");
  else              Write(".Transmit \"" + text + "\" & Chr$(13)\n");
}

void ftRBSFile::GenerateWaitFor(const wxString& text)
{
  const wxString pdp_11_spec = "Chr$(10) & ";
  Write(".WaitForString " + pdp_11_spec + "\"" + text + "\", 0, rcAllowKeyStrokes\n");
  Write(".Wait 1, rcAllowKeyStrokes\n");
}

void ftRBSFile::Header()
{
  Write("' Script generated by: " + wxTheApp->GetAppName() + ": " + wxDateTime::Now().Format(EX_TIMESTAMP_FORMAT) + "\n" +
        "' Do not modify this file, all changes will be lost!\n\n" +
        "Option Explicit\n" +
        "Sub Main\n\n" +
        "With Application\n\n");
}

bool ftRBSFile::Substitute(
  wxString& text,
  const wxString& pattern,
  const wxString& new_pattern,
  const bool is_required)
{
  size_t pos_pattern;
  if ((pos_pattern = text.find(pattern)) == wxString::npos)
  {
    if (is_required)
    {
      wxLogError("Cannot find pattern: " + pattern + " in: " + text);
    }

    return false;
  }

  text = text.substr(0, pos_pattern) + new_pattern + text.substr(pos_pattern + pattern.length());

  return true;
}
#endif
