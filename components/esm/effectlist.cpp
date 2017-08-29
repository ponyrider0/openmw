#include "effectlist.hpp"

#include "loadskil.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM {

void EffectList::load(ESMReader &esm)
{
    mList.clear();
    while (esm.isNextSub("ENAM")) {
        add(esm);
    }
}

void EffectList::add(ESMReader &esm)
{
    ENAMstruct s;
    esm.getHT(s, 24);
    mList.push_back(s);
}

void EffectList::save(ESMWriter &esm) const
{
    for (std::vector<ENAMstruct>::const_iterator it = mList.begin(); it != mList.end(); ++it) {
        esm.writeHNT<ENAMstruct>("ENAM", *it, 24);
    }
}

void ENAMstruct::exportTES4EFID(ESMWriter &esm) const
{
/*
	std::string effSIG;

	switch (mEffectID)
	{
	case 85: // Absorb Attribute
		effSIG = "ABAT";
		break;
	case 88: // Absorb Fatigue
		effSIG = "ABFA";
		break;
	case 86: // Absorb Health
		effSIG = "ABHE";
		break;
	case 87: // Absorb Magicka
		effSIG = "ABSP";
		break;
	case 89: // Absorb Skill
		effSIG = "????";
		break;
	case 63: // Almsivi Intervention
		effSIG = "????";
		break;
	case 47: // Blind
		effSIG = "????";
		break;
	case 123: // Bound Battle Axe
		effSIG = "BWAX";
		break;
	case 129: // Bound Boots
		effSIG = "BABO";
		break;
	case 127: // Bound Cuirass
		effSIG = "BACU";
		break;
	case 120: // Bound Dagger
		effSIG = "BWDA";
		break;
	case 131: // Bound Gloves
		effSIG = "BAGA";
		break;
	case 128: // Bound Helm
		effSIG = "BAHE";
		break;
	case 125: // Bound Longbow
		effSIG = "BWBO";
		break;
	case 121: // Bound Longsword
		effSIG = "BWSW";
		break;
	case 122: // Bound Mace
		effSIG = "BWMA";
		break;
	case 130: // Bound Shield
		effSIG = "BASH";
		break;
	case 124: // Bound Spear
		effSIG = "????";
		break;
	case 7: // Burden
		effSIG = "BRDN";
		break;
	case 50: // Calm Creature
		effSIG = "CALM"; // CALM
		break;
	case 49: // Calm Humanoid
		effSIG = "CALM"; // CALM
		break;
	case 40: // Chameleon
		effSIG = "CHML";
		break;
	case 44: // Charm
		effSIG = "CHRM";
		break;
	case 118: // Command Creature
		effSIG = "COCR";
		break;
	case 119: // Command Humanoid
		effSIG = "COHU";
		break;
	case 132: // Corprus
		effSIG = "????";
		break;
	case 70: // Cure Blight
		effSIG = "????";
		break;
	case 69: // Cure Common Disease
		effSIG = "CUDI";
		break;
	case 71: // Cure Corprus Disease
		effSIG = "????";
		break;
	case 73: // Cure Paralyzation
		effSIG = "CUPA";
		break;
	case 72: // Cure Posion
		effSIG = "CUPO";
		break;
	case 22: // Damage Attribute
		effSIG = "DGAT";
		break;
	case 25: // Damage Fatigue
		effSIG = "DGFA";
		break;
	case 23: // Damage Health
		effSIG = "DGHE";
		break;
	case 24: // Damage Magicka
		effSIG = "DGSP";
		break;
	case 26: // Damage Skill
		effSIG = "????";
		break;
	case 54: // Demoralize Creature
		effSIG = "????";
		break;
	case 53: // Demoralize Humanoid
		effSIG = "????";
		break;
	case 64: // Detect Animal
		effSIG = "DTCT"; // detect life
		break;
	case 65: // Detect Enchantment
		effSIG = "????";
		break;
	case 66: // Detect Key
		effSIG = "????";
		break;
	case 38: // Disintegrate Armor
		effSIG = "DIAR";
		break;
	case 37: // Disintegrate Weapon
		effSIG = "DIWE";
		break;
	case 57: // Dispel
		effSIG = "DSPL";
		break;
	case 62: // Divine Intervention
		effSIG = "????";
		break;
	case 17: // Drain Attribute
		effSIG = "DRAT";
		break;
	case 20: // Drain Fatigue
		effSIG = "DRFA";
		break;
	case 18: // Drain Health
		effSIG = "DRHE";
		break;
	case 19: // Drain Magicka
		effSIG = "DRSP";
		break;
	case 21: // Drain Skill
		effSIG = "DRSK";
		break;
	case 8: // Feather
		effSIG = "FTHR";
		break;
	case 14: // Fire Damage
		effSIG = "FIDG";
		break;
	case 4: // Fire Shield
		effSIG = "FISH";
		break;
	case 117: // Fortify Attack
		effSIG = "????";
		break;
	case 79: // Fortify Attribute
		effSIG = "FOAT";
		break;
	case 82: // Fortify Fatigue
		effSIG = "FOFA";
		break;
	case 80: // Fortify Health
		effSIG = "FOHE";
		break;
	case 81: // Fortify Magicka
		effSIG = "FOMM"; // fortify magicka multiplier
		break;
	case 84: // Fortify Maximum Magicka
		effSIG = "????";
		break;
	case 83: // Fortify Skill
		effSIG = "FOSK";
		break;
	case 52: // Frenzy Creature
		effSIG = "FRNZ"; // FRENZY
		break;
	case 51: // Frenzy Humanoid
		effSIG = "FRNZ"; // FRENZY
		break;
	case 16: // Frost Damage
		effSIG = "FRDG";
		break;
	case 6: // Frost Shield
		effSIG = "FRSH";
		break;
	case 39: // Invisibility
		effSIG = "INVI";
		break;
	case 9: // Jump
		effSIG = "????";
		break;
	case 10: // Levitate
		effSIG = "????";
		break;
	case 41: // Light
		effSIG = "LGHT";
		break;
	case 5: // Lightning Shield
		effSIG = "LISH";
		break;
	case 12: // Lock
		effSIG = "????"; // DO NOT USE? LOCK
		break;
	case 60: // Mark
		effSIG = "????";
		break;
	case 43: // Night Eye
		effSIG = "NEYE";
		break;
	case 13: // Open
		effSIG = "OPEN";
		break;
	case 45: // Paralyze
		effSIG = "PARA";
		break;
	case 27: // Poison
		effSIG = "????";
		break;
	case 56: // Rally Creature
		effSIG = "RALY"; // RALLY
		break;
	case 55: // Rally Humanoid
		effSIG = "RALY"; // RALLY
		break;
	case 61: // Recall
		effSIG = "????";
		break;
	case 68: // Reflect
		effSIG = "REDG"; // reflect damage
		break;
	case 100: // Remove Curse
		effSIG = "????";
		break;
	case 95: // Resist Blight
		effSIG = "????";
		break;
	case 94: // Resist Common Disease
		effSIG = "RSDI";
		break;
	case 96: // Resist Corprus
		effSIG = "????";
		break;
	case 90: // Resist Fire
		effSIG = "RSFI";
		break;
	case 91: // Resist Frost
		effSIG = "RSFR";
		break;
	case 93: // Resist Magicka
		effSIG = "RSMA"; // resist magic
		break;
	case 98: // Resist Normal Weapons
		effSIG = "RSNW";
		break;
	case 99: // Resist Paralysis
		effSIG = "RSPA";
		break;
	case 97: // Resist Poison
		effSIG = "RSPO";
		break;
	case 92: // Resist Shock
		effSIG = "RSSH";
		break;
	case 74: // Restore Attribute
		effSIG = "REAT";
		break;
	case 77: // Restore Fatigue
		effSIG = "REFA";
		break;
	case 75: // Restore Health
		effSIG = "REHE";
		break;
	case 76: // Restore Magicka
		effSIG = "RESP";
		break;
	case 78: // Restore Skill
		effSIG = "????";
		break;
	case 42: // Sanctuary
		effSIG = "????";
		break;
	case 3: // Shield
		effSIG = "SHLD";
		break;
	case 15: // Shock Damage
		effSIG = "SHDG";
		break;
	case 46: // Silence
		effSIG = "SLNC";
		break;
	case 11: // SlowFall
		effSIG = "????";
		break;
	case 58: // Soultrap
		effSIG = "STRP";
		break;
	case 48: // Sound
		effSIG = "????";
		break;
	case 67: // Spell Absorption
		effSIG = "SABS";
		break;
	case 136: // Stunted Magicka
		effSIG = "STMA";
		break;
	case 106: // Summon Ancestral Ghost
		effSIG = "ZGHO"; // SUMMON GHOST VS SUMMON AG
		break;
	case 110: // Summon Bonelord
		effSIG = "ZLIC"; // LICHE
		break;
	case 108: // Summon Bonewalker
		effSIG = "ZZOM"; // ZOMBIE
		break;
	case 134: // Summon Centurion Sphere
		effSIG = "????";
		break;
	case 103: // Summon Clannfear
		effSIG = "ZCLA";
		break;
	case 104: // Summon Daedroth
		effSIG = "ZDAE";
		break;
	case 105: // Summon Dremora
		effSIG = "ZDRE";
		break;
	case 114: // Summon Flame Atronach
		effSIG = "ZFIA";
		break;
	case 115: // Summon Frost Atronach
		effSIG = "ZFRA";
		break;
	case 113: // Summon Golden Saint
		effSIG = "Z010";
		break;
	case 109: // Summon Greater Bonewalker
		effSIG = "ZZLIC"; // LICHE
		break;
	case 112: // Summon Hunger
		effSIG = "Z007";
		break;
	case 102: // Summon Scamp
		effSIG = "ZSCA";
		break;
	case 107: // Summon Skeletal Minion
		effSIG = "ZSKA"; // SKELETAL GUARDIAN
		break;
	case 116: // Sommon Storm Atronach
		effSIG = "ZSTA";
		break;
	case 111: // Summon Winged Twilight
		effSIG = "????";
		break;
	case 135: // Sun Damage
		effSIG = "SUDG";
		break;
	case 1: // Swift Swim
		effSIG = "????";
		break;
	case 59: // Telekinesis
		effSIG = "TELE";
		break;
	case 101: // Turn Undead
		effSIG = "TURN";
		break;
	case 133: // Vampirism
		effSIG = "VAMP";
		break;
	case 0: // Water Breathing
		effSIG = "WABR";
		break;
	case 2: // Water Walking
		effSIG = "WAWA";
		break;
	case 33: // Weakness to Blight
		effSIG = "????";
		break;
	case 32: // Weakness to Common Disease
		effSIG = "WKDI";
		break;
	case 34: // Weakness to Corprus
		effSIG = "????";
		break;
	case 28: // Weakness to Fire
		effSIG = "WKFI";
		break;
	case 29: // Weakness to Frost
		effSIG = "WKFR";
		break;
	case 31: // Weakness to Magicka
		effSIG = "WKMA";
		break;
	case 36: // Weakness to Normal Weapons
		effSIG = "WKNW";
		break;
	case 35: // Weakness to Poison
		effSIG = "WKPO";
		break;
	case 30: // Weakness to Shock
		effSIG = "WKSH";
		break;
	}
*/
	std::string effSIG = esm.intToMagEffIDTES4(mEffectID);
	esm.writeName(effSIG);
}

void ENAMstruct::exportTES4EFIT(ESMWriter &esm) const
{
	// write char[4] magic effect
	exportTES4EFID(esm);
	// magnitude (uint32)
	esm.writeT<uint32_t>((mMagnMin+mMagnMax)/2);
	// area (uint32)
	esm.writeT<uint32_t>(mArea);
	// duration (uint32)
	esm.writeT<uint32_t>(mDuration);
	// type (uint32) -- enum{self, touch, target}
	esm.writeT<uint32_t>(mRange);
	// actor value (int32)
	int32_t actorVal=-1;
	if (mSkill != -1)
	{
		actorVal = esm.skillToActorValTES4(mSkill);
	}
	else if (mAttribute != -1)
	{
		actorVal = esm.attributeToActorValTES4(mAttribute);
	}
	esm.writeT<int32_t>(actorVal);

}

} // end namespace
