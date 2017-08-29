#include "loadltex.hpp"

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
#endif

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int LandTexture::sRecordId = REC_LTEX;

    void LandTexture::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
        bool hasIndex = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'I','N','T','V'>::value:
                    esm.getHT(mIndex);
                    hasIndex = true;
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    mTexture = esm.getHString();
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
        if (!hasIndex)
            esm.fail("Missing INTV subrecord");
    }
    void LandTexture::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        esm.writeHNT("INTV", mIndex);
        esm.writeHNCString("DATA", mTexture);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
        }
    }

	void LandTexture::exportTESx(ESMWriter &esm, bool skipBaseRecords, int export_type) const
	{
		std::string tempStr;
		std::ostringstream debugstream, iconpath;

		// EDID
		tempStr = esm.generateEDIDTES4(mId, false);
		debugstream << "LTEX: EDID=[" << tempStr << "]; ";
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

//		esm.writeHNT("INTV", mIndex);

		// ICON
		tempStr = esm.generateEDIDTES4(mTexture, true);
		int extIndex = tempStr.find("Ptga");
		if (extIndex != tempStr.npos)
		{
			tempStr.replace(extIndex, 4, ".dds");
		}
		iconpath << "morro\\" << tempStr;
		debugstream << "ICON=[" << iconpath.str() << "]; ";
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(iconpath.str());
		esm.endSubRecordTES4("ICON");

		debugstream << std::endl;
//		OutputDebugString(debugstream.str().c_str());

	}

    void LandTexture::blank()
    {
        mTexture.clear();
        mIndex = -1;
    }
}
