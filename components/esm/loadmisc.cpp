#include "loadmisc.hpp"

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
    unsigned int Miscellaneous::sRecordId = REC_MISC;

    void Miscellaneous::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'M','C','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
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
            esm.fail("Missing MCDT subrecord");
    }

    void Miscellaneous::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("MCDT", mData, 12);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
    }

	bool Miscellaneous::exportTESx(ESMWriter &esm, int export_format) const
	{
		return false;
	}
	bool Miscellaneous::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;
		std::ostringstream modelPath;

		tempStr = esm.generateEDIDTES4(mId, 0, "MISC");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		if (mName.size() > 0)
		{
			esm.startSubRecordTES4("FULL");
			esm.writeHCString(mName);
			esm.endSubRecordTES4("FULL");
		}

		// MODL == Model Filename
		if (mModel.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mModel, 1);
			tempStr.replace(tempStr.size()-4, 4, ".nif");
			modelPath << "clutter\\morro\\" << tempStr;
			esm.QueueModelForExport(mModel, modelPath.str(), 3);
			esm.startSubRecordTES4("MODL");
			esm.writeHCString(modelPath.str());
			esm.endSubRecordTES4("MODL");

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
			std::string nifInputName = "meshes/" + Misc::ResourceHelpers::correctActorModelPath(mModel, doc.getVFS());
			doc.getVFS()->normalizeFilename(nifInputName);
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
				std::cout << "Misc::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
			}

			// MODB == Bound Radius
			esm.startSubRecordTES4("MODB");
			esm.writeT<float>(5.0);
			esm.endSubRecordTES4("MODB");
			// MODT
		}

		// ICON, mIcon
		if (mIcon.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mIcon, 1);
			tempStr.replace(tempStr.size()-4, 4, ".dds");
			tempPath.str(""); tempPath.clear();
			tempPath << "clutter\\morro\\" << tempStr;
			esm.mDDSToExportList.push_back(std::make_pair(mIcon, std::make_pair(tempPath.str(), 1)));
//			esm.mDDSToExportList[mIcon] = std::make_pair(tempPath.str(), 1);
			esm.startSubRecordTES4("ICON");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("ICON");
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

		// DATA
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		return true;
	}

    void Miscellaneous::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mIsKey = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}
