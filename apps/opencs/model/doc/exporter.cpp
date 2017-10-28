#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

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
    if (Misc::StringUtils::lowerCase( filename.extension().string() ) == ".esm3" ||
		Misc::StringUtils::lowerCase(filename.extension().string()) == ".esp3" )
    {
		mExportOperation = new ExportToTES3();
		std::cout << "TES3 Export Path = " << mExportPath << std::endl;
    }
    else // default to ESM4 format
    {
		mExportOperation = new ExportToTES4();
		std::cout << "TES4 Export Path = " << mExportPath << std::endl;
	}

	OutputDebugString("\n***************Starting Export***************\n");
    
    mStatePtr = new SavingState(*mExportOperation, mExportPath, mEncoding);
    
    mExportOperation->defineExportOperation(mDocument, *mStatePtr); // must querypath and create savingstate before defining operation
//    mExportOperation->defineExportOperation(mDocument, SavingState(*mExportOperation, mExportPath, mEncoding));
    mExportManager.setOperation(mExportOperation); // assign task to thread

    mExportManager.start(); // start thread
}
