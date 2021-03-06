#include "cellref.hpp"

#include <iostream>

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <apps/opencs/model/doc/document.hpp>

void ESM::RefNum::load (ESMReader& esm, bool wide, const std::string& tag)
{
    if (wide)
        esm.getHNT (*this, tag.c_str(), 8);
    else
        esm.getHNT (mIndex, tag.c_str());
}

void ESM::RefNum::save (ESMWriter &esm, bool wide, const std::string& tag) const
{
    if (wide)
        esm.writeHNT (tag, *this, 8);
    else
    {
        int refNum = (mIndex & 0xffffff) | ((hasContentFile() ? mContentFile : 0xff)<<24);

        esm.writeHNT (tag, refNum, 4);
    }
}


void ESM::CellRef::load (ESMReader& esm, bool &isDeleted, bool wideRefNum)
{
    loadId(esm, wideRefNum);
    loadData(esm, isDeleted);
}

void ESM::CellRef::loadId (ESMReader& esm, bool wideRefNum)
{
    // According to Hrnchamd, this does not belong to the actual ref. Instead, it is a marker indicating that
    // the following refs are part of a "temp refs" section. A temp ref is not being tracked by the moved references system.
    // Its only purpose is a performance optimization for "immovable" things. We don't need this, and it's problematic anyway,
    // because any item can theoretically be moved by a script.
    if (esm.isNextSub ("NAM0"))
        esm.skipHSub();

    blank();

    mRefNum.load (esm, wideRefNum);

    mRefID = esm.getHNOString ("NAME");
    if (mRefID.empty())
    {
        std::ios::fmtflags f(std::cerr.flags());
        std::cerr << "Warning: got CellRef with empty RefId in " << esm.getName() << " 0x" << std::hex << esm.getFileOffset() << std::endl;
        std::cerr.flags(f);
    }
}

void ESM::CellRef::loadData(ESMReader &esm, bool &isDeleted)
{
    isDeleted = false;

    bool isLoaded = false;
    while (!isLoaded && esm.hasMoreSubs())
    {
        esm.getSubName();
        switch (esm.retSubName().intval)
        {
            case ESM::FourCC<'U','N','A','M'>::value:
                esm.getHT(mReferenceBlocked);
                break;
            case ESM::FourCC<'X','S','C','L'>::value:
                esm.getHT(mScale);
                break;
            case ESM::FourCC<'A','N','A','M'>::value:
                mOwner = esm.getHString();
                break;
            case ESM::FourCC<'B','N','A','M'>::value:
                mGlobalVariable = esm.getHString();
                break;
            case ESM::FourCC<'X','S','O','L'>::value:
                mSoul = esm.getHString();
                break;
            case ESM::FourCC<'C','N','A','M'>::value:
                mFaction = esm.getHString();
                break;
            case ESM::FourCC<'I','N','D','X'>::value:
                esm.getHT(mFactionRank);
                break;
            case ESM::FourCC<'X','C','H','G'>::value:
                esm.getHT(mEnchantmentCharge);
                break;
            case ESM::FourCC<'I','N','T','V'>::value:
                esm.getHT(mChargeInt);
                break;
            case ESM::FourCC<'N','A','M','9'>::value:
                esm.getHT(mGoldValue);
                break;
            case ESM::FourCC<'D','O','D','T'>::value:
                esm.getHT(mDoorDest);
                mTeleport = true;
                break;
            case ESM::FourCC<'D','N','A','M'>::value:
                mDestCell = esm.getHString();
                break;
            case ESM::FourCC<'F','L','T','V'>::value:
                esm.getHT(mLockLevel);
                break;
            case ESM::FourCC<'K','N','A','M'>::value:
                mKey = esm.getHString();
                break;
            case ESM::FourCC<'T','N','A','M'>::value:
                mTrap = esm.getHString();
                break;
            case ESM::FourCC<'D','A','T','A'>::value:
                esm.getHT(mPos, 24);
                break;
            case ESM::FourCC<'N','A','M','0'>::value:
                esm.skipHSub();
                break;
            case ESM::SREC_DELE:
                esm.skipHSub();
                isDeleted = true;
                break;
            default:
                esm.cacheSubName();
                isLoaded = true;
                break;
        }
    }
}

void ESM::CellRef::save (ESMWriter &esm, bool wideRefNum, bool inInventory, bool isDeleted) const
{
    mRefNum.save (esm, wideRefNum);

    esm.writeHNCString("NAME", mRefID);

    if (isDeleted) {
        esm.writeHNCString("DELE", "");
        return;
    }

    if (mScale != 1.0) {
        esm.writeHNT("XSCL", mScale);
    }

    esm.writeHNOCString("ANAM", mOwner);
    esm.writeHNOCString("BNAM", mGlobalVariable);
    esm.writeHNOCString("XSOL", mSoul);

    esm.writeHNOCString("CNAM", mFaction);
    if (mFactionRank != -2) {
        esm.writeHNT("INDX", mFactionRank);
    }

    if (mEnchantmentCharge != -1)
        esm.writeHNT("XCHG", mEnchantmentCharge);

    if (mChargeInt != -1)
        esm.writeHNT("INTV", mChargeInt);

    if (mGoldValue != 1) {
        esm.writeHNT("NAM9", mGoldValue);
    }

    if (!inInventory && mTeleport)
    {
        esm.writeHNT("DODT", mDoorDest);
        esm.writeHNOCString("DNAM", mDestCell);
    }

    if (!inInventory && mLockLevel != 0) {
        esm.writeHNT("FLTV", mLockLevel);
    }

    if (!inInventory)
        esm.writeHNOCString ("KNAM", mKey);

    if (!inInventory)
        esm.writeHNOCString ("TNAM", mTrap);

    if (mReferenceBlocked != -1)
        esm.writeHNT("UNAM", mReferenceBlocked);

    if (!inInventory)
        esm.writeHNT("DATA", mPos, 24);
}

