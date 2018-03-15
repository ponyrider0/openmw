#include "loadstat.hpp"

#include <iostream>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

#include <boost/filesystem.hpp>
#include "../nif/nifstream.hpp"

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
	bool Static::exportTESx(ESMWriter & esm, int export_format) const
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
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		modelPath << "morro\\" << tempStr;
		int recordType = 0;
		if (Misc::StringUtils::lowerCase(modelPath.str()).find("\\x\\") != std::string::npos ||
			Misc::StringUtils::lowerCase(modelPath.str()).find("terrain") != std::string::npos )
		{
			recordType = 4;
		}
		esm.QueueModelForExport(mModel, modelPath.str(), recordType);
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(modelPath.str());
		esm.endSubRecordTES4("MODL");

		float modelBounds = 100.0f;
		// ** Load NIF and get model's true Bound Radius
#ifdef _WIN32
		std::string outputRoot = "C:/";
		std::string logRoot = "";
#else
		std::string outputRoot = getenv("HOME");
		outputRoot += "/";
		std::string logRoot = outputRoot;
#endif
		std::string nifInputName = "meshes/" + Misc::ResourceHelpers::correctActorModelPath(mModel, doc.getVFS());
		boost::filesystem::path rootDir(outputRoot + "Oblivion.output/Data/Meshes/");
		if (boost::filesystem::exists(rootDir) == false)
		{
			boost::filesystem::create_directories(rootDir);
		}
		try
		{
			auto fileStream = doc.getVFS()->get(nifInputName);
			std::ofstream newNIFFile;
			// create output subdirectories
			boost::filesystem::path p(outputRoot + "Oblivion.output/Data/Meshes/" + modelPath.str());
			if (boost::filesystem::exists(p.parent_path()) == false)
			{
				boost::filesystem::create_directories(p.parent_path());
			}
			newNIFFile.open(p.string(), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
			int len = 0;
			char buffer[1024];
			while (fileStream->eof() == false)
			{
				fileStream->read(buffer, sizeof(buffer));
				len = fileStream->gcount();
				newNIFFile.write(buffer, len);
			}
			newNIFFile.close();
		}
		catch (std::runtime_error e)
		{
			std::cout << "Error: (" << nifInputName << ") " << e.what() << "\n";
		}
		catch (...)
		{
			std::cout << "ERROR: something else bad happened!\n";
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
