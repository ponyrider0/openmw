#ifndef OPENMW_ESM_ENCH_H
#define OPENMW_ESM_ENCH_H

#include <string>

#include "effectlist.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Enchantments
 */

struct Enchantment
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Enchantment"; }

    enum Type
    {
        CastOnce = 0,
        WhenStrikes = 1,
        WhenUsed = 2,
        ConstantEffect = 3
    };

    struct ENDTstruct
    {
        int mType;
        int mCost;
        int mCharge;
        int mAutocalc; // Guessing this is 1 if we are supposed to auto
        // calculate
    };

    std::string mId;
    ENDTstruct mData;
    EffectList mEffects;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;
	void exportTESx(ESMWriter &esm, int export_type = 4) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
