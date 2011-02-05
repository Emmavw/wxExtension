/******************************************************************************\
* File:          process.cpp
* Purpose:       Implementation of class 'wxExProcess'
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/infobar.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h> // for wxTextInputStream
#include <wx/extension/configdlg.h>
#include <wx/extension/log.h>
#include <wx/extension/util.h> // for wxExConfigFirstOf
#include <wx/extension/report/process.h>
#include <wx/extension/report/defs.h>
#include <wx/extension/report/frame.h>
#include <wx/extension/report/listitem.h>
#include <wx/extension/report/listview.h>

BEGIN_EVENT_TABLE(wxExProcess, wxProcess)
  EVT_TIMER(-1, wxExProcess::OnTimer)
END_EVENT_TABLE()

wxString wxExProcess::m_Command;
wxString wxExProcess::m_WorkingDirKey = _("Process folder");

wxExProcess::wxExProcess(
  wxExFrameWithHistory* frame,
  const wxString& command)
  : wxProcess(wxPROCESS_REDIRECT)
  , m_Frame(frame)
  , m_ListView(NULL)
  , m_Timer(this)
{
  if (!command.empty())
  {
    m_Command = command;
  }
}

bool wxExProcess::CheckInput()
{
  if (m_ListView == NULL)
  {
    return false;
  }

  bool hasInput = false;

  // This assumes that the output is always line buffered.
  wxString line;

  if (IsInputAvailable())
  {
    wxTextInputStream tis(*GetInputStream());
    line << tis.ReadLine();
    hasInput = true;
  }
  else if (IsErrorAvailable())
  {
    wxTextInputStream tis(*GetErrorStream());
    line << tis.ReadLine();
    hasInput = true;
  }

  if (hasInput && !line.empty())
  {
    wxString lineno;
    wxString path;

    // Check on error in php script output.
    const wxRegEx regex(".*in \\(.*\\) on line \\(.*\\)", wxRE_ADVANCED);

    if (regex.Matches(line))
    {
      size_t start, len;

      if (regex.GetMatch(&start, &len, 1))
      {
        path = line.substr(start, len);
      }

      if (regex.GetMatch(&start, &len, 2))
      {
        lineno = line.substr(start, len);
      }
    }
    else
    {
      // Check on error in gcc output (and some others).
      wxStringTokenizer tkz(line, ':');
      path = tkz.GetNextToken();

      if (tkz.HasMoreTokens())
      {
        const wxString number = tkz.GetNextToken();
        if (atoi(number.c_str()) != 0) lineno = number;
      }
    }

    wxFileName fn(path);

    fn.Normalize();

    if (fn.FileExists())
    {
      wxExListItem item(m_ListView, fn);
      item.Insert();
      item.SetItem(_("Line"), line);
      item.SetItem(_("Line No"), lineno);
    }
    else
    {
      m_ListView->InsertItem(m_ListView->GetItemCount(), line, -1);
    }

    // If nothing selected, then ensure last line is visible.
    // Otherwise you are busy inspecting some line, and
    // it would be irritating to get it out of view.
    if (m_ListView->GetSelectedItemCount() == 0)
    {
      m_ListView->EnsureVisible(m_ListView->GetItemCount() - 1);
    }

#if wxUSE_STATUSBAR
    m_ListView->UpdateStatusBar();
#endif
  }

  return hasInput;
}

int wxExProcess::ConfigDialog(
  wxWindow* parent,
  const wxString& title)
{
  std::vector<wxExConfigItem> v;

  v.push_back(wxExConfigItem(
    _("Process"), 
    CONFIG_COMBOBOX, 
    wxEmptyString));

  v.push_back(wxExConfigItem(
    m_WorkingDirKey, 
    CONFIG_COMBOBOXDIR, 
    wxEmptyString,
    true,
    1000));

  const auto result = wxExConfigDialog(parent,
    v,
    title).ShowModal();

  if (result == wxID_OK)
  {
    m_Command = wxExConfigFirstOf(_("Process"));
  }

  return result;
}

long wxExProcess::Execute()
{
  if (m_Command.empty())
  {
    if (ConfigDialog(m_ListView) == wxID_CANCEL)
    {
      return 0;
    }
  }

  wxString cwd;
  const wxString dir = wxExConfigFirstOf(m_WorkingDirKey);

  if (!dir.empty())
  {
    cwd = wxGetCwd();
    if (!wxSetWorkingDirectory(dir))
    {
      wxLogError(_("Cannot set working directory"));
      return 0;
    }
  }

  m_ListView = m_Frame->Activate(wxExListViewStandard::LIST_PROCESS);

  if (m_ListView == NULL)
  {
    // Only show a message, and continue,
    // easier for testing.
    wxInfoBar(m_Frame).ShowMessage(_("No listview to collect output"));
  }

  const long pid = wxExecute(m_Command, wxEXEC_ASYNC, this);

  if (pid > 0)
  {
#if wxUSE_STATUSBAR
    wxExFrame::StatusText(m_Command);
#endif
    wxExLog::Get()->Log(m_Command);

    m_Timer.Start(1000); // each 1000 milliseconds
  }
  else
  {
    wxLogError(_("Cannot execute") + ": " + m_Command);
  }

  if (!cwd.empty())
  {
    wxSetWorkingDirectory(cwd);
  }

  return pid;
}

bool wxExProcess::IsRunning() const
{
  if (GetPid() < 0)
  {
    return false;
  }

  return Exists(GetPid());
}

wxKillError wxExProcess::Kill(wxSignal sig)
{
  if (!IsRunning())
  {
    return wxKILL_NO_PROCESS;
  }

  m_Timer.Stop();
  
#if wxUSE_STATUSBAR
  wxExFrame::StatusText(_("Stopped"));
#endif

  DeletePendingEvents();

  return wxProcess::Kill(GetPid(), sig);
}

void wxExProcess::OnTerminate(
  int WXUNUSED(pid), 
  int WXUNUSED(status))
{
  m_Timer.Stop();

  // Collect remaining input.
  while (CheckInput())
  {
    // Do nothing.
  }

#if wxUSE_STATUSBAR
  wxExFrame::StatusText(_("Ready"));
#endif

  wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_TERMINATED_PROCESS);
  wxPostEvent(wxTheApp->GetTopWindow(), event);
}

void wxExProcess::OnTimer(wxTimerEvent& event)
{
  while (CheckInput())
  {
    // Do nothing.
  }
}