#include "loadcont.hpp"

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

#include <boost/filesystem.hpp>
#include "../nif/niffile.hpp"
#include "../nif/controlled.hpp"

#include <apps/opencs/model/doc/document.hpp>
#include <components/vfs/manager.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace ESM
{

    void InventoryList::add(ESMReader &esm)
    {
        ContItem ci;
        esm.getHT(ci, 36);
        mList.push_back(ci);
    }

    void InventoryList::save(ESMWriter &esm) const
    {
        for (std::vector<ContItem>::const_iterator it = mList.begin(); it != mList.end(); ++it)
        {
            esm.writeHNT("NPCO", *it, 36);
        }
    }

    unsigned int Container::sRecordId = REC_CONT;

    void Container::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mInventory.mList.clear();

        bool hasName = false;
        bool hasWeight = false;
        bool hasFlags = false;
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
                case ESM::FourCC<'C','N','D','T'>::value:
                    esm.getHT(mWeight, 4);
                    hasWeight = true;
                    break;
                case ESM::FourCC<'F','L','A','G'>::value:
                    esm.getHT(mFlags, 4);
                    if (mFlags & 0xf4)
                        esm.fail("Unknown flags");
                    if (!(mFlags & 0x8))
                        esm.fail("Flag 8 not set");
                    hasFlags = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'N','P','C','O'>::value:
                    mInventory.add(esm);
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
        if (!hasWeight && !isDeleted)
            esm.fail("Missing CNDT subrecord");
        if (!hasFlags && !isDeleted)
            esm.fail("Missing FLAG subrecord");
    }

    void Container::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CNDT", mWeight, 4);
        esm.writeHNT("FLAG", mFlags, 4);

        esm.writeHNOCString("SCRI", mScript);

        mInventory.save(esm);
    }
	bool Container::exportTESx(ESMWriter & esm, int export_format) const
	{
		return false;
	}
	bool Container::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream debugstream;
		std::ostringstream modelPath;
		bool bSubstitute = false;

		bool isTakeOnly = false;
		if (mFlags & Flags::Organic)
		{
			isTakeOnly = true;
		}

		tempStr = esm.generateEDIDTES4(mId, 0, "CONT");
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
		if (bSubstitute) {
			tempStr = Misc::StringUtils::lowerCase(mModel);
			if (tempStr.find("activeusignpostu01") != std::string::npos) {
				modelPath << "morro\\f\\ActiveUsignpostU01.nif";
			}
			if (tempStr.find("activeusignpostu02") != std::string::npos) {
				modelPath << "morro\\f\\ActiveUsignpostU02.nif";
			}
		}
		else {
			tempStr = esm.generateEDIDTES4(mModel, 1);
			tempStr.replace(tempStr.size() - 4, 4, ".nif");
			modelPath << "morro\\" << tempStr;
			esm.QueueModelForExport(mModel, modelPath.str());

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

			//		std::cout << "DEBUG Container::exportTESx() [" << nifInputName << "] " << "outputting file?" << "\n";
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
							//						std::cout << "DEBUG Container::exportTESx() [" << nifInputName << "] " << "File Outputted!" << "\n";
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
				std::cout << "Container::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
			}

		}

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(50.0);
		esm.endSubRecordTES4("MODB");
		// MODT

		// **** TODO: if isTakeOnly, modify script */
		// SCRI
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// CNTO: {formID, uint32}
		for (auto inventoryItem = mInventory.mList.begin(); inventoryItem != mInventory.mList.end(); inventoryItem++)
		{
			tempFormID = esm.crossRefStringID(inventoryItem->mItem.toString(), "INV_CNT");
			if (tempFormID != 0)
			{
				esm.startSubRecordTES4("CNTO");
				esm.writeT<uint32_t>(tempFormID);
				esm.writeT<int32_t>(inventoryItem->mCount);
				esm.endSubRecordTES4("CNTO");
			}
		}

		// DATA, float (item weight)
		uint8_t flags=0;
		if (mFlags & Container::Respawn)
			flags = 0x02;
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint8_t>(flags); // flags
		esm.writeT<float>(mWeight); // weight
		esm.endSubRecordTES4("DATA");

		// SNAM, open sound formID
		// QNAM, close sound formID

		return true;
	}

	bool Container::exportAsFlora(CSMDoc::Document &doc, ESMWriter& esm, std::string ingredientEDID) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream debugstream;
		std::ostringstream modelPath;

		bool isTakeOnly = false;
		if (mFlags & Flags::Organic)
		{
			isTakeOnly = true;
		}

		strEDID = esm.generateEDIDTES4(mId, 0, "FLOR");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size() - 4, 4, ".nif");
		modelPath << "morro\\" << tempStr;
		esm.QueueModelForExport(mModel, modelPath.str());
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
			std::cout << "Flora::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
		}

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(50.0);
		esm.endSubRecordTES4("MODB");
		// MODT

		// SCRI
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// Process non-organic containers
		// PFIG
		if (ingredientEDID == "")
		{
			// throw error, then give default ingredient... ?
			std::string errStr = "WARNING: Flora export: could not resolve ingredientID: " + strEDID + " - " + ingredientEDID;
			OutputDebugString(errStr.c_str());
		}
		tempFormID = esm.crossRefStringID(ingredientEDID, "INGR", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("PFIG");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("PFIG");
		}

		// PFPC
		esm.startSubRecordTES4("PFPC");
		esm.writeT<uint8_t>(50); // spring
		esm.writeT<uint8_t>(50); // summer
		esm.writeT<uint8_t>(50); // fall
		esm.writeT<uint8_t>(50); // winter
		esm.endSubRecordTES4("PFPC");

		return true;
	}

    void Container::blank()
    {
        mName.clear();
        mModel.clear();
        mScript.clear();
        mWeight = 0;
        mFlags = 0x8; // set default flag value
        mInventory.mList.clear();
    }
}
