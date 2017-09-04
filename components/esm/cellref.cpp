#include "cellref.hpp"

#include <iostream>

#include "esmreader.hpp"
#include "esmwriter.hpp"

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

void ESM::CellRef::exportTES4 (ESMWriter &esm, uint32_t teleportRefID) const
{
//	mRefNum.save (esm, wideRefNum);
	// NAME = FormID
	// lookup base record's FormID based on mRefID or mRefNum
	uint32_t baseFormID;

	// Substitutions
	if (mRefID == "DoorMarker")
		baseFormID = 0x01;
	else if (mRefID == "TravelMarker")
		baseFormID = 0x02;
	else if (mRefID == "NorthMarker")
		baseFormID = 0x03;
	else if (mRefID == "DivineMarker")
		baseFormID = 0x05;
	else if (mRefID == "TempleMarker")
		baseFormID = 0x06;
	else
		baseFormID = esm.crossRefStringID(mRefID);

	esm.startSubRecordTES4("NAME");
	esm.writeT<uint32_t>(baseFormID);
	esm.endSubRecordTES4("NAME");

	// TODO: if (base type == door):
	//   1. create searchable list of doorRefID+Cell+location to match
	// If (mTeleport == true) 
	//   1. add this reference to stack for second pass processing,
	//   2. save writer context to go back to replace with correct RefID
	// After all references created, make a second pass to:
	//   1. iterate through stack of all door references with mTeleport==true
	//   2. match mDestCell+mDoorDest with searchable list of doorRefID+Cell+location
	//   3. replace XTEL subrecord with referenceID from #2
	if (mTeleport == true)
	{
		// find teleport door (reference) near the doordest location...
		if (teleportRefID != 0)
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
	//	esm.writeHNCString("NAME", mRefID);
//	esm.startSubRecordTES4("EDID");
//	esm.writeHCString(mRefID);
//	esm.endSubRecordTES4("EDID");

	// XRGD, ragdoll
	// XESP, parent object
	if ( (mOwner != "") || (mFaction != "") )
	{
		esm.startSubRecordTES4("XOWN");
		if (mFaction != "")
			esm.writeT<uint32_t>(esm.crossRefStringID(mFaction) );
		else
			esm.writeT<uint32_t>(esm.crossRefStringID(mOwner) );
		esm.endSubRecordTES4("XOWN");
	}
	// XGLB
	if ( (mOwner != "") && (mGlobalVariable != "") )
	{
		esm.startSubRecordTES4("XGLB");
		esm.writeT<uint32_t>(esm.crossRefStringID(mGlobalVariable) );
		esm.endSubRecordTES4("XGLB");
	}
	// XRNK, faction rank
	if ( (mFaction != "") && (mFactionRank != -2) )
	{
		esm.startSubRecordTES4("XRNK");
		esm.writeT<int32_t>(mFactionRank);
		esm.endSubRecordTES4("XRNK");
	}

	// XSCL
	if (mScale != 1.0) {
//		esm.writeHNT("XSCL", mScale);
		esm.startSubRecordTES4("XSCL");
		esm.writeT<float>(mScale);
		esm.endSubRecordTES4("XSCL");
	}

	// DATA	
	esm.startSubRecordTES4("DATA");
	esm.writeT<float>(mPos.pos[0]);
	esm.writeT<float>(mPos.pos[1]);
	esm.writeT<float>(mPos.pos[2]);
	esm.writeT<float>(mPos.rot[0]);
	esm.writeT<float>(mPos.rot[1]);
	esm.writeT<float>(mPos.rot[2]);
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
