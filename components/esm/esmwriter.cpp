#include "esmwriter.hpp"

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#include <sstream>
#include <cassert>
#include <fstream>
#include <stdexcept>

#include <components/misc/stringops.hpp>
#include <components/to_utf8/to_utf8.hpp>

namespace ESM
{
    ESMWriter::ESMWriter()
        : mRecords()
        , mStream(NULL)
        , mHeaderPos()
        , mEncoder(NULL)
        , mRecordCount(0)
        , mCounting(true)
        , mHeader()
    {}

    unsigned int ESMWriter::getVersion() const
    {
        return mHeader.mData.version;
    }

    void ESMWriter::setVersion(unsigned int ver)
    {
        mHeader.mData.version = ver;
    }

    void ESMWriter::setType(int type)
    {
        mHeader.mData.type = type;
    }

    void ESMWriter::setAuthor(const std::string& auth)
    {
        mHeader.mData.author.assign (auth);
    }

    void ESMWriter::setDescription(const std::string& desc)
    {
        mHeader.mData.desc.assign (desc);
    }

    void ESMWriter::setRecordCount (int count)
    {
        mHeader.mData.records = count;
    }

    void ESMWriter::setFormat (int format)
    {
        mHeader.mFormat = format;
    }

    void ESMWriter::clearMaster()
    {
        mHeader.mMaster.clear();
		mESMoffset = 0;
    }

    void ESMWriter::addMaster(const std::string& name, uint64_t size)
    {
        Header::MasterData d;
        d.name = name;
        d.size = size;
        mHeader.mMaster.push_back(d);
		mESMoffset += 0x01000000;
    }

    void ESMWriter::save(std::ostream& file)
    {
        mRecordCount = 0;
        mRecords.clear();
        mCounting = true;
        mStream = &file;

        startRecord("TES3", 0);

        mHeader.save (*this);

        endRecord("TES3");
    }

	void ESMWriter::exportTES4(std::ostream& file)
	{
		HeaderTES4 headerTES4;

		mRecordCount = 0;
		mRecords.clear();
		mCounting = true;
		mStream = &file;

		uint32_t flags=0;
		startRecordTES4("TES4", flags, 0);

		mHeader.exportTES4 (*this);

		endRecordTES4("TES4");
	}

	void ESMWriter::updateTES4()
	{
		std::streampos currentPos = mStream->tellp();

		mStream->seekp(std::ios_base::end);

		if ( mStream->tellp() >= (34+4) )
		{
			mStream->seekp(34, std::ios_base::beg);
			mCounting = false;
			writeT<uint32_t>(getNextAvailableFormID());
			mCounting = true;
		}

		mStream->seekp(currentPos);
	}

    void ESMWriter::close()
    {
        if (!mRecords.empty())
            throw std::runtime_error ("Unclosed record remaining");
    }

    void ESMWriter::startRecord(const std::string& name, uint32_t flags)
    {
        mRecordCount++;

        writeName(name);
        RecordData rec;
        rec.name = name;
        rec.position = mStream->tellp();
        rec.size = 0;
        writeT<uint32_t>(0); // Size goes here
        writeT<uint32_t>(0); // Unused header?
        writeT(flags);
        mRecords.push_back(rec);

        assert(mRecords.back().size == 0);
    }

    void ESMWriter::startRecord (uint32_t name, uint32_t flags)
    {
        std::string type;
        for (int i=0; i<4; ++i)
            /// \todo make endianess agnostic
            type += reinterpret_cast<const char *> (&name)[i];

        startRecord (type, flags);
    }

	void ESMWriter::startRecordTES4(const std::string& name, uint32_t flags, uint32_t formID, const std::string& stringID)
	{
		std::stringstream debugstream;
		uint32_t activeID = formID;
		mRecordCount++;

		writeName(name);
		RecordData rec;
		rec.name = name;
		rec.position = mStream->tellp();
		rec.size = 0;

		writeT<uint32_t>(0); // Size goes here (must convert 32bit to 64bit)
		writeT<uint32_t>(flags);

		if (name == "TES4")
			activeID = 0;
		else if (name != "TES4" && activeID == 0)
		{
			// auto-assign formID
			activeID = getNextAvailableFormID();
			activeID = reserveFormID(activeID, stringID);
		}
		if (mUniqueIDcheck.find(activeID) != mUniqueIDcheck.end())
		{
//			throw std::runtime_error("ESMWRITER ERROR: non-unique FormID was written to ESM.");
			debugstream << "ESMWRITER ERROR: non-unique FormID was written to ESM." << std::endl;
			OutputDebugString(debugstream.str().c_str());
		}
		writeT<uint32_t>(activeID);
		mUniqueIDcheck.insert( std::make_pair(activeID, mUniqueIDcheck.size()) );

		writeT<uint32_t>(0); // version control

		mRecords.push_back(rec);
		assert(mRecords.back().size == 0);
	}

	void ESMWriter::startRecordTES4 (uint32_t name, uint32_t flags, uint32_t formID, const std::string& stringID)
	{
		std::string type;
		for (int i=0; i<4; ++i)
			/// \todo make endianess agnostic
			type += reinterpret_cast<const char *> (&name)[i];
		type[4]='\0';
		startRecordTES4 (type, flags, formID, stringID);
	}

	void ESMWriter::startGroupTES4(const std::string& label, uint32_t groupType)
	{
//		mRecordCount++;

		writeFixedSizeString("GRUP", 4);
		RecordData rec;
		rec.name = "GRUP";
		rec.position = mStream->tellp();
		rec.size = 20;

		writeT<uint32_t>(0); // Size goes here 
		writeFixedSizeString(label, 4); // Group Label, aka record SIG for topgroup
		writeT<uint32_t>(groupType); // group type
		writeT<uint32_t>(0); // date stamp

		mRecords.push_back(rec);
		assert(mRecords.back().size == 20);
	}

