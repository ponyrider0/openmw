#include "loadland.hpp"

#include <utility>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Land::sRecordId = REC_LAND;

    Land::Land()
        : mFlags(0)
        , mX(0)
        , mY(0)
        , mPlugin(0)
        , mDataTypes(0)
        , mLandData(NULL)
    {
    }

    void transposeTextureData(const uint16_t *in, uint16_t *out)
    {
        int readPos = 0; //bit ugly, but it works
        for ( int y1 = 0; y1 < 4; y1++ )
            for ( int x1 = 0; x1 < 4; x1++ )
                for ( int y2 = 0; y2 < 4; y2++)
                    for ( int x2 = 0; x2 < 4; x2++ )
                        out[(y1*4+y2)*16+(x1*4+x2)] = in[readPos++];
    }

    Land::~Land()
    {
        delete mLandData;
    }

    void Land::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mPlugin = esm.getIndex();

        bool hasLocation = false;
        bool isLoaded = false;
        while (!isLoaded && esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'I','N','T','V'>::value:
                    esm.getSubHeaderIs(8);
                    esm.getT<int>(mX);
                    esm.getT<int>(mY);
                    hasLocation = true;
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mFlags);
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

        if (!hasLocation)
            esm.fail("Missing INTV subrecord");

        mContext = esm.getContext();

        mLandData = NULL;

        // Skip the land data here. Load it when the cell is loaded.
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'V','N','M','L'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VNML;
                    break;
                case ESM::FourCC<'V','H','G','T'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VHGT;
                    break;
                case ESM::FourCC<'W','N','A','M'>::value:
                    esm.getHExact(mWnam, sizeof(mWnam));
                    mDataTypes |= DATA_WNAM;
                    break;
                case ESM::FourCC<'V','C','L','R'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VCLR;
                    break;
                case ESM::FourCC<'V','T','E','X'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VTEX;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void Land::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.startSubRecord("INTV");
        esm.writeT(mX);
        esm.writeT(mY);
        esm.endRecord("INTV");

        esm.writeHNT("DATA", mFlags);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        if (mLandData)
        {
            if (mDataTypes & Land::DATA_VNML) {
                esm.writeHNT("VNML", mLandData->mNormals, sizeof(mLandData->mNormals));
            }
            if (mDataTypes & Land::DATA_VHGT) {
                VHGT offsets;
                offsets.mHeightOffset = mLandData->mHeights[0] / HEIGHT_SCALE;
                offsets.mUnk1 = mLandData->mUnk1;
                offsets.mUnk2 = mLandData->mUnk2;

                float prevY = mLandData->mHeights[0];
                int number = 0; // avoid multiplication
                for (int i = 0; i < LAND_SIZE; ++i) {
                    float diff = (mLandData->mHeights[number] - prevY) / HEIGHT_SCALE;
                    offsets.mHeightData[number] =
                        (diff >= 0) ? (int8_t) (diff + 0.5) : (int8_t) (diff - 0.5);

                    float prevX = prevY = mLandData->mHeights[number];
                    ++number;

                    for (int j = 1; j < LAND_SIZE; ++j) {
                        diff = (mLandData->mHeights[number] - prevX) / HEIGHT_SCALE;
                        offsets.mHeightData[number] =
                            (diff >= 0) ? (int8_t) (diff + 0.5) : (int8_t) (diff - 0.5);

                        prevX = mLandData->mHeights[number];
                        ++number;
                    }
                }
                esm.writeHNT("VHGT", offsets, sizeof(VHGT));
            }
        }

        if (mDataTypes & Land::DATA_WNAM) {
            esm.writeHNT("WNAM", mWnam, 81);
        }

        if (mLandData)
        {
            if (mDataTypes & Land::DATA_VCLR) {
                esm.writeHNT("VCLR", mLandData->mColours, 3*LAND_NUM_VERTS);
            }
            if (mDataTypes & Land::DATA_VTEX) {
                uint16_t vtex[LAND_NUM_TEXTURES];
                transposeTextureData(mLandData->mTextures, vtex);
                esm.writeHNT("VTEX", vtex, sizeof(vtex));
            }
        }

    }

	void Land::exportSubCellTES4(ESMWriter &esm, int offsetX, int offsetY) const
	{
		int i, j;
		const LandData *landData;

		// Load LandData once and pass off to each call to exportSubCell
//		landData = getLandData(Land::DATA_VNML|Land::DATA_VHGT|Land::DATA_VCLR|Land::DATA_VTEX);
	
		// DATA 4-bytes
		esm.startSubRecordTES4("DATA");
		// write correct bitmask for enabled subrecords
		unsigned char flags=0;
		esm.writeT<unsigned char>(flags); // bitmask of subrecords present
		esm.writeT<unsigned char>(0);
		esm.writeT<unsigned char>(0);
		esm.writeT<unsigned char>(0);
		esm.endSubRecordTES4("DATA");
		
		// VNML 33x33 grid of 3-bytes vertex normals
		landData = getLandData(Land::DATA_VNML);
		if (landData != 0)
		{
			esm.startSubRecordTES4("VNML");
			for (int u=0; u < 33; u++)
			{
				for (int v=0; v < 33; v++)
				{
					// convert 65x65 to 33x33
					i = u + (32 * offsetX);
					j = v + (32 * offsetY);
					int vnormalIndex = ((i*65)+j)*3;
					if (vnormalIndex >= (65*65*3))
						throw std::runtime_error("loadland.cpp: landscape index calculation error.");
					esm.writeT<signed char>(landData->mNormals[vnormalIndex]); // X
					vnormalIndex = (i*65)+j+1;
					if (vnormalIndex >= (65*65*3))
						throw std::runtime_error("loadland.cpp: landscape index calculation error.");
					esm.writeT<signed char>(landData->mNormals[vnormalIndex]); // Y
					vnormalIndex = (i*65)+j+2;
					if (vnormalIndex >= (65*65*3))
						throw std::runtime_error("loadland.cpp: landscape index calculation error.");
					esm.writeT<signed char>(landData->mNormals[vnormalIndex]); // Z

				}
			}
			esm.endSubRecordTES4("VNML");
		}
		
		// VHGT gradient of 33x33 grid of height data,
		landData = getLandData(Land::DATA_VHGT);
		if (landData != 0)
		{
			unsigned char deltaVal;
			float oldVal = landData->mHeightOffset;
			esm.startSubRecordTES4("VHGT");
			esm.writeT<float>(landData->mHeightOffset); // 32-bit float (Offset)
			for (int u=0; u < 33; u++)
			{
				for (int v=0; v < 33; v++)
				{
					// convert 65x64 to 33x33
					i = u + (32 * offsetX);
					j = v + (32 * offsetY);
					float newVal = landData->mHeights[(i*65)+j];
					deltaVal = newVal - oldVal;
					esm.writeT<signed char>(deltaVal); // 1 byte for each point
					newVal = oldVal;
				}
			}
			// final 3 bytes unknown
			esm.writeT<unsigned char>(0); // X
			esm.writeT<unsigned char>(0); // Y
			esm.writeT<unsigned char>(0); // Z
			esm.endSubRecordTES4("VHGT");
		}
		
		// VCLR 33x33 grid of 24-bit Vertex Color
		landData = getLandData(Land::DATA_VCLR);
		if (landData != 0)
		{
			esm.startSubRecordTES4("VCLR");
			for (int u=0; u < 33; u++)
			{
				for (int v=0; v < 33; v++)
				{
					// convert 65x64 to 33x33
					i = u + (32 * offsetX);
					j = v + (32 * offsetY);
					esm.writeT<unsigned char>(landData->mColours[(i*65)+j]); // R
					esm.writeT<unsigned char>(landData->mColours[(i*65)+j+1]); // G
					esm.writeT<unsigned char>(landData->mColours[(i*65)+j+2]); // B
				}
			}
			esm.endSubRecordTES4("VCLR");
		}

/*
		// BTXT, Base Texture (up to 4 formID+quadrant pairs)
		esm.startSubRecordTES4("BTXT");
		esm.writeT<uint32_t>(0); // formID
		esm.writeT<unsigned char>(0); // quadrant
		esm.writeT<unsigned char>(0); // unk?
		esm.writeT<unsigned char>(0); // unk?
		esm.writeT<unsigned char>(0); // unk?
		esm.endSubRecordTES4("BTXT");
		
		// ATXT, Additional Texture
		esm.startSubRecordTES4("ATXT");
		esm.writeT<uint32_t>(0); // formID
		esm.writeT<unsigned char>(0); // quadrant
		esm.writeT<unsigned char>(0); // unk
		esm.writeT<uint16_t>(0); // Layer
		esm.endSubRecordTES4("ATXT");
		
		// VTXT, Additional Texture Opacity
		esm.startSubRecordTES4("VTXT");
		esm.writeT<uint16_t>(0); // Position
		esm.writeT<unsigned char>(0); // quadrant
		esm.writeT<unsigned char>(0); // unk
		esm.writeT<float>(0); // 32bit-float Opacity
		esm.endSubRecordTES4("VTXT");
*/
		
		// VTEX, LTEX formIDs
		landData = getLandData(Land::DATA_VTEX);
		if (landData != 0)
		{
			esm.startSubRecordTES4("VTEX");
			// multiple formIDs
//			esm.writeT<uint32_t>(0); // formID
			// ....
			esm.endSubRecordTES4("VTEX");
		}
	
	}

	void Land::exportTESx(ESMWriter &esm, int export_type) const
	{
		int x, y;

		for (x=0; x < 2; x++)
		{
			for (y=0; y < 2; y++)
			{
				exportSubCellTES4(esm, x, y);
			}
		}
	}
	
    void Land::loadData(int flags, LandData* target) const
    {
        // Create storage if nothing is loaded
        if (!target && !mLandData)
        {
            mLandData = new LandData;
        }

        if (!target)
            target = mLandData;

        // Try to load only available data
        flags = flags & mDataTypes;
        // Return if all required data is loaded
        if ((target->mDataLoaded & flags) == flags) {
            return;
        }

        ESM::ESMReader reader;
        reader.restoreContext(mContext);

        if (reader.isNextSub("VNML")) {
            condLoad(reader, flags, target->mDataLoaded, DATA_VNML, target->mNormals, sizeof(target->mNormals));
        }

        if (reader.isNextSub("VHGT")) {
            VHGT vhgt;
            if (condLoad(reader, flags, target->mDataLoaded, DATA_VHGT, &vhgt, sizeof(vhgt))) {
                target->mMinHeight = FLT_MAX;
                target->mMaxHeight = -FLT_MAX;
                float rowOffset = vhgt.mHeightOffset;
                for (int y = 0; y < LAND_SIZE; y++) {
                    rowOffset += vhgt.mHeightData[y * LAND_SIZE];

                    target->mHeights[y * LAND_SIZE] = rowOffset * HEIGHT_SCALE;
                    if (rowOffset * HEIGHT_SCALE > target->mMaxHeight)
                        target->mMaxHeight = rowOffset * HEIGHT_SCALE;
                    if (rowOffset * HEIGHT_SCALE < target->mMinHeight)
                        target->mMinHeight = rowOffset * HEIGHT_SCALE;

                    float colOffset = rowOffset;
                    for (int x = 1; x < LAND_SIZE; x++) {
                        colOffset += vhgt.mHeightData[y * LAND_SIZE + x];
                        target->mHeights[x + y * LAND_SIZE] = colOffset * HEIGHT_SCALE;

                        if (colOffset * HEIGHT_SCALE > target->mMaxHeight)
                            target->mMaxHeight = colOffset * HEIGHT_SCALE;
                        if (colOffset * HEIGHT_SCALE < target->mMinHeight)
                            target->mMinHeight = colOffset * HEIGHT_SCALE;
                    }
                }
                target->mUnk1 = vhgt.mUnk1;
                target->mUnk2 = vhgt.mUnk2;
            }
        }

        if (reader.isNextSub("WNAM"))
            reader.skipHSub();

        if (reader.isNextSub("VCLR"))
            condLoad(reader, flags, target->mDataLoaded, DATA_VCLR, target->mColours, 3 * LAND_NUM_VERTS);
        if (reader.isNextSub("VTEX")) {
            uint16_t vtex[LAND_NUM_TEXTURES];
            if (condLoad(reader, flags, target->mDataLoaded, DATA_VTEX, vtex, sizeof(vtex))) {
                transposeTextureData(vtex, target->mTextures);
            }
        }
    }

    void Land::unloadData() const
    {
        if (mLandData)
        {
            delete mLandData;
            mLandData = NULL;
        }
    }

    bool Land::condLoad(ESM::ESMReader& reader, int flags, int& targetFlags, int dataFlag, void *ptr, unsigned int size) const
    {
        if ((targetFlags & dataFlag) == 0 && (flags & dataFlag) != 0) {
            reader.getHExact(ptr, size);
            targetFlags |= dataFlag;
            return true;
        }
        reader.skipHSubSize(size);
        return false;
    }

    bool Land::isDataLoaded(int flags) const
    {
        return mLandData && (mLandData->mDataLoaded & flags) == (flags & mDataTypes);
    }

    Land::Land (const Land& land)
    : mFlags (land.mFlags), mX (land.mX), mY (land.mY), mPlugin (land.mPlugin),
      mContext (land.mContext), mDataTypes (land.mDataTypes),
      mLandData (land.mLandData ? new LandData (*land.mLandData) : 0)
    {}

    Land& Land::operator= (Land land)
    {
        swap (land);
        return *this;
    }

    void Land::swap (Land& land)
    {
        std::swap (mFlags, land.mFlags);
        std::swap (mX, land.mX);
        std::swap (mY, land.mY);
        std::swap (mPlugin, land.mPlugin);
        std::swap (mContext, land.mContext);
        std::swap (mDataTypes, land.mDataTypes);
        std::swap (mLandData, land.mLandData);
    }

    const Land::LandData *Land::getLandData (int flags) const
    {
        if (!(flags & mDataTypes))
            return 0;

        loadData (flags);
        return mLandData;
    }

    const Land::LandData *Land::getLandData() const
    {
        return mLandData;
    }

    Land::LandData *Land::getLandData()
    {
        return mLandData;
    }

    void Land::add (int flags)
    {
        if (!mLandData)
            mLandData = new LandData;

        mDataTypes |= flags;
        mLandData->mDataLoaded |= flags;
    }

    void Land::remove (int flags)
    {
        mDataTypes &= ~flags;

        if (mLandData)
        {
            mLandData->mDataLoaded &= ~flags;

            if (!mLandData->mDataLoaded)
            {
                delete mLandData;
                mLandData = 0;
            }
        }
    }
}
