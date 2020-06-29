#include "loadalch.hpp"

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
    unsigned int Potion::sRecordId = REC_ALCH;

    void Potion::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mEffects.mList.clear();

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
                case ESM::FourCC<'T','E','X','T'>::value: // not ITEX here for some reason
                    mIcon = esm.getHString();
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'A','L','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'E','N','A','M'>::value:
                    mEffects.add(esm);
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
            esm.fail("Missing ALDT subrecord");
    }
    void Potion::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("TEXT", mIcon);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("ALDT", mData, 12);
        mEffects.save(esm);
    }
	bool Potion::exportTESx(ESMWriter & esm, int export_format) const
	{
		return false;
	}
	bool Potion::exportTESx(CSMDoc::Document &doc, ESMWriter &esm, int export_format) const
	{
		std::string tempStr;
		std::ostringstream tempPath;
		std::ostringstream modelPath;
		bool bSubstitute = false;

		tempStr = esm.generateEDIDTES4(mId, 0, "ALCH");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		std::string nifInputName;
		if (mModel.size() > 4) {
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
				std::cout << "Book::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
			}
		}

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size() - 4, 4, ".nif");
		modelPath << "clutter\\potions\\morro\\" << tempStr;
		esm.QueueModelForExport(mModel, modelPath.str());
		if (bSubstitute) {
			tempStr = Misc::StringUtils::lowerCase(modelPath.str());
			if (tempStr.find("bargain") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Potions\\PotionBargain.nif");
			}
			else if (tempStr.find("cheap") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Potions\\PotionCheap.nif");
			}
			else if (tempStr.find("exclusive") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Potions\\PotionExclusive.nif");
			}
			else if (tempStr.find("fresh") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Potions\PotionFresh.nif");
			}
			else if (tempStr.find("quality") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Potions\\PotionQuality.nif");
			}
			else if (tempStr.find("standard") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Potions\\PotionStandard.nif");
			}
			else if (tempStr.find("comberry") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Booze\\ComberryWine.nif");
			}
			else if (tempStr.find("cyro") != std::string::npos) {
				modelPath.str("Morroblivion\\Clutter\\Booze\\CyroWhiskey.nif");
			}
			else {
				//default
				modelPath.str("Morroblivion\\Clutter\\Booze\\LocalBrew.nif");

			}

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
				std::cout << "Alch::exportTESx() Error: (" << nifInputName << ") " << errString << "\n";
			}

		}

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(16.0);
		esm.endSubRecordTES4("MODB");

		// ICON, mIcon
		tempPath.str(""); tempPath.clear();
		if (mIcon.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mIcon, 1);
			tempStr.replace(tempStr.size() - 4, 4, ".dds");
			tempPath << "clutter\\potions\\morro\\" << tempStr;
		}
		esm.mDDSToExportList.push_back(std::make_pair(mIcon, std::make_pair(tempPath.str(), 1)));
		//		esm.mDDSToExportList[mIcon] = std::make_pair(tempPath.str(), 1);
		if (bSubstitute) {
			tempStr = Misc::StringUtils::lowerCase(tempPath.str());
			if (tempStr.find("bargain") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\bargainpotion.dds");
			}
			else if (tempStr.find("cheap") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\cheappotion.dds");
			}
			else if (tempStr.find("exclusive") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\exclusivepotion.dds");
			}
			else if (tempStr.find("fresh") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\freshpotion.dds");
			}
			else if (tempStr.find("quality") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\qualitypotion.dds");
			}
			else if (tempStr.find("standard") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\standardpotion.dds");
			}
			else if (tempStr.find("comberry") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\comberrywine.dds");
			}
			else if (tempStr.find("cyro") != std::string::npos) {
				tempPath.str("darthsouth\\Potions\\cyrowhiskey.dds");
			}
			else {
				//default
				tempPath.str("darthsouth\\Potions\\localbrew.dds");

			}
		}
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("ICON");

		// SCRI --mScript (formID)
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		uint32_t tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// ENIT {long, byte[1], byte[3]} -- flags
		esm.startSubRecordTES4("ENIT");
		// value
		esm.writeT<int32_t>(mData.mValue);
		// flags [ no autocalc, food item ]
		int flags=0;
		if (mData.mAutoCalc == 0)
			flags |= 0x01; // no-autocalc on
		esm.writeT<uint8_t>(flags);
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.endSubRecordTES4("ENIT");

		// wbEffects...
		for (auto magicEffect = mEffects.mList.begin(); magicEffect != mEffects.mList.end(); magicEffect++)
		{
			// EFID
			esm.startSubRecordTES4("EFID");
			// write char[4] magic effect
			magicEffect->exportTES4EFID(esm);
			esm.endSubRecordTES4("EFID");
			// EFIT
			esm.startSubRecordTES4("EFIT");
			magicEffect->exportTES4EFIT(esm);
			esm.endSubRecordTES4("EFIT");
			// SCIT
			std::string effSIG = esm.intToMagEffIDTES4(magicEffect->mEffectID);
			bool isScriptEffect = (effSIG == "SEFF");
			if (isScriptEffect)
			{
				esm.startSubRecordTES4("SCIT");
				// formID (SCPT)
				tempFormID = 0x1470351; // mwSpellCastScript
				esm.writeT<uint32_t>(tempFormID);
				// magic school (uint32)
				esm.writeT<uint32_t>(0);
				// visual effect name (char[4]) (uint32)
				esm.writeT<uint32_t>(0);
				// flags (uint8) [Hostile]
				uint8_t flags = 0;
				esm.writeT<uint8_t>(flags);
				// unused x3 (uint8)
				esm.writeT<uint8_t>(0);
				esm.writeT<uint8_t>(0);
				esm.writeT<uint8_t>(0);
				esm.endSubRecordTES4("SCIT");
			}

		}

		return true;
	}

    void Potion::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mAutoCalc = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEffects.mList.clear();
    }
}
