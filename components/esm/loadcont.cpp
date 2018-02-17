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
	bool Container::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempStream, debugstream;

		bool isTakeOnly = false;
		if (mFlags & Flags::Organic)
		{
			isTakeOnly = true;
		}

		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		tempStream << "morro\\" << tempStr;
		esm.QueueModelForExport(mModel, tempStream.str());
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempStream.str());
		esm.endSubRecordTES4("MODL");
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

	bool Container::exportAsFlora(ESMWriter& esm, std::string ingredientEDID) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream tempStream, debugstream;

		bool isTakeOnly = false;
		if (mFlags & Flags::Organic)
		{
			isTakeOnly = true;
		}

		strEDID = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size() - 4, 4, ".nif");
		tempStream << "morro\\" << tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempStream.str());
		esm.endSubRecordTES4("MODL");
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
