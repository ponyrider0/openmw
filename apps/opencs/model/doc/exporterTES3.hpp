#ifndef CSM_DOC_EXPORTER_TES3_H
#define CSM_DOC_EXPORTER_TES3_H

#include "exporter.hpp"

namespace CSMDoc
{
	class Exporter;
	class SavingState;

    class TES3Exporter : public Exporter
    {
		Q_OBJECT

		public:
			TES3Exporter(Document& document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding);
			virtual void defineExportOperation();

    };

}

#endif
