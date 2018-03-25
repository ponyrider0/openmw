#include "loadltex.hpp"

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
#endif
#include <components/misc/stringops.hpp>

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
		std::string tempStr, strEDID;
		std::ostringstream debugstream, iconpath;

		// EDID
		strEDID = esm.generateEDIDTES4(mId, 0, "LTEX");
		debugstream << "LTEX: EDID=[" << strEDID << "]; ";
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

//		esm.writeHNT("INTV", mIndex);

		// ICON
		tempStr = esm.generateEDIDTES4(mTexture, 1);
		if (tempStr.size() > 4)
		{
			tempStr.replace(tempStr.size()-4, 4, ".dds");
		}
		iconpath << "morro\\" << tempStr;
		debugstream << "ICON=[" << iconpath.str() << "]; ";
		esm.mDDSToExportList.push_back(std::make_pair(mTexture, std::make_pair(iconpath.str(), 0)));
//		esm.mDDSToExportList[mTexture] = std::make_pair(iconpath.str(), 0);
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(iconpath.str());
		esm.endSubRecordTES4("ICON");

		// HNAM, Havok Data
		// TODO: change material type to Morroblivion equivalent
		esm.startSubRecordTES4("HNAM");
		// material type uint8_t
		uint8_t matType = 0; // stone=0, dirt=2, grass=4, water=8, wood=9, snow=14
		// each conditional block is supposed to override previous block
		Misc::StringUtils::lowerCaseInPlace(strEDID);
		if (strEDID.find("stone") != strEDID.npos ||
			strEDID.find("rock") != strEDID.npos ||
			strEDID.find("lava") != strEDID.npos ||
			strEDID.find("gravel") != strEDID.npos ||
			strEDID.find("floor") != strEDID.npos ||
			strEDID.find("street") != strEDID.npos ||
			strEDID.find("ash") != strEDID.npos)
		{
			matType = 0;
		}
		if (strEDID.find("dirt") != strEDID.npos ||
			strEDID.find("sand") != strEDID.npos ||
			strEDID.find("mud") != strEDID.npos ||
			strEDID.find("clover") != strEDID.npos ||
			strEDID.find("farm") != strEDID.npos ||
			strEDID.find("ground") != strEDID.npos ||
			strEDID.find("muck") != strEDID.npos)
		{
			matType = 2;
		}
		if (strEDID.find("scrub") != strEDID.npos ||
			strEDID.find("grass") != strEDID.npos ||
			strEDID.find("clover") != strEDID.npos ||
			strEDID.find("leave") != strEDID.npos ||
			strEDID.find("pine") != strEDID.npos ||
			strEDID.find("growth") != strEDID.npos)
		{
			matType = 4;
		}
		if (strEDID.find("water") != strEDID.npos)
		{
			matType = 8;
		}

		esm.writeT<uint8_t>(matType);
		uint8_t friction = 30;
		esm.writeT<uint8_t>(friction);
		uint8_t restitution = 30;
		esm.writeT<uint8_t>(restitution);
		esm.endSubRecordTES4("HNAM");

		esm.startSubRecordTES4("SNAM");
		uint8_t specularExponent = 30;
		esm.writeT<uint8_t>(specularExponent);
		esm.endSubRecordTES4("SNAM");

		// Grasses Array
//		esm.startSubRecordTES4("GNAM");
//		uint32_t grassFormID = 0;
//		esm.writeT<uint32_t>(grassFormID);
//		esm.endSubRecordTES4("GNAM");

		debugstream << std::endl;
//		OutputDebugString(debugstream.str().c_str());

	}

    void LandTexture::blank()
    {
        mTexture.clear();
        mIndex = -1;
    }
}
