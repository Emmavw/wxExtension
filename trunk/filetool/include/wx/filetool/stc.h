/******************************************************************************\
* File:          stc.h
* Purpose:       Declaration of class 'exSTCWithFrame'
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _FTSTC_H
#define _FTSTC_H

class exFrameWithHistory;

#include <wx/extension/stc.h>

/// Adds a frame and drag/drop to exSTC. 
/// The frame is assigned in the Initialize.
class exSTCWithFrame : public exSTC
{
public:
  /// Menu types, they determine how the context menu will appear.
  /// These values extend the menu types used by exSTC.
  enum
  {
    STC_MENU_TOOL           = 0x0100, ///< for adding tool menu
    STC_MENU_REPORT_FIND    = 0x0200, ///< for adding find in files
    STC_MENU_REPORT_REPLACE = 0x0400, ///< for adding replace in files
    STC_MENU_COMPARE_OR_SVN = 0x1000, ///< for adding compare or SVN menu
  };

  /// Extra open flags.
  enum
  {
    STC_OPEN_IS_PROJECT = 0x0100,
  };

  /// Constructor. Does not open a file, but sets text to specified value.
  exSTCWithFrame(wxWindow* parent,
    long type = STC_MENU_DEFAULT,
    const wxString& value = wxEmptyString,
    wxWindowID id = wxID_ANY,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0,
    const wxString& name = wxSTCNameStr);

  /// Constructor, opens the file.
  exSTCWithFrame(wxWindow* parent,
    const exFileName& filename,
    int line_number = 0,
    const wxString& match = wxEmptyString,
    long flags = 0,
    long type = STC_MENU_DEFAULT,
    wxWindowID id = wxID_ANY,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0,
    const wxString& name = wxSTCNameStr);

  /// Copy constructor from an exSTC.
  exSTCWithFrame(const exSTC& stc);

  /// Calls base and sets recent file if base call succeeded.
  virtual bool Open(
    const exFileName& filename,
    int line_number = 0,
    const wxString& match = wxEmptyString,
    long flags = 0);

  /// Invokes base properties message and sets the frame title.
  virtual void PropertiesMessage();
protected:
  /// Builds the popup menu.
  virtual void BuildPopupMenu(exMenu& menu);

  void OnCommand(wxCommandEvent& command);

  DECLARE_EVENT_TABLE()
private:
  bool Initialize();
  exFrameWithHistory* m_Frame;
};

#endif
