#include "loadclas.hpp"
#include <components\esm\loadskil.hpp>

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Class::sRecordId = REC_CLAS;

    const Class::Specialization Class::sSpecializationIds[3] = {
      Class::Combat,
      Class::Magic,
      Class::Stealth
    };

    const char *Class::sGmstSpecializationIds[3] = {
      "sSpecializationCombat",
      "sSpecializationMagic",
      "sSpecializationStealth"
    };

    int& Class::CLDTstruct::getSkill (int index, bool major)
    {
        if (index<0 || index>=5)
            throw std::logic_error ("skill index out of range");

        return mSkills[index][major ? 1 : 0];
    }

    int Class::CLDTstruct::getSkill (int index, bool major) const
    {
        if (index<0 || index>=5)
            throw std::logic_error ("skill index out of range");

        return mSkills[index][major ? 1 : 0];
    }

    void Class::load(ESMReader &esm, bool &isDeleted)
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
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'C','L','D','T'>::value:
                    esm.getHT(mData, 60);
                    if (mData.mIsPlayable > 1)
                        esm.fail("Unknown bool value");
                    hasData = true;
                    break;
                case ESM::FourCC<'D','E','S','C'>::value:
                    mDescription = esm.getHString();
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
            esm.fail("Missing CLDT subrecord");
    }
    void Class::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CLDT", mData, 60);
        esm.writeHNOString("DESC", mDescription);
    }

	void Class::exportTESx(ESMWriter &esm, int export_type) const
	{
		std::string tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId, false);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// DESC, mText
		esm.startSubRecordTES4("DESC");
		esm.writeHCString(mDescription);
		esm.endSubRecordTES4("DESC");

		// ICON, 

		// DATA
		uint32_t tempval=0;
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint32_t>(mData.mAttribute[0]); // primary attributes 1
		esm.writeT<uint32_t>(mData.mAttribute[1]); // primary attributes 2
		esm.writeT<uint32_t>(mData.mSpecialization); // specialization
		esm.writeT<uint32_t>(getSkillTES4(0)); // major skills 1
		esm.writeT<uint32_t>(getSkillTES4(1)); // major skills 2
		esm.writeT<uint32_t>(getSkillTES4(2)); // major skills 3
		esm.writeT<uint32_t>(getSkillTES4(3)); // major skills 4
		esm.writeT<uint32_t>(getSkillTES4(4)); // major skills 5
		esm.writeT<uint32_t>(getSkillTES4(5)); // major skills 6
		esm.writeT<uint32_t>(getSkillTES4(6)); // major skills 7
		tempval = 0;
		if (mData.mIsPlayable)
			tempval |= 0x01;
		esm.writeT<uint32_t>(tempval); // flags
		tempval = 0;
		esm.writeT<uint32_t>(tempval); // buy/sells/services
		esm.writeT<int8_t>(-1); // skill trained
		esm.writeT<int8_t>(0); // max training level
		esm.writeT<uint8_t>(tempval); // unused
		esm.writeT<uint8_t>(tempval); // unused
		esm.endSubRecordTES4("DATA");

	}

	uint32_t Class::getSkillTES4(int ESM4index) const
	{
		int ESM3index, ESM3index2;
		uint32_t tempval=0;

		if (ESM4index < 5)
		{
			ESM3index = ESM4index;
			ESM3index2 = 1;
		}
		else if (ESM4index >= 5 && ESM4index < 10)
		{
			ESM3index = ESM4index % 5;
			ESM3index2 = 0;
		}
		else
			throw std::runtime_error ("ERROR: Class skill index out of bounds");

		tempval = ESM::ESMWriter::skillToActorValTES4(mData.mSkills[ESM3index][ESM3index2]);

		return tempval;
	}

    void Class::blank()
    {
        mName.clear();
        mDescription.clear();

        mData.mAttribute[0] = mData.mAttribute[1] = 0;
        mData.mSpecialization = 0;
        mData.mIsPlayable = 0;
        mData.mCalc = 0;

        for (int i=0; i<5; ++i)
            for (int i2=0; i2<2; ++i2)
                mData.mSkills[i][i2] = 0;
    }
}
