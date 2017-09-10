#include "loadcell.hpp"

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
#endif

#include <string>
#include <sstream>
#include <list>

#include <boost/concept_check.hpp>

#include <components/misc/stringops.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"
#include "cellid.hpp"

namespace
{
    ///< Translate 8bit/24bit code (stored in refNum.mIndex) into a proper refNum
    void adjustRefNum (ESM::RefNum& refNum, ESM::ESMReader& reader)
    {
        unsigned int local = (refNum.mIndex & 0xff000000) >> 24;

        // If we have an index value that does not make sense, assume that it was an addition
        // by the present plugin (but a faulty one)
        if (local && local <= reader.getGameFiles().size())
        {
            // If the most significant 8 bits are used, then this reference already exists.
            // In this case, do not spawn a new reference, but overwrite the old one.
            refNum.mIndex &= 0x00ffffff; // delete old plugin ID
            refNum.mContentFile = reader.getGameFiles()[local-1].index;
        }
        else
        {
            // This is an addition by the present plugin. Set the corresponding plugin index.
            refNum.mContentFile = reader.getIndex();
        }
    }
}

namespace ESM
{
    unsigned int Cell::sRecordId = REC_CELL;

    // Some overloaded compare operators.
    bool operator== (const MovedCellRef& ref, const RefNum& refNum)
    {
        return ref.mRefNum == refNum;
    }

    bool operator== (const CellRef& ref, const RefNum& refNum)
    {
        return ref.mRefNum == refNum;
    }

    void Cell::load(ESMReader &esm, bool &isDeleted, bool saveContext)
    {
        loadNameAndData(esm, isDeleted);
        loadCell(esm, saveContext);
    }

    void Cell::loadNameAndData(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        blank();

        bool hasData = false;
        bool isLoaded = false;
        while (!isLoaded && esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
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

        if (!hasData)
            esm.fail("Missing DATA subrecord");

        mCellId.mPaged = !(mData.mFlags & Interior);

        if (mCellId.mPaged)
        {
            mCellId.mWorldspace = ESM::CellId::sDefaultWorldspace;
            mCellId.mIndex.mX = mData.mX;
            mCellId.mIndex.mY = mData.mY;
        }
        else
        {
            mCellId.mWorldspace = Misc::StringUtils::lowerCase (mName);
            mCellId.mIndex.mX = 0;
            mCellId.mIndex.mY = 0;
        }
    }

    void Cell::loadCell(ESMReader &esm, bool saveContext)
    {
        bool isLoaded = false;
        while (!isLoaded && esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'I','N','T','V'>::value:
                    int waterl;
                    esm.getHT(waterl);
                    mWater = static_cast<float>(waterl);
                    mWaterInt = true;
                    break;
                case ESM::FourCC<'W','H','G','T'>::value:
                    esm.getHT(mWater);
                    mWaterInt = false;
                    break;
                case ESM::FourCC<'A','M','B','I'>::value:
                    esm.getHT(mAmbi);
                    break;
                case ESM::FourCC<'R','G','N','N'>::value:
                    mRegion = esm.getHString();
                    break;
                case ESM::FourCC<'N','A','M','5'>::value:
                    esm.getHT(mMapColor);
                    break;
                case ESM::FourCC<'N','A','M','0'>::value:
                    esm.getHT(mRefNumCounter);
                    break;
                default:
                    esm.cacheSubName();
                    isLoaded = true;
                    break;
            }
        }

        if (saveContext) 
        {
            mContextList.push_back(esm.getContext());
            esm.skipRecord();
        }
    }

    void Cell::postLoad(ESMReader &esm)
    {
        // Save position of the cell references and move on
        mContextList.push_back(esm.getContext());
        esm.skipRecord();
    }

    void Cell::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNOCString("NAME", mName);
        esm.writeHNT("DATA", mData, 12);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        if (mData.mFlags & Interior)
        {
            if (mWaterInt) {
                int water =
                    (mWater >= 0) ? (int) (mWater + 0.5) : (int) (mWater - 0.5);
                esm.writeHNT("INTV", water);
            } else {
                esm.writeHNT("WHGT", mWater);
            }

            if (mData.mFlags & QuasiEx)
                esm.writeHNOCString("RGNN", mRegion);
            else
                esm.writeHNT("AMBI", mAmbi, 16);
        }
        else
        {
            esm.writeHNOCString("RGNN", mRegion);
            if (mMapColor != 0)
                esm.writeHNT("NAM5", mMapColor);
        }

