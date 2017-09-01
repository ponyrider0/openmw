#include "loadregn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Region::sRecordId = REC_REGN;

    void Region::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'W','E','A','T'>::value:
                {
                    esm.getSubHeader();
                    if (esm.getVer() == VER_12)
                    {
                        mData.mA = 0;
                        mData.mB = 0;
                        esm.getExact(&mData, sizeof(mData) - 2);
                    }
                    else if (esm.getVer() == VER_13)
                    {
                        // May include the additional two bytes (but not necessarily)
                        if (esm.getSubSize() == sizeof(mData))
                        {
                            esm.getExact(&mData, sizeof(mData));
                        }
                        else
                        {
                            mData.mA = 0;
                            mData.mB = 0;
                            esm.getExact(&mData, sizeof(mData)-2);
                        }
                    }
                    else
                    {
                        esm.fail("Don't know what to do in this version");
                    }
                    break;
                }
                case ESM::FourCC<'B','N','A','M'>::value:
                    mSleepList = esm.getHString();
                    break;
                case ESM::FourCC<'C','N','A','M'>::value:
                    esm.getHT(mMapColor);
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                {
                    SoundRef sr;
                    esm.getHT(sr, 33);
                    mSoundList.push_back(sr);
                    break;
                }
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

    void Region::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("FNAM", mName);

        if (esm.getVersion() == VER_12)
            esm.writeHNT("WEAT", mData, sizeof(mData) - 2);
        else
            esm.writeHNT("WEAT", mData);

        esm.writeHNOCString("BNAM", mSleepList);

        esm.writeHNT("CNAM", mMapColor);
        for (std::vector<SoundRef>::const_iterator it = mSoundList.begin(); it != mSoundList.end(); ++it)
        {
            esm.writeHNT<SoundRef>("SNAM", *it);
        }
    }

	bool Region::exportTESx(ESMWriter &esm, int export_format) const
	{
//		esm.writeHNCString("NAME", mId);
//		esm.writeHNOCString("FNAM", mName);
//		if (esm.getVersion() == VER_12)
//			esm.writeHNT("WEAT", mData, sizeof(mData) - 2);
//		else
//			esm.writeHNT("WEAT", mData);
//		esm.writeHNOCString("BNAM", mSleepList);
//		esm.writeHNT("CNAM", mMapColor);
//		for (std::vector<SoundRef>::const_iterator it = mSoundList.begin(); it != mSoundList.end(); ++it)
//			esm.writeHNT<SoundRef>("SNAM", *it);

		std::string tempStr;
		tempStr = esm.generateEDIDTES4(mId, 2);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// ICON

		// RCLR (8bit x Red,Green,Blue,Unused)
		esm.startSubRecordTES4("RCLR");
		esm.writeT<uint32_t>(mMapColor);
		esm.endSubRecordTES4("RCLR");

		// WNAM - Worldspace

		// Region Areas? RPLI, RPLD(X,Y)

		// Region Data Entries?
			// Objects, Map, Grass, Sound, Weather
		esm.startSubRecordTES4("RDAT");
		esm.writeT<uint32_t>(4); // Type (Map == 4)
		esm.writeT<uint8_t>(0); // Flags
		esm.writeT<uint8_t>(50); // Priority
		esm.writeT<uint8_t>(4); // Unused
		esm.writeT<uint8_t>(4); // Unused
		esm.endSubRecordTES4("RDAT");
		esm.startSubRecordTES4("RDMP");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("RDMP");

		esm.startSubRecordTES4("RDAT");
		esm.writeT<uint32_t>(3); // Type (Weather == 3)
		esm.writeT<uint8_t>(0); // Flags
		esm.writeT<uint8_t>(50); // Priority
		esm.writeT<uint8_t>(4); // Unused
		esm.writeT<uint8_t>(4); // Unused
		esm.endSubRecordTES4("RDAT");
		esm.startSubRecordTES4("RDWT");
		exportWeatherListTES4(esm);
		esm.endSubRecordTES4("RDWT");

		return true;
	}

	bool Region::exportClimateTESx(ESMWriter &esm, int export_format) const
	{
		std::ostringstream tempStream;
		std::string tempStr;
		tempStr = esm.generateEDIDTES4(mId);
		tempStream << tempStr << "Clmt";
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStream.str());
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("WLST");
		exportWeatherListTES4(esm);
		esm.endSubRecordTES4("WLST");

		// FNAM, sun
		// GNAM, sun glare
		// MODL, night sky
			// MODB
		// TNAM, Settings

		return true;
	}

	void Region::exportWeatherListTES4(ESMWriter &esm) const
	{
		uint32_t weatherFormID=0;
		weatherFormID = esm.crossRefStringID("Clear");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Clear==0x38EEE)
			esm.writeT<uint32_t>(mData.mClear); // Chance
		}
		weatherFormID = esm.crossRefStringID("Cloudy");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Cloudy==0x38EF0)
			esm.writeT<uint32_t>(mData.mCloudy); // Chance
		}
		weatherFormID = esm.crossRefStringID("Fog");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Fog==0x38EEF)
			esm.writeT<uint32_t>(mData.mFoggy); // Chance
		}
		weatherFormID = esm.crossRefStringID("Overcast");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Overcast==0x38EEC)
			esm.writeT<uint32_t>(mData.mOvercast); // Chance
		}
		weatherFormID = esm.crossRefStringID("Rain");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Rain==0x38EF2)
			esm.writeT<uint32_t>(mData.mRain); // Chance
		}
		weatherFormID = esm.crossRefStringID("Thunderstorm");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Thunder==0x38EF1)
			esm.writeT<uint32_t>(mData.mThunder); // Chance
		}
		weatherFormID = esm.crossRefStringID("Ashstorm");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Ash==0x460000)
			esm.writeT<uint32_t>(mData.mAsh); // Chance
		}
		weatherFormID = esm.crossRefStringID("Blightstorm");
		if (weatherFormID != 0)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Blight==??)
			esm.writeT<uint32_t>(mData.mBlight); // Chance
		}
		weatherFormID = esm.crossRefStringID("Snow");
		if (weatherFormID != 0 && esm.getVersion() == ESM::VER_13)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Blight==??)
			esm.writeT<uint32_t>(mData.mA); // Chance
		}
		weatherFormID = esm.crossRefStringID("Blizzard");
		if (weatherFormID != 0 && esm.getVersion() == ESM::VER_13)
		{
			esm.writeT<uint32_t>(weatherFormID); // Weather FormID (Blight==??)
			esm.writeT<uint32_t>(mData.mB); // Chance
		}
	}

    void Region::blank()
    {
        mName.clear();

        mData.mClear = mData.mCloudy = mData.mFoggy = mData.mOvercast = mData.mRain =
            mData.mThunder = mData.mAsh, mData.mBlight = mData.mA = mData.mB = 0;

        mMapColor = 0;

        mName.clear();
        mSleepList.clear();
        mSoundList.clear();
    }
}
