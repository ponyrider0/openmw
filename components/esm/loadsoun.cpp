#include "loadsoun.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Sound::sRecordId = REC_SOUN;

    void Sound::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'F','N','A','M'>::value:
                    mSound = esm.getHString();
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mData, 3);
                    hasData = true;
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
            esm.fail("Missing DATA subrecord");
    }

    void Sound::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("FNAM", mSound);
        esm.writeHNT("DATA", mData, 3);
    }

	void Sound::exportTESx(ESMWriter &esm, int export_type) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;

		// EDID
		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// ICON, sIconNames[]
		if (mSound.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mSound, true);
			tempStr.replace(tempStr.size()-4, 1, ".");
			tempPath << "fx\\morro\\" << tempStr;
			esm.startSubRecordTES4("FNAM");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("FNAM");
		}

		// DATA
		esm.startSubRecordTES4("SNDX");
		esm.writeT<uint8_t>(mData.mMinRange);
		esm.writeT<uint8_t>(mData.mMaxRange);
		esm.writeT<int8_t>(0); // frequency adjustment %
		esm.writeT<uint8_t>(0); // unused
		uint16_t flags=0;
		esm.writeT<uint16_t>(flags);
		esm.writeT<uint16_t>(0); // unused
		esm.writeT<uint16_t>(0); // static attenuation
		esm.writeT<uint8_t>(0); // stop time
		esm.writeT<uint8_t>(0); // start time
		esm.endSubRecordTES4("SNDX");

	}

    void Sound::blank()
    {
        mSound.clear();

        mData.mVolume = 128;
        mData.mMinRange = 0;
        mData.mMaxRange = 255;
    }
}
