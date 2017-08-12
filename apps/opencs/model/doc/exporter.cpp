#include <iostream>

#include "exporter.hpp"


CSMDoc::Exporter::Exporter(Document& document, const boost::filesystem::path exportPath, ToUTF8::FromType encoding)
  : mDocument(document),
  mExportPath(exportPath),
  mEncoding(encoding)
{
    mExportOperation = new Operation(State_Exporting, true, true), // type=exporting, ordered=true, finalAlways=true
    mExportManager.setOperation(mExportOperation); // assign task to thread
	std::cout << "Export Path: " << mExportPath << std::endl;
}

CSMDoc::Exporter::~Exporter()
{
	if (mStatePtr != 0)
    {
        mStatePtr->getStream().close();
		delete mStatePtr;
    }
}

void CSMDoc::Exporter::queryExportPath()
{
    mStatePtr = new SavingState(*mExportOperation, mExportPath, mEncoding);
}

void CSMDoc::Exporter::defineExportOperation()
{
}

void CSMDoc::Exporter::startExportOperation()
{
    // clear old data
    
    if (mStatePtr != 0)
    {
        mStatePtr->getStream().close();
        delete mStatePtr;
    }
    delete mExportOperation;
    
    mExportOperation = new Operation(State_Exporting, true, true), // type=exporting, ordered=true, finalAlways=true
    queryExportPath();
    defineExportOperation(); // must querypath and create savingstate before defining operation

    mExportManager.setOperation(mExportOperation); // assign task to thread
    mExportManager.start(); // start thread
}