	void ESMWriter::startGroupTES4(const uint32_t label, uint32_t groupType)
	{
		//		mRecordCount++;

		writeFixedSizeString("GRUP", 4);
		RecordData rec;
		rec.name = "GRUP";
		rec.position = mStream->tellp();
		rec.size = 20;

		writeT<uint32_t>(0); // Size goes here 
		writeT<uint32_t>(label); // Label, actual data depends on GroupType
		writeT<uint32_t>(groupType); // group type
		writeT<uint32_t>(0); // date stamp

		mRecords.push_back(rec);
		assert(mRecords.back().size == 20);
	}

    void ESMWriter::startSubRecord(const std::string& name)
    {
        // Sub-record hierarchies are not properly supported in ESMReader. This should be fixed later.
        assert (mRecords.size() <= 1);

        writeName(name);
        RecordData rec;
        rec.name = name;
        rec.position = mStream->tellp();
        rec.size = 0;
        writeT<uint32_t>(0); // Size goes here
        mRecords.push_back(rec);

        assert(mRecords.back().size == 0);
    }

	void ESMWriter::startSubRecordTES4(const std::string& name)
	{
		writeName(name);
		RecordData rec;
		rec.name = name;
		rec.position = mStream->tellp();
		rec.size = 0;
		writeT<uint16_t>(0); // Size goes here
		mRecords.push_back(rec);

		assert(mRecords.back().size == 0);
	}

    void ESMWriter::endRecord(const std::string& name)
    {
        RecordData rec = mRecords.back();
        assert(rec.name == name);
        mRecords.pop_back();

        mStream->seekp(rec.position);

        mCounting = false;
        write (reinterpret_cast<const char*> (&rec.size), sizeof(uint32_t));
        mCounting = true;

        mStream->seekp(0, std::ios::end);

    }

	void ESMWriter::endRecordTES4(const std::string& name)
	{
		RecordData rec = mRecords.back();
		assert(rec.name == name);
		mRecords.pop_back();

		mStream->seekp(rec.position);

		mCounting = false;
		write (reinterpret_cast<const char*> (&rec.size), sizeof(uint32_t)); // rec.size is only 32bit
		mCounting = true;

		mStream->seekp(0, std::ios::end);

	}

	void ESMWriter::endRecordTES4 (uint32_t name)
	{
		std::string type;
		for (int i=0; i<4; ++i)
			/// \todo make endianess agnostic
			type += reinterpret_cast<const char *> (&name)[i];

		endRecordTES4 (type);
	}


	void ESMWriter::endGroupTES4(const std::string& name)
	{
		RecordData rec = mRecords.back();
		assert(rec.name == "GRUP");
		mRecords.pop_back();

		mStream->seekp(rec.position);

		mCounting = false;
		write (reinterpret_cast<const char*> (&rec.size), sizeof(uint32_t)); 
		mCounting = true;

		mStream->seekp(0, std::ios::end);

	}

	void ESMWriter::endGroupTES4(const uint32_t name)
	{
		std::string sSIG;
		for (int i=0; i<4; ++i)
			/// \todo make endianess agnostic
			sSIG += reinterpret_cast<const char *> (&name)[i];

		endGroupTES4 (sSIG);
	}

	void ESMWriter::endSubRecordTES4(const std::string& name)
	{
		RecordData rec = mRecords.back();
		assert(rec.name == name);
		mRecords.pop_back();

		mStream->seekp(rec.position);

		mCounting = false;
		write (reinterpret_cast<const char*> (&rec.size), sizeof(uint16_t));
		mCounting = true;

		mStream->seekp(0, std::ios::end);

	}

	void ESMWriter::endRecord (uint32_t name)
    {
        std::string type;
        for (int i=0; i<4; ++i)
            /// \todo make endianess agnostic
            type += reinterpret_cast<const char *> (&name)[i];

        endRecord (type);
    }

    void ESMWriter::writeHNString(const std::string& name, const std::string& data)
    {
        startSubRecord(name);
        writeHString(data);
        endRecord(name);
    }

    void ESMWriter::writeHNString(const std::string& name, const std::string& data, size_t size)
    {
        assert(data.size() <= size);
        startSubRecord(name);
        writeHString(data);

        if (data.size() < size)
        {
            for (size_t i = data.size(); i < size; ++i)
                write("\0",1);
        }

        endRecord(name);
    }

    void ESMWriter::writeFixedSizeString(const std::string &data, int size)
    {
        std::string string;
        if (!data.empty())
            string = mEncoder ? mEncoder->getLegacyEnc(data) : data;
        string.resize(size);
        write(string.c_str(), string.size());
    }

    void ESMWriter::writeHString(const std::string& data)
    {
        if (data.size() == 0)
            write("\0", 1);
        else
        {
            // Convert to UTF8 and return
            std::string string = mEncoder ? mEncoder->getLegacyEnc(data) : data;

            write(string.c_str(), string.size());
        }
    }

    void ESMWriter::writeHCString(const std::string& data)
    {
        writeHString(data);
        if (data.size() > 0 && data[data.size()-1] != '\0')
            write("\0", 1);
    }

    void ESMWriter::writeName(const std::string& name)
    {
        assert((name.size() == 4 && name[3] != '\0'));
        write(name.c_str(), name.size());
    }

