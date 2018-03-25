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

#include <apps/opencs/model/doc/document.hpp>

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

	bool Armor::exportTESx(ESMWriter & esm, int export_format) const
	{
		return false;
	}

	bool Armor::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream tempPath;

		strEDID = esm.generateEDIDTES4(mId, 0, "ARMO");
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
			std::cout << "WARNING: enchanted item already has script: " << strEDID << " - " << strScript << std::endl;
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
/*
			if (mParts.mParts.begin()->mMale.size() > 0)
				maleStr = mParts.mParts.begin()->mMale;
			else
				maleStr = mModel;
			if (mParts.mParts.begin()->mFemale.size() > 0)
				femaleStr = mParts.mParts.begin()->mFemale;
//				femaleStr = maleStr;
			else
				femaleStr = "";
*/
			// each body part must be added to commandline
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
					cmdlineM << " -bp " << (int) part_it->mPart << " " << partmodel;
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
			case ESM::Armor::Type::Helmet:
				OblivionType = 1;
				break;

			case ESM::Armor::Type::Cuirass:
				// insert default arms
				if (cmdlineM.str() != "")
				{
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_RPauldron << " " << "tr\\a\\tr_a_silver_pauld_CL.nif";
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_LPauldron << " " << "tr\\a\\tr_a_silver_pauld_CL.nif";
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_RUpperarm << " " << "tr\\a\\tr_a_silver_pauld_UA.nif";
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_LUpperarm << " " << "tr\\a\\tr_a_silver_pauld_UA.nif";
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_RWrist << " " << "tr\\a\\tr_a_silver_bracer_W.nif";
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_LWrist << " " << "tr\\a\\tr_a_silver_bracer_W.nif";
//					cmdlineM << " -bp 2 b\\B_N_Breton_F_Neck.nif";

/*
					cmdlineM << " -bp 8 b\\B_N_Breton_M_Wrist.nif";
					cmdlineM << " -bp 9 b\\B_N_Breton_M_Wrist.nif";
					cmdlineM << " -bp 11 b\\B_N_Breton_M_Forearm.nif";
					cmdlineM << " -bp 12 b\\B_N_Breton_M_Forearm.nif";
					cmdlineM << " -bp 13 \"b\\B_N_Breton_M_Upper Arm.nif\"";
					cmdlineM << " -bp 14 \"b\\B_N_Breton_M_Upper Arm.nif\"";
*/
// Better Clothes
					cmdlineM << " -bp 8 bc\\BC_common_shirt_t.nif";
					cmdlineM << " -bp 9 bc\\BC_common_shirt_t.nif";
					cmdlineM << " -bp 11 bc\\BC_common_shirt_t.nif";
					cmdlineM << " -bp 12 bc\\BC_common_shirt_t.nif";
					cmdlineM << " -bp 13 bc\\BC_common_shirt_t.nif";
					cmdlineM << " -bp 14 bc\\BC_common_shirt_t.nif";
/*
// TR ------------
					cmdlineM << " -bp 8 TR\\c\\tr_c_shirt_com_001_w.nif";
					cmdlineM << " -bp 9 TR\\c\\tr_c_shirt_com_001_w.nif";
					cmdlineM << " -bp 11 TR\\c\\tr_c_shirt_com_001_fa.nif";
					cmdlineM << " -bp 12 TR\\c\\tr_c_shirt_com_001_fa.nif";
					cmdlineM << " -bp 13 TR\\c\\tr_c_shirt_com_001_ua.nif";
					cmdlineM << " -bp 14 TR\\c\\tr_c_shirt_com_001_ua.nif";
*/
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_RPauldron << " " << "tr\\a\\tr_a_silver_pauld_CL.nif";
//					cmdlineM << " -bp " << ESM::PartReferenceType::PRT_LPauldron << " " << "tr\\a\\tr_a_silver_pauld_CL.nif";
				}
				if (cmdlineF.str() != "")
				{
/*
					cmdlineF << " -bp 8 b\\B_N_Breton_F_Wrist.nif";
					cmdlineF << " -bp 9 b\\B_N_Breton_F_Wrist.nif";
					cmdlineF << " -bp 11 b\\B_N_Breton_F_Forearm.nif";
					cmdlineF << " -bp 12 b\\B_N_Breton_F_Forearm.nif";
					cmdlineF << " -bp 13 \"b\\B_N_Breton_F_Upper Arm.nif\"";
					cmdlineF << " -bp 14 \"b\\B_N_Breton_F_Upper Arm.nif\"";
*/
// Better Clothes
					cmdlineF << " -bp 8 bc\\BC_common_shirt_t.nif";
					cmdlineF << " -bp 9 bc\\BC_common_shirt_t.nif";
					cmdlineF << " -bp 11 bc\\BC_common_shirt_t.nif";
					cmdlineF << " -bp 12 bc\\BC_common_shirt_t.nif";
					cmdlineF << " -bp 13 bc\\BC_common_shirt_t.nif";
					cmdlineF << " -bp 14 bc\\BC_common_shirt_t.nif";
/*
					cmdlineF << " -bp 8 TR\\c\\tr_c_shirt_com_001_w.nif";
					cmdlineF << " -bp 9 TR\\c\\tr_c_shirt_com_001_w.nif";
					cmdlineF << " -bp 11 TR\\c\\tr_c_shirt_com_001_fa.nif";
					cmdlineF << " -bp 12 TR\\c\\tr_c_shirt_com_001_fa.nif";
					cmdlineF << " -bp 13 TR\\c\\tr_c_shirt_com_001_ua.nif";
					cmdlineF << " -bp 14 TR\\c\\tr_c_shirt_com_001_ua.nif";
*/
				}
				OblivionType = 2;
				break;

			case ESM::Armor::Type::Greaves:
				if (cmdlineM.str() != "")
				{
					cmdlineM << " -bp 17 b\\B_N_Breton_M_Ankle.nif";
					cmdlineM << " -bp 18 b\\B_N_Breton_M_Ankle.nif";
					cmdlineM << " -bp 19 b\\B_N_Breton_M_Knee.nif";
					cmdlineM << " -bp 20 b\\B_N_Breton_M_Knee.nif";
/*
					cmdlineM << " -bp 17 TR\\c\\tr_c_pant_com_001_a.nif";
					cmdlineM << " -bp 18 TR\\c\\tr_c_pant_com_001_a.nif";
					cmdlineM << " -bp 19 TR\\c\\tr_c_pant_com_001_k.nif";
					cmdlineM << " -bp 20 TR\\c\\tr_c_pant_com_001_k.nif";
*/
				}
				if (cmdlineF.str() != "")
				{
					cmdlineF << " -bp 17 b\\B_N_Breton_F_Ankle.nif";
					cmdlineF << " -bp 18 b\\B_N_Breton_F_Ankle.nif";
					cmdlineF << " -bp 19 b\\B_N_Breton_F_Knee.nif";
					cmdlineF << " -bp 20 b\\B_N_Breton_F_Knee.nif";
/*
					cmdlineF << " -bp 17 TR\\c\\tr_c_pant_com_001_a.nif";
					cmdlineF << " -bp 18 TR\\c\\tr_c_pant_com_001_a.nif";
					cmdlineF << " -bp 19 TR\\c\\tr_c_pant_com_001_k.nif";
					cmdlineF << " -bp 20 TR\\c\\tr_c_pant_com_001_k.nif";
*/
				}
				OblivionType = 3;
				break;

			case ESM::Armor::Type::Boots:
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
			maleStr = modelStr;
		}
		std::string prefix = "armor\\morro\\";
		std::string postfix = postFixStream.str();
		std::string modelStem;
		if (Misc::StringUtils::lowerCase(mModel).find("_gnd") != std::string::npos)
			modelStem = mModel.substr(0, mModel.find_last_of("_"));
		else
			modelStem = mModel.substr(0, mModel.find_last_of("."));
		modelStr = mModel;
		std::string newMaleStr, newFemaleStr, newGndStr, newIconStr;
		int queueType = (mData.mType == 0) ? -1 : -mData.mType;
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
//			esm.exportBipedModelTES4("armor\\morro\\", postFixStream.str(), maleStr, femaleStr, modelStr, mIcon, ESMWriter::ExportBipedFlags::postfix_gnd | ESMWriter::ExportBipedFlags::postfixF);
			esm.exportBipedModelTES4("", "", newMaleStr, newFemaleStr, newGndStr, newIconStr, ESMWriter::ExportBipedFlags::noNameMangling);
		}
		else
		{
			maleStr = esm.substituteArmorModel(mName, 0);
			femaleStr = esm.substituteArmorModel(mName, 1);
			modelStr = esm.substituteArmorModel(mName, 2);
//			iconStr = esm.substituteArmorModel(mName, 4);
			esm.exportBipedModelTES4("", "", maleStr, femaleStr, modelStr, newIconStr, ESMWriter::ExportBipedFlags::noNameMangling);
		}

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint16_t>(mData.mArmor * 100);
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<uint32_t>(mData.mHealth);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

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
