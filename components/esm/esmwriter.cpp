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
#include <apps/opencs/model/doc/document.hpp>

namespace ESM
{
	// static class member
	std::map<std::string, std::string> ESMWriter::mMorroblivionEDIDmap;

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

    void ESMWriter::addMaster(const std::string& name, uint64_t size, bool updateOffset)
    {
        Header::MasterData d;
        d.name = name;
        d.size = size;
        mHeader.mMaster.push_back(d);
		if (updateOffset == true)
		{
			mESMoffset += 0x01000000;
		}
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
		if (mStream == NULL)
			return;

		std::streampos currentPos = mStream->tellp();

		mStream->seekp(0, std::ios_base::end);

		if (mStream->tellp() >= (34 + 4))
		{
			mStream->seekp(30, std::ios_base::beg);
			mCounting = false;
			// write total records
			writeT<uint32_t>(mRecordCount);
			// write next available ID
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

	bool ESMWriter::startRecordTES4(const std::string& name, uint32_t flags, uint32_t formID, const std::string& stringID)
	{
		std::stringstream debugstream;
		uint32_t activeID = formID;
		bool bSuccess = true;

		if (name == "TES4")
			activeID = 0;
		else if (name != "TES4" && activeID == 0)
		{
			// auto-assign formID
			activeID = getNextAvailableFormID();
			activeID = reserveFormID(activeID, stringID, name);
		}
		if (mUniqueIDcheck.find(activeID) != mUniqueIDcheck.end())
		{
			// check to see if this is a duplication of record
			std::string overRideStringID = crossRefFormID(activeID);
			uint32_t overRideFormID = crossRefStringID(overRideStringID, name, false);
			if (overRideStringID == stringID && overRideFormID == formID)
			{
				std::cout << "ESMWRITER WARNING: duplicate record written for: [" << name << "](" << stringID << ") " << std::hex << activeID << std::endl;
			}
			else
			{
				//			throw std::runtime_error("ESMWRITER ERROR: non-unique FormID was written to ESM.");
				debugstream << "ESMWRITER ERROR: non-unique FormID was written to ESM: [" << name << "](" << overRideStringID << ") " << std::hex << formID <<
					", overwritten by: (" << stringID << ") " << std::hex << activeID << std::endl;
				OutputDebugString(debugstream.str().c_str());
//				std::cout << debugstream.str();
			}
//			bSuccess = false;
//			return bSuccess;
		}
		
		mRecordCount++;
		writeName(name);

		RecordData rec;
		rec.name = name;
		rec.position = mStream->tellp();
		rec.size = 0;

		writeT<uint32_t>(0); // Size goes here (must convert 32bit to 64bit)
		writeT<uint32_t>(flags);
		writeT<uint32_t>(activeID);
		mUniqueIDcheck.insert( std::make_pair(activeID, mUniqueIDcheck.size()) );

		writeT<uint32_t>(0); // version control

		mRecords.push_back(rec);
		assert(mRecords.back().size == 0);

		return bSuccess;
	}

	bool ESMWriter::startRecordTES4 (uint32_t name, uint32_t flags, uint32_t formID, const std::string& stringID)
	{
		std::string type;
		for (int i=0; i<4; ++i)
			/// \todo make endianess agnostic
			type += reinterpret_cast<const char *> (&name)[i];
		type[4]='\0';

		return startRecordTES4 (type, flags, formID, stringID);
	}

	void ESMWriter::startGroupTES4(const std::string& label, uint32_t groupType)
	{
		mRecordCount++;

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
		mRecordCount++;

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
		uint32_t groupSize = rec.size;
		write (reinterpret_cast<const char*> (&groupSize), sizeof(uint32_t)); 
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


	bool ESMWriter::evaluateOpString(std::string opString, float& opValue, const ESM::Position& opData)
	{
		int char_index = 0;
		int eval_state = 0; // start state
		int operation_state = 0; // no operation
		float left_operand = 0.0f;
		bool binary_operation = false;
		struct EvalOpContext
		{
			int eval_state;
			int operation_state;
			float left_operand;
			bool binary_operation;
		};
		std::vector<struct EvalOpContext> operation_stack;

		// return false if any parsing error...
		// examples: empty paren, two paren with no operator, two operators no operand
		//     right operator without right operand
		while (char_index < opString.size())
		{
			if (eval_state == 0)
			{
				// peek at the next letter position until we can switch into another state
				if (opString[char_index] == '-' || opString[char_index] == '+')
				{
					eval_state = 1; // operator state
				}
				else if (opString[char_index] >= '0' && opString[char_index] <= '9')
				{
					eval_state = 2; // operand - number literal state
				}
				else if (opString[char_index] >= 'a' && opString[char_index] <= 'z')
				{
					eval_state = 3; // operand - variable state
				}
				else if (opString[char_index] == '(')
				{
					eval_state = 4; // open parentheses state
				}
				else if (opString[char_index] == ')')
				{
					eval_state = 5; // close parentheses state
				}
				else
				{
					//advance to next character and continue in state 0
					char_index++;
				}
				continue;
			}
			else if (eval_state == 1)
			{
				// prepare for operation with next operand
				// binary or unary operation?
				// save operation state then continue with next token
				// return to state 0
				if (opString[char_index] == '+')
					operation_state = 1;
				else if (opString[char_index] == '-')
					operation_state = 2;
				else if (opString[char_index] == '*')
					operation_state = 3;
				eval_state = 0;
				char_index++;
				continue;
			}
			else if (eval_state == 2)
			{
				std::string sLiteral = "";
				float right_operand=0;
				// operand - literal: keep reading until full literal is read
				while (char_index < opString.size() && ((opString[char_index] >= '0' && opString[char_index] <= '9') || opString[char_index] == '.'))
				{
					sLiteral += opString[char_index];
					char_index++;
				}
				right_operand = atof(sLiteral.c_str());
				// perform currently active operation state with literal
				switch (operation_state)
				{
				case 1:
					// addition
					left_operand += right_operand;
					break;
				case 2:
					// subtraction
					left_operand -= right_operand;
					break;
				case 3:
					// multiplication
					if (binary_operation == false)
						left_operand = right_operand;
					else
						left_operand *= right_operand;
					break;
				case 0:
					// no operator, assume no operand and just skip
					left_operand = right_operand;
					break;
				}
				binary_operation = true;
				// reset eval and operation state
				eval_state = 0; operation_state = 0;
			}
			else if (eval_state == 3)
			{
				// operand - variable (max two letters)
				std::string sVariable="";
				float right_operand;
				while (char_index < opString.size() && opString[char_index] >= 'a' && opString[char_index] <= 'z')
				{
					sVariable += opString[char_index];
					char_index++;
				}
				// replace variable with a literal
				if (sVariable == "x")
				{
					right_operand = opData.pos[0];
				}
				else if (sVariable == "y")
				{
					right_operand = opData.pos[1];
				}
				else if (sVariable == "z")
				{
					right_operand = opData.pos[2];
				}
				else if (sVariable == "rx")
				{
					right_operand = opData.rot[0] * 57.2958;
				}
				else if (sVariable == "ry")
				{
					right_operand = opData.rot[1] * 57.2958;
				}
				else if (sVariable == "rz")
				{
					right_operand = opData.rot[2] * 57.2958;
				}
				else
				{
					// throw error
					return false;
				}
				// then perform currently active operation with literal
				switch (operation_state)
				{
				case 1:
					// addition
					left_operand += right_operand;
					break;
				case 2:
					// subtraction
					left_operand -= right_operand;
					break;
				case 3:
					// multiplication
					if (binary_operation == false)
						left_operand = right_operand;
					else
						left_operand *= right_operand;
					break;
				case 0:
					// no operator, assume no operand and just skip
					left_operand = right_operand;
					break;
				}
				binary_operation = true;
				// reset eval and operation state
				eval_state = 0; operation_state = 0;
			}
			else if (eval_state == 4)
			{
				// push outside operation and left operand onto stack...
				struct EvalOpContext *newContext = new struct EvalOpContext;
				newContext->eval_state = eval_state;
				newContext->operation_state = operation_state;
				newContext->left_operand = left_operand;
				newContext->binary_operation = binary_operation;
				operation_stack.push_back(*newContext);
				left_operand = 0; operation_state = 0; eval_state = 0; binary_operation = false;
				// advance character
				char_index++;
			}
			else if (eval_state = 5)
			{
				// pop stack and eval outside expression
				float right_operand = left_operand;
				eval_state = operation_stack.back().eval_state;
				operation_state = operation_stack.back().operation_state;
				left_operand = operation_stack.back().left_operand;
				binary_operation = operation_stack.back().binary_operation;
				operation_stack.pop_back();
				// eval left and right
				switch (operation_state)
				{
				case 1:
					// addition
					left_operand += right_operand;
					break;
				case 2:
					// subtraction
					left_operand -= right_operand;
					break;
				case 3:
					// multiplication
					if (binary_operation == false)
						left_operand = right_operand;
					else
						left_operand *= right_operand;
					break;
				case 0:
					// no operator, assume no operand and just skip
					left_operand = right_operand;
					break;
				}
				binary_operation = true;
				// advance character
				char_index++;
			}
		}

		opValue = left_operand;

		return true;
	}

	uint32_t ESMWriter::getNextAvailableFormID()
	{

		if (mFormIDMap.size() == 0)
		{
			mLowestAvailableID = 0x10001 | mESMoffset;;
			return mLowestAvailableID;
		}

		// sanity check
		if (mLowestAvailableID < mESMoffset)
		{
			uint32_t tempID = (mLowestAvailableID & 0x00FFFFFF);
			mLowestAvailableID = tempID | mESMoffset;
		}

		while ( mFormIDMap.find(mLowestAvailableID) != mFormIDMap.end() )
		{
			mLowestAvailableID++;
		}

		return mLowestAvailableID;
	}

	uint32_t ESMWriter::getLastReservedFormID()
	{
		return mLastReservedFormID;
	}

	uint32_t ESMWriter::reserveFormID(uint32_t paramformID, const std::string& stringID, std::string sSIG, bool setup_phase)
	{
		std::stringstream debugstream;
		uint32_t formID = paramformID;
//		if (addESMOffset == true)
//			formID |= mESMoffset;

		if (formID == 0x01380001 && stringID != "wrldmorrowind-dummycell" && setup_phase == false)
		{
			formID = getNextAvailableFormID();
		}

		// make sure formID is not already used
		if ( mFormIDMap.find(formID) != mFormIDMap.end() || formID == 0)
		{
			// if requested formID:stringID pair == stored formID:stringID,
			// then just issue warning and return formID
			auto matchedPair = mStringIDMap.find(Misc::StringUtils::lowerCase(stringID));
			if (matchedPair != mStringIDMap.end() && formID == matchedPair->second)
			{
				std::cout << "WARNING: reserveID: [" << stringID << "] duplicate reserve request for " << std::hex << formID << ", ignoring duplicate." << std::endl;
				return formID;
			}

			if (setup_phase == false)
			{
				if (formID != 0)
				{
					// formID already used so issue warning and get a new formID
					std::cout << "WARNING: reserveID: [" << stringID << "] requested formID (" << std::hex << formID << ") already used, will reserve new ID. " << std::endl;
				}
				formID = getNextAvailableFormID();
			}
			else
			{
				if (formID == 0)
				{
					std::cout << "ERROR: reserveID Setup: [" << stringID << "] requested formID is 0." << std::endl;
					return 0;
				}
			}
		}
		// this step important mainly for keeping track of reserved formIDs,
		// but also for formID to stringID crossreferencing
		mFormIDMap.insert( std::make_pair(formID, stringID) );

		// create entry for the stringID to formID crossreference
		mStringIDMap.insert( std::make_pair(Misc::StringUtils::lowerCase(stringID), formID) );
		mStringTypeMap.insert( std::make_pair(Misc::StringUtils::lowerCase(stringID), sSIG) );

		if (setup_phase == false && formID > mESMoffset)
		{
			mLastReservedFormID = formID;
			if (formID == mLowestAvailableID)
			{
				mLowestAvailableID++;
			}
		}

		return formID;
	}

	void ESMWriter::clearReservedFormIDs()
	{
		mLastReservedFormID = 0;
		mLowestAvailableID = 0x10001;
		mFormIDMap.clear();
		mStringIDMap.clear();
		mStringTransformMap.clear();
		mStringTypeMap.clear();
		mUniqueIDcheck.clear();
		mCellnameMgr.clear();
	}

	uint32_t ESMWriter::crossRefStringID(const std::string& stringID, const std::string &sSIG, bool convertToEDID, bool creating_record)
	{
		if (stringID == "")
			return 0;

		std::string tempString = stringID;
		if (convertToEDID)
		{
			if (Misc::StringUtils::lowerCase(sSIG) == "race")
			{
				uint32_t raceID = substituteRaceID(stringID);
				if (raceID != 0)
					return raceID;
				tempString = generateEDIDTES4(stringID);
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "clas")
			{
				tempString = generateEDIDTES4(stringID, 2);
				uint32_t classID = crossRefStringID(tempString, sSIG, false, true);
				if (classID != 0)
					return classID;
				tempString = generateEDIDTES4(stringID, 0);
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "cell")
			{
				tempString = generateEDIDTES4(stringID, 1);
				uint32_t cellID = crossRefStringID(tempString, sSIG, false, creating_record);
				if (cellID != 0)
					return cellID;
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "dial")
			{
				tempString = generateEDIDTES4(stringID, 4);
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "fact")
			{
				if (Misc::StringUtils::lowerCase(stringID) == "mages guild")
				{
					tempString = "fbmwMagesGuild";
				}
				else if (Misc::StringUtils::lowerCase(stringID) == "fighters guild")
				{
					tempString = "fbmwFightersGuild";
				}
				else
				{
					tempString = generateEDIDTES4(stringID, 0, sSIG);
				}
			}
			else
			{
				tempString = generateEDIDTES4(stringID, 0, sSIG);
			}
		}

		auto searchResult = mStringIDMap.find(Misc::StringUtils::lowerCase(tempString));
		auto typeResult = mStringTypeMap.find(Misc::StringUtils::lowerCase(tempString));

		std::string tempSIG = Misc::StringUtils::lowerCase(sSIG);
/*
		if ( sSIG != "LVLI" && sSIG != "LVLC" && sSIG != "INV_CNT" && sSIG != "INV_NPC" && sSIG != "INV_CRE" &&
			sSIG != "BASEREF" && 
			typeResult != mStringTypeMap.end() &&
			typeResult->second != "" && typeResult->second != "unkn" &&
			tempSIG != "" && 
			tempSIG != Misc::StringUtils::lowerCase(typeResult->second) )
		{
				// throw warning, but allow to continue
				std::cout << "WARNING: crossRefStringID: Stored type does not match request: " << tempString << ":" << sSIG << " vs " << typeResult->second << std::endl;
		}
*/
		if (searchResult == mStringIDMap.end())
		{
			if (!creating_record)
			{
				if (unMatchedEDIDmap.find(tempString) != unMatchedEDIDmap.end())
				{
					if (unMatchedEDIDmap[tempString].first != sSIG)
					{
						if (unMatchedEDIDmap[tempString].first.find(sSIG, 0) == std::string::npos)
						{
							unMatchedEDIDmap[tempString].first += "+" + sSIG;
						}
					}
					unMatchedEDIDmap[tempString].second++;
				}
				else
				{
					unMatchedEDIDmap[tempString].first = sSIG;
					unMatchedEDIDmap[tempString].second = 1;
				}
			}
			return 0;
		}

		return searchResult->second;

	}

	std::string ESMWriter::crossRefFormID(uint32_t formID)
	{
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

	std::string ESMWriter::generateEDIDTES4(const std::string& name, int conversion_mode, const std::string& sSIG)
	{
		std::string tempEDID, finalEDID;
		std::string preFix = "", postFix = "";
		bool removeOther=false;
		bool addScript=false;
		bool addRegion=false;

		if (name == "")
			return name;

		// Do EDID substitutions
		if ( (Misc::StringUtils::lowerCase(name) == "player") ||
			(Misc::StringUtils::lowerCase(name) == "playerref") )
			return name;
		if (Misc::StringUtils::lowerCase(name) == "northmarker")
			return name;
		if (Misc::StringUtils::lowerCase(name) == "gold_001")
			return "Gold001";

		if (sSIG != "")
		{
			if (Misc::StringUtils::lowerCase(sSIG) == "qust")
			{
				conversion_mode = 5;
			}
			if (Misc::StringUtils::lowerCase(sSIG) == "glob")
			{
//				conversion_mode = 1;
//				postFix = "Uglobal";
				conversion_mode = 2;
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "scpt")
			{
				conversion_mode = 3;
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "regn")
			{
				conversion_mode = 2;
				addRegion = true;
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "cell")
			{
				conversion_mode = 1;
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "dial")
			{
				conversion_mode = 4;
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "pref")
			{
				conversion_mode = 5; // like quest but add REF postfix
				postFix = "REF";
			}
			else if (Misc::StringUtils::lowerCase(sSIG) == "squst")
			{
				conversion_mode = 3; // like quest but add REF postfix
				preFix = "mw";
				postFix = "Quest";
			}

		}

		switch (conversion_mode)
		{
		case 0: // default: leading zero (most records)
			if (name.size() > 0 && name[0] != '0')
				tempEDID = "0" + name;
			break;
		case 1: // no leading zero (most filenames, some records)
			tempEDID = name;
			break;
		case 2: // no leading zero and remove extra non-letter/digit characters (some records)
			tempEDID = name;
			removeOther = true;
			break;
		case 3: // only for scripts: no leading zero, +append "script"
			if (name.size() > 0 && name[0] != '0')
			{
				tempEDID = name;
				addScript = true;
			}
			break;
		case 4: // for dialog: leading "1"
			if (name.size() > 0 && name[0] != '0')
				tempEDID = "1" + name;
			break;
		case 5: // for quests (and some other records): leading "mw" and no extra-char types
			tempEDID = "mw" + name;
			removeOther = true;
			break;
		}
		tempEDID = preFix + tempEDID;
		
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
			case '\\':
				finalEDID = (removeOther) ? finalEDID : finalEDID + '\\';
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
			case '~':
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

		if (addScript)
		{
			if (finalEDID.size() > 2 && (Misc::StringUtils::lowerCase(finalEDID).find("sc", finalEDID.size() - 2) == finalEDID.npos) &&
				(Misc::StringUtils::lowerCase(finalEDID).find("script", 0) == finalEDID.npos))
			{
				finalEDID += "Script";
			}
		}

		if (addRegion)
		{
			if (Misc::StringUtils::lowerCase(finalEDID).find("region", 0) == finalEDID.npos)
			{
				finalEDID += "Region";
			}
		}

		// add generic postFix
		if (postFix != "")
		{
			finalEDID += postFix;
		}

		finalEDID = substituteMorroblivionEDID(finalEDID, sSIG);

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
//		if (true)
		if (tempString.find("torch") != std::string::npos)
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

	std::vector<std::string> ESMWriter::substituteCreatureModel(const std::string & strEDID, int modelType)
	{
		std::string creatureEDID = Misc::StringUtils::lowerCase(strEDID);
		std::vector<std::string> stringList;

//		if (creatureEDID.find("deer") != std::string::npos)
		if (true)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Deer\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("antlar8point.nif");
				stringList.push_back("deereyes.nif");
				stringList.push_back("skinbuck.nif");
				break;
			}
		}

		// default
		if (creatureEDID.find("minotaur") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("creatures\\minotaur\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("minotaurbodyhair.nif");
				stringList.push_back("minotaur.nif");
				stringList.push_back("hornsb.nif");
				stringList.push_back("head.nif");
				stringList.push_back("hair02.nif");
				stringList.push_back("hair01.nif");
				stringList.push_back("goz.nif");
				stringList.push_back("eyelids.nif");
				break;
			}
		}

		if (creatureEDID.find("goblin") != std::string::npos ||
			creatureEDID.find("gob") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Goblin\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("goblegs01.nif");
				stringList.push_back("goblinchest01.nif");
				stringList.push_back("goblinhandl.nif");
				stringList.push_back("gonblinhandr.nif");
				stringList.push_back("goblinhead.nif");
				break;
			}
		}

		if (creatureEDID.find("ghost") != std::string::npos ||
			creatureEDID.find("ghst") != std::string::npos ||
			creatureEDID.find("wrait") != std::string::npos ||
			creatureEDID.find("spirit") != std::string::npos ||
			creatureEDID.find("spr") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Ghost\\Skeleton.nif");
				break;

			case 1:
				stringList.push_back("heademissive.nif");
				stringList.push_back("innerbodyemissive.nif");
				stringList.push_back("lefthandemissive.nif");
				stringList.push_back("outerbodyemissive.nif");
				stringList.push_back("righthandemissive.nif");
				stringList.push_back("shrink.nif");
				break;
			}
		}

		if (creatureEDID.find("skeleton") != std::string::npos ||
			creatureEDID.find("lich") != std::string::npos ||
			creatureEDID.find("skel") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Skeleton\\skeleton.NIF");
				break;

			case 1:
				stringList.push_back("skellie.nif");
				stringList.push_back("skull.nif");
				break;
			}
		}

		if (creatureEDID.find("zombie") != std::string::npos ||
			creatureEDID.find("bonewalker") != std::string::npos ||
			creatureEDID.find("uboneld") != std::string::npos ||
			creatureEDID.find("ubonewk") != std::string::npos ||
			creatureEDID.find("mwuundumum") != std::string::npos ||
			creatureEDID.find("mummy") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Zombie\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("hair02.nif");
				stringList.push_back("head02.nif");
				stringList.push_back("leftarm02.nif");
				stringList.push_back("lefthand02.nif");
				stringList.push_back("leftleg01.nif");
				stringList.push_back("lefttorso01.nif");
				stringList.push_back("neckplug.nif");
				stringList.push_back("rightarm02.nif");
				stringList.push_back("righthand02.nif");
				stringList.push_back("rightleg02.nif");
				stringList.push_back("righttorso02.nif");
				break;
			}
		}