    void ESMWriter::write(const char* data, size_t size)
    {
        if (mCounting && !mRecords.empty())
        {
            for (std::list<RecordData>::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
                it->size += size;
        }

        mStream->write(data, size);
    }

    void ESMWriter::setEncoder(ToUTF8::Utf8Encoder* encoder)
    {
        mEncoder = encoder;
    }

	uint32_t ESMWriter::getNextAvailableFormID()
	{
		uint32_t returnVal;
//		if (mReservedFormIDs.size() == 0)
		if (mFormIDMap.size() == 0 || mLastReservedFormID == 0)
		{
			returnVal = 0x10001;
			return returnVal | mESMoffset;
		}

//		returnVal = mReservedFormIDs.back().first;

		returnVal = mLastReservedFormID + 1;
		while ( mFormIDMap.find(returnVal) != mFormIDMap.end() )
		{
			returnVal++;
		}

		// strip existing plugin offset and assign current offset
		returnVal = (returnVal & 0x00FFFFFF) | mESMoffset;

		return returnVal;
	}

	uint32_t ESMWriter::getLastReservedFormID()
	{
		uint32_t returnVal;
		returnVal = mLastReservedFormID;
		return returnVal;
	}

	uint32_t ESMWriter::reserveFormID(uint32_t paramformID, const std::string& stringID, bool setup_phase)
	{
		std::stringstream debugstream;
		uint32_t formID = paramformID;
//		if (addESMOffset == true)
//			formID |= mESMoffset;

		if (formID == 0x01380001 && stringID != "wrldmorrowind-dummycell")
		{
			formID = getNextAvailableFormID();
		}

/*
		std::vector<std::pair<uint32_t, std::string>>::iterator insertionPoint;
		std::pair<uint32_t, std::string> recordID(formID, std::string(stringID));
		std::pair<std::string, uint32_t> stringMap(std::string(stringID), formID);

		// check case for a pre-calc'ed formID
		if (mReservedFormIDs.size() > 0)
			if (mReservedFormIDs.back().first < formID)
			{
				mStringIDMap.insert(stringMap);
				mReservedFormIDs.push_back(recordID);
				mLastReservedFormID = formID;
				return mLastReservedFormID;
			}

		// check special cases for size 0, 1 and 2
		if (mReservedFormIDs.size() == 0)
		{
			uint32_t newID = 0x10001 | mESMoffset;
			mStringIDMap.insert(std::make_pair(std::string(stringID), newID));
			mReservedFormIDs.push_back(std::make_pair(newID, std::string(stringID)));
			mLastReservedFormID = newID;
			return mLastReservedFormID;
		}
		else if (mReservedFormIDs.size() == 1)
		{
			insertionPoint = mReservedFormIDs.begin();
			if (insertionPoint->first == formID)
			{
				uint32_t newID = (mReservedFormIDs.back().first+1) | mESMoffset;
				mStringIDMap.insert(std::make_pair(std::string(stringID), newID));
				mReservedFormIDs.push_back(std::make_pair(newID, std::string(stringID)));
				mLastReservedFormID = newID;
				return mLastReservedFormID;
			}
			else if (insertionPoint->first > formID)
				mReservedFormIDs.insert(insertionPoint, recordID);
			else
				mReservedFormIDs.push_back(recordID);
			mStringIDMap.insert(stringMap);
			mLastReservedFormID = formID;
			return mLastReservedFormID;
		}
		else if (mReservedFormIDs.size() == 2)
		{
			insertionPoint = mReservedFormIDs.begin();
			if (insertionPoint->first == formID)
			{
				uint32_t newID = (mReservedFormIDs.back().first+1) | mESMoffset;
				mStringIDMap.insert(std::make_pair(std::string(stringID), newID));
				mReservedFormIDs.push_back(std::make_pair(newID, std::string(stringID)));
				mLastReservedFormID = newID;
				return mLastReservedFormID;
			}
			else if (insertionPoint->first > formID)
				mReservedFormIDs.insert(insertionPoint, recordID);
			else if ( (++insertionPoint)->first > formID )
				mReservedFormIDs.insert(insertionPoint, recordID);
			else
				mReservedFormIDs.push_back(recordID);
			mStringIDMap.insert(stringMap);
			mLastReservedFormID = formID;
			return mLastReservedFormID;
		}

		int currentIndex, midPoint = mReservedFormIDs.size() / 2;
		if (mReservedFormIDs[midPoint].first == formID)
		{
			uint32_t newID = (mReservedFormIDs.back().first+1) | mESMoffset;
			mStringIDMap.insert(std::make_pair(std::string(stringID), newID));
			mReservedFormIDs.push_back(std::make_pair(newID, std::string(stringID)));
			mLastReservedFormID = newID;
			return mLastReservedFormID;
		}

		// worst case: N/2
		if (mReservedFormIDs[midPoint].first > formID)
		{
			// start searching down from midPoint
			for (currentIndex=midPoint-1; currentIndex >= 0; currentIndex--)
			{
				if (mReservedFormIDs[currentIndex].first == formID)
				{
					uint32_t newID = (mReservedFormIDs.back().first+1) | mESMoffset;
					mStringIDMap.insert(std::make_pair(std::string(stringID), newID));
					mReservedFormIDs.push_back(std::make_pair(newID, std::string(stringID)));
					mLastReservedFormID = newID;
					return mLastReservedFormID;
				}
				if (mReservedFormIDs[currentIndex].first < formID)
				{
					currentIndex++;
					break;
				}
			}
			// walk iterator to correct point
			insertionPoint = mReservedFormIDs.begin();
			for (int i=0; i < currentIndex; i++)
				insertionPoint++;
		}
		else
		{				
			// start searching up from midPoint
			for (currentIndex=midPoint+1; currentIndex < mReservedFormIDs.size(); currentIndex++) //this takes 19ms
			{
				if (mReservedFormIDs[currentIndex].first == formID)
				{
					uint32_t newID = (mReservedFormIDs.back().first+1) | mESMoffset;
					mStringIDMap.insert(std::make_pair(std::string(stringID), newID));
					mReservedFormIDs.push_back(std::make_pair(newID, std::string(stringID)));
					mLastReservedFormID = newID;
					return mLastReservedFormID;
				}
				if (mReservedFormIDs[currentIndex].first > formID)
				{
					break;
				}
			}
			// walk iterator to correct point
			insertionPoint = mReservedFormIDs.end();
			for (int i=mReservedFormIDs.size(); i > currentIndex; i--)
				insertionPoint--;
		}
*/
//		mStringIDMap.insert(stringMap);
//		mReservedFormIDs.insert(insertionPoint, recordID);


		// make sure formID is not already used
		if ( mFormIDMap.find(formID) != mFormIDMap.end() || formID == 0)
		{
			if (setup_phase == false)
			{
				// formID already used so get a new formID
				formID = getNextAvailableFormID();
			}
			else
			{
				return 0;
			}
		}
		mFormIDMap.insert( std::make_pair(formID, stringID) );

		// create entry for the stringID crossreference
		mStringIDMap.insert( std::make_pair(Misc::StringUtils::lowerCase(stringID), formID) );

		if (setup_phase == false && formID != 0)
		{
			if ((formID & 0xFF000000) < 0x04000000)
			{
				debugstream.str(""); debugstream.clear();
				debugstream << "ERROR: invalid formID requested: " << formID << std::endl;
				throw std::runtime_error(debugstream.str().c_str());
			}
			mLastReservedFormID = formID;
		}

		return formID;
	}

	void ESMWriter::clearReservedFormIDs()
	{
		mLastReservedFormID = 0;
//		mReservedFormIDs.clear();
		mFormIDMap.clear();
		mStringIDMap.clear();
		mUniqueIDcheck.clear();
		mCellnameMgr.clear();
	}

	uint32_t ESMWriter::crossRefStringID(const std::string& stringID, bool convertToEDID)
	{
//		std::vector<std::pair<uint32_t, std::string>>::iterator currentRecord;
		std::map<std::string, uint32_t>::iterator searchResult;

		if (stringID == "")
			return 0;

		// worst case: N
/*
		for (currentRecord=mReservedFormIDs.begin(); currentRecord != mReservedFormIDs.end(); currentRecord++) // this takes 130ms
		{
			if (stringID == currentRecord->second)
				return currentRecord->first;
		}
*/
		std::string tempString = stringID;
		if (convertToEDID)
		{
			tempString = generateEDIDTES4(stringID);
		}

		searchResult = mStringIDMap.find(Misc::StringUtils::lowerCase(tempString));
		if (searchResult == mStringIDMap.end())
		{
			return 0;
		}
		else
		{
			return searchResult->second;
		}

//		return 0;
	}

	std::string ESMWriter::crossRefFormID(uint32_t formID)
	{
//		std::vector<std::pair<uint32_t, std::string>>::iterator currentRecord;
/*
		if (mReservedFormIDs.size() == 0)
			return "";

		int currentIndex = mReservedFormIDs.size() / 2;

		if (mReservedFormIDs[currentIndex].first == formID)
			return mReservedFormIDs[currentIndex].second;

		// worst case: N/2 (re-implement as binary search, NlogN)
		if (mReservedFormIDs[currentIndex].first > formID)
		{
			// start searching down
			while (--currentIndex >= 0)
			{
				if (mReservedFormIDs[currentIndex].first == formID)
					return mReservedFormIDs[currentIndex].second;
			}
		}
		else
		{
			// start searching up
			while (++currentIndex < mReservedFormIDs.size())
			{
				if (mReservedFormIDs[currentIndex].first == formID)
					return mReservedFormIDs[currentIndex].second;
			}
		}
*/
		std::string retstring = "";

		auto searchResult = mFormIDMap.find(formID);
		if ( searchResult == mFormIDMap.end() )
			retstring = "";
		else
			retstring = searchResult->second;

		return retstring;
	}

	std::string ESMWriter::intToMagEffIDTES4(int magEffVal)
	{
		std::string effSIG;

		switch (magEffVal)
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
			effSIG = "SEFF";
			break;
		case 63: // Almsivi Intervention
			effSIG = "SEFF";
			break;
		case 47: // Blind
			effSIG = "SEFF";
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
			effSIG = "SEFF";
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
			effSIG = "SEFF";
			break;
		case 70: // Cure Blight
			effSIG = "SEFF";
			break;
		case 69: // Cure Common Disease
			effSIG = "CUDI";
			break;
		case 71: // Cure Corprus Disease
			effSIG = "SEFF";
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
			effSIG = "SEFF";
			break;
		case 54: // Demoralize Creature
			effSIG = "SEFF";
			break;
		case 53: // Demoralize Humanoid
			effSIG = "SEFF";
			break;
		case 64: // Detect Animal
			effSIG = "DTCT"; // detect life
			break;
		case 65: // Detect Enchantment
			effSIG = "SEFF";
			break;
		case 66: // Detect Key
			effSIG = "SEFF";
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
			effSIG = "SEFF";
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
			effSIG = "SEFF";
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
			effSIG = "SEFF";
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
			effSIG = "SEFF";
			break;
		case 10: // Levitate
			effSIG = "SEFF";
			break;
		case 41: // Light
			effSIG = "LGHT";
			break;
		case 5: // Lightning Shield
			effSIG = "LISH";
			break;
		case 12: // Lock
			effSIG = "SEFF"; // DO NOT USE? LOCK
			break;
		case 60: // Mark
			effSIG = "SEFF";
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
			effSIG = "SEFF";
			break;
		case 56: // Rally Creature
			effSIG = "RALY"; // RALLY
			break;
		case 55: // Rally Humanoid
			effSIG = "RALY"; // RALLY
			break;
		case 61: // Recall
			effSIG = "SEFF";
			break;
		case 68: // Reflect
			effSIG = "REDG"; // reflect damage
			break;
		case 100: // Remove Curse
			effSIG = "SEFF";
			break;
		case 95: // Resist Blight
			effSIG = "SEFF";
			break;
		case 94: // Resist Common Disease
			effSIG = "RSDI";
			break;
		case 96: // Resist Corprus
			effSIG = "SEFF";
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
			effSIG = "SEFF";
			break;
		case 42: // Sanctuary
			effSIG = "SEFF";
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
			effSIG = "SEFF";
			break;
		case 58: // Soultrap
			effSIG = "STRP";
			break;
		case 48: // Sound
			effSIG = "SEFF";
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
			effSIG = "SEFF";
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
			effSIG = "ZZLI"; // LICHE
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
			effSIG = "SEFF";
			break;
		case 135: // Sun Damage
			effSIG = "SUDG";
			break;
		case 1: // Swift Swim
			effSIG = "SEFF";
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
			effSIG = "SEFF";
			break;
		case 32: // Weakness to Common Disease
			effSIG = "WKDI";
			break;
		case 34: // Weakness to Corprus
			effSIG = "SEFF";
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
		default:
			effSIG = "SEFF";
		}

		if (effSIG.size() != 4)
			throw std::runtime_error("ESMWriter: returned magic effect value is invalid");

		return effSIG;
	}

