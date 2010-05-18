/******************************************************************************\
* File:          test.cpp
* Purpose:       Implementation for wxExtension report cpp unit testing
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
* Created:       za 17 jan 2009 11:51:20 CET
*
* Copyright (c) 2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <TestCaller.h>
#include <wx/config.h>
#include "test.h"

#define TEST_FILE "./test.h"
#define TEST_PRJ "./test-rep.prj"

void wxExReportAppTestFixture::testConfig()
{
  wxConfig* cfg = new wxConfig(wxEmptyString, wxEmptyString, "test.cfg", wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
  const int max = 100000;

  wxStopWatch sw;

  sw.Start();

  for (int j = 0; j < max; j++)
  {
    cfg->Read("test", 0l);
  }

  const long config = sw.Time();

  printf("wxConfig::Read:%ld\n", config);
}

void wxExReportAppTestFixture::testDirWithListView()
{
  wxExFrameWithHistory* frame = (wxExFrameWithHistory *)wxTheApp->GetTopWindow();
  wxExListViewFile* listView = new wxExListViewFile(frame, frame, TEST_FILE);
  
  wxExDirWithListView dir(listView, "./");
  CPPUNIT_ASSERT(dir.FindFiles());
}

void wxExReportAppTestFixture::testFrameWithHistory()
{
  wxExFrameWithHistory* frame = (wxExFrameWithHistory *)wxTheApp->GetTopWindow();
  
  CPPUNIT_ASSERT(!frame->OpenFile(wxExFileName(TEST_FILE))); // as we have no focused stc
  CPPUNIT_ASSERT(!frame->GetRecentFile().Contains("test.h"));
  CPPUNIT_ASSERT(!frame->OpenFile(
    wxExFileName(TEST_PRJ),
    0,
    wxEmptyString,
    wxExSTCWithFrame::STC_WIN_IS_PROJECT));
  CPPUNIT_ASSERT(!frame->GetRecentProject().Contains("test-rep.prj"));
}

void wxExReportAppTestFixture::testListItem()
{
  wxExFrameWithHistory* frame = (wxExFrameWithHistory *)wxTheApp->GetTopWindow();
  wxExListViewStandard* listView = new wxExListViewStandard(frame, LIST_FILE);
  
  wxExListItem item1(listView, wxExFileName("./test.h"));
  item1.Insert();
  wxExListItem item2(listView, wxExFileName("./test.cpp"));
  item2.Insert();
  wxExListItem item3(listView, wxExFileName("./main.cpp"));
  item3.Insert();
  listView->SortColumn(_("File Name"), SORT_ASCENDING);
  
  wxExListItem test(listView, 0);
  CPPUNIT_ASSERT(test.GetColumnText(_("File Name")) == "./main.cpp");
}
  
void wxExReportAppTestFixture::testListViewFile()
{
  wxExFrameWithHistory* frame = (wxExFrameWithHistory *)wxTheApp->GetTopWindow();
  wxExListViewFile* listView = new wxExListViewFile(frame, frame, TEST_FILE);
  
  listView->InsertColumn(wxExColumn("String", wxExColumn::COL_STRING));
  listView->InsertColumn(wxExColumn("Number", wxExColumn::COL_INT));
  // Remember that listview file already has columns.
  CPPUNIT_ASSERT(listView->FindColumn("String") > 1);
  CPPUNIT_ASSERT(listView->FindColumn("Number") > 1);

  CPPUNIT_ASSERT(listView->FileLoad(TEST_PRJ));
  CPPUNIT_ASSERT(listView->ItemFromText("test1\ntest2\n"));
}

void wxExReportAppTestFixture::testProcess()
{
  wxExFrameWithHistory* frame = (wxExFrameWithHistory *)wxTheApp->GetTopWindow();
  wxExProcess process(frame, "wc test.h");
  
  CPPUNIT_ASSERT(process.IsSelected());
  long pid = process.Execute();
  // CPPUNIT_ASSERT(wxProcess::Exists(pid));
}

void wxExReportAppTestFixture::testSTCWithFrame()
{
  wxExFrameWithHistory* frame = (wxExFrameWithHistory *)wxTheApp->GetTopWindow();
  wxExSTCWithFrame stc(frame, frame, wxExFileName(TEST_FILE));
  
  CPPUNIT_ASSERT(stc.GetFileName().GetFullPath().Contains("test.h"));
}
  
wxExReportTestSuite::wxExReportTestSuite()
  : CppUnit::TestSuite("wxexreport test suite")
{
  addTest(new CppUnit::TestCaller<wxExReportAppTestFixture>(
    "testConfig",
    &wxExReportAppTestFixture::testConfig));
    
  addTest(new CppUnit::TestCaller<wxExReportAppTestFixture>(
    "testDirWithListView",
    &wxExReportAppTestFixture::testDirWithListView));
    
  addTest(new CppUnit::TestCaller<wxExReportAppTestFixture>(
    "testFrameWithHistory",
    &wxExReportAppTestFixture::testFrameWithHistory));
    
  addTest(new CppUnit::TestCaller<wxExReportAppTestFixture>(
    "testListItem",
    &wxExReportAppTestFixture::testListItem));
    
  addTest(new CppUnit::TestCaller<wxExReportAppTestFixture>(
    "testListViewFile",
    &wxExReportAppTestFixture::testListViewFile));
    
  addTest(new CppUnit::TestCaller<wxExReportAppTestFixture>(
    "testProcess",
    &wxExReportAppTestFixture::testProcess));
    
  addTest(new CppUnit::TestCaller<wxExReportAppTestFixture>(
    "testSTCWithFrame",
    &wxExReportAppTestFixture::testSTCWithFrame));
}
