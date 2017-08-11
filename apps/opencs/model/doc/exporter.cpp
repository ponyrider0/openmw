#include <iostream>

#include "exporter.hpp"


CSMDoc::Exporter::Exporter(Document& document, const boost::filesystem::path exportPath, ToUTF8::FromType encoding)
  : mDocument(document),
  mExportPath(exportPath),
  mExportOperation(State_Exporting, true, true), // type=exporting, ordered=true, finalAlways=true
  mEncoding(encoding)
{
    mExportManager.setOperation(&mExportOperation); // assign task to thread
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
	if (mExportDefined == false)
	{
		defineExportOperation();
		mExportDefined = true;
	}
	mExportManager.start(); // start thread
}