	int ESMWriter::attributeToActorValTES4(int attributeval)
	{
		int tempval = -1;

		switch (attributeval)
		{
		case ESM::Attribute::AttributeID::Strength:
			tempval = 0;
			break;
		case ESM::Attribute::AttributeID::Intelligence:
			tempval = 1;
			break;
		case ESM::Attribute::AttributeID::Willpower:
			tempval = 2;
			break;
		case ESM::Attribute::AttributeID::Agility:
			tempval = 3;
			break;
		case ESM::Attribute::AttributeID::Speed:
			tempval = 4;
			break;
		case ESM::Attribute::AttributeID::Endurance:
			tempval = 5;
			break;
		case ESM::Attribute::AttributeID::Personality:
			tempval = 6;
			break;
		case ESM::Attribute::AttributeID::Luck:
			tempval = 7;
			break;
		}

		return tempval;
	}

	int ESMWriter::skillToActorValTES4(int skillval)
	{
		int tempval = -1;

		switch (skillval)
		{
		case ESM::Skill::SkillEnum::Acrobatics:
			tempval = 26;
			break;
		case ESM::Skill::SkillEnum::Alchemy:
			tempval = 19;
			break;
		case ESM::Skill::SkillEnum::Alteration:
			tempval = 20;
			break;
		case ESM::Skill::SkillEnum::Armorer:
			tempval = 12;
			break;
		case ESM::Skill::SkillEnum::Athletics:
			tempval = 13;
			break;
		case ESM::Skill::SkillEnum::Axe:
		case ESM::Skill::SkillEnum::BluntWeapon:
		case ESM::Skill::SkillEnum::Spear:
			tempval = 16;
			break;
		case ESM::Skill::SkillEnum::Block:
			tempval = 15;
			break;
		case ESM::Skill::SkillEnum::Conjuration:
		case ESM::Skill::SkillEnum::Enchant:
			tempval = 21;
			break;
		case ESM::Skill::SkillEnum::Destruction:
			tempval = 22;
			break;
		case ESM::Skill::SkillEnum::HandToHand:
			tempval = 17;
			break;
		case ESM::Skill::SkillEnum::HeavyArmor:
		case ESM::Skill::SkillEnum::MediumArmor:
			tempval = 18;
			break;
		case ESM::Skill::SkillEnum::Illusion:
			tempval = 23;
			break;
		case ESM::Skill::SkillEnum::LightArmor:
		case ESM::Skill::SkillEnum::Unarmored:
			tempval = 27;
			break;
		case ESM::Skill::SkillEnum::LongBlade:
		case ESM::Skill::SkillEnum::ShortBlade:
			tempval = 14; // blade
			break;
		case ESM::Skill::SkillEnum::Marksman:
			tempval = 28;
			break;
		case ESM::Skill::SkillEnum::Mercantile:
			tempval = 29;
			break;
		case ESM::Skill::SkillEnum::Mysticism:
			tempval = 24;
			break;
		case ESM::Skill::SkillEnum::Restoration:
			tempval = 25;
			break;
		case ESM::Skill::SkillEnum::Security:
			tempval = 30;
			break;
		case ESM::Skill::SkillEnum::Sneak:
			tempval = 31;
			break;
		case ESM::Skill::SkillEnum::Speechcraft:
			tempval = 32;
			break;
		}

		return tempval;
	}

