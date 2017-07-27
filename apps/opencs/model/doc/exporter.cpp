#include <iostream>

#include "exporter.hpp"


CSMDoc::Exporter::Exporter(Document& document, const boost::filesystem::path exportPath, ToUTF8::FromType encoding)
  : mDocument(document),
  mExportPath(exportPath),
  mExportOperation(State_Operation, true, true),
  mExportManager(&mExportOperation),
  mEncoding(encoding)
{
	std::cout << "Export Path: " << mExportPath << std::endl;
}

CSMDoc::Exporter::~Exporter()
{
	if (mStatePtr != 0)
		delete mStatePtr;
}

void CSMDoc::Exporter::queryExportPath()
{
	if (mStatePtr != 0)
		delete mStatePtr;
	mStatePtr = new SavingState(mExportOperation, mExportPath, mEncoding);
}

void CSMDoc::Exporter::defineExportOperation()
{
}

void CSMDoc::Exporter::startExportOperation()
{
	queryExportPath();
	defineExportOperation();
	mExportManager.start();
}