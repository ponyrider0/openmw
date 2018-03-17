#include "loadstat.hpp"

#include <iostream>

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
			// read stream into NIF parser...
			Nif::NIFFile nifFile(fileStream,nifInputName);
			// for each texturesource node, change name
			for (int i = 0; i < nifFile.numRecords(); i++)
			{
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
							// remove "textures/"
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
			size_t len = 0;
			int intVal = 0;
			uint16_t shortVal = 0;
			uint32_t uintVal = 0;
			uint8_t byteVal = 0;
			char buffer[4096];
			size_t readsize = nifFile.mHeaderSize;
			// write header
/*
			// fake header
			// getVersionString: Version String (eol terminated?)
			char VersionString[] = "Gamebryo File Format, Version 20.0.0.5\n";
//			char VersionString[] = "NetImmerse File Format, Version 4.0.0.2\n";
			len = strlen(VersionString);
			strncpy(buffer, VersionString, len);
			newNIFFile.write(buffer, len);
			// getUInt: BCD version
//			uintVal = 0x04000002; 
			uintVal = 0x14000005;
			for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
			len = 4;
			newNIFFile.write(buffer, len);
			// NEW: Endian Type
			byteVal = 1; // LITTLE ENDIAN
			buffer[0] = byteVal;
			len = 1;
			newNIFFile.write(buffer, len);
			// NEW: User Version
			uintVal = 11;
			for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
			len = 4;
			newNIFFile.write(buffer, len);
			// getInt: number of records
			uintVal = nifFile.numRecords();
			for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
			len = 4;
			newNIFFile.write(buffer, len);
			// NEW: User Version 2
			uintVal = 11;
			for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
			len = 4;
			newNIFFile.write(buffer, len);
			// NEW: ExportInfo
			// --ExportInfo: 3 short strings (byte + string)
			for (int i = 0; i < 3; i++)
			{
				byteVal = 1;
				buffer[0] = byteVal;
				len = 1;
				newNIFFile.write(buffer, len);
				len = 1;
				strncpy(buffer, "\0", len);
				newNIFFile.write(buffer, len);
			}
			// NEW: Number of Block Types (uint16)
			shortVal = 8;
			for (int j = 0; j < 3; j++) buffer[j] = reinterpret_cast<char *>(&shortVal)[j];
			len = 2;
			newNIFFile.write(buffer, len);
			// NEW: Block Types (int32 + string)
			std::string blockTypes[] = { "NiNode","NiTriShape","NiTexturingProperty","NiSourceTexture","NiAlphaProperty","NiStencilProperty","NiMaterialProperty","NiTriShapeData","","" };
			for (int i = 0; i < shortVal; i++)
			{
				std::string blockType = blockTypes[i];
				uintVal = blockType.size();
				for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
				len = 4;
				newNIFFile.write(buffer, len);
				len = blockType.size();
				strncpy(buffer, blockType.c_str(), len);
				newNIFFile.write(buffer, len);
			}
			// NEW: Block Type Index (uint16) x Number of Records
			int blockIndexes[] = {0,0,0,0,1,2, 3,4,5,6,7, 1,2,3,7,1, 2,3,7,1,2, 3,7,1,7,1, 2,3,7,1,2, 7};
			for (int i = 0; i < nifFile.numRecords(); i++)
			{
				shortVal = blockIndexes[i];
				for (int j = 0; j < 3; j++) buffer[j] = reinterpret_cast<char *>(&shortVal)[j];
				len = 2;
				newNIFFile.write(buffer, len);
			}
			// NEW: Unknown Int 2 (int32)
			uintVal = 0;
			for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
			len = 4;
			newNIFFile.write(buffer, len);
*/
			// original header
			while (readsize > sizeof(buffer))
			{
				fileStream->read(buffer, sizeof(buffer));
				len = fileStream->gcount();
				newNIFFile.write(buffer, len);
				readsize -= len;
			}
			fileStream->read(buffer, readsize);
			len = fileStream->gcount();
			newNIFFile.write(buffer, len);

			// write records
			for (int i=0; i < nifFile.numRecords(); i++)
			{
				if (nifFile.getRecord(i)->recType == Nif::RC_NiSourceTexture)
				{
					Nif::NiSourceTexture *texture = dynamic_cast<Nif::NiSourceTexture*>(nifFile.getRecord(i));
					if (texture != NULL && texture->external == true)
					{
						// 32bit strlen + recordtype
						char recordName[] = "NiSourceTexture";
						len = 15; // recordName is hardcoded without null terminator
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&len)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						len = 15; // recordName is hardcoded without null terminator
						strncpy(buffer, recordName, len);
						newNIFFile.write(buffer, len);

						// 32bit strlen + recordname
						len = texture->name.size();
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&len)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						len = texture->name.size();
						strncpy(buffer, texture->name.c_str(), len);
						newNIFFile.write(buffer, len);
						// then read Extra.index then Controller.index
						int index = (texture->extra.empty() == false) ? texture->extra->recIndex : -1;
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&index)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						index = (texture->controller.empty() == false) ? texture->controller->recIndex : -1;
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&index)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						// 8bit (bool external = true)
						buffer[0] = true;
						len = 1;
						newNIFFile.write(buffer, len);
						// 32bit strlen + filename
						len = texture->filename.size();
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&len)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						len = texture->filename.size();
						strncpy(buffer, texture->filename.c_str(), len);
						newNIFFile.write(buffer, len);
						// 32bit int (pixel)
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&texture->pixel)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						// 32bit int (mipmap)
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&texture->mipmap)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						// 32bit int (alpha)
						for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&texture->alpha)[j];
						len = 4;
						newNIFFile.write(buffer, len);
						// byte (always 1)
						buffer[0] = 1;
						len = 1;
						newNIFFile.write(buffer, len);

						// move read pointer ahead
						readsize = nifFile.mRecordSizes[i];
						fileStream->seekg(readsize, std::ios_base::cur);
/*
						while (readsize > sizeof(buffer))
						{
							fileStream->read(buffer, sizeof(buffer));
							len = fileStream->gcount();
//							newNIFFile.write(buffer, len);
							readsize -= len;
						}
						fileStream->read(buffer, readsize);
						len = fileStream->gcount();
//						newNIFFile.write(buffer, len);
*/
						continue;
					}
				}
//				int strLen;
//				fileStream->read(buffer, 4);
//				strLen = (buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24);
//				fileStream->seekg(strLen, std::ios_base::cur);
//				readsize = nifFile.mRecordSizes[i] - (strLen+4);
				readsize = nifFile.mRecordSizes[i];
				while (readsize > sizeof(buffer))
				{
					fileStream->read(buffer, sizeof(buffer));
					len = fileStream->gcount();
					newNIFFile.write(buffer, len);
					readsize -= len;
				}
				fileStream->read(buffer, readsize);
				len = fileStream->gcount();
				newNIFFile.write(buffer, len);
			}
			// write tailing data
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
