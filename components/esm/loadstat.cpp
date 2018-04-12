#include "loadstat.hpp"

#include <iostream>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

#include <boost/filesystem.hpp>
#include "../nif/niffile.hpp"
#include "../nif/controlled.hpp"
#include "../nif/data.hpp"
#include "../nif/node.hpp"

#include <apps/opencs/model/doc/document.hpp>
#include <components/vfs/manager.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace ESM
{
    unsigned int Static::sRecordId = REC_STAT;

    void Static::load(ESMReader &esm, bool &isDeleted)
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
    void Static::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
        }
        else
        {
            esm.writeHNCString("MODL", mModel);
        }
    }
	bool Static::exportTESx(ESMWriter &esm, int export_format) const
	{
		return false;
	}
	bool Static::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		std::ostringstream modelPath;
		std::string tempStr;

		tempStr = esm.generateEDIDTES4(mId, 0, "STAT");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size() - 4, 4, ".nif");
		modelPath << "morro\\" << tempStr;
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
            std::cout << "Static::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
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

		// Optional: MODT == "Texture Files Hashes" (byte array)

		return true;
	}

    void Static::blank()
    {
        mModel.clear();
    }
}
