#include <iostream>
#include <QFileDialog>
#include <QObject>

#include "exporter.hpp"
#include "exportToTES4.hpp"
#include "exportToTES3.hpp"

CSMDoc::Exporter::Exporter(Document& document, ToUTF8::FromType encoding)
  : mDocument(document),
    mEncoding(encoding)
{
    std::cout << "Exporter Base Initialized." << std::endl;
}

CSMDoc::Exporter::~Exporter()
{
	if (mStatePtr != 0)
    {
        mStatePtr->getStream().close();
		delete mStatePtr;
    }
}

void CSMDoc::Exporter::startExportOperation(boost::filesystem::path filename)
{
    // clear old data
    if (mStatePtr != 0)
    {
        mStatePtr->getStream().close();
        delete mStatePtr;
    }
    if (mExportOperation != 0)
    {
        delete mExportOperation;
    }

    mExportPath = filename;
    std::cout << "Extension = " << filename.extension().string() << std::endl;
    if (filename.extension().string() == ".ESM4")
    {
        mExportOperation = new ExportToTES4();
        std::cout << "TES4 Export Path = " << mExportPath << std::endl;
    }
    else
    {
        mExportOperation = new ExportToTES3();
        std::cout << "TES3 Export Path = " << mExportPath << std::endl;
    }
    
    mStatePtr = new SavingState(*mExportOperation, mExportPath, mEncoding);
    
    mExportOperation->defineExportOperation(mDocument, *mStatePtr); // must querypath and create savingstate before defining operation
//    mExportOperation->defineExportOperation(mDocument, SavingState(*mExportOperation, mExportPath, mEncoding));
    mExportManager.setOperation(mExportOperation); // assign task to thread

    mExportManager.start(); // start thread
}