void ESM::CellRef::exportTES3 (ESMWriter &esm, bool wideRefNum, bool inInventory, bool isDeleted) const
{
	mRefNum.save (esm, wideRefNum);

	esm.writeHNCString("NAME", mRefID);

	if (isDeleted) {
//		esm.writeHNCString("DELE", "");
		esm.writeHNString("DELE", "d,H", 4);
		return;
	}

	if (mScale != 1.0) {
		esm.writeHNT("XSCL", mScale);
	}

	esm.writeHNOCString("ANAM", mOwner);
	esm.writeHNOCString("BNAM", mGlobalVariable);
	esm.writeHNOCString("XSOL", mSoul);

	esm.writeHNOCString("CNAM", mFaction);
	if (mFactionRank != -2) {
		esm.writeHNT("INDX", mFactionRank);
	}

	if (mEnchantmentCharge != -1)
		esm.writeHNT("XCHG", mEnchantmentCharge);

	if (mChargeInt != -1)
		esm.writeHNT("INTV", mChargeInt);

//	if (mGoldValue != 1) {
	if (mGoldValue != 1 && mGoldValue != 0) {
		esm.writeHNT("NAM9", mGoldValue);
	}

	if (!inInventory && mTeleport)
	{
		esm.writeHNT("DODT", mDoorDest);
		esm.writeHNOCString("DNAM", mDestCell);
	}

	if (!inInventory && mLockLevel != 0) {
		esm.writeHNT("FLTV", mLockLevel);
	}

	if (!inInventory)
		esm.writeHNOCString ("KNAM", mKey);

	if (!inInventory)
		esm.writeHNOCString ("TNAM", mTrap);

	if (mReferenceBlocked != -1)
		esm.writeHNT("UNAM", mReferenceBlocked);

	if (!inInventory)
		esm.writeHNT("DATA", mPos, 24);
}

