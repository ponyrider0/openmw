#ifndef OPENMW_ESM_ACTI_H
#define OPENMW_ESM_ACTI_H

#include <string>

namespace CSMDoc
{
	class Document;
}

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Activator
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Activator"; }

    std::string mId, mName, mScript, mModel;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;
	bool exportTESx(ESMWriter &esm, int export_format) const;
	bool exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