	std::string ESMWriter::generateEDIDTES4(const std::string& name, int conversion_mode)
	{
		std::string tempEDID, finalEDID;
		bool removeOther=false;

		switch (conversion_mode)
		{
		case 0:
			if (name.size() > 0 && name[0] != '0')
				tempEDID = "0" + name;
			break;
		case 1:
			tempEDID = name;
			break;
		case 2:
			tempEDID = name;
			removeOther = true;
			break;
		}

		int len = tempEDID.length();

		finalEDID = "";
		for (int index=0; index < len; index++)
		{
			switch (tempEDID[index])
			{
			case '_':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'U';
				break;
			case '+':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			case '-':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'T';
				break;
			case ' ':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'S';
				break;
			case ',':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'V';
				break;
			case '\'':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'A';
				break;
			case ':':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			case '.':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'P';
				break;
			case '#':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			case '[':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			case ']':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			case '’':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			case '(':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			case ')':
				finalEDID = (removeOther) ? finalEDID : finalEDID + 'X';
				break;
			default:
				finalEDID = finalEDID + tempEDID[index];
			}
		}

		return finalEDID;
	}

	void ESMWriter::exportMODxTES4(std::string sSIG, std::string sFilename,
			std::string sPrefix, std::string sPostfix, std::string sExt, int flags)
	{
		std::string tempStr, sSIGB="";
		std::ostringstream tempPath;

		// remove extension, if it exists
		tempStr = sFilename;
		if (tempStr.size() > 4)
		{
			if (tempStr[tempStr.size()-4] == '.')
				tempStr.erase(tempStr.size()-4, 4);
		}

		if ((flags & ESMWriter::ExportBipedFlags::noNameMangling) == 0)
			tempStr = generateEDIDTES4(tempStr, 1);
		tempPath << sPrefix << tempStr << sPostfix << sExt;
		startSubRecordTES4(sSIG);
		writeHCString(tempPath.str());
		endSubRecordTES4(sSIG);

		if (sSIG == "MODL")
			sSIGB = "MODB";
		else if (sSIG == "MOD2")
			sSIGB = "MO2B";
		else if (sSIG == "MOD3")
			sSIGB = "MO3B";
		else if (sSIG == "MOD4")
			sSIGB = "MO4B";

		if (sSIGB != "")
		{
			startSubRecordTES4(sSIGB);
			writeT<float>(0.0);
			endSubRecordTES4(sSIGB);
		}

	}

