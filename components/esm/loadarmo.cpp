#include "loadarmo.hpp"
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

    void PartReferenceList::add(ESMReader &esm)
    {
        PartReference pr;
        esm.getHT(pr.mPart); // The INDX byte
        pr.mMale = esm.getHNOString("BNAM");
        pr.mFemale = esm.getHNOString("CNAM");
        mParts.push_back(pr);

    }

    void PartReferenceList::load(ESMReader &esm)
    {
        mParts.clear();
        while (esm.isNextSub("INDX"))
        {
            add(esm);
        }
    }

    void PartReferenceList::save(ESMWriter &esm) const
    {
        for (std::vector<PartReference>::const_iterator it = mParts.begin(); it != mParts.end(); ++it)
        {
            esm.writeHNT("INDX", it->mPart);
            esm.writeHNOString("BNAM", it->mMale);
            esm.writeHNOString("CNAM", it->mFemale);
        }
    }

    unsigned int Armor::sRecordId = REC_ARMO;

    void Armor::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'A','O','D','T'>::value:
                    esm.getHT(mData, 24);
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

    void Armor::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNT("AODT", mData, 24);
        esm.writeHNOCString("ITEX", mIcon);
        mParts.save(esm);
        esm.writeHNOCString("ENAM", mEnchant);
    }
	bool Armor::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream tempPath;

		strEDID = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// ENAM (enchantment formID) mEnchant
		tempFormID = esm.crossRefStringID(mEnchant);
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
		uint32_t flags=0;
		switch (mData.mType)
		{
		case Armor::Type::Helmet:
			flags = 0x0002; // head=0x01, hair=0x02
			break;
		case Armor::Type::Cuirass:
			flags = 0x0004; // upperbody 0x04
			break;
		case Armor::Type::Greaves:
			flags = 0x0008; // lowerbody 0x08
			break;
		case Armor::Type::Boots:
			flags = 0x0020; // foot 0x20
			break;
		case Armor::Type::Shield:
			flags = 0x2000; // shield 0x2000
			break;
		case Armor::Type::RGauntlet:
			flags = 0x0010; // hand 0x10
			break;
		}
		esm.startSubRecordTES4("BMDT");
		esm.writeT<uint32_t>(flags);
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
//		esm.exportBipedModelTES4("armor\\morro\\", postFixStream.str(), maleStr, femaleStr, modelStr, mIcon, ESMWriter::ExportBipedFlags::postfix_gnd | ESMWriter::ExportBipedFlags::postfixF);

		maleStr = esm.substituteArmorModel(mName, 0);
		femaleStr = esm.substituteArmorModel(mName, 1);
		modelStr = esm.substituteArmorModel(mName, 2);
		iconStr = esm.substituteArmorModel(mName, 4);
		esm.exportBipedModelTES4("", "", maleStr, femaleStr, modelStr, iconStr, ESMWriter::ExportBipedFlags::noNameMangling);

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint16_t>(mData.mArmor * 100);
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<uint32_t>(mData.mHealth);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// SCRI (script formID) mScript
		std::string strScript = esm.generateEDIDTES4(mScript);
		if (mEnchant != "" && mScript == "")
		{
			// TODO: use ItemScript or TargetItemScript based on enchantment
			strScript = "mwCWUItemScript";
		}
		else if (mScript != "")
		{
			std::cout << "WARNING: enchanted item already has script: " << strEDID << std::endl;
		}
		if (Misc::StringUtils::lowerCase(strScript).find("sc", strScript.size()-2) == strScript.npos)
		{
			strScript += "Script";
		}
		tempFormID = esm.crossRefStringID(strScript, false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		return true;
	}

    void Armor::blank()
    {
        mData.mType = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mHealth = 0;
        mData.mEnchant = 0;
        mData.mArmor = 0;
        mParts.mParts.clear();
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEnchant.clear();
    }
}
