#include "loadcont.hpp"

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
		std::string *tempStr;
		std::ostringstream tempStream;

		tempStr = esm.generateEDIDTES4(mId, false);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(*tempStr);
		esm.endSubRecordTES4("EDID");
		delete tempStr;

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, true);
		tempStr->replace(tempStr->size()-4, 4, ".nif");
		tempStream << "morro\\" << *tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempStream.str());
		esm.endSubRecordTES4("MODL");
		delete tempStr;
		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(1.0);
		esm.endSubRecordTES4("MODB");
		// MODT

		// SCRI
		tempFormID = esm.crossRefStringID(mScript);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// Process non-organic containers
		if ((mFlags & Flags::Organic) == 0)
		{
			// CNTO: {formID, uint32}
			for (auto inventoryItem = mInventory.mList.begin(); inventoryItem != mInventory.mList.end(); inventoryItem++)
			{
				tempFormID = esm.crossRefStringID(inventoryItem->mItem.toString());
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
			if (mFlags == Container::Respawn)
				flags = 0x02;
			esm.startSubRecordTES4("DATA");
			esm.writeT<uint8_t>(flags); // flags
			esm.writeT<float>(mWeight); // weight
			esm.endSubRecordTES4("DATA");

			// SNAM, open sound formID
			// QNAM, close sound formID
		}
		else // Organic containers
		{
			// PFIG
			auto ingredient = mInventory.mList.begin();
			if (ingredient != mInventory.mList.end())
			{
				tempFormID = esm.crossRefStringID(ingredient->mItem.toString());
				if (tempFormID != 0)
				{
					esm.startSubRecordTES4("PFIG");
					esm.writeT<uint32_t>(tempFormID);
					esm.endSubRecordTES4("PFIG");
				}
			}

			// PFPC
			esm.startSubRecordTES4("PFPC");
			esm.writeT<uint8_t>(50); // spring
			esm.writeT<uint8_t>(50); // summer
			esm.writeT<uint8_t>(50); // fall
			esm.writeT<uint8_t>(50); // winter
			esm.endSubRecordTES4("PFPC");
		}

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
