#include "loadbook.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Book::sRecordId = REC_BOOK;

    void Book::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

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
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'B','K','D','T'>::value:
                    esm.getHT(mData, 20);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
                    break;
                case ESM::FourCC<'E','N','A','M'>::value:
                    mEnchant = esm.getHString();
                    break;
                case ESM::FourCC<'T','E','X','T'>::value:
                    mText = esm.getHString();
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
            esm.fail("Missing BKDT subrecord");
    }
    void Book::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("BKDT", mData, 20);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNOString("TEXT", mText);
        esm.writeHNOCString("ENAM", mEnchant);
    }
	bool Book::exportTESx(ESMWriter &esm, int export_format) const
	{
		std::string *tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId, false);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(*tempStr);
		esm.endSubRecordTES4("EDID");
		delete tempStr;

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// DESC, mText
		esm.startSubRecordTES4("DESC");
		esm.writeHCString(mText);
		esm.endSubRecordTES4("DESC");

		// SCRI

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, true);
		tempStr->replace(tempStr->size()-4, 4, ".nif");
		tempPath << "clutter\\books\\morro\\" << *tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("MODL");
		delete tempStr;

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(1.0);
		esm.endSubRecordTES4("MODB");

		// MODT
		// ICON, mIcon
		tempStr = esm.generateEDIDTES4(mIcon, true);
		tempStr->replace(tempStr->size()-4, 4, ".dds");
		tempPath.str(""); tempPath.clear();
		tempPath << "clutter\\morro\\" << *tempStr;
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("ICON");
		delete tempStr;

		// ANAM, enchantment points
		// ENAM, enchantment formID

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		uint8_t flags=0;
		if (mData.mIsScroll)
			flags |= 0x01;
		esm.writeT<uint8_t>(flags); // flags
		esm.writeT<uint8_t>(0xFF); // teaches
		esm.writeT<uint32_t>(mData.mValue); // value
		esm.writeT<float>(mData.mWeight); // weight
		esm.endSubRecordTES4("DATA");

		return true;
	}

    void Book::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mIsScroll = 0;
        mData.mSkillId = 0;
        mData.mEnchant = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEnchant.clear();
        mText.clear();
    }
}
