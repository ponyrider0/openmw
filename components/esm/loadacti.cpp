#include "loadacti.hpp"

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
    unsigned int Activator::sRecordId = REC_ACTI;

    void Activator::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
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
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
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
    }
    void Activator::save(ESMWriter &esm, bool isDeleted) const
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
    }
	bool Activator::exportTESx(ESMWriter & esm, int export_format) const
	{
		return false;
	}
	bool Activator::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		std::string tempStr;
		std::ostringstream modelPath;

		tempStr = esm.generateEDIDTES4(mId, 0, "ACTI");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		modelPath << "morro\\" << tempStr;
//		esm.QueueModelForExport(mModel, modelPath.str());
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
			std::cout << "Error: (" << nifInputName << ") " << e.what() << "\n";
		}

		int recordType = 0;
		if (vwdMode != VWD_MODE_LOD_ONLY)
		{
			esm.QueueModelForExport(mModel, modelPath.str(), recordType);
		}
		if (vwdMode != VWD_MODE_NORMAL_ONLY && modelBounds >= vwdThreshold)
		{
			recordType = 4;
			esm.QueueModelForExport(mModel, modelPath.str(), recordType);
		}

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(modelBounds);
		esm.endSubRecordTES4("MODB");
		// MODT

		// SCRI (script formID)
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		uint32_t tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// SNAM (sound formID)

		return true;
	}

    void Activator::blank()
    {
        mName.clear();
        mScript.clear();
        mModel.clear();
    }
}
