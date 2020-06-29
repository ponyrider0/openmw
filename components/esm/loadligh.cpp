#include "loadligh.hpp"

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
    unsigned int Light::sRecordId = REC_LIGH;

    void Light::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
                    break;
                case ESM::FourCC<'L','H','D','T'>::value:
                    esm.getHT(mData, 24);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                    mSound = esm.getHString();
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
            esm.fail("Missing LHDT subrecord");
    }
    void Light::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNT("LHDT", mData, 24);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("SNAM", mSound);
    }
	bool Light::exportTESx(ESMWriter &esm, int export_format) const
	{
		return false;
	}
	bool Light::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;
		std::ostringstream modelPath;
		bool bSubstitute = false;

		tempStr = esm.generateEDIDTES4(mId, 0, "LIGH");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// MODL == Model Filename
		std::string nifInputName;
		if (mModel.size() > 4)
		{
			nifInputName = "meshes/" + Misc::ResourceHelpers::correctActorModelPath(mModel, doc.getVFS());
			doc.getVFS()->normalizeFilename(nifInputName);
			try {
				// Sanity CHECK: Make sure BSA is not blacklisted
				std::string archiveName = Misc::StringUtils::lowerCase(doc.getVFS()->lookupArchive(nifInputName));
				if (archiveName.find("morrowind.bsa") != std::string::npos ||
					archiveName.find("tribunal.bsa") != std::string::npos ||
					archiveName.find("bloodmoon.bsa") != std::string::npos)
				{
					bSubstitute = true;
				}
			}
			catch (std::runtime_error e)
			{
				std::string errString(e.what());
				std::cout << "Light::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
			}

			tempStr = esm.generateEDIDTES4(mModel, 1);
			tempStr.replace(tempStr.size() - 4, 4, ".nif");
			modelPath << "lights\\morro\\" << tempStr;
			esm.QueueModelForExport(mModel, modelPath.str());
			if (bSubstitute) {
				modelPath.str(esm.substituteLightModel(modelPath.str(), 0));
			}
			esm.startSubRecordTES4("MODL");
			esm.writeHCString(modelPath.str());
			esm.endSubRecordTES4("MODL");

			if (bSubstitute == false) {
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
					std::cout << "Light::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
				}

			}

			// MODB == Bound Radius
			esm.startSubRecordTES4("MODB");
			esm.writeT<float>(50.0);
			esm.endSubRecordTES4("MODB");
			// MODT
		}

		// SCRI (script formID) mScript
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		if (mName.size() > 0)
		{
			esm.startSubRecordTES4("FULL");
			esm.writeHCString(mName);
			esm.endSubRecordTES4("FULL");
		}

		// ICON, mIcon
		tempPath.str(""); tempPath.clear();
		if (mIcon.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mIcon, 1);
			tempStr.replace(tempStr.size() - 4, 4, ".dds");
			tempPath << "lights\\morro\\" << tempStr;
		}
		esm.mDDSToExportList.push_back(std::make_pair(mIcon, std::make_pair(tempPath.str(), 1)));
//			esm.mDDSToExportList[mIcon] = std::make_pair(tempPath.str(), 1);
		if (bSubstitute) {
			tempPath.str(esm.substituteLightModel(tempPath.str(), 1));
		}
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("ICON");

		// DATA, {time, radius, color, flags, falloff, fov, value, weight}
		esm.startSubRecordTES4("DATA");
		esm.writeT<int32_t>(mData.mTime);
		esm.writeT<uint32_t>(mData.mRadius);
		esm.writeT<uint32_t>(mData.mColor);
		esm.writeT<uint32_t>(mData.mFlags);
		esm.writeT<float>(1); // falloff exponent
		esm.writeT<float>(90); // FOV
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// FNAM, fade value
		esm.startSubRecordTES4("FNAM");
		esm.writeT<float>(0.33);
		esm.endSubRecordTES4("FNAM");

		// SNAM (sound formID)
		tempFormID = esm.crossRefStringID(mSound, "SOUN");
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SNAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SNAM");
		}

		return true;
	}

    void Light::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mTime = 0;
        mData.mRadius = 0;
        mData.mColor = 0;
        mData.mFlags = 0;
        mSound.clear();
        mScript.clear();
        mModel.clear();
        mIcon.clear();
        mName.clear();
    }
}
