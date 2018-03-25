#include "loadweap.hpp"
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
    unsigned int Weapon::sRecordId = REC_WEAP;

    void Weapon::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

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
                case ESM::FourCC<'W','P','D','T'>::value:
                    esm.getHT(mData, 32);
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
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing WPDT subrecord");
    }
    void Weapon::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("WPDT", mData, 32);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNOCString("ENAM", mEnchant);
    }

	bool Weapon::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream tempPath;

		strEDID = esm.generateEDIDTES4(mId, 0, "WEAP");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		if (mModel.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mModel, 1);
			tempStr.replace(tempStr.size()-4, 4, ".nif");
			tempPath << "weapons\\morro\\" << tempStr;
			esm.QueueModelForExport(mModel, tempPath.str(), 2);
//			tempPath << esm.substituteWeaponModel(mName, 0);
			esm.startSubRecordTES4("MODL");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("MODL");
			// MODB == Bound Radius
			esm.startSubRecordTES4("MODB");
			esm.writeT<float>(25.0);
			esm.endSubRecordTES4("MODB");
		}

		// ICON, mIcon
		if (mIcon.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mIcon, 1);
			tempStr.replace(tempStr.size()-4, 4, ".dds");
			tempPath.str(""); tempPath.clear();
			tempPath << "weapons\\morro\\" << tempStr;
//			tempPath << esm.substituteWeaponModel(mName, 1);
			esm.mDDSToExportList.push_back(std::make_pair(mIcon, std::make_pair(tempPath.str(), 1)));
//			esm.mDDSToExportList[mIcon] = std::make_pair(tempPath.str(), 1);
			esm.startSubRecordTES4("ICON");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("ICON");
		}

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
		if (strScript != "")
		{
			tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
			if (tempFormID != 0)
			{
				esm.startSubRecordTES4("SCRI");
				esm.writeT<uint32_t>(tempFormID);
				esm.endSubRecordTES4("SCRI");
			}
		}

		// ENAM (enchantment formID) mEnchant
		if (mEnchant != "")
		{
			tempFormID = esm.crossRefStringID(mEnchant, "ENCH");
			if (tempFormID != 0)
			{
				esm.startSubRecordTES4("ENAM");
				esm.writeT<uint32_t>(tempFormID);
				esm.endSubRecordTES4("ENAM");
			}
		}

		// ANAM (enchantment points)
		esm.startSubRecordTES4("ANAM");
		esm.writeT<uint16_t>(mData.mEnchant);
		esm.endSubRecordTES4("ANAM");

		// DATA
		esm.startSubRecordTES4("DATA");
		// uint32 type
		int type;
		switch (mData.mType)
		{
		case Weapon::Type::LongBladeOneHand:
		case Weapon::Type::ShortBladeOneHand:
		case Weapon::Type::MarksmanThrown:
			type = 0; // one-hand blade
			break;
		case Weapon::Type::SpearTwoWide:
		case Weapon::Type::LongBladeTwoHand:
			type = 1; // two-hand blade
			break;
		case Weapon::Type::AxeOneHand:
		case Weapon::Type::BluntOneHand:
			type = 2; // one-hand blunt
			break;
		case Weapon::Type::AxeTwoHand:
		case Weapon::Type::BluntTwoClose:
		case Weapon::Type::BluntTwoWide:
			type = 3; // two-hand blunt
			break;
		case Weapon::Type::MarksmanBow:
		case Weapon::Type::MarksmanCrossbow:
			type = 5;
			break;
		}
		esm.writeT<uint32_t>(type);
		// float speed
		esm.writeT<float>(mData.mSpeed);
		// float reach
		esm.writeT<float>(mData.mReach);
		// uint32 flags [ ignores normal weapon resistance ]
		int flags=0;
		if (mData.mFlags > 0)
			flags |= 0x01;
		esm.writeT<uint32_t>(flags);
		// uint32 value
		esm.writeT<uint32_t>(mData.mValue);
		// uint32 health
		esm.writeT<uint32_t>(mData.mHealth);
		// float weight
		esm.writeT<float>(mData.mWeight);
		// uint16 damage
		uint16_t damage=0;
		int chop = (mData.mChop[0] + mData.mChop[1])/2;
		int slash = (mData.mSlash[0] + mData.mSlash[1])/2;
		int thrust = (mData.mThrust[0] + mData.mThrust[1])/2;
		damage = (chop > slash) ? chop : slash;
		damage = (damage > thrust) ? damage : thrust;
		esm.writeT<uint16_t>(damage);
		esm.endSubRecordTES4("DATA");

		return true;
	}

	bool Weapon::exportAmmoTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId, 0, "AMMO");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		if (mModel.size() > 4)
		{
//			tempStr = esm.generateEDIDTES4(mModel, 1);
//			tempStr.replace(tempStr.size()-4, 4, ".nif");
//			tempPath << "weapons\\morro\\" << tempStr;
			tempPath << esm.substituteWeaponModel(mName, 0);
			esm.startSubRecordTES4("MODL");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("MODL");
			// MODB == Bound Radius
			esm.startSubRecordTES4("MODB");
			esm.writeT<float>(5.0);
			esm.endSubRecordTES4("MODB");
		}

		// ICON, mIcon
		if (mIcon.size() > 4)
		{
//			tempStr = esm.generateEDIDTES4(mIcon, 1);
//			tempStr.replace(tempStr.size()-4, 4, ".dds");
			tempPath.str(""); tempPath.clear();
//			tempPath << "weapons\\morro\\" << tempStr;
			tempPath << esm.substituteWeaponModel(mName, 1);
			esm.startSubRecordTES4("ICON");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("ICON");
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

		// DATA
		esm.startSubRecordTES4("DATA");
		// float speed
		esm.writeT<float>(mData.mSpeed);
		// uint32 flags [ ignores normal weapon resistance ]
		int flags=0;
		if (mData.mFlags > 0)
			flags |= 0x01;
		esm.writeT<uint8_t>(flags);
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused						
		// uint32 value
		esm.writeT<uint32_t>(mData.mValue);
		// float weight
		esm.writeT<float>(mData.mWeight);
		// uint16 damage
		uint16_t damage=0;
		int chop = (mData.mChop[0] + mData.mChop[1])/2;
		int slash = (mData.mSlash[0] + mData.mSlash[1])/2;
		int thrust = (mData.mThrust[0] + mData.mThrust[1])/2;
		damage = (chop > slash) ? chop : slash;
		damage = (damage > thrust) ? damage : thrust;
		esm.writeT<uint16_t>(damage);
		esm.endSubRecordTES4("DATA");

		return true;
	}


    void Weapon::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mType = 0;
        mData.mHealth = 0;
        mData.mSpeed = 0;
        mData.mReach = 0;
        mData.mEnchant = 0;
        mData.mChop[0] = mData.mChop[1] = 0;
        mData.mSlash[0] = mData.mSlash[1] = 0;
        mData.mThrust[0] = mData.mThrust[1] = 0;
        mData.mFlags = 0;

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant.clear();
        mScript.clear();
    }
}
