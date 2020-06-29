#include "loadbook.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

#include <boost/filesystem.hpp>
#include "../nif/niffile.hpp"
#include "../nif/controlled.hpp"

#include <apps/opencs/model/doc/document.hpp>
#include <components/vfs/manager.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace ESM
{
    unsigned int Book::sRecordId = REC_BOOK;

    void Book::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'B','K','D','T'>::value:
                    esm.getHT(mData, 20);
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
                case ESM::FourCC<'T','E','X','T'>::value:
                    mText = esm.getHString();
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
            esm.fail("Missing BKDT subrecord");
    }
    void Book::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("BKDT", mData, 20);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNOString("TEXT", mText);
        esm.writeHNOCString("ENAM", mEnchant);
    }
	bool Book::exportTESx(ESMWriter & esm, int export_format) const
	{
		return false;
	}
	bool Book::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;
		std::ostringstream modelPath;
		bool bSubstitute = false;

		tempStr = esm.generateEDIDTES4(mId, 0, "BOOK");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		std::string nifInputName = "meshes/" + Misc::ResourceHelpers::correctActorModelPath(mModel, doc.getVFS());
		doc.getVFS()->normalizeFilename(nifInputName);
		// Sanity CHECK: Make sure BSA is not blacklisted
		std::string archiveName = Misc::StringUtils::lowerCase(doc.getVFS()->lookupArchive(nifInputName));
		if (archiveName.find("morrowind.bsa") != std::string::npos ||
			archiveName.find("tribunal.bsa") != std::string::npos ||
			archiveName.find("bloodmoon.bsa") != std::string::npos)
		{
			bSubstitute = true;
		}

		// MODL == Model Filename
		if (bSubstitute)
			modelPath << esm.substituteBookModel(mName + (mData.mIsScroll ? "sc" : ""), 0);
		else {
			tempStr = esm.generateEDIDTES4(mModel, 1);
			tempStr.replace(tempStr.size() - 4, 4, ".nif");
			modelPath << "clutter\\books\\morro\\" << tempStr;
			esm.QueueModelForExport(mModel, modelPath.str());
		}
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(modelPath.str());
		esm.endSubRecordTES4("MODL");

		if (bSubstitute == false)
		{
			bool bBlenderOutput = false;
			if (esm.mConversionOptions.find("#blender") != std::string::npos)
				bBlenderOutput = true;

			int vwdMode = VWD_MODE_NORMAL_ONLY;
			if (esm.mConversionOptions.find("#vwd") != std::string::npos)
				vwdMode = VWD_MODE_NORMAL_AND_LOD; // normals + vwd
			if (esm.mConversionOptions.find("#vwdonly") != std::string::npos)
			{
				vwdMode = VWD_MODE_LOD_ONLY; // no normal nifs
				if (esm.mConversionOptions.find("#vwdonly++") != std::string::npos)
					vwdMode = VWD_MODE_LOD_AND_LARGE_NORMAL; // vwds + large normals
			}

			float vwdThreshold = VWD_QUAL_MEDIUM;
			if (esm.mConversionOptions.find("#vwdfast") != std::string::npos)
			{
				vwdThreshold = VWD_QUAL_LOW;
			}
			if (esm.mConversionOptions.find("#vwdhd") != std::string::npos)
			{
				vwdThreshold = VWD_QUAL_HIGH;
			}
			if (esm.mConversionOptions.find("#vwdultra") != std::string::npos)
			{
				vwdThreshold = VWD_QUAL_ULTRA;
			}

			float modelBounds = 0.0f;
			// ** Load NIF and get model's true Bound Radius
			try
			{
				Files::IStreamPtr fileStream = NULL;
				fileStream = doc.getVFS()->get(nifInputName);
				// read stream into NIF parser...
				Nif::NIFFile nifFile(fileStream, nifInputName);
				modelBounds = nifFile.mModelBounds;

				if (bBlenderOutput)
				{
					nifFile.prepareExport(doc, esm, modelPath.str());
					if (vwdMode != VWD_MODE_LOD_ONLY)
					{
						if (vwdMode == VWD_MODE_LOD_AND_LARGE_NORMAL && modelBounds < vwdThreshold)
						{
							// skip
						}
						else
						{
							std::string filePath = Nif::NIFFile::CreateResourcePaths(modelPath.str());
							nifFile.exportFileNif(esm, fileStream, filePath);
						}
					}

					if (vwdMode != VWD_MODE_NORMAL_ONLY && modelBounds >= vwdThreshold)
					{
						std::string filePath = Nif::NIFFile::CreateResourcePaths(modelPath.str());
						nifFile.exportFileNifFar(esm, fileStream, filePath);
					}
				}

			}
			catch (std::runtime_error e)
			{
				std::string errString(e.what());
				std::cout << "Book::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
			}

		}

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(30.0);
		esm.endSubRecordTES4("MODB");
		// MODT
		// ICON, mIcon
		tempPath.str(""); tempPath.clear();
		if (bSubstitute) {
			tempPath << esm.substituteBookModel(mName + (mData.mIsScroll ? "sc" : ""), 1);
		}
		else {
			if (mIcon.size() > 4)
			{
				tempStr = esm.generateEDIDTES4(mIcon, 1);
				tempStr.replace(tempStr.size() - 4, 4, ".dds");
				tempPath << "clutter\\morro\\" << tempStr;
			}
			esm.mDDSToExportList.push_back(std::make_pair(mIcon, std::make_pair(tempPath.str(), 1)));
			//		esm.mDDSToExportList[mIcon] = std::make_pair(tempPath.str(), 1);

		}
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("ICON");

		// SCRI (script formID) mScript
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
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

		// DESC, mText		
		// replace all Morrowind variable strings with static words
		std::string newText = mText;
		// There are two passes for each variable: 
		//	the first pass is the ideal match which will replace the leading space,
		//	the second pass is a fuzzy match which will add an extra leading space.
		newText = Misc::StringUtils::replaceAll(newText, " %PCName", " outlander");
		newText = Misc::StringUtils::replaceAll(newText, "%PCName", "Outlander");
		// first pass catches uncapitalized instances of words
		// second pass should catch remaining instances needing capitalization
		newText = Misc::StringUtils::replaceAll(newText, " %PCRace", " outlander");
		newText = Misc::StringUtils::replaceAll(newText, "%PCRace", "Outlander");
		newText = Misc::StringUtils::replaceAll(newText, " %PCClass", " outlander");
		newText = Misc::StringUtils::replaceAll(newText, "%PCClass", "Outlander");
		newText = Misc::StringUtils::replaceAll(newText, " %PCRank", " outlander");
		newText = Misc::StringUtils::replaceAll(newText, "%PCRank", "Outlander");

		esm.startSubRecordTES4("DESC");
		esm.writeHCString(newText);
		esm.endSubRecordTES4("DESC");

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		uint8_t flags=0;
		if (mData.mIsScroll)
			flags |= 0x01;
		esm.writeT<uint8_t>(flags); // flags
		int skill = esm.skillToActorValTES4(mData.mSkillId);
		if (skill >= 12)
			skill = skill-12;
		esm.writeT<uint8_t>(skill); // teaches skillenum (0-22)
		esm.writeT<uint32_t>(mData.mValue); // value
		esm.writeT<float>(mData.mWeight); // weight
		esm.endSubRecordTES4("DATA");

		return true;
	}

    void Book::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mIsScroll = 0;
        mData.mSkillId = 0;
        mData.mEnchant = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEnchant.clear();
        mText.clear();
    }
}
