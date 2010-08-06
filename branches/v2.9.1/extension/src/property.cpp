////////////////////////////////////////////////////////////////////////////////
// Name:      property.cpp
// Purpose:   Implementation of wxExProperty class
// Author:    Anton van Wezenbeek
// Created:   2010-02-08
// RCS-ID:    $Id$
// Copyright: (c) 2010 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/log.h> 
#include <wx/stc/stc.h>
#include <wx/extension/property.h>

wxExProperty::wxExProperty(const wxXmlNode* node)
{
  if (node != NULL)
  {
    Set(node);
  }
}

void wxExProperty::Apply(wxStyledTextCtrl* stc) const
{
  if (IsOk())
  {
    stc->SetProperty(m_Name, m_Value);
  }
}

void wxExProperty::ApplyReset(wxStyledTextCtrl* stc) const
{
  stc->SetProperty(m_Name, wxEmptyString);
}

bool wxExProperty::IsOk() const
{
  return !m_Name.empty() && !m_Value.empty();
}

void wxExProperty::Set(const wxXmlNode* node)
{
  m_Name = node->GetAttribute("name", "0");
  m_Value = node->GetNodeContent().Strip(wxString::both);

  if (!IsOk())
  {
    wxLogError(_("Illegal property on line: %d"), node->GetLineNumber());
  }
}
