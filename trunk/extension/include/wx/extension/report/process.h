/******************************************************************************\
* File:          process.h
* Purpose:       Declaration of class 'wxExProcess'
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EX_REPORT_PROCESS_H
#define _EX_REPORT_PROCESS_H

#include <wx/process.h>

class wxExFrameWithHistory;
class wxExListView;

/// Offers a wxProcess with output to a listview.
class wxExProcess : public wxProcess
{
public:
  /// Default constructor.
  wxExProcess();

  /// Constructor.
  wxExProcess(
    wxExFrameWithHistory* frame, 
    const wxString& command = wxEmptyString);

  /// Shows a config dialog, sets the command 
  /// and returns dialog return code.
  static int ConfigDialog(
    wxWindow* parent,
    const wxString& title = _("Select Process"));

  /// Executes the process asynchronously (this call immediately returns).
  /// The process output is added to the listview.
  /// The return value is the process id and zero value indicates 
  /// that the command could not be executed.
  /// When the process is finished, a ID_TERMINATED_PROCESS command event
  /// is sent to the frame as specified in the constructor.
  long Execute();
  
  /// Returns true if this process is running.
  bool IsRunning() const;

  /// Returns whether a process has been selected.
  static bool IsSelected() {
    return !m_Command.empty();};
    
  /// Kills the process (sends specified signal if process still running).
  wxKillError Kill(wxSignal sig = wxSIGKILL);
protected:
  virtual void OnTerminate(int pid, int status); // overriden
  void OnTimer(wxTimerEvent& event);
private:
  // Checks whether input is available from process and updates the listview.
  bool CheckInput();

  static wxString m_Command;

  wxExFrameWithHistory* m_Frame;
  wxExListView* m_ListView;

  wxTimer m_Timer;

  static wxExProcess* m_Self;

  DECLARE_EVENT_TABLE()
};
#endif
