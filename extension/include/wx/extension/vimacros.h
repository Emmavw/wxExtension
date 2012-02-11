////////////////////////////////////////////////////////////////////////////////
// Name:      vimacros.h
// Purpose:   Declaration of class wxExViMacros
// Author:    Anton van Wezenbeek
// Copyright: (c) 2011 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#ifndef _EXVIMACROS_H
#define _EXVIMACROS_H

#include <map>
#include <vector>
#include <wx/filename.h>
#include <wx/xml/xml.h>

#if wxUSE_GUI

class wxExEx;

/// Offers the macro collection, and allows
/// recording and playback to vi (ex) component.
class WXDLLIMPEXP_BASE wxExViMacros
{
public:  
  /// Default constructor.
  wxExViMacros();
  
  /// Returns all macros (names) as an array of strings.
  const wxArrayString Get() const;
  
  /// Returns contents of macro (for testing).
  const std::vector< wxString > Get(const wxString& macro) const;
  
  /// Returns current or last macro.
  const wxString& GetMacro() {return m_Macro;};
  
  /// Is macro recorded.
  bool IsRecorded(const wxString& macro = wxEmptyString) const;
  
  /// Are we playing back?
  bool IsPlayback() const {return m_IsPlayback;};
  
  /// Are we recording?
  bool IsRecording() const {return m_IsRecording;};
  
  /// Plays back macro a number of repeat times on the ex component.
  /// Returns true if all records could be executed.
  bool Playback(wxExEx* ex, const wxString& macro, int repeat = 1);
  
  /// Records text to current macro as a new command.
  /// The text to be recorded should be valid ex command,
  /// though it is not checked here.
  /// If you playback this macro the text
  /// is sent to the ex component to execute it, and then should be
  /// a valid command.
  void Record(
    /// text to record
    const wxString& text, 
    /// normally each record is a new command, if not,
    /// the text is appended after the last command
    bool new_command = true);
  
  /// Starts recording a macro (overwrites 
  /// existing macro if macro starts with lower case).
  void StartRecording(const wxString& macro);
  
  /// Stops recording.
  void StopRecording();
  
  /// Returns the filename with xml document.
  static const wxFileName GetFileName();
  
  /// Loads all macros from xml document.
  static void LoadDocument();
  
  /// Saves all macros to xml document.
  static void SaveDocument();
private:  
  static bool Load(wxXmlDocument& doc);
  static const wxString Encode(const wxString& text, bool& encoded);
  static const wxString Decode(const wxString& text);
    
  static std::map <wxString, std::vector< wxString > > m_Macros;
  bool m_IsPlayback;
  bool m_IsRecording;
  wxString m_Macro;
};
#endif // wxUSE_GUI
#endif