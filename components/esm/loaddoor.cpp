#include "loaddoor.hpp"

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
    unsigned int Door::sRecordId = REC_DOOR;

    void Door::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'S','N','A','M'>::value:
                    mOpenSound = esm.getHString();
                    break;
                case ESM::FourCC<'A','N','A','M'>::value:
                    mCloseSound = esm.getHString();
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

    void Door::save(ESMWriter &esm, bool isDeleted) const
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
        esm.writeHNOCString("SNAM", mOpenSound);
        esm.writeHNOCString("ANAM", mCloseSound);
    }
	bool Door::exportTESx(ESMWriter &esm, int export_format) const
	{
		return false;
	}
	bool Door::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		std::string tempStr;
		std::ostringstream modelPath;

		// EDID
		tempStr = esm.generateEDIDTES4(mId, 0, "DOOR");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// FULL
		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL, MODB, MODT
		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		modelPath << "morro\\" << tempStr;
//		esm.QueueModelForExport(mModel, modelPath.str(), 1);
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(modelPath.str());
		esm.endSubRecordTES4("MODL");

		bool bBlenderOutput = false;
		if (esm.mConversionOptions.find("#blender") != std::string::npos)
			bBlenderOutput = true;

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
				std::string filePath = Nif::NIFFile::CreateResourcePaths(modelPath.str());
				nifFile.prepareExport(doc, esm, modelPath.str());
				nifFile.exportFileNif(esm, fileStream, filePath);
				if (modelBounds >= 200)
				{
					nifFile.exportFileNifFar(esm, fileStream, filePath);
				}
			}

		}
		catch (std::runtime_error e)
		{
			std::cout << "Error: (" << nifInputName << ") " << e.what() << "\n";
		}

		int recordType = 1;
		if (modelBounds >= 200)
		{
			recordType = 4;
		}
		esm.QueueModelForExport(mModel, modelPath.str(), recordType);

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(modelBounds);
		esm.endSubRecordTES4("MODB");

		// SCRI (script formID) mScript
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		uint32_t tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// SNAM, OpenSound
		tempFormID = esm.crossRefStringID(mOpenSound, "SOUN");
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SNAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SNAM");
		}

		// ANAM, CloseSound
		tempFormID = esm.crossRefStringID(mCloseSound, "SOUN");
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("ANAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("ANAM");
		}

		// BNAM, LoopSound

		// FNAM, Flags 0x01=Oblivion Gate, 0x02=Automatic, 0x04=Hidden, 0x08=Minimal Use
		uint8_t flags=0;
		esm.startSubRecordTES4("FNAM");
		esm.writeT<uint8_t>(flags);
		esm.endSubRecordTES4("FNAM");

		// TNAM, Array: Teleport Destination: [Cell, WRLD]
		return true;
	}

    void Door::blank()
    {
        mName.clear();
        mModel.clear();
        mScript.clear();
        mOpenSound.clear();
        mCloseSound.clear();
    }
}
