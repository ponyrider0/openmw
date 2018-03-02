#include "loadglob.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Global::sRecordId = REC_GLOB;

    void Global::load (ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mId = esm.getHNString ("NAME");

        if (esm.isNextSub ("DELE"))
        {
            esm.skipHSub();
            isDeleted = true;
        }
        else
        {
            mValue.read (esm, ESM::Variant::Format_Global);
        }
    }

    void Global::save (ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString ("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString ("DELE", "");
        }
        else
        {
            mValue.write (esm, ESM::Variant::Format_Global);
        }
    }

	void Global::exportTESx (ESMWriter &esm) const
	{

		// must convert mID into a TES4 compatible EditorID
		std::string sEditorID = esm.generateEDIDTES4(mId, 0, "GLOB");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(sEditorID);
		esm.endSubRecordTES4("EDID");

		// FNAM
		esm.startSubRecordTES4("FNAM");
		char fnam_val=0;
		switch (mValue.getType() )
		{
		case VT_Short:
			fnam_val = 's';
			break;
		case VT_Long:
			fnam_val = 'l';
			break;
		case VT_Float:
			fnam_val = 'f';
			break;
		default:
			fnam_val = 0;
		}
		esm.writeT<char>(fnam_val);
		esm.endSubRecordTES4("FNAM");

		// FLTV
//		mValue.write (esm, ESM::Variant::Format_Global);
		float glob_val=0;
		esm.startSubRecordTES4("FLTV");
		if (fnam_val != 0)
			glob_val = mValue.getFloat();
		esm.writeT<float>(glob_val);
		esm.endSubRecordTES4("FLTV");		

	}

    void Global::blank()
    {
        mValue.setType (ESM::VT_None);
    }

    bool operator== (const Global& left, const Global& right)
    {
        return left.mId==right.mId && left.mValue==right.mValue;
    }
}
