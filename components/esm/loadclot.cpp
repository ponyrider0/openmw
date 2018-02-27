#include "loadclot.hpp"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Clothing::sRecordId = REC_CLOT;

    void Clothing::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mParts.mParts.clear();

        bool hasName = false;
        bool hasData = false;
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
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'C','T','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
                    break;
                case ESM::FourCC<'E','N','A','M'>::value:
                    mEnchant = esm.getHString();
                    break;
                case ESM::FourCC<'I','N','D','X'>::value:
                    mParts.add(esm);
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
        if (!hasData && !isDeleted)
            esm.fail("Missing CTDT subrecord");
    }

    void Clothing::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CTDT", mData, 12);

        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);

        mParts.save(esm);

        esm.writeHNOCString("ENAM", mEnchant);
    }
	bool Clothing::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream tempPath;

		strEDID = esm.generateEDIDTES4(mId, 0, "CLOT");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// SCRI (script formID) mScript
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		if (mEnchant != "" && mScript == "")
		{
			// TODO: use ItemScript or TargetItemScript based on enchantment
			strScript = "mwCWUItemScript";
		}
		else if (mScript != "")
		{
			std::cout << "WARNING: enchanted item already has script: " << strEDID << std::endl;
		}
		tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// ENAM (enchantment formID) mEnchant
		tempFormID = esm.crossRefStringID(mEnchant, "ENCH");
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("ENAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("ENAM");
		}

		// ANAM (enchantment points)
		esm.startSubRecordTES4("ANAM");
		esm.writeT<uint16_t>(mData.mEnchant);
		esm.endSubRecordTES4("ANAM");

		// BMDT Flags (dword)
		int flags=0;
		switch (mData.mType)
		{
		case Clothing::Type::Shirt:
			flags = 0x0004; // upperbody 0x04
			break;
		case Clothing::Type::Skirt:
		case Clothing::Type::Pants:
			flags = 0x0008; // lowerbody 0x08
			break;
		case Clothing::Type::Robe:
			flags = 0x0004 | 0x0008; // upperbody 0x04, lowerbody 0x08
			break;
		case Clothing::Type::Shoes:
			flags = 0x0020; // foot 0x20
			break;
		case Clothing::Type::LGlove:
		case Clothing::Type::RGlove:
			flags = 0x0010; // hand 0x10
			break;
		case Clothing::Type::Belt:
			flags = 0x8000; // tail 0x8000
			break;
		case Clothing::Type::Ring:
			flags = 0x0040; // right ring or (0x80 left ring)
			break;
		case Clothing::Type::Amulet:
			flags = 0x100; // right ring or (0x80 left ring)
			break;
		}
		esm.startSubRecordTES4("BMDT");
		esm.writeT<uint16_t>(flags); // biped flags
		flags=0;
		esm.writeT<uint8_t>(flags); // general flags
		esm.writeT<uint8_t>(0); // unused
		esm.endSubRecordTES4("BMDT");

		std::ostringstream postFixStream;
		std::string maleStr, femaleStr, modelStr, iconStr;
		if (mParts.mParts.size() > 0)
		{
			if (mParts.mParts.begin()->mMale.size() > 0)
				maleStr = mParts.mParts.begin()->mMale;
			else
				maleStr = mModel;
			if (mParts.mParts.begin()->mFemale.size() > 0)
				femaleStr = maleStr;
			else
				femaleStr = "";
			postFixStream << mData.mType;
		}
		else
		{
			maleStr = mModel;
		}
		modelStr = mModel;
//		esm.exportBipedModelTES4("clothes\\morro\\", postFixStream.str(), maleStr, femaleStr, modelStr, mIcon, ESMWriter::ExportBipedFlags::postfixF);

		maleStr = esm.substituteClothingModel(mName, 0);
		femaleStr = esm.substituteClothingModel(mName, 1);
		modelStr = esm.substituteClothingModel(mName, 2);
		iconStr = esm.substituteClothingModel(mName, 4);
		esm.exportBipedModelTES4("", "", maleStr, femaleStr, modelStr, iconStr, ESMWriter::ExportBipedFlags::noNameMangling);

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		return true;
	}

    void Clothing::blank()
    {
        mData.mType = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mEnchant = 0;
        mParts.mParts.clear();
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant.clear();
        mScript.clear();
    }
}