	void ESMWriter::exportBipedModelTES4(std::string sPrefix, std::string sPostfix,
			std::string sFilename, std::string sFilenameFem, 
			std::string sFilenameGnd, std::string sFilenameIcon, int flags)
	{
		std::string tempStr, sSIG;
		std::ostringstream tempPath;

		// MODL, male model
		sSIG = "MODL";
		tempStr = sFilename;
		if (sFilename.size() > 0)
			exportMODxTES4(sSIG, tempStr, sPrefix, sPostfix, ".nif", flags);
		// MODT

		// MOD2, male gnd model
		sSIG = "MOD2";
		if (sFilenameGnd.size() > 4)
		{
			tempStr = sFilenameGnd;
			if (flags & ESMWriter::ExportBipedFlags::postfix_gnd)
				exportMODxTES4(sSIG, tempStr, sPrefix, "_gnd", ".nif", flags);
			else
				exportMODxTES4(sSIG, tempStr, sPrefix, "", ".nif", flags);
			// MO2T
		}
		// ICON, mIcon
		sSIG = "ICON";
		if (sFilenameIcon.size() > 4)
		{
			tempStr = sFilenameIcon;
			exportMODxTES4(sSIG, tempStr, sPrefix, "", ".dds", flags);
		}

		// MOD3, MO3B, MO3T
		sSIG = "MOD3";
		if (sFilenameFem.size() > 0)
		{
			// MOD3, female model
			tempStr = sFilenameFem;
			if (flags & ESMWriter::ExportBipedFlags::postfixF)
				exportMODxTES4(sSIG, tempStr, sPrefix, sPostfix + "F", ".nif", flags);
			else
				exportMODxTES4(sSIG, tempStr, sPrefix, sPostfix, ".nif", flags);

			sSIG = "MOD4";
			if (sFilenameGnd.size() > 4)
			{
				tempStr = sFilenameGnd;
				if (flags & ESMWriter::ExportBipedFlags::postfix_gnd)
					exportMODxTES4(sSIG, tempStr, sPrefix, "_gnd", ".nif", flags);
				else
					exportMODxTES4(sSIG, tempStr, sPrefix, "", ".nif", flags);
				// MO2T
			}
			// ICON, mIcon
			sSIG = "ICO2";
			if (sFilenameIcon.size() > 4)
			{
				tempStr = sFilenameIcon;
				exportMODxTES4(sSIG, tempStr, sPrefix, "", ".dds", flags);
			}

		}

	}

	uint32_t ESMWriter::substituteRaceID(const std::string& raceName)
	{
		uint32_t retVal;
		std::string tempStr;

		tempStr = Misc::StringUtils::lowerCase(raceName);

		if (tempStr == "argonian")
			retVal = 0x23FE9;
		else if (tempStr == "breton")
			retVal = 0x224FC;
		else if (tempStr == "dark elf")
			retVal = 0x191C1;
		else if (tempStr == "high elf")
			retVal = 0x19204;
		else if (tempStr == "imperial")
			retVal = 0x907;
		else if (tempStr == "khajiit")
			retVal = 0x223C7;
		else if (tempStr == "nord")
			retVal = 0x224FD;
		else if (tempStr == "orc")
			retVal = 0x191C0;
		else if (tempStr == "redguard")
			retVal = 0xD43;
		else if (tempStr == "wood elf")
			retVal = 0x223C8;
		else
			retVal = 0; // no match found

		return retVal;		
	}

	std::string ESMWriter::substituteArmorModel(const std::string& model, int modelType)
	{
		std::string retString = "";
		std::string tempString;

		tempString = Misc::StringUtils::lowerCase(model);

		// leather armor suite
//		if (tempString.find("leather") != std::string::npos)
		if (true)
		{

			// default model if nothing else found
			if (true)
//			if (tempString.find("cuirass") != std::string::npos)
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Armor\\Thief\\M\\Cuirass.NIF";
					break;
				case 1: // female model
					retString = "Armor\\Thief\\F\\Cuirass.NIF";
					break;
				case 2: // male gnd
					retString = "Armor\\Thief\\M\\Cuirass_gnd.NIF";
					break;
				case 3: // female gnd
					retString = "Armor\\Thief\\F\\Cuirass_gnd.NIF";
					break;
				case 4: // male ico
					retString = "Armor\\Thief\\M\\Cuirass.dds";
					break;
				case 5: // female ico
					retString = "Armor\\Thief\\F\\Cuirass.dds";
					break;
				}
			}

