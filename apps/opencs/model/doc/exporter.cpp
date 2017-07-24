#include "exporter.hpp"

CSMDoc::Exporter::Exporter(Document& document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding)
  : OperationHolder (&mExportOperation),
  mExportOperation(State_Operation, true, true), 
  mDocument(document), 
  mState(mExportOperation, projectPath, encoding)
{
	// Set up Operation object
	defineExportOperation();

	// Assign Operation to parent object (sets up thread and signal/slot connections)
	//			OperationHolder::setOperation(&mExportOperation);

}

void CSMDoc::Exporter::defineExportOperation()
{
}
