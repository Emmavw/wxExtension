/******************************************************************************\
* File:          appl.h
* Purpose:       Declaration of classes for syncsocketserver
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 2007-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <list>
#include <wx/socket.h>
#include <wx/taskbar.h>
#include <wx/extension/app.h>
#include <wx/extension/shell.h>
#include <wx/extension/report/report.h>

#if wxUSE_TASKBARICON
class MyTaskBarIcon;
#endif

class MyApp : public wxExApp
{
public:
  virtual bool OnInit();
};

class MyFrame : public wxExFrameWithHistory
{
public:
  MyFrame();
 ~MyFrame();
  bool ServerNotListening() const {
    return m_SocketServer == NULL;}
protected:
  void OnClose(wxCloseEvent& event);
  void OnCommand(wxCommandEvent& event);
  void OnSocket(wxSocketEvent& event);
  void OnTimer(wxTimerEvent& event);
  void OnUpdateUI(wxUpdateUIEvent& event);
private:
  virtual void ConfigDialogApplied(wxWindowID dialogid);
  virtual wxExGrid* GetGrid();
  virtual wxExSTC* GetSTC();
  virtual bool OpenFile(
    const wxExFileName& filename,
    int line_number = 0,
    const wxString& match = wxEmptyString,
    long flags = 0);
  virtual void StatusBarDoubleClicked(int field, const wxPoint& point);

  void LogConnection(
    wxSocketBase* sock,
    bool accepted = true,
    bool show_clients = true);
  bool SetupSocketServer();
  bool SocketCheckError(const wxSocketBase* sock);
  const wxString SocketDetails(const wxSocketBase* sock) const;
  void SocketLost(wxSocketBase* sock, bool remove_from_clients);
  void TimerDialog();
  void WriteDataToClient(const wxCharBuffer& data, wxSocketBase* client);
  void WriteDataWindowToClients();

  std::list<wxSocketBase*> m_Clients;

  wxExSTCWithFrame* m_DataWindow;
  wxExSTCWithFrame* m_LogWindow;
  wxExSTCShell* m_Shell;

  wxExStatistics < long > m_Statistics;

  wxSocketServer* m_SocketServer;
  wxTimer m_Timer;

#if wxUSE_TASKBARICON
  MyTaskBarIcon* m_TaskBarIcon;
#endif

  DECLARE_EVENT_TABLE()
};

#if wxUSE_TASKBARICON
class MyTaskBarIcon: public wxTaskBarIcon
{
public:
  MyTaskBarIcon(MyFrame* frame)
    : m_Frame(frame) {}
protected:
  virtual wxMenu* CreatePopupMenu();
  void OnCommand(wxCommandEvent& event);
  void OnTaskBarIcon(wxTaskBarIconEvent&) {
    m_Frame->Show();}
  void OnUpdateUI(wxUpdateUIEvent& event) {
    event.Enable(m_Frame->ServerNotListening());}
private:
  MyFrame* m_Frame;

  DECLARE_EVENT_TABLE()
};
#endif

enum
{
  ID_MENU_FIRST,

  ID_CLEAR_STATISTICS,
  ID_CLIENT_BUFFER_SIZE,
  ID_CLIENT_ECHO,
  ID_CLIENT_LOG_DATA,
  ID_CLIENT_LOG_DATA_COUNT_ONLY,
  ID_HIDE,
  ID_RECENT_FILE_MENU,
  ID_SERVER_CONFIG,
  ID_TIMER_STOP,
  ID_TIMER_START,
  ID_VIEW_DATA,
  ID_VIEW_LOG,
  ID_VIEW_SHELL,
  ID_VIEW_STATISTICS,
  ID_WRITE_DATA,

  ID_MENU_LAST,

  ID_SERVER,
  ID_CLIENT,
};