			if (tempString.find("greaves") != std::string::npos)
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Armor\\Thief\\M\\Greaves.NIF";
					break;
				case 1: // female model
					retString = "Armor\\Thief\\F\\Greaves.NIF";
					break;
				case 2: // male gnd
					retString = "Armor\\Thief\\M\\Greaves_gnd.NIF";
					break;
				case 4: // male ico
					retString = "Armor\\Thief\\M\\Greaves.dds";
					break;
				case 3: // female gnd
				case 5: // female ico
					retString = "";
					break;
				}
			}

			if (tempString.find("gauntlet") != std::string::npos)
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Armor\\Thief\\M\\Gauntlets.NIF";
					break;
				case 1: // female model
					retString = "Armor\\Thief\\F\\Gauntlets.NIF";
					break;
				case 2: // male gnd
					retString = "Armor\\Thief\\M\\Gauntlets_gnd.NIF";
					break;
				case 4: // male ico
					retString = "Armor\\Thief\\M\\Gauntlets.dds";
					break;
				case 3: // female gnd
				case 5: // female ico
					retString = "";
					break;
				}
			}

			if (tempString.find("boots") != std::string::npos)
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Armor\\Thief\\M\\Boots.NIF";
					break;
				case 1: // female model
					retString = "Armor\\Thief\\F\\Boots.NIF";
					break;
				case 2: // male gnd
					retString = "Armor\\Thief\\M\\Boots_gnd.NIF";
					break;
				case 4: // male ico
					retString = "Armor\\Thief\\M\\Boots.dds";
					break;
				case 3: // female gnd
				case 5: // female ico
					retString = "";
					break;
				}
			}

			if (tempString.find("helm") != std::string::npos)
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Armor\\Thief\\Helmet.NIF";
					break;
				case 1: // female model
					retString = "Armor\\Thief\\Helmet.NIF";
					break;
				case 2: // male gnd
					retString = "Armor\\Thief\\Helmet_gnd.NIF";
					break;
				case 4: // male ico
					retString = "Armor\\Thief\\M\\Helmet.dds";
					break;
				case 3: // female gnd
				case 5: // female ico
					retString = "";
					break;
				}
			}

			if ( (tempString.find("shield") != std::string::npos) ||
				(tempString.find("buckler") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
				case 2: // male gnd
					retString = "Armor\\Leather\\Shield.NIF";
					break;
				case 4: // male ico
				case 5: // female ico
					retString = "Armor\\Leather\\Shield.dds";
					break;
				case 1: // female model
				case 3: // female gnd
					retString = "";
					break;
				}
			}

		}
		
		return retString;
	}

	std::string ESMWriter::substituteClothingModel(const std::string& model, int modelType)
	{
		std::string retString = "";
		std::string tempString;

		tempString = Misc::StringUtils::lowerCase(model);

		// default model if nothing else found
		if (true)
//		if (tempString.find("amulet") != std::string::npos)
		{
			switch (modelType)
			{
			case 0: // male model
				retString = "Morroblivion\\Clothes\\Amulets\\AmuletExpensive3.nif";
				break;
			case 2: // male gnd
				retString = "Morroblivion\\Clothes\\Amulets\\AmuletExpensive3GND.nif";
				break;
			case 4: // male ico
				retString = "darthsouth\\Clothing\\Amulets\\expensiveamulet3.dds";
				break;
			case 1: // female model
			case 3: // female gnd
			case 5: // female ico
				retString = "";
				break;
			}
		}

		if ((tempString.find("shirt") != std::string::npos) ||
			(tempString.find("vest") != std::string::npos)
			)
		{
			switch (modelType)
			{
			case 0: // male model
				retString = "Clothes\\LowerClass\\05\\M\\Shirt.NIF";
				break;
			case 1: // female model
				retString = "Clothes\\LowerClass\\05\\F\\Shirt.NIF";
				break;
			case 2: // male gnd
				retString = "Clothes\\LowerClass\\05\\M\\Shirt_gnd.NIF";
				break;
			case 3: // female gnd
				retString = "Clothes\\LowerClass\\05\\F\\Shirt_gnd.NIF";
				break;
			case 4: // male ico
				retString = "Clothes\\LowerClass\\05\\M\\Shirt.dds";
				break;
			case 5: // female ico
				retString = "Clothes\\LowerClass\\05\\F\\Shirt.dds";
				break;
			}
		}

		if ( (tempString.find("pants") != std::string::npos) ||
			(tempString.find("skirt") != std::string::npos) ||
			(tempString.find("trousers") != std::string::npos)
			)
		{
			switch (modelType)
			{
			case 0: // male model
				retString = "Clothes\\LowerClass\\05\\M\\Pants.NIF";
				break;
			case 1: // female model
				retString = "Clothes\\LowerClass\\05\\F\\Pants.NIF";
				break;
			case 2: // male gnd
				retString = "Clothes\\LowerClass\\05\\M\\Pants_gnd.NIF";
				break;
			case 3: // female gnd
				retString = "Clothes\\LowerClass\\05\\F\\Pants_gnd.NIF";
				break;
			case 4: // male ico
				retString = "Clothes\\LowerClass\\05\\M\\Pants.dds";
				break;
			case 5: // female ico
				retString = "Clothes\\LowerClass\\05\\F\\Pants.dds";
				break;
			}
		}

		if (tempString.find("shoes") != std::string::npos)
		{
			switch (modelType)
			{
			case 0: // male model
				retString = "Clothes\\LowerClass\\05\\M\\Shoes.NIF";
				break;
			case 1: // female model
				retString = "Clothes\\LowerClass\\05\\F\\Shoes.NIF";
				break;
			case 2: // male gnd
				retString = "Clothes\\LowerClass\\05\\M\\Shoes_gnd.NIF";
				break;
			case 3: // female gnd
				retString = "Clothes\\LowerClass\\05\\F\\Shoes_gnd.NIF";
				break;
			case 4: // male ico
				retString = "Clothes\\LowerClass\\05\\M\\Shoes.dds";
				break;
			case 5: // female ico
				retString = "Clothes\\LowerClass\\05\\F\\Shoes.dds";
				break;
			}
		}

		if (tempString.find("robe") != std::string::npos)
		{
			switch (modelType)
			{
			case 0: // male model
				retString = "Clothes\\RobeLC01\\M\\RobeLC01M.NIF";
				break;
			case 1: // female model
				retString = "Clothes\\RobeLC01\\F\\RobeLC01F.NIF";
				break;
			case 2: // male gnd
				retString = "Clothes\\RobeLC01\\M\\RobeLC01M_gnd.NIF";
				break;
			case 4: // male ico
				retString = "Clothes\\Monk\\Shirt.dds";
				break;
			case 5: // female ico
			case 3: // female gnd
				retString = "";
				break;
			}
		}

		if (tempString.find("glove") != std::string::npos)
		{
			switch (modelType)
			{
			case 0: // male model
				retString = "Morroblivion\\Clothes\\Gloves\\extravagantgloves.nif";
				break;
			case 2: // male gnd
				retString = "Morroblivion\\Clothes\\Gloves\\extravagantgloves_gnd.nif";
				break;
			case 1: // female model
			case 3: // female gnd
				retString = "";
				break;
			case 4: // male ico
			case 5: // female ico
				retString = "Morroblivion\\Clothes\\Gloves\\ExtravagantGloves.dds";
				break;
			}
		}


		if (tempString.find("belt") != std::string::npos)
		{
			switch (modelType)
			{
			case 0: // male model
			case 1: // female model
			case 3: // female gnd
			case 5: // female ico
				retString = "";
				break;
			case 2: // male gnd
				retString = "Morroblivion\\Clothes\\Belts\\CommonBelt1.nif";
				break;
			case 4: // male ico
				retString = "Morroblivion\\Clothes\\Belts\\CommonBelt1.dds";
				break;
			}
		}

		if (tempString.find("ring") != std::string::npos)
		{
			switch (modelType)
			{
			case 0: // male model
			case 2: // male gnd
				retString = "Clothes\\Ring\\RingSilver.NIF";
				break;
			case 1: // female model
			case 3: // female gnd
			case 5: // female ico
				retString = "";
				break;
			case 4: // male ico
				retString = "darthsouth\\Clothing\\Rings\\expensive1.dds";
				break;
			}
		}

		return retString;
	}

	std::string ESMWriter::substituteBookModel(const std::string& model, int modelType)
	{
		std::string retString = "";
		std::string tempString;

		tempString = Misc::StringUtils::lowerCase(model);

		// default model
		if (true)
//		if (tempString.find("book") != std::string::npos)
		{
			switch (modelType)
			{
			case 0:
				retString = "Clutter\\Books\\Quarto04.NIF";
				break;
			case 1:
				retString = "Clutter\\IconBook12.dds";
			}
		}

		if (tempString.find("scroll") != std::string::npos)
		{
			switch (modelType)
			{
			case 0:
				retString = "Clutter\\Books\\Scroll08.NIF";
				break;
			case 1:
				retString = "Clutter\\IconScroll2.dds";
			}
		}

		return retString;

	}

	std::string ESMWriter::substituteLightModel(const std::string& model, int modelType)
	{
		std::string retString = "";
		std::string tempString;

		tempString = Misc::StringUtils::lowerCase(model);

		// default model
		if (true)
//		if (tempString.find("torch") != std::string::npos)
		{
			switch (modelType)
			{
			case 0:
				retString = "Morroblivion\\Lights\\TorchNoHavok.nif";
				break;
			case 1:
				retString = "Lights\\IconTorch02.dds";
			}
		}

		if (tempString.find("candle") != std::string::npos)
		{
			switch (modelType)
			{
			case 0:
				retString = "Morroblivion\\Lights\\Dunmer\\candle_03.nif";
				break;
			case 1:
				retString = "lights\\Candle Icon.dds";
			}
		}

		if (tempString.find("lantern") != std::string::npos)
		{
			switch (modelType)
			{
			case 0:
				retString = "Morroblivion\\Lights\\Common\\lantern_01.nif";
				break;
			case 1:
				retString = "lights\\0lightUcomUlanterU01.dds";
			}
		}

		return retString;

	}

	std::string ESMWriter::substituteWeaponModel(const std::string& model, int modelType)
	{
		std::string retString = "";
		std::string tempString;

		tempString = Misc::StringUtils::lowerCase(model);

		// metal type
		if (true)
		{

			// default model if nothing else found
			if (true)
//			if ( (tempString.find("longsword") != std::string::npos) ||
//				(tempString.find("saber") != std::string::npos) ||
//				(tempString.find("sword") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\longsword.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blades\\ironlongsword.dds";
					break;
				}
			}

			if ( (tempString.find("shortsword") != std::string::npos) ||
				(tempString.find("tanto") != std::string::npos) ||
				(tempString.find("blade") != std::string::npos) || 
				(tempString.find("wakizashi") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\shortsword.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blades\\ironshortsword.dds";
					break;
				}
			}

			if ( (tempString.find("mace") != std::string::npos) ||
				(tempString.find("club") != std::string::npos) ||
				(tempString.find("morningstar") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\mace.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blunt\\ironmace.dds";
					break;
				}
			}

			if ((tempString.find("axe") != std::string::npos))
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\battleaxe.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Axes\\ironbattleaxe.dds";
					break;
				}
			}

			if ( (tempString.find("halberd") != std::string::npos) ||
				(tempString.find("cleaver") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\halberd.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blades\\ironhalberd.dds";
					break;
				}
			}

			if ( (tempString.find("hammer") != std::string::npos) ||
				(tempString.find("maul") != std::string::npos) ||
				(tempString.find("bat") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\warhammer.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blunt\\ironwarhammer.dds";
					break;
				}
			}

			if ((tempString.find("spear") != std::string::npos) ||
				(tempString.find("skewer") != std::string::npos) ||
				(tempString.find("javelin") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\spear.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blades\\ironspear.dds";
					break;
				}
			}

			if ( (tempString.find("claymore") != std::string::npos) ||
				(tempString.find("slayer") != std::string::npos) ||
				(tempString.find("dai") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\claymore.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blades\\ironclaymore.dds";
					break;
				}
			}

			if ( (tempString.find("dagger") != std::string::npos) ||
				(tempString.find("knife") != std::string::npos) || 
				(tempString.find("fang") != std::string::npos) ||
				(tempString.find("dart") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Iron\\dagger.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Blades\\irondagger.dds";
					break;
				}
			}

			if ( (tempString.find("bow") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Bows\\longbow.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Bows\\longbow.dds";
					break;
				}
			}

			if ((tempString.find("staff") != std::string::npos))
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Morroblivion\\Weapons\\Staffs\\silver.nif";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Staff\\silverstaff.dds";
					break;
				}
			}

			if ( (tempString.find("arrow") != std::string::npos) || 
				(tempString.find("bolt") != std::string::npos) )
			{
				switch (modelType)
				{
				case 0: // male model
					retString = "Weapons\\Iron\\Arrow.NIF";
					break;
				case 1: // male ico
					retString = "darthsouth\\Weapons\\Arrows\\ironarrow.dds";
					break;
				}
			}

		} // default metal type

		return retString;
	}

	float ESM::ESMWriter::calcDistance(ESM::Position pointA, ESM::Position pointB)
	{
		float deltaX = pointB.pos[0] - pointA.pos[0];
		float deltaY = pointB.pos[1] - pointA.pos[1];
		float deltaZ = pointB.pos[2] - pointA.pos[2];

		float distance_XY = sqrtf( (deltaX*deltaX) + (deltaY*deltaY) );
		float distance_XYZ = sqrtf( (distance_XY*distance_XY) + (deltaZ*deltaZ) );

		return distance_XYZ;		
	}



}