		if (creatureEDID.find("draugr") != std::string::npos ||
			creatureEDID.find("drgr") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Aa_Blood\\Draugr\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("body.nif");
				break;
			}
		}

		if (creatureEDID.find("dog") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Dog\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("dogbody.nif");
				stringList.push_back("dogeyes01.nif");
				stringList.push_back("doghead.nif");
				break;
			}
		}

		if (creatureEDID.find("elytra") != std::string::npos ||
			creatureEDID.find("ornada") != std::string::npos ||
			creatureEDID.find("orn") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Elytra\\Skeleton.nif");
				break;

			case 1:
				stringList.push_back("elytrabody.nif");
				stringList.push_back("head1.nif");
				stringList.push_back("stinger.nif");
				break;
			}
		}

		if (creatureEDID.find("netch") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("morroblivion\\creatures\\bullnetch\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("body.nif");
				stringList.push_back("jelly.nif");
				break;
			}
		}

		if (creatureEDID.find("kwama") != std::string::npos ||
			creatureEDID.find("kwa") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Morroblivion\\Creatures\\Kwama\\KwamaForager\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("mesh.nif");
				break;
			}
		}

		if (creatureEDID.find("nix") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("creatures\\nixhound\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("nixhound.nif");
				break;
			}
		}

		if (creatureEDID.find("drid") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("creatures\\spiderdaedra\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("daedra hair.nif");
				stringList.push_back("trdridreaf.nif");
				break;
			}
		}

		if (creatureEDID.find("fish") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Slaughterfish\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("slaughterfish.nif");
				break;
			}
		}

		if (creatureEDID.find("fly") != std::string::npos ||
			creatureEDID.find("mwufauuswfly") != std::string::npos ||
			creatureEDID.find("muskf") != std::string::npos )
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("morroblivion\\creatures\\swampfly\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("body.nif");
				break;
			}
		}

		if (creatureEDID.find("crab") != std::string::npos ||
			creatureEDID.find("mudc") != std::string::npos ||
			creatureEDID.find("molec") != std::string::npos ||
			creatureEDID.find("moonc") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\MudCrab\\skeleton.NIF");
				break;

			case 1:
				stringList.push_back("mud crbeye l00.nif");
				stringList.push_back("mud crbeye r00.nif");
				stringList.push_back("mudcrabjawl.nif");
				stringList.push_back("mudcrabjawr.nif");
				stringList.push_back("skincrab.nif");
				break;
			}
		}

		if (creatureEDID.find("rat") != std::string::npos ||
			creatureEDID.find("mouse") != std::string::npos ||
			creatureEDID.find("mous") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Rat\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("eyes.nif");
				stringList.push_back("head.nif");
				stringList.push_back("mange.nif");
				stringList.push_back("rat.nif");
				stringList.push_back("whiskers.nif");
				break;
			}
		}

		if (creatureEDID.find("bear") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Bear\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("bearbody.nif");
				stringList.push_back("bearhead.nif");
				break;
			}
		}

		if (creatureEDID.find("boar") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Boar\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("blackhair.nif");
				stringList.push_back("boarbody.nif");
				stringList.push_back("boarhead.nif");
				break;
			}
		}

		if (creatureEDID.find("cow") != std::string::npos ||
			creatureEDID.find("hrs") != std::string::npos ||
			creatureEDID.find("horse") != std::string::npos ||
			creatureEDID.find("donkey") != std::string::npos ||
			creatureEDID.find("donk") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Horse\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("bridle.nif");
				stringList.push_back("horseheadpaint.nif");
				stringList.push_back("horsepaint.nif");
				stringList.push_back("lefteye.nif");
				stringList.push_back("mane.nif");
				stringList.push_back("righteyeblue.nif");
				stringList.push_back("saddle.nif");
				stringList.push_back("tail.nif");
				break;
			}
		}

		if (creatureEDID.find("wolf") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Dog\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("wolfbody.nif");
				stringList.push_back("wolfeyes.nif");
				stringList.push_back("wolfhead.nif");
				break;
			}
		}

		if (creatureEDID.find("troll") != std::string::npos ||
			creatureEDID.find("trll") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Troll\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("troll.nif");
				break;
			}
		}

		if (creatureEDID.find("chicken") != std::string::npos ||
			creatureEDID.find("clannfear") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Clannfear\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("clannfear.nif");
				stringList.push_back("head.nif");
				break;
			}
		}

		if (creatureEDID.find("daedroth") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Daedroth\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("daedroth.nif");
				break;
			}
		}

		if (creatureEDID.find("dreu") != std::string::npos ||
			creatureEDID.find("cephalopod") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\dreugh\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("body.nif");
				break;
			}
		}

		if (creatureEDID.find("atronach") != std::string::npos ||
			creatureEDID.find("atro") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\FrostAtronach\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("frostatronach.nif");
				break;
			}
		}

		if (creatureEDID.find("guar") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Morroblivion\\Creatures\\Guar\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("stripedguar.nif");
				break;
			}
		}

		if (creatureEDID.find("scrib") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Morroblivion\\Creatures\\Kwama\\Scrib\\skeleton.nif");
				break;

			case 1:
//				stringList.push_back("skeleton.nif");
				break;
			}
		}

		if (creatureEDID.find("ogrim") != std::string::npos ||
			creatureEDID.find("ogrm") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\Ogre\\Skeleton.NIF");
				break;

			case 1:
				stringList.push_back("flask.nif");
				stringList.push_back("furtufts.nif");
				stringList.push_back("head.nif");
				stringList.push_back("ogre.nif");
				stringList.push_back("pouch.nif");
				break;
			}
		}

		if (creatureEDID.find("centurion") != std::string::npos)
		{
			stringList.clear();
			switch (modelType)
			{
			case 0:
				stringList.push_back("Creatures\\SphereCenturion\\skeleton.nif");
				break;

			case 1:
				stringList.push_back("body.nif");
				break;
			}
		}


		return stringList;
	}

	std::string ESMWriter::substituteHairID(const std::string & hairName)
	{
		return std::string();
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

	std::string ESMWriter::substituteMorroblivionEDID(const std::string & genericEDID, const std::string & recordSIG)
	{
		if (recordSIG.size() != 4)
			return genericEDID;

		uint32_t recordType = 0;
		uint8_t *byteptr;
		byteptr = (uint8_t *) &recordType;

		for (int i = 0; i < 4; i++)
		{
			byteptr[i] = recordSIG[i];
		}

		return substituteMorroblivionEDID(genericEDID, (ESM::RecNameInts) recordType);
	}

	std::string ESMWriter::substituteMorroblivionEDID(const std::string & genericEDID, ESM::RecNameInts recordType)
	{
		std::string morroblivionEDID = genericEDID;

		if (mMorroblivionEDIDmap.find(genericEDID) != mMorroblivionEDIDmap.end())
		{
			morroblivionEDID = mMorroblivionEDIDmap[genericEDID];
			if (morroblivionEDID == "")
				throw std::runtime_error("ERROR: Creature - empty EDID substituted for: " + genericEDID);
			return morroblivionEDID;
		}

		switch (recordType)
		{
		case ESM::REC_FACT:
			// inline or call subroutine and break
			if (genericEDID == "0FightersSGuild")
				morroblivionEDID = "fbmwFightersGuild";
			else if (genericEDID == "0MagesSGuild")
				morroblivionEDID = "fbmwMagesGuild";
			break;

		case ESM::REC_QUES:
			if (genericEDID == "mwTRDBAttack")
				morroblivionEDID = "fbmwTRDBAttack";

		case ESM::REC_LTEX:
			if (genericEDID == "0Sand")
				morroblivionEDID = "mwComSand01Tex";
			if (genericEDID == "0RockUCoastal")
				morroblivionEDID = "mwComRockCoastalTex";
			if (genericEDID == "0RoadSDirt")
				morroblivionEDID = "mwComRoadDirtTex";
			break;
	
		default:
			if (genericEDID == "0ActiveUComUBarUDoor")
				morroblivionEDID = "mwComBarDoor";
			if (genericEDID == "0ActiveUDeUBarUDoor")
				morroblivionEDID = "mwDeBarDoor";
			if (genericEDID == "0potionUcyroUbrandyU01")
				morroblivionEDID = "PotionCyrodiilicBrandy";
			if (genericEDID == "0MiscUQuill")
				morroblivionEDID = "Quill01";
			if (genericEDID == "0ingredUfrostUsaltsU01")
				morroblivionEDID = "FrostSalts";
			if (genericEDID == "0CollisionSWallSTSINVISO")
				morroblivionEDID = "CollisionBoxStatic";
//			if (genericEDID == "0TUImpUDrinkUWineSurilieBrU01")
//				morroblivionEDID = "DrinkWine1SurilieGood";
//			if (genericEDID == "0TUImpUDrinkUWineFreeEstatU01")
//				morroblivionEDID = "DrinkWine0Cheap";
//			if (genericEDID == "0TRUm1UFWCEUCTPosU05UUG")
//				morroblivionEDID = "0miscUlwUplatter";

		}

		return morroblivionEDID;
	}

	// compareOperator: "=, !=, <, <=, >, >="
	void ESMWriter::exportConditionalExpression(uint32_t compareFunction, uint32_t compareArg1,
		const std::string& compareOperator, float compareValue, uint8_t condFlags, uint32_t compareArg2)
	{

		int conditionType = 0;
		if (compareOperator == "=") conditionType = 0x00;
		if (compareOperator == "!=") conditionType = 0x20;
		if (compareOperator == ">") conditionType = 0x40;
		if (compareOperator == ">=") conditionType = 0x60;
		if (compareOperator == "<") conditionType = 0x80;
		if (compareOperator == "<=") conditionType = 0xA0;
		conditionType |= condFlags;

		startSubRecordTES4("CTDA");
		writeT<uint8_t>(conditionType); // type
		writeT<uint8_t>(0); // unused x3
		writeT<uint8_t>(0); // unused x3
		writeT<uint8_t>(0); // unused x3
		writeT<float>(compareValue); // comparison value
		writeT<uint32_t>(compareFunction); // comparison function
		writeT<uint32_t>(compareArg1); // comparison argument
		writeT<uint32_t>(compareArg2); // comparison argument
		writeT<uint32_t>(0); // unused
		endSubRecordTES4("CTDA");
	}

	void ESMWriter::QueueModelForExport(std::string origString, std::string convertedString, int recordType)
	{
		mModelsToExportList[origString] = std::make_pair(convertedString, recordType);
	}

	void ESMWriter::RegisterBaseObjForScriptedREF(const std::string &stringID, std::string sSIG, int nMode)
	{
		int numTimesReferenced = 1;

		std::string stringID_lowercase = Misc::StringUtils::lowerCase(stringID);
		auto record = mBaseObjToScriptedREFList.find(stringID_lowercase);
		if (record != mBaseObjToScriptedREFList.end())
		{
			numTimesReferenced = record->second.second + 1;
		}
		mBaseObjToScriptedREFList[stringID_lowercase] = std::make_pair(sSIG, numTimesReferenced);

	}

	void ESMWriter::RegisterScriptToQuest(const std::string & scriptID, int nMode)
	{
		std::string scriptEDID_lowercase = Misc::StringUtils::lowerCase(generateEDIDTES4(scriptID, 0, "SCPT"));
		mScriptToQuestList[scriptEDID_lowercase] = std::make_pair(scriptID, nMode);
	}


	bool ESMWriter::lookup_reference(const CSMDoc::Document &doc, const std::string &baseName, std::string &refEDID, std::string &refSIG, std::string &refValString)
	{
		bool result = true;

		std::string refString = baseName;
		std::string refScript = "";
		std::string refFact = "";

		std::pair<int, CSMWorld::UniversalId::Type> refRecord = doc.getData().getReferenceables().getDataSet().searchId(refString);
		if (refRecord.first == -1)
		{
			if (refRecord.first = doc.getData().getCells().searchId(refString) != -1)
			{
				refSIG = "CELL";
			}
			else if (refRecord.first = doc.getData().getJournals().searchId(refString) != -1)
			{
				refSIG = "QUST";
			}
			else if (refRecord.first = doc.getData().getFactions().searchId(refString) != -1)
			{
				refSIG = "FACT";
			}
			else if (refRecord.first = doc.getData().getGlobals().searchId(refString) != -1)
			{
				refSIG = "GLOB";
			}
			else if (refRecord.first = doc.getData().getScripts().searchId(refString) != -1)
			{
				refScript = refString;
				refSIG = "SCPT";
			}
			else if (refRecord.first = doc.getData().getSpells().searchId(refString) != -1)
			{
				refSIG = "SPEL";
			}
			else if (doc.getData().getClasses().searchId(refString) != -1)
			{
				refSIG = "CLAS";
			}
			else if (doc.getData().getMagicEffects().searchId(refString) != -1)
			{
				//				refSIG = "CLAS";
			}
			else if (doc.getData().getPathgrids().searchId(refString) != -1)
			{
				refSIG = "PGRD";
			}
			else if (doc.getData().getRaces().searchId(refString) != -1)
			{
				refSIG = "RACE";
			}
			else if (doc.getData().getRegions().searchId(refString) != -1)
			{
				refSIG = "REGN";
			}
			else if (doc.getData().getSkills().searchId(refString) != -1)
			{
				refSIG = "SKIL";
			}
			else if (doc.getData().getSounds().searchId(refString) != -1)
			{
				refSIG = "SOUN";
			}
			else if (doc.getData().getTopics().searchId(refString) != -1)
			{
				//				refSIG = "TOPI";
			}
			else
			{
				result = false;
			}
		}

		switch (refRecord.second)
		{
		case CSMWorld::UniversalId::Type_Npc:
			refScript = doc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(refRecord.first).get().mScript;
			refFact = doc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(refRecord.first).get().mFaction;
			refSIG = "NPC_";
			break;

		case CSMWorld::UniversalId::Type_Book:
			refScript = doc.getData().getReferenceables().getDataSet().getBooks().mContainer.at(refRecord.first).get().mScript;
			refSIG = "BOOK";
			break;

		case CSMWorld::UniversalId::Type_Activator:
			refScript = doc.getData().getReferenceables().getDataSet().getActivators().mContainer.at(refRecord.first).get().mScript;
			refSIG = "ACTI";
			break;

		case CSMWorld::UniversalId::Type_Potion:
			refScript = doc.getData().getReferenceables().getDataSet().getPotions().mContainer.at(refRecord.first).get().mScript;
			refSIG = "ALCH";
			break;

		case CSMWorld::UniversalId::Type_Apparatus:
			refScript = doc.getData().getReferenceables().getDataSet().getApparati().mContainer.at(refRecord.first).get().mScript;
			refSIG = "APPA";
			break;

		case CSMWorld::UniversalId::Type_Armor:
			refScript = doc.getData().getReferenceables().getDataSet().getArmors().mContainer.at(refRecord.first).get().mScript;
			refSIG = "ARMO";
			break;

		case CSMWorld::UniversalId::Type_Clothing:
			refScript = doc.getData().getReferenceables().getDataSet().getClothing().mContainer.at(refRecord.first).get().mScript;
			refSIG = "CLOT";
			break;

		case CSMWorld::UniversalId::Type_Container:
			refScript = doc.getData().getReferenceables().getDataSet().getContainers().mContainer.at(refRecord.first).get().mScript;
			refSIG = "CONT";
			break;

		case CSMWorld::UniversalId::Type_Creature:
			refScript = doc.getData().getReferenceables().getDataSet().getCreatures().mContainer.at(refRecord.first).get().mScript;
			refSIG = "CREA";
			break;

		case CSMWorld::UniversalId::Type_Door:
			refScript = doc.getData().getReferenceables().getDataSet().getDoors().mContainer.at(refRecord.first).get().mScript;
			refSIG = "DOOR";
			break;

		case CSMWorld::UniversalId::Type_Ingredient:
			refScript = doc.getData().getReferenceables().getDataSet().getIngredients().mContainer.at(refRecord.first).get().mScript;
			refSIG = "INGR";
			break;

		case CSMWorld::UniversalId::Type_Light:
			refScript = doc.getData().getReferenceables().getDataSet().getLights().mContainer.at(refRecord.first).get().mScript;
			refSIG = "LIGH";
			break;

		case CSMWorld::UniversalId::Type_Miscellaneous:
			refScript = doc.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(refRecord.first).get().mScript;
			refSIG = "MISC";
			break;

		case CSMWorld::UniversalId::Type_Weapon:
			refScript = doc.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(refRecord.first).get().mScript;
			refSIG = "WEAP";
			break;

		default:
			result = false;
			break;
		}

		// Generate Reference EDID to be used in TES4 Script
		//  For scriptable world objects, this is usually a persistent reference.
		//  For scripts, this is a quest reference.
		//  For inventory objects... ?
		//  For cells?
		// refEDID = mESM.generateEDIDTES4(refString, 0, refSIG);
		std::string refSIG_lowercase = Misc::StringUtils::lowerCase(refSIG);
		if (refSIG_lowercase == "npc_" ||
			refSIG_lowercase == "crea" ||
			refSIG_lowercase == "acti" ||
			refSIG_lowercase == "door")
		{
			refEDID = generateEDIDTES4(baseName, 0, "PREF");
			// register baseobj for persistent ref creation
			RegisterBaseObjForScriptedREF(baseName, refSIG);
		}
		else if (refSIG_lowercase == "scpt")
		{
			refEDID = generateEDIDTES4(baseName, 0, "SQUST");
			// register script for Script_to_Quest creation
			RegisterScriptToQuest(baseName);
		}
		else
		{
			refEDID = generateEDIDTES4(baseName, 0, refSIG);
		}

		if (Misc::StringUtils::lowerCase(refValString) == "script")
			refValString = refScript;
		else if (Misc::StringUtils::lowerCase(refValString) == "faction")
			refValString = refFact;
		else
			refValString = "";


		return result;
	}


}
