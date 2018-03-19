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
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		modelPath << "morro\\" << tempStr;
		//int recordType = 0;
		//if (Misc::StringUtils::lowerCase(modelPath.str()).find("\\x\\") != std::string::npos ||
		//	Misc::StringUtils::lowerCase(modelPath.str()).find("terrain") != std::string::npos )
		//{
		//	recordType = 4;
		//}
		//esm.QueueModelForExport(mModel, modelPath.str(), recordType);
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(modelPath.str());
		esm.endSubRecordTES4("MODL");

		float modelBounds = 0.0f;
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
			// read stream into NIF parser...
			Nif::NIFFile nifFile(fileStream,nifInputName);
			modelBounds = nifFile.mModelBounds;
			// look through each record, process accordingly
			for (int i = 0; i < nifFile.numRecords(); i++)
			{
				// for each texturesource node, change name
				if (nifFile.getRecord(i)->recType == Nif::RC_NiSourceTexture)
				{
					Nif::NiSourceTexture *texture = dynamic_cast<Nif::NiSourceTexture*>(nifFile.getRecord(i));
					if (texture != NULL)
					{
						std::string resourceName = Misc::ResourceHelpers::correctTexturePath(texture->filename, doc.getVFS());
						std::string texFilename = texture->filename;
						if (Misc::StringUtils::lowerCase(texFilename).find("textures/") == 0 || 
							Misc::StringUtils::lowerCase(texFilename).find("textures\\") == 0 )
						{
							// remove "textures/" path prefix
							texFilename = texFilename.substr(strlen("textures/"));
						}
						// extract modelPath dir and paste onto texture
						boost::filesystem::path modelP(modelPath.str());
						std::stringstream tempPath;
						std::string tempStr = esm.generateEDIDTES4(texFilename, 1);
						tempStr[texFilename.find_last_of(".")] = '.'; // restore '.' before filename extension
						tempPath << modelP.parent_path().string();
						tempPath << "\\" << tempStr;
						esm.mDDSToExportList[resourceName] = std::make_pair(tempPath.str(), 3);
						texture->filename = tempPath.str();
					}
				}
			}

			// serialize modified NIF and output to newNIFFILe
			fileStream.get()->seekg(std::ios_base::beg);
			std::ofstream newNIFFile;
			// create output subdirectories
			boost::filesystem::path p(outputRoot + "Oblivion.output/Data/Meshes/" + modelPath.str());
			if (boost::filesystem::exists(p.parent_path()) == false)
			{
				boost::filesystem::create_directories(p.parent_path());
			}

			newNIFFile.open(p.string(), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
			// write header
			nifFile.exportHeader(fileStream, newNIFFile);
			// write records
			for (int i=0; i < nifFile.mRecordSizes.size(); i++)
			{
				nifFile.exportRecord(fileStream, newNIFFile, i);
			}
			// write footer data
			nifFile.exportFooter(fileStream, newNIFFile);
			newNIFFile.close();

			if (modelBounds >= 200)
			{
				// repeat for _far.nif...
				// full res poly, but half-size textures
				// create _far.nif filePath
				std::string path_farNIF = p.string();
				path_farNIF = path_farNIF.substr(0, path_farNIF.size()-4) + "_far.nif";
				std::ofstream farNIFFile;
				farNIFFile.open(path_farNIF, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
				fileStream->clear(); // reset state
				nifFile.exportHeader(fileStream, farNIFFile);
				for (int i = 0; i < nifFile.mRecordSizes.size(); i++)
				{
					if (nifFile.getRecord(i)->recType == Nif::RC_NiSourceTexture)
					{
						// rename texture filepath to /textures/lowres/...
						nifFile.exportRecordSourceTexture(fileStream, farNIFFile, i, "lowres\\");
					}
					else
					{
						nifFile.exportRecord(fileStream, farNIFFile, i);
					}
					// add lowres filepath to queue for export
				}
				// write footer data
				nifFile.exportFooter(fileStream, farNIFFile);
				farNIFFile.close();

			}

		}
		catch (std::runtime_error e)
		{
			std::cout << "Error: (" << nifInputName << ") " << e.what() << "\n";
		}
		catch (...)
		{
			std::cout << "ERROR: something else bad happened!\n";
		}

		int recordType = 0;
		if (modelBounds >= 200)
		{
			recordType = 4;
		}
		esm.QueueModelForExport(mModel, modelPath.str(), recordType);

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
