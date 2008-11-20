/******************************************************************************\
* File:          shell.cpp
* Purpose:       Implementation of class exSTCShell
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2008 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/tokenzr.h>
#include <wx/extension/extension.h>
#include <wx/extension/shell.h>

#if wxUSE_GUI

using namespace std;

/// \todo WX_DELETE is not coming in as an event?
BEGIN_EVENT_TABLE(exSTCShell, exSTC)
//  EVT_CHAR(exSTCShell::OnChar)
  EVT_KEY_DOWN(exSTCShell::OnKey)
  EVT_MENU(wxID_PASTE, exSTCShell::OnCommand)
END_EVENT_TABLE()

exSTCShell::exSTCShell(
  wxWindow* parent,
  const wxString& prompt,
  const wxString& command_end,
  bool echo,
  int commands_save_in_config,
  long type)
  : exSTC(parent, type)
  , m_Command(wxEmptyString)
  , m_CommandEnd(command_end)
  , m_CommandStartPosition(0)
  , m_Echo(echo)
  // take a char that is not likely to appear inside commands
  , m_CommandsInConfigDelimiter(wxChar(0x03))
  , m_CommandsSaveInConfig(commands_save_in_config)
  , m_Prompt(prompt)
{
  // Override defaults from config.
  SetEdgeMode(wxSTC_EDGE_NONE);
  ResetMargins(false); // do not reset divider margin

  // Start with a prompt.
  Prompt();

  if (m_CommandsSaveInConfig == -1)
  {
    // Fill the list with an empty command.
    KeepCommand();
  }
  else
  {
    // Get all previous commands.
    wxStringTokenizer tkz(exApp::GetConfig("Shell"),
      m_CommandsInConfigDelimiter);

    while (tkz.HasMoreTokens())
    {
      const wxString val = tkz.GetNextToken();
      m_Commands.push_front(val);
    }

    // Take care that m_CommandsIterator is valid.
    if (!m_Commands.empty())
    {
      m_CommandsIterator = m_Commands.end();
    }
    else
    {
      KeepCommand();
    }
  }
}

exSTCShell::~exSTCShell()
{
  if (m_CommandsSaveInConfig > 0)
  {
    wxString values;
    int items = 0;

    for (
      list < wxString >::reverse_iterator it = m_Commands.rbegin();
      it != m_Commands.rend() && items < m_CommandsSaveInConfig;
      it++)
    {
      values += *it + m_CommandsInConfigDelimiter;
      items++;
    }

    exApp::SetConfig("Shell", values);
  }
}

bool exSTCShell::GetHistory(const wxString& short_command)
{
  const int no_asked_for = atoi(short_command.c_str());

  if (no_asked_for > 0)
  {
    int no = 1;

    for (
      list < wxString >::iterator it = m_Commands.begin();
      it != m_Commands.end();
      it++)
    {
      if (no == no_asked_for)
      {
        m_Command = *it;
        return true;
      }

      no++;
    }
  }
  else
  {
    wxString short_command_check;

    if (m_CommandEnd == GetEOL())
    {
      short_command_check = short_command;
    }
    else
    {
      short_command_check = 
        short_command.substr(
          0, 
          short_command.length() - m_CommandEnd.length());
    }

    for (
      list < wxString >::reverse_iterator it = m_Commands.rbegin();
      it != m_Commands.rend();
      it++)
    {
      const wxString command = *it;
      
      if (command.StartsWith(short_command_check))
      {
        m_Command = command;
        return true;
      }
    }
  }

  return false;
}

void exSTCShell::KeepCommand()
{
  m_Commands.remove(m_Command);
  m_Commands.push_back(m_Command);

  if (m_Commands.size() == 1)
  {
    m_CommandsIterator = m_Commands.end();
  }
}

void exSTCShell::OnChar(wxKeyEvent& event)
{
  const int key = event.GetKeyCode();
  wxLogMessage("key: %d pos: %d csp: %d", key, GetCurrentPos(), m_CommandStartPosition);
  event.Skip();
}

void exSTCShell::OnCommand(wxCommandEvent& command)
{
  switch (command.GetId())
  {
    case wxID_PASTE:
      // Take care that we cannot paste somewhere inside.
      if (GetCurrentPos() < m_CommandStartPosition)
      {
        DocumentEnd();
      }

      Paste();
      break;

    default: command.Skip();
      break;
  }
}

void exSTCShell::OnKey(wxKeyEvent& event)
{
  const int key = event.GetKeyCode();

  // Enter key pressed, we might have entered a command.
  if (key == WXK_RETURN)
  {
    // First get the command.
    SetTargetStart(GetTextLength());
    SetTargetEnd(0);
    SetSearchFlags(wxSTC_FIND_REGEXP);

    if (SearchInTarget("^" + m_Prompt + ".*") != -1)
    {
      m_Command = GetText().substr(
        GetTargetStart() + m_Prompt.length(),
        GetTextLength() - 1);
      m_Command.Trim();
    }

    if (m_Command.empty())
    {
      Prompt();
    }
    else if (
      m_CommandEnd == GetEOL() ||
      m_Command.EndsWith(m_CommandEnd))
    {
      // We have a command.
      EmptyUndoBuffer();

      // History command.
      if (m_Command == wxString("history") +
         (m_CommandEnd == GetEOL() ? wxString(wxEmptyString): m_CommandEnd))
      {
        KeepCommand();
        ShowHistory();
        Prompt();
      }
      // !.. command, get it from history.
      else if (m_Command.StartsWith("!"))
      {
        if (GetHistory(m_Command.substr(1)))
        {
          AppendText(m_Command);
          KeepCommand();
          wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_SHELL_COMMAND);
          event.SetString(m_Command);
          GetParent()->AddPendingEvent(event);
        }
        else
        {
          Prompt(_("event not found"));
        }
      }
      // Other command, send to parent.
      else
      {
        KeepCommand();
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_SHELL_COMMAND);
        event.SetString(m_Command);
        GetParent()->AddPendingEvent(event);
      }

      m_Command.clear();
    }
    else
    {
      if (m_Echo) event.Skip();
    }

    if (m_Commands.size() > 1)
    {
      m_CommandsIterator = m_Commands.end();
    }
  }
  // Up or down key pressed, and at the end of document.
  else if ((key == WXK_UP || key == WXK_DOWN) &&
            GetCurrentPos() == GetTextLength())
  {
    // There is always an empty command in the list.
    if (m_Commands.size() > 1)
    {
      ShowCommand(key);
    }
  }
  // Home key pressed.
  else if (key == WXK_HOME)
  {
    Home();

    const wxString line = GetLine(GetCurrentLine());

    if (line.StartsWith(m_Prompt))
    {
      GotoPos(GetCurrentPos() + m_Prompt.length());
    }
  }
  // Ctrl-Q pressed, used to stop processing.
  else if (event.GetModifiers() == wxMOD_CONTROL && key == 'Q')
  {
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_SHELL_COMMAND_STOP);
    GetParent()->AddPendingEvent(event);
  }
  // Ctrl-V pressed, used for pasting as well.
  else if (event.GetModifiers() == wxMOD_CONTROL && key == 'V')
  {
    if (GetCurrentPos() < m_CommandStartPosition) DocumentEnd();
    if (m_Echo) event.Skip();
  }
  // Backspace or delete key pressed.
  else if (key == WXK_BACK || key == WXK_DELETE)
  {
    if (GetCurrentPos() <= m_CommandStartPosition)
    {
      // Ignore, so do nothing.
    }
    else
    {
      // Allow.
      if (m_Echo) event.Skip();
    }
  }
  // The rest.
  else
  {
    // If we enter regular text and not already building a command, first goto end.
    if (event.GetModifiers() == wxMOD_NONE &&
        key < WXK_START &&
        GetCurrentPos() < m_CommandStartPosition)
    {
      DocumentEnd();
    }

    if (m_Commands.size() > 1)
    {
      m_CommandsIterator = m_Commands.end();
    }

    if (m_Echo) event.Skip();
  }
}

void exSTCShell::Prompt(const wxString& text)
{
  if (!text.empty())
  {
    AppendText(text);
  }

  if (GetTextLength() > 0)
  {
    AppendText(GetEOL());
  }

  AppendText(m_Prompt);

  DocumentEnd();

  m_CommandStartPosition = GetCurrentPos();

  EmptyUndoBuffer();
}

void exSTCShell::ShowCommand(int key)
{
  SetTargetStart(GetTextLength());
  SetTargetEnd(0);
  SetSearchFlags(wxSTC_FIND_REGEXP);

  if (SearchInTarget("^" + m_Prompt + ".*") != -1)
  {
    SetTargetEnd(GetTextLength());

    if (key == WXK_UP)
    {
      if (m_CommandsIterator != m_Commands.begin())
      {
        m_CommandsIterator--;
      }
    }
    else
    {
      if (m_CommandsIterator != m_Commands.end())
      {
        m_CommandsIterator++;
      }
    }

    if (m_CommandsIterator != m_Commands.end())
    {
      ReplaceTarget(m_Prompt + *m_CommandsIterator);
    }
    else
    {
      ReplaceTarget(m_Prompt);
    }

    DocumentEnd();
  }
}

void exSTCShell::ShowHistory()
{
  int command_no = 1;

  for (
    list < wxString >::iterator it = m_Commands.begin();
    it != m_Commands.end();
    it++)
  {
    AppendText(
      wxString::Format("\n%d %s", 
        command_no++, 
        wxString(*it).c_str()));
  }
}

#endif // wxUSE_GUI
