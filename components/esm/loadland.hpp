#ifndef OPENMW_ESM_LAND_H
#define OPENMW_ESM_LAND_H

#include <stdint.h>

#include "esmcommon.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Landscape data.
 */

struct Land
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Land"; }

    Land();
    ~Land();

    int mFlags; // Only first four bits seem to be used, don't know what
    // they mean.
    int mX, mY; // Map coordinates.
    int mPlugin; // Plugin index, used to reference the correct material palette.

    // File context. This allows the ESM reader to be 'reset' to this
    // location later when we are ready to load the full data set.
    ESM_Context mContext;

    int mDataTypes;

    enum
    {
        DATA_VNML = 1,
        DATA_VHGT = 2,
        DATA_WNAM = 4,
        DATA_VCLR = 8,
        DATA_VTEX = 16
    };

    // default height to use in case there is no Land record
    static const int DEFAULT_HEIGHT = -2048;

    // number of vertices per side
    static const int LAND_SIZE = 65;

    // cell terrain size in world coords
    static const int REAL_SIZE = 8192;

    // total number of vertices
    static const int LAND_NUM_VERTS = LAND_SIZE * LAND_SIZE;

    static const int HEIGHT_SCALE = 8;

    //number of textures per side of land
    static const int LAND_TEXTURE_SIZE = 16;

    //total number of textures per land
    static const int LAND_NUM_TEXTURES = LAND_TEXTURE_SIZE * LAND_TEXTURE_SIZE;

#pragma pack(push,1)
    struct VHGT
    {
        float mHeightOffset;
        int8_t mHeightData[LAND_NUM_VERTS];
        short mUnk1;
        char mUnk2;
    };
#pragma pack(pop)

    typedef signed char VNML;

    struct LandData
    {
        LandData()
            : mDataLoaded(0)
        {
        }

        // Initial reference height for the first vertex, only needed for filling mHeights
        float mHeightOffset;
        // Height in world space for each vertex
        float mHeights[LAND_NUM_VERTS];
        float mMinHeight;
        float mMaxHeight;

        // 24-bit normals, these aren't always correct though. Edge and corner normals may be garbage.
        VNML mNormals[LAND_NUM_VERTS * 3];

        // 2D array of texture indices. An index can be used to look up an ESM::LandTexture,
        // but to do so you must subtract 1 from the index first!
        // An index of 0 indicates the default texture.
        uint16_t mTextures[LAND_NUM_TEXTURES];

        // 24-bit RGB color for each vertex
        unsigned char mColours[3 * LAND_NUM_VERTS];

        // ???
        short mUnk1;
        uint8_t mUnk2;

        int mDataLoaded;
    };

    // low-LOD heightmap (used for rendering the global map)
    signed char mWnam[81];

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;
	void exportTESx(ESMWriter &esm, int export_type) const;
	void exportSubCellTES4(ESMWriter &esm, int offsetX, int offsetY) const;

    void blank() {}

    /**
     * Actually loads data into target
     * If target is NULL, assumed target is mLandData
     */
    void loadData(int flags, LandData* target = NULL) const;

    /**
     * Frees memory allocated for mLandData
     */
    void unloadData() const;

    /// Check if given data type is loaded
    /// @note We only check data types that *can* be loaded (present in mDataTypes)
    bool isDataLoaded(int flags) const;

        Land (const Land& land);

        Land& operator= (Land land);

        void swap (Land& land);

        /// Return land data with at least the data types specified in \a flags loaded (if they
        /// are available). Will return a 0-pointer if there is no data for any of the
        /// specified types.
        const LandData *getLandData (int flags) const;

        /// Return land data without loading first anything. Can return a 0-pointer.
        const LandData *getLandData() const;

        /// Return land data without loading first anything. Can return a 0-pointer.
        LandData *getLandData();

        /// \attention Must not be called on objects that aren't fully loaded.
        ///
        /// \note Added data fields will be uninitialised
        void add (int flags);

        /// \attention Must not be called on objects that aren't fully loaded.
        void remove (int flags);

    private:

        /// Loads data and marks it as loaded
        /// \return true if data is actually loaded from file, false otherwise
        /// including the case when data is already loaded
        bool condLoad(ESM::ESMReader& reader, int flags, int& targetFlags, int dataFlag, void *ptr, unsigned int size) const;

        mutable LandData *mLandData;
};

}
#endif
