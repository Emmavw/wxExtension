/******************************************************************************\
* File:          stc.h
* Purpose:       Declaration of class 'wxExSTCWithFrame'
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EX_REPORT_STC_H
#define _EX_REPORT_STC_H

class wxExFrameWithHistory;

#include <wx/extension/stc.h>

/// Adds a frame to wxExSTC.
class WXDLLIMPEXP_BASE wxExSTCWithFrame : public wxExSTC
{
public:
  /// Menu types, they determine how the context menu will appear.
  /// These values extend the menu types used by wxExSTC.
  enum
  {
    STC_MENU_TOOL           = 0x0100, ///< for adding tool menu
    STC_MENU_REPORT_FIND    = 0x0200, ///< for adding find in files
    STC_MENU_REPORT_REPLACE = 0x0400, ///< for adding replace in files
  };

  /// Extra open flags.
  enum
  {
    STC_WIN_IS_PROJECT = 0x0100,
  };

  /// Constructor. Does not open a file, but sets text to specified value.
  wxExSTCWithFrame(wxWindow* parent,
    wxExFrameWithHistory* frame,
    const wxString& value,
    long flags = 0,
    const wxString& title = wxEmptyString,
    long type = STC_MENU_DEFAULT,
    wxWindowID id = wxID_ANY,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0);

  /// Constructor, opens the file.
  wxExSTCWithFrame(wxWindow* parent,
    wxExFrameWithHistory* frame,
    const wxExFileName& filename,
    int line_number = 0,
    const wxString& match = wxEmptyString,
    long flags = 0,
    long type = STC_MENU_DEFAULT,
    wxWindowID id = wxID_ANY,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0);

  /// Copy constructor from another wxExSTC.
  wxExSTCWithFrame(const wxExSTC& stc, wxExFrameWithHistory* frame);

  /// Calls base and sets recent file if base call succeeded.
  virtual bool Open(
    const wxExFileName& filename,
    int line_number = 0,
    const wxString& match = wxEmptyString,
    long flags = 0);

  /// Invokes base properties message and sets the frame title.
  virtual void PropertiesMessage();
protected:
  void OnCommand(wxCommandEvent& command);

  DECLARE_EVENT_TABLE()
private:
  wxExFrameWithHistory* m_Frame;
};

#endif