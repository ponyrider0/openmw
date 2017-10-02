#ifndef OPENMW_ESM_WRITER_H
#define OPENMW_ESM_WRITER_H

#include <iosfwd>
#include <list>
#include <map>

#include "loadskil.hpp"
#include "attr.hpp"
#include "esmcommon.hpp"
#include "loadtes3.hpp"
#include "loadtes4.hpp"

namespace ToUTF8
{
    class Utf8Encoder;
}

namespace ESM {

class ESMWriter
{
        struct RecordData
        {
            std::string name;
            std::streampos position;
            uint32_t size;
        };

    public:

        ESMWriter();

        unsigned int getVersion() const;

        // Set various header data (ESM::Header::Data). All of the below functions must be called before writing,
        // otherwise this data will be left uninitialized.
        void setVersion(unsigned int ver = 0x3fa66666);
        void setType(int type);
        void setEncoder(ToUTF8::Utf8Encoder *encoding);
        void setAuthor(const std::string& author);
        void setDescription(const std::string& desc);

        // Set the record count for writing it in the file header
        void setRecordCount (int count);
        // Counts how many records we have actually written.
        // It is a good idea to compare this with the value you wrote into the header (setRecordCount)
        // It should be the record count you set + 1 (1 additional record for the TES3 header)
        int getRecordCount() { return mRecordCount; }
        void setFormat (int format);

        void clearMaster();

        void addMaster(const std::string& name, uint64_t size);

        void save(std::ostream& file);
        ///< Start saving a file by writing the TES3 header.
		void exportTES4(std::ostream& file);

        void close();
        ///< \note Does not close the stream.
		void updateTES4();

        void writeHNString(const std::string& name, const std::string& data);
        void writeHNString(const std::string& name, const std::string& data, size_t size);
        void writeHNCString(const std::string& name, const std::string& data)
        {
            startSubRecord(name);
            writeHCString(data);
            endRecord(name);
        }
        void writeHNOString(const std::string& name, const std::string& data)
        {
            if (!data.empty())
                writeHNString(name, data);
        }
        void writeHNOCString(const std::string& name, const std::string& data)
        {
            if (!data.empty())
                writeHNCString(name, data);
        }

        template<typename T>
        void writeHNT(const std::string& name, const T& data)
        {
            startSubRecord(name);
            writeT(data);
            endRecord(name);
        }

private:
        // Prevent using writeHNT with strings. This already happened by accident and results in
        // state being discarded without any error on writing or reading it. :(
        // writeHNString and friends must be used instead.
        void writeHNT(const std::string& name, const std::string& data)
        {
        }
        void writeT(const std::string& data)
        {
        }
public:

        template<typename T>
        void writeHNT(const std::string& name, const T& data, int size)
        {
            startSubRecord(name);
            writeT(data, size);
            endRecord(name);
        }

        template<typename T>
        void writeT(const T& data)
        {
            write((char*)&data, sizeof(T));
        }

        template<typename T>
        void writeT(const T& data, size_t size)
        {
            write((char*)&data, size);
        }

        void startRecord(const std::string& name, uint32_t flags = 0);
        void startRecord(uint32_t name, uint32_t flags = 0);
        /// @note Sub-record hierarchies are not properly supported in ESMReader. This should be fixed later.
        void startSubRecord(const std::string& name);
        void endRecord(const std::string& name);
        void endRecord(uint32_t name);
        void writeFixedSizeString(const std::string& data, int size);
        void writeHString(const std::string& data);
        void writeHCString(const std::string& data);
        void writeName(const std::string& data);
        void write(const char* data, size_t size);

		void startRecordTES4(const std::string& name, uint32_t flags = 0, uint32_t formID = 0, const std::string& stringID="");
		void startRecordTES4(uint32_t name, uint32_t flags = 0, uint32_t formID = 0, const std::string& stringID="");
		void startGroupTES4(const std::string& name, uint32_t groupType);
		void startGroupTES4(const uint32_t name, uint32_t groupType);
		void startSubRecordTES4(const std::string& name);
		void endRecordTES4(const std::string& name);
		void endRecordTES4(uint32_t name);
		void endSubRecordTES4(const std::string& name);
		void endGroupTES4(const std::string& name);
		void endGroupTES4(const uint32_t name);

//		std::vector<std::pair<uint32_t, std::string> > mReservedFormIDs;
		std::map<uint32_t, std::string> mFormIDMap;
		std::map<std::string, uint32_t> mStringIDMap;
		std::map<uint32_t, int> mUniqueIDcheck;

		uint32_t mLastReservedFormID=0;
		uint32_t getNextAvailableFormID();
		uint32_t getLastReservedFormID();
		uint32_t reserveFormID(uint32_t formID, const std::string& stringID, bool setup_phase=false);
		void clearReservedFormIDs();
		uint32_t crossRefStringID(const std::string& mId, const std::string &sSIG="", bool convertToEDID=true, bool setup_phase=false);
		std::string crossRefFormID(uint32_t formID);

		uint32_t mESMoffset=0;

		static std::string generateEDIDTES4(const std::string& name, int conversion_mode=0);
		static int skillToActorValTES4(int skillval);
		static int attributeToActorValTES4(int attributeval);
		static std::string intToMagEffIDTES4(int magEffVal);

		void exportMODxTES4(std::string sSIG, 
			std::string sFilename, std::string sPrefix, std::string sPostfix, std::string sExt, int flags=0);

		enum ExportBipedFlags { postfixF=0x01, postfix_gnd=0x02, noNameMangling=0x04 };

		void exportBipedModelTES4(std::string sPrefix, std::string sPostfix,
			std::string sFilename, std::string sFilenameFem="", 
			std::string sFilenameGnd="", std::string sFilenameIcon="", int flags=0);

		std::map<std::string, int> mCellnameMgr;
		static uint32_t substituteRaceID(const std::string& raceName);
		static std::string substituteArmorModel(const std::string& model, int modelType);
		static std::string substituteClothingModel(const std::string& model, int modelType);
		static std::string substituteBookModel(const std::string& model, int modelType);
		static std::string substituteLightModel(const std::string& model, int modelType);
		static std::string substituteWeaponModel(const std::string& model, int modelType);
		static std::vector<std::string> substituteCreatureModel(const std::string& creatureEDID, int modelType);

		static std::string substituteHairID(const std::string& hairName);
		void exportFaceGen(const std::string& headName);
		static float calcDistance(ESM::Position pointA, ESM::Position pointB);

		std::string substituteMorroblivionEDID(const std::string& genericEDID, ESM::RecNameInts recordType);
		std::map<std::string, std::string> mMorroblivionEDIDmap;
		std::map<std::string, std::pair<std::string, int>> unMatchedEDIDmap;

    private:
        std::list<RecordData> mRecords;
        std::ostream* mStream;
        std::streampos mHeaderPos;
        ToUTF8::Utf8Encoder* mEncoder;
        int mRecordCount;
        bool mCounting;

        Header mHeader;
    };
}

#endif
