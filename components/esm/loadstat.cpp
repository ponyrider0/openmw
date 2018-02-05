#include "loadstat.hpp"

#include <iostream>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Static::sRecordId = REC_STAT;

    void Static::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'M','O','D','L'>::value:
                    mModel = esm.getHString();
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
    }
    void Static::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
        }
        else
        {
            esm.writeHNCString("MODL", mModel);
        }
    }
	bool Static::exportTESx(ESMWriter &esm, int export_format) const
	{
		std::ostringstream modelPath;
		std::string tempStr;

		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		modelPath << "morro\\" << tempStr;
		int recordType = 0;
		if (Misc::StringUtils::lowerCase(modelPath.str()).find("\\x\\") != std::string::npos ||
			Misc::StringUtils::lowerCase(modelPath.str()).find("terrain") != std::string::npos )
		{
			recordType = 4;
		}
		esm.QueueModelForExport(mModel, modelPath.str(), recordType);
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(modelPath.str());
		esm.endSubRecordTES4("MODL");

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(100.0);
		esm.endSubRecordTES4("MODB");

		// Optional: MODT == "Texture Files Hashes" (byte array)

		return true;
	}

    void Static::blank()
    {
        mModel.clear();
    }
}
