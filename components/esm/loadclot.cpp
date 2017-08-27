#include "loadclot.hpp"

#include <iostream>

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
		std::string *tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId, false);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(*tempStr);
		esm.endSubRecordTES4("EDID");
		delete tempStr;

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// BMDT Flags (dword)
		uint32_t flags=0;
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
		case Clothing::Type::RGlove:
			flags = 0x0010; // hand 0x10
			break;
		case Clothing::Type::Belt:
			flags = 0x8000; // tail 0x8000
			break;
		}
		esm.startSubRecordTES4("BMDT");
		esm.writeT<uint32_t>(flags);
		esm.endSubRecordTES4("BMDT");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, true);
//		std::cout << "CLOTHING EXPORT: mModel=" << *tempStr << std::endl;
		tempStr->replace(tempStr->size()-4, 4, ".nif");
		tempPath << "clothes\\morro\\" << *tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("MODL");
		delete tempStr;
		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(0.0);
		esm.endSubRecordTES4("MODB");

		// MOD2, MOD3, MOD4
		tempStr = esm.generateEDIDTES4(mModel, true);
		tempStr->replace(tempStr->size()-4, 4, "_gnd");
		tempPath.str(""); tempPath.clear();
		tempPath << "clothes\\morro\\" << *tempStr << ".nif";
		esm.startSubRecordTES4("MOD2");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("MOD2");
		delete tempStr;
		// MODB == Bound Radius
		esm.startSubRecordTES4("MO2B");
		esm.writeT<float>(0.0);
		esm.endSubRecordTES4("MO2B");

		// MO2B, MO3B, MO4B
		// MODT
		// MO2T, MO3T, MO4T

		// ICON, mIcon
		tempStr = esm.generateEDIDTES4(mIcon, true);
		if (tempStr->size() > 4)
		{
			tempStr->replace(tempStr->size()-4, 4, ".dds");
			tempPath.str(""); tempPath.clear();
			tempPath << "clothes\\morro\\" << *tempStr;
			esm.startSubRecordTES4("ICON");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("ICON");
		}
		delete tempStr;

		// ICO2

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// SCRI (script formID) mScript
		// ENAM (enchantment formID) mEnchant

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
