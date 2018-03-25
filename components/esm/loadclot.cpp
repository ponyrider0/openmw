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

#include <apps/opencs/model/doc/document.hpp>

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
	bool Clothing::exportTESx(ESMWriter & esm, int export_format) const
	{
		return false;
	}

	bool Clothing::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
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
/*
			if (mParts.mParts.begin()->mMale.size() > 0)
				maleStr = mParts.mParts.begin()->mMale;
			else
				maleStr = mModel;
			if (mParts.mParts.begin()->mFemale.size() > 0)
				femaleStr = maleStr;
			else
				femaleStr = "";
			postFixStream << mData.mType;
*/
			std::stringstream cmdlineM, cmdlineF;
			for (auto part_it = mParts.mParts.begin(); part_it != mParts.mParts.end(); part_it++)
			{
				if (part_it->mMale != "")
				{
					// resolve part_it->mMale to body part record
					int index = doc.getData().getBodyParts().getIndex(part_it->mMale);
					// hardcoded workarounds
					auto bodypartRecord = doc.getData().getBodyParts().getRecord(index);
					std::string partmodel = bodypartRecord.get().mModel;
					if (part_it->mPart == ESM::PartReferenceType::PRT_Shield)
					{
						cmdlineM << " " << partmodel;
						break;
					}
					cmdlineM << " -bp " << (int)part_it->mPart << " " << partmodel;
					if (part_it->mPart == ESM::PartReferenceType::PRT_RHand)
						cmdlineM << " -bp " << (int)ESM::PartReferenceType::PRT_LHand << " " << partmodel;
					if (part_it->mPart == ESM::PartReferenceType::PRT_RWrist)
						cmdlineM << " -bp " << (int)ESM::PartReferenceType::PRT_LWrist << " " << partmodel;
				}
				if (part_it->mFemale != "")
				{
					int index = doc.getData().getBodyParts().getIndex(part_it->mFemale);
					auto bodypartRecord = doc.getData().getBodyParts().getRecord(index);
					std::string partmodel = bodypartRecord.get().mModel;
					int partnum = 0;
					cmdlineF << " -bp " << (int)part_it->mPart << " " << partmodel;
					if (part_it->mPart == ESM::PartReferenceType::PRT_RHand)
						cmdlineF << " -bp " << (int)ESM::PartReferenceType::PRT_LHand << " " << partmodel;
					if (part_it->mPart == ESM::PartReferenceType::PRT_RWrist)
						cmdlineF << " -bp " << (int)ESM::PartReferenceType::PRT_LWrist << " " << partmodel;
				}
			}
			int OblivionType;
			switch (mData.mType)
			{
			case Clothing::Type::Robe:
				if (cmdlineM.str() != "")
				{
					//					cmdlineM << " -bp 2 b\\B_N_Breton_F_Neck.nif";

					cmdlineM << " -bp 8 b\\B_N_Breton_M_Wrist.nif";
					cmdlineM << " -bp 9 b\\B_N_Breton_M_Wrist.nif";
					cmdlineM << " -bp 11 b\\B_N_Breton_M_Forearm.nif";
					cmdlineM << " -bp 12 b\\B_N_Breton_M_Forearm.nif";
					cmdlineM << " -bp 13 \"b\\B_N_Breton_M_Upper Arm.nif\"";
					cmdlineM << " -bp 14 \"b\\B_N_Breton_M_Upper Arm.nif\"";
					cmdlineM << " -bp 17 b\\B_N_Breton_M_Ankle.nif";
					cmdlineM << " -bp 18 b\\B_N_Breton_M_Ankle.nif";
//					cmdlineM << " -bp 19 b\\B_N_Breton_M_Knee.nif";
//					cmdlineM << " -bp 20 b\\B_N_Breton_M_Knee.nif";
				}
				if (cmdlineF.str() != "")
				{
					cmdlineF << " -bp 8 b\\B_N_Breton_F_Wrist.nif";
					cmdlineF << " -bp 9 b\\B_N_Breton_F_Wrist.nif";
					cmdlineF << " -bp 11 b\\B_N_Breton_F_Forearm.nif";
					cmdlineF << " -bp 12 b\\B_N_Breton_F_Forearm.nif";
					cmdlineF << " -bp 13 \"b\\B_N_Breton_F_Upper Arm.nif\"";
					cmdlineF << " -bp 14 \"b\\B_N_Breton_F_Upper Arm.nif\"";
					cmdlineF << " -bp 17 b\\B_N_Breton_F_Ankle.nif";
					cmdlineF << " -bp 18 b\\B_N_Breton_F_Ankle.nif";
//					cmdlineF << " -bp 19 b\\B_N_Breton_F_Knee.nif";
//					cmdlineF << " -bp 20 b\\B_N_Breton_F_Knee.nif";
				}
				OblivionType = 2;
				break;

			case Clothing::Type::Shirt:
				// insert default arms
				if (cmdlineM.str() != "")
				{
					//					cmdlineM << " -bp 2 b\\B_N_Breton_F_Neck.nif";

					cmdlineM << " -bp 8 b\\B_N_Breton_M_Wrist.nif";
					cmdlineM << " -bp 9 b\\B_N_Breton_M_Wrist.nif";
					cmdlineM << " -bp 11 b\\B_N_Breton_M_Forearm.nif";
					cmdlineM << " -bp 12 b\\B_N_Breton_M_Forearm.nif";
					cmdlineM << " -bp 13 \"b\\B_N_Breton_M_Upper Arm.nif\"";
					cmdlineM << " -bp 14 \"b\\B_N_Breton_M_Upper Arm.nif\"";
				}
				if (cmdlineF.str() != "")
				{
					cmdlineF << " -bp 8 b\\B_N_Breton_F_Wrist.nif";
					cmdlineF << " -bp 9 b\\B_N_Breton_F_Wrist.nif";
					cmdlineF << " -bp 11 b\\B_N_Breton_F_Forearm.nif";
					cmdlineF << " -bp 12 b\\B_N_Breton_F_Forearm.nif";
					cmdlineF << " -bp 13 \"b\\B_N_Breton_F_Upper Arm.nif\"";
					cmdlineF << " -bp 14 \"b\\B_N_Breton_F_Upper Arm.nif\"";
				}
				OblivionType = 2;
				break;

			case Clothing::Type::Skirt:
			case Clothing::Type::Pants:
				if (cmdlineM.str() != "")
				{
					cmdlineM << " -bp 17 b\\B_N_Breton_M_Ankle.nif";
					cmdlineM << " -bp 18 b\\B_N_Breton_M_Ankle.nif";
//					cmdlineM << " -bp 19 b\\B_N_Breton_M_Knee.nif";
//					cmdlineM << " -bp 20 b\\B_N_Breton_M_Knee.nif";
				}
				if (cmdlineF.str() != "")
				{
					cmdlineF << " -bp 17 b\\B_N_Breton_F_Ankle.nif";
					cmdlineF << " -bp 18 b\\B_N_Breton_F_Ankle.nif";
//					cmdlineF << " -bp 19 b\\B_N_Breton_F_Knee.nif";
//					cmdlineF << " -bp 20 b\\B_N_Breton_F_Knee.nif";
				}
				OblivionType = 3;
				break;

			case Clothing::Type::Shoes:
				OblivionType = 5;
				break;

			default:
				OblivionType = 6;
				break;
			}
			maleStr = cmdlineM.str();
			femaleStr = cmdlineF.str();

			postFixStream << mData.mType;
			if (OblivionType != 6)
			{
				std::stringstream oblivionTypeStr;
				oblivionTypeStr << OblivionType;
				if (maleStr != "")
				{
					maleStr = "-cl " + oblivionTypeStr.str() + " " + maleStr;
				}
				if (femaleStr != "")
				{
					femaleStr = "-cl " + oblivionTypeStr.str() + " " + femaleStr;
				}
			}

		}
		else
		{
			maleStr = mModel;
		}
		std::string prefix = "clothes\\morro\\";
		std::string postfix = postFixStream.str();
		std::string modelStem;
		if (Misc::StringUtils::lowerCase(mModel).find("_gnd") != std::string::npos)
			modelStem = mModel.substr(0, mModel.find_last_of("_"));
		else
			modelStem = mModel.substr(0, mModel.find_last_of("."));
		modelStr = mModel;
		int queueType = (mData.mType == 0) ? -1 : -mData.mType;
		std::string newMaleStr, newFemaleStr, newGndStr, newIconStr;
		if (maleStr != "")
		{
			newMaleStr = prefix + esm.generateEDIDTES4(modelStem, 1) + postfix + ".nif";
			esm.QueueModelForExport(maleStr, newMaleStr, queueType);
		}
		if (femaleStr != "")
		{
			newFemaleStr = prefix + esm.generateEDIDTES4(modelStem, 1) + postfix + "F.nif";
			esm.QueueModelForExport(femaleStr, newFemaleStr, queueType);
		}
		if (modelStr != "")
		{
			newGndStr = prefix + esm.generateEDIDTES4(modelStem, 1) + postfix + "_gnd.nif";
			esm.QueueModelForExport(modelStr, newGndStr, queueType);
		}
		if (mIcon != "")
		{
			std::string iconStr = mIcon.substr(0, mIcon.find_last_of("."));
			newIconStr = prefix + esm.generateEDIDTES4(iconStr, 1) + ".dds";
			esm.mDDSToExportList.push_back(std::make_pair(mIcon, std::make_pair(newIconStr, 1)));
//			esm.mDDSToExportList[mIcon] = std::make_pair(newIconStr, 1);
		}

		bool bConvertArmor = false;
		if (esm.mConversionOptions.find("#armor") != std::string::npos)
			bConvertArmor = true;

		if (bConvertArmor)
		{
//			esm.exportBipedModelTES4("clothes\\morro\\", postFixStream.str(), maleStr, femaleStr, modelStr, mIcon, ESMWriter::ExportBipedFlags::postfixF);
			esm.exportBipedModelTES4("", "", newMaleStr, newFemaleStr, newGndStr, newIconStr, ESMWriter::ExportBipedFlags::noNameMangling);
		}
		else
		{
			maleStr = esm.substituteClothingModel(mName, 0);
			femaleStr = esm.substituteClothingModel(mName, 1);
			modelStr = esm.substituteClothingModel(mName, 2);
			iconStr = esm.substituteClothingModel(mName, 4);
			esm.exportBipedModelTES4("", "", maleStr, femaleStr, modelStr, iconStr, ESMWriter::ExportBipedFlags::noNameMangling);
		}

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