        if (mRefNumCounter != 0)
            esm.writeHNT("NAM0", mRefNumCounter);
    }

	void Cell::exportTES3(ESMWriter &esm, bool isDeleted) const
	{
//		esm.writeHNOCString("NAME", mName);
		esm.writeHNCString("NAME", mName);
		esm.writeHNT("DATA", mData, 12);

		if (isDeleted)
		{
			esm.writeHNCString("DELE", "");
			return;
		}

		if (mData.mFlags & Interior)
		{
			if (mWaterInt) {
				int water =
					(mWater >= 0) ? (int)(mWater + 0.5) : (int)(mWater - 0.5);
				esm.writeHNT("INTV", water);
			}
			else {
				esm.writeHNT("WHGT", mWater);
			}

			if (mData.mFlags & QuasiEx)
				esm.writeHNOCString("RGNN", mRegion);
			else
				esm.writeHNT("AMBI", mAmbi, 16);
		}
		else
		{
			esm.writeHNOCString("RGNN", mRegion);
			if (mMapColor != 0)
				esm.writeHNT("NAM5", mMapColor);
		}

		if (mRefNumCounter != 0)
			esm.writeHNT("NAM0", mRefNumCounter);
	}

	void Cell::exportTES4(ESMWriter &esm) const
	{
        // First: If Exterior, then Divide Cell into quarters and repeat rest of steps for all four quarters
        
        if (isExterior())
        {
            // find base x,y (multiply by 2)
            int BaseX = mData.mX * 2;
            int BaseY = mData.mY * 2;
            
            // PSEUDO-CODE: iterate through four quarters:
            // for (x=0; x<2; x++)
            //      for (y=0; y<2; y++)
            //          do(BaseX+x,BaseY+y)
            for (int x=0, offset=0; x < 2; x++)
            {
                for (int y=0; y < 2; y++)
                {
                    exportSubCellTES4(esm, BaseX+x, BaseY+y, offset++);
                }
            }
            
        }
        else
        {
        //    do(0);
            exportSubCellTES4(esm, 0, 0, 0);
        }
    }
    
    void Cell::exportSubCellTES4(ESMWriter &esm, int subX, int subY, int offset) const
    {
        // Write EDID (TES4-style editor string identifier)
		std::string newEDID = esm.generateEDIDTES4(mName, 1);
        char *charbuf = new char[newEDID.size()+6];
		std::ostringstream debugstream;

		bool isValidCellname = false;
		int nameOffset = offset;

		if (isExterior() == false)
			isValidCellname = true;

		while ( (isValidCellname != true) && (newEDID.size() > 0) )
        {
			int len = snprintf(charbuf, newEDID.size()+6, "%s%02d", newEDID.c_str(), nameOffset);
			if ((len < 0) || (len > newEDID.size()+6))
			{
				throw std::runtime_error("exportSubCellTES4 EDID snprintf error");
			}
			charbuf[len] = '\0';
			if ( esm.mCellnameMgr.find(charbuf) != esm.mCellnameMgr.end() )
			{
				// name found, increase offset and try again
				nameOffset += 4;
				continue;
			}
			else
			{
				// name not found, add to mCellnameMgr and break;
				newEDID = std::string(charbuf);
				esm.mCellnameMgr.insert(std::make_pair(newEDID, 0));
				isValidCellname = true;
			}
		}

		debugstream << "EDID=[" << newEDID << "]; ";
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(newEDID);
		esm.endSubRecordTES4("EDID");

        // Write FULL (human readable name)
		if (isExterior() == false)
		{
			esm.startSubRecordTES4("FULL");
			esm.writeHCString(mName);
			esm.endSubRecordTES4("FULL");
		}

        // Write DATA (flags)
		char dataFlags=0;
		dataFlags |= 0x40; // DEFAULT: 0x40, Hand changed
		if (mData.mFlags & Interior) // Interior Cell
			dataFlags |= 0x01; // Can't fast travel? Interior?
		if (mData.mFlags & HasWater)
			dataFlags |= 0x02; // Has Water
		// 0x04, Invert Fast Travel Behavior?
        // 0x08, Force hide land (exterior cell) / Oblivion interior
		// 0x10, Unknown or reserved?
        if ((mData.mFlags & NoSleep))
            dataFlags |= 0x20; // 0x20, Public place
        if (mData.mFlags & QuasiEx)
            dataFlags |= 0x08; // 0x80, Behave like exterior
		esm.startSubRecordTES4("DATA");
		esm.writeT<char>(dataFlags);
		esm.endSubRecordTES4("DATA");

        // Write XCLL (lighting)
		if (!isExterior())
		{
			esm.startSubRecordTES4("XCLL");
			esm.writeT<uint32_t>(mAmbi.mAmbient); // ambient color
			esm.writeT<uint32_t>(mAmbi.mSunlight); // direction color
			esm.writeT<uint32_t>(mAmbi.mFog); // fog color
			//	TODO: figure out how to convert mAmbi.mFogDensity to Oblivion fog system
			esm.writeT<float>(0); // fog near
			esm.writeT<float>(0);  // fog far
			esm.writeT<uint32_t>(0); // rotation xy
			esm.writeT<uint32_t>(0); // rotation z
			esm.writeT<float>(0); // directional fade
			esm.writeT<float>(0); // fog clip distance
			esm.endSubRecordTES4("XCLL");
		}

        // Write XCLW (water level)
        if ( (mData.mFlags & HasWater) && (mWater != 0) )
        {
            esm.startSubRecordTES4("XCLW");
            esm.writeT<float>(mWater);
            esm.endSubRecordTES4("XCLW");
        }
        
        // Write XCLC (X,Y coordinates)
        if (isExterior())
        {
            esm.startSubRecordTES4("XCLC");
            esm.writeT<long>(subX);
            esm.writeT<long>(subY);
            esm.endSubRecordTES4("XCLC");
			debugstream << "X,Y=[" << (subX) <<"," << (subY) << "]; ";
        }
               
        // XOWN == ?? morrowind equivalent unknown
		debugstream << std::endl;
//		OutputDebugString(debugstream.str().c_str());
	}

    void Cell::restore(ESMReader &esm, int iCtx) const
    {
        esm.restoreContext(mContextList.at (iCtx));
    }

    std::string Cell::getDescription() const
    {
        if (mData.mFlags & Interior)
        {
            return mName;
        }
        else
        {
            std::ostringstream stream;
            stream << mData.mX << ", " << mData.mY;
            return stream.str();
        }
    }

    bool Cell::getNextRef(ESMReader &esm, CellRef &ref, bool &isDeleted, bool ignoreMoves, MovedCellRef *mref)
    {
        isDeleted = false;

        // TODO: Try and document reference numbering, I don't think this has been done anywhere else.
        if (!esm.hasMoreSubs())
            return false;

        // NOTE: We should not need this check. It is a safety check until we have checked
        // more plugins, and how they treat these moved references.
        if (esm.isNextSub("MVRF"))
        {
            if (ignoreMoves)
            {
                esm.getHT (mref->mRefNum.mIndex);
                esm.getHNOT (mref->mTarget, "CNDT");
                adjustRefNum (mref->mRefNum, esm);
            }
            else
            {
                // skip rest of cell record (moved references), they are handled elsewhere
                esm.skipRecord(); // skip MVRF, CNDT
                return false;
            }
        }

        if (esm.peekNextSub("FRMR"))
        {
            ref.load (esm, isDeleted);

            // Identify references belonging to a parent file and adapt the ID accordingly.
            adjustRefNum (ref.mRefNum, esm);
            return true;
        }
        return false;
    }

    bool Cell::getNextMVRF(ESMReader &esm, MovedCellRef &mref)
    {
        esm.getHT(mref.mRefNum.mIndex);
        esm.getHNOT(mref.mTarget, "CNDT");

        adjustRefNum (mref.mRefNum, esm);

        return true;
    }

    void Cell::blank()
    {
        mName.clear();
        mRegion.clear();
        mWater = 0;
        mWaterInt = false;
        mMapColor = 0;
        mRefNumCounter = 0;

        mData.mFlags = 0;
        mData.mX = 0;
        mData.mY = 0;

        mAmbi.mAmbient = 0;
        mAmbi.mSunlight = 0;
        mAmbi.mFog = 0;
        mAmbi.mFogDensity = 0;
    }

    const CellId& Cell::getCellId() const
    {
        return mCellId;
    }
}
