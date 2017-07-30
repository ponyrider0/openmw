#include "esmwriter.hpp"

#include <cassert>
#include <fstream>
#include <stdexcept>

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
    }

    void ESMWriter::addMaster(const std::string& name, uint64_t size)
    {
        Header::MasterData d;
        d.name = name;
        d.size = size;
        mHeader.mMaster.push_back(d);
		mLastReservedFormID += 0x01000000;
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
		startRecordTES4("TES4", flags);

		mHeader.exportTES4 (*this);

		endRecordTES4("TES4");
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

	void ESMWriter::startRecordTES4(const std::string& name, uint32_t flags, uint32_t formID)
	{
		mRecordCount++;

		writeName(name);
		RecordData rec;
		rec.name = name;
		rec.position = mStream->tellp();
		rec.size = 0;

		writeT<uint32_t>(0); // Size goes here (must convert 32bit to 64bit)
		writeT(flags);
		if (name != "TES4" && formID == 0)
		{
			// auto-assign formID
			formID = getNextAvailableFormID();
			reserveFormID(formID);
		}
		writeT<uint32_t>(formID);
		writeT<uint32_t>(0); // version control

		mRecords.push_back(rec);
		assert(mRecords.back().size == 0);
	}

	void ESMWriter::startRecordTES4 (uint32_t name, uint32_t flags, uint32_t formID)
	{
		std::string type;
		for (int i=0; i<4; ++i)
			/// \todo make endianess agnostic
			type += reinterpret_cast<const char *> (&name)[i];
		type[4]='\0';
		startRecordTES4 (type, flags, formID);
	}

	void ESMWriter::startGroupTES4(const std::string& name, uint32_t groupType)
	{
//		mRecordCount++;

		writeFixedSizeString("GRUP", 4);
		RecordData rec;
		rec.name = "GRUP";
		rec.position = mStream->tellp();
		rec.size = 20;

		writeT<uint32_t>(0); // Size goes here (must convert 32bit to 64bit)
		writeFixedSizeString(name, 4); // Group Label, aka record SIG
		writeT<uint32_t>(groupType); // group type
		writeT<uint32_t>(0); // date stamp

		mRecords.push_back(rec);
		assert(mRecords.back().size == 20);
	}

	void ESMWriter::startGroupTES4(const uint32_t name, uint32_t groupType)
	{
		//		mRecordCount++;

		writeFixedSizeString("GRUP", 4);
		RecordData rec;
		rec.name = "GRUP";
		rec.position = mStream->tellp();
		rec.size = 20;

		writeT<uint32_t>(0); // Size goes here (must convert 32bit to 64bit)
		writeT<uint32_t>(name); // Group Label, aka record SIG
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

		if (name == "TES4")
		{
			mStream->seekp(4, std::ios_base::cur);
			writeT<uint64_t>(getNextAvailableFormID());
		}
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
		RecordData rec = mRecords.back();
		assert(rec.name == "GRUP");
		mRecords.pop_back();

		mStream->seekp(rec.position);

		mCounting = false;
		write (reinterpret_cast<const char*> (&rec.size), sizeof(uint32_t));
		mCounting = true;

		mStream->seekp(0, std::ios::end);

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

	uint64_t ESMWriter::getNextAvailableFormID()
	{
		uint64_t returnVal;
		returnVal = mLastReservedFormID + 1;
		return returnVal;
	}

	uint64_t ESMWriter::getLastReservedFormID()
	{
		uint64_t returnVal;
		returnVal = mLastReservedFormID;
		return returnVal;
	}

	bool ESMWriter::reserveFormID(uint64_t formID)
	{
		// lookup formID in mUsedFormIDs
		// add to mUsedFormIDs if not present, then return true
		if (true)
		{
			mReservedFormIDs.insert(mReservedFormIDs.end(), formID);
			mLastReservedFormID = formID;
			return true;
		}
		// otherwise return false
		return false;
	}

	void ESMWriter::clearReservedFormIDs()
	{
		mLastReservedFormID = 0;
		mReservedFormIDs.clear();
	}
}
