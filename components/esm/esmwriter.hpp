#ifndef OPENMW_ESM_WRITER_H
#define OPENMW_ESM_WRITER_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include <zlib.h>

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

namespace CSMDoc
{
	class Document;
}

namespace ESM {

struct RefTransformOp
{
	std::string op_x, op_y, op_z;
	std::string op_rx, op_ry, op_rz;
	float fscale;
};

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

        void addMaster(const std::string& name, uint64_t size, bool updateOffset=true);

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

		bool startRecordTES4(const std::string& name, uint32_t flags = 0, uint32_t formID = 0, const std::string& stringID="");
		bool startRecordTES4(uint32_t name, uint32_t flags = 0, uint32_t formID = 0, const std::string& stringID="");
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
		std::map<std::string, struct RefTransformOp> mStringTransformMap;
		std::map<std::string, std::string> mStringTypeMap;

		bool evaluateOpString(std::string opString, float& opValue, const ESM::Position& opData);

		uint32_t mLowestAvailableID= 0x10001;
		uint32_t mLastReservedFormID=0;
		uint32_t getNextAvailableFormID();
		uint32_t getLastReservedFormID();
		uint32_t reserveFormID(uint32_t formID, const std::string& stringID, std::string sSIG="", bool setup_phase=false);
		void clearReservedFormIDs();
		uint32_t crossRefStringID(const std::string& mId, const std::string &sSIG, bool convertToEDID=true, bool creating_record=false);
		std::string crossRefFormID(uint32_t formID);

		uint32_t mESMoffset=0;

		static std::string generateEDIDTES4(const std::string& name, int conversion_mode, const std::string& sSIG="");
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

		static std::string substituteMorroblivionEDID(const std::string& genericEDID, const std::string &recordSIG);
		static std::string substituteMorroblivionEDID(const std::string& genericEDID, ESM::RecNameInts recordType);
		static std::map<std::string, std::string> mMorroblivionEDIDmap;
		std::map<std::string, std::pair<std::string, int>> unMatchedEDIDmap;

		std::map<std::string, int> mScriptHelperVarmap; // mwScriptHelper var index
		std::map<std::string, int> mLocalVarIndexmap; // local var, mwDialogHelper var index
		std::map<std::string, int> mUnresolvedLocalVars; // local var, occurences

		void exportConditionalExpression(std::string condExpression);
		void exportConditionalExpression(uint32_t compareFunction, uint32_t compareArg1,
			const std::string& compareOperator, float compareValue, uint8_t condFlags=0,
			uint32_t compareArg2=0);

		// Model List for NIF conversion batch file generation
		std::map<std::string, std::pair<std::string, int>> mModelsToExportList; // mModel string, model type
		std::vector<std::pair<std::string, std::pair<std::string, int>>> mArmorToExportList; // mModel string, model type

		void QueueModelForExport(std::string origString, std::string convertedString, int recordType=0);

		// Info, PNAM (Previous Record) map
		std::map<uint32_t, uint32_t> mPNAMINFOmap;
		// ESM Masters List for Header
		std::map<std::string, std::vector<std::string>> mESMMastersmap;

		// DDS List for export
		std::map<std::string, std::pair<std::string, int>> mDDSToExportList;
		// SOUND List for export
		std::map<std::string, std::pair<std::string, int>> mSOUNDToExportList;

		// BaseOjbect stringID map for creation of Persistent REFs
		std::map<std::string, std::pair<std::string, int>> mBaseObjToScriptedREFList;
		void RegisterBaseObjForScriptedREF(const std::string &stringID, std::string sSIG, int nMode=0);

		// Script stringID map for creation of ScriptToQuest records
		std::map<std::string, std::pair<std::string, int>> mScriptToQuestList; // scriptName, questName?, mode?
		void RegisterScriptToQuest(const std::string &scriptID, int nMode=0);
		std::map<std::string, int> mAutoStartScriptEDID;

		bool lookup_reference(const CSMDoc::Document &doc, const std::string &baseName, std::string &refEDID, std::string &refSIG, std::string &refValString);

		bool CompressNextRecord();
		std::string mConversionOptions;

    private:
        std::list<RecordData> mRecords;
		std::list<RecordData> mSubrecords;
		std::ostream* mStream;
        std::streampos mHeaderPos;
        ToUTF8::Utf8Encoder* mEncoder;
        int mRecordCount;
        bool mCounting;
		bool mCompressNextRecord;
		bool mEnableCompressionWriteRedirect;
		std::ofstream* mCompressionStream;
		char mTempfilename[MAX_PATH];

        Header mHeader;
    };
}

#endif