void ESM::CellRef::exportTES4 (ESMWriter &esm, const CSMDoc::Document &doc, const std::string &refEDID, uint32_t teleportRefID, ESM::Position *returnPosition) const
{
//	mRefNum.save (esm, wideRefNum);
	// NAME = FormID
	// lookup base record's FormID based on mRefID or mRefNum
	uint32_t baseFormID=0;
	ESM::Position substitutionOffset;
	float substitutionScale=0;
	std::string baseEDID="";

	for (int i=0; i < 3; i++)
	{
		substitutionOffset.pos[i] = 0;
		substitutionOffset.rot[i] = 0;
	}

	if (baseFormID == 0)
	{
		baseEDID = esm.generateEDIDTES4(mRefID, 0, "");
		baseFormID = esm.crossRefStringID(baseEDID, "BASEREF", false);
	}

	// over-ride all of the above if RefTransformOp found

	if (baseEDID != "" && esm.mStringTransformMap.find(baseEDID) != esm.mStringTransformMap.end())
	{
		float evalResult=0.0f;
		std::string opstring="";
		opstring = esm.mStringTransformMap[baseEDID].op_x;
		if (opstring != "")
		{
			if (esm.evaluateOpString(opstring, evalResult, mPos) == false)
			{
				// issue error/warning
				evalResult = 0.0f;
			}
			substitutionOffset.pos[0] = evalResult;
		}
		opstring = esm.mStringTransformMap[baseEDID].op_y;
		if (opstring != "")
		{
			if (esm.evaluateOpString(opstring, evalResult, mPos) == false)
			{
				// issue error/warning
				evalResult = 0.0f;
			}
			substitutionOffset.pos[1] = evalResult;
		}
		opstring = esm.mStringTransformMap[baseEDID].op_z;
		if (opstring != "")
		{
			if (esm.evaluateOpString(opstring, evalResult, mPos) == false)
			{
				// issue error/warning
				evalResult = 0.0f;
			}
			substitutionOffset.pos[2] = evalResult;
		}
		opstring = esm.mStringTransformMap[baseEDID].op_rx;
		if (opstring != "")
		{
			if (esm.evaluateOpString(opstring, evalResult, mPos) == false)
			{
				// issue error/warning
				evalResult = 0.0f;
			}
			substitutionOffset.rot[0] = evalResult / 57.2958;
		}
		opstring = esm.mStringTransformMap[baseEDID].op_ry;
		if (opstring != "")
		{
			if (esm.evaluateOpString(opstring, evalResult, mPos) == false)
			{
				// issue error/warning
				evalResult = 0.0f;
			}
			substitutionOffset.rot[1] = evalResult / 57.2958;
		}
		opstring = esm.mStringTransformMap[baseEDID].op_rz;
		if (opstring != "")
		{
			if (esm.evaluateOpString(opstring, evalResult, mPos) == false)
			{
				// issue error/warning
				evalResult = 0.0f;
			}
			substitutionOffset.rot[2] = evalResult / 57.2958;
		}
		substitutionScale = esm.mStringTransformMap[baseEDID].fscale;

	}

	esm.startSubRecordTES4("NAME");
	if (baseFormID == 0)
		esm.writeT<uint32_t>(0x3B);
	else
		esm.writeT<uint32_t>(baseFormID);
	esm.endSubRecordTES4("NAME");

	if (mTeleport == true)
	{
		// find teleport door (reference) near the doordest location...
		if (teleportRefID != 0 && returnPosition != 0)
		{
			esm.startSubRecordTES4("XTEL");
			esm.writeT<uint32_t>(teleportRefID);

			esm.writeT<float>(mDoorDest.pos[0]);
			esm.writeT<float>(mDoorDest.pos[1]);
			esm.writeT<float>(mDoorDest.pos[2]);
			esm.writeT<float>(mDoorDest.rot[0]);
			esm.writeT<float>(mDoorDest.rot[1]);
			esm.writeT<float>(mDoorDest.rot[2]);

			esm.endSubRecordTES4("XTEL");
		}
	}

	// EDID = EditorID
	if (refEDID != "")
	{
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(refEDID);
		esm.endSubRecordTES4("EDID");
	}
	else if (baseFormID == 0)
	{
		esm.startSubRecordTES4("EDID");
		esm.writeHCString("NULLREF" + baseEDID);
		esm.endSubRecordTES4("EDID");
	}

	// XRGD, ragdoll
	// XESP, parent object
	uint32_t factionID = esm.crossRefStringID(mFaction, "FACT");
	uint32_t ownerID = 0;
	if (factionID == 0)
		ownerID = esm.crossRefStringID(mOwner, "NPC_");
	else
		ownerID = factionID;

	if (esm.mConversionOptions.find("#noowners") != std::string::npos)
		ownerID = 0;
	if (ownerID != 0)
	{
		esm.startSubRecordTES4("XOWN");
		esm.writeT<uint32_t>(ownerID);
		esm.endSubRecordTES4("XOWN");

		// XRNK, faction rank
		if (factionID != 0 && mFactionRank != -2)
		{
			esm.startSubRecordTES4("XRNK");
			esm.writeT<int32_t>(mFactionRank);
			esm.endSubRecordTES4("XRNK");
		}

	}

	// XGLB
	uint32_t globalVarID = esm.crossRefStringID(mGlobalVariable, "GLOB");
	if (ownerID != 0 && globalVarID != 0)
	{
		esm.startSubRecordTES4("XGLB");
		esm.writeT<uint32_t>(globalVarID);
		esm.endSubRecordTES4("XGLB");
	}

	// XSCL
	if (mScale != 1.0) 
	{
		esm.startSubRecordTES4("XSCL");
		if (substitutionScale != 0)
			esm.writeT<float>(substitutionScale);
		else
			esm.writeT<float>(mScale);
		esm.endSubRecordTES4("XSCL");
	}

	// DATA	
	esm.startSubRecordTES4("DATA");
	for (int i=0; i < 3; i++)
	{
		float floatVal = mPos.pos[i]+substitutionOffset.pos[i];
		esm.writeT<float>(floatVal);
	}
	for (int i=0; i < 3; i++)
	{
		float floatVal = mPos.rot[i]+substitutionOffset.rot[i];
		esm.writeT<float>(floatVal);
	}
	esm.endSubRecordTES4("DATA");

}

void ESM::CellRef::blank()
{
    mRefNum.unset();
    mRefID.clear();    
    mScale = 1;
    mOwner.clear();
    mGlobalVariable.clear();
    mSoul.clear();
    mFaction.clear();
    mFactionRank = -2;
    mChargeInt = -1;
    mChargeIntRemainder = 0.0f;
    mEnchantmentCharge = -1;
    mGoldValue = 0;
    mDestCell.clear();
    mLockLevel = 0;
    mKey.clear();
    mTrap.clear();
    mReferenceBlocked = -1;
    mTeleport = false;
    
    for (int i=0; i<3; ++i)
    {
        mDoorDest.pos[i] = 0;
        mDoorDest.rot[i] = 0;
        mPos.pos[i] = 0;
        mPos.rot[i] = 0;
    }
}

bool ESM::operator== (const RefNum& left, const RefNum& right)
{
    return left.mIndex==right.mIndex && left.mContentFile==right.mContentFile;
}

bool ESM::operator< (const RefNum& left, const RefNum& right)
{
    if (left.mIndex<right.mIndex)
        return true;

    if (left.mIndex>right.mIndex)
        return false;

    return left.mContentFile<right.mContentFile;
}
