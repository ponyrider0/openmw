#include "loadscpt.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

#include <iostream>
#include <apps/opencs/model/doc/scriptconverter.hpp>


namespace ESM
{
    unsigned int Script::sRecordId = REC_SCPT;

    void Script::loadSCVR(ESMReader &esm)
    {
        int s = mData.mStringTableSize;

        std::vector<char> tmp (s);
        // not using getHExact, vanilla doesn't seem to mind unused bytes at the end
        esm.getSubHeader();
        int left = esm.getSubSize();
        if (left < s)
            esm.fail("SCVR string list is smaller than specified");
        esm.getExact(&tmp[0], s);
        if (left > s)
            esm.skip(left-s); // skip the leftover junk

        // Set up the list of variable names
        mVarNames.resize(mData.mNumShorts + mData.mNumLongs + mData.mNumFloats);

        // The tmp buffer is a null-byte separated string list, we
        // just have to pick out one string at a time.
        char* str = &tmp[0];
        for (size_t i = 0; i < mVarNames.size(); i++)
        {
            // Support '\r' terminated strings like vanilla.  See Bug #1324.
            char *termsym = strchr(str, '\r');
            if(termsym) *termsym = '\0';
            mVarNames[i] = std::string(str);
            str += mVarNames[i].size() + 1;

            if (str - &tmp[0] > s)
            {
                // Apparently SCVR subrecord is not used and variable names are
                // determined on the fly from the script text.  Therefore don't throw
                // an exeption, just log an error and continue.
                std::stringstream ss;

                ss << "ESM Error: " << "String table overflow";
                ss << "\n  File: " << esm.getName();
                ss << "\n  Record: " << esm.getContext().recName.toString();
                ss << "\n  Subrecord: " << "SCVR";
                ss << "\n  Offset: 0x" << std::hex << esm.getFileOffset();
                std::cerr << ss.str() << std::endl;
                break;
            }

        }
    }

    void Script::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mVarNames.clear();

        bool hasHeader = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'S','C','H','D'>::value:
                {
                    SCHD data;
                    esm.getHT(data, 52);
                    mData = data.mData;
                    mId = data.mName.toString();
                    hasHeader = true;
                    break;
                }
                case ESM::FourCC<'S','C','V','R'>::value:
                    // list of local variables
                    loadSCVR(esm);
                    break;
                case ESM::FourCC<'S','C','D','T'>::value:
                {
                    // compiled script
                    esm.getSubHeader();
                    uint32_t subSize = esm.getSubSize();

                    if (subSize != static_cast<uint32_t>(mData.mScriptDataSize))
                    {
                        std::stringstream ss;
                        ss << "ESM Warning: Script data size defined in SCHD subrecord does not match size of SCDT subrecord";
                        ss << "\n  File: " << esm.getName();
                        ss << "\n  Offset: 0x" << std::hex << esm.getFileOffset();
                        std::cerr << ss.str() << std::endl;
                    }

                    mScriptData.resize(subSize);
                    esm.getExact(&mScriptData[0], mScriptData.size());
                    break;
                }
                case ESM::FourCC<'S','C','T','X'>::value:
                    mScriptText = esm.getHString();
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasHeader)
            esm.fail("Missing SCHD subrecord");
    }

    void Script::save(ESMWriter &esm, bool isDeleted) const
    {
        std::string varNameString;
        if (!mVarNames.empty())
            for (std::vector<std::string>::const_iterator it = mVarNames.begin(); it != mVarNames.end(); ++it)
                varNameString.append(*it);

        SCHD data;
        memset(&data, 0, sizeof(data));

        data.mData = mData;
        data.mName.assign(mId);

        esm.writeHNT("SCHD", data, 52);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        if (!mVarNames.empty())
        {
            esm.startSubRecord("SCVR");
            for (std::vector<std::string>::const_iterator it = mVarNames.begin(); it != mVarNames.end(); ++it)
            {
                esm.writeHCString(*it);
            }
            esm.endRecord("SCVR");
        }

        esm.startSubRecord("SCDT");
        esm.write(reinterpret_cast<const char * >(&mScriptData[0]), mData.mScriptDataSize);
        esm.endRecord("SCDT");

        esm.writeHNOString("SCTX", mScriptText);
    }

	void Script::exportTESx(ESMWriter & esm, int export_type) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
//		std::istringstream inputScriptText(mScriptText);
//		std::ostringstream convertedScriptText;

		// EDID
		strEDID = esm.generateEDIDTES4(mId, 3, "SCPT");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

		ESM::ScriptConverter scriptConverter(mScriptText, esm);

		// SCHR... (basic script data)
		// [unused x4, refcount, compiled size, varcount, script type]
		uint32_t refCount = scriptConverter.mReferenceList.size();
		uint32_t compiledSize = scriptConverter.GetByteBufferSize();
		uint32_t varCount = scriptConverter.mLocalVarList.size();
		uint32_t scriptType = 0x0; // default = object script
		if (Misc::StringUtils::lowerCase(strEDID).find("quest") != std::string::npos)
			scriptType = 1; // quest script
		if (Misc::StringUtils::lowerCase(strEDID).find("effect") != std::string::npos)
			scriptType = 0x100; // effect script

		// SCHD (unknown script header)

		// SCHR data...
/*/
		esm.startSubRecordTES4("SCHR");
		esm.writeT<uint32_t>(0); // unused x4
		esm.writeT<uint32_t>(refCount); // refcount (uint32)
		esm.writeT<uint32_t>(0); // compiledsize
		esm.writeT<uint32_t>(varCount); // var count (uint32)
		esm.writeT<uint32_t>(scriptType); // type (uint32) 0=object, 1=quest, $100 = magic effect
		esm.endSubRecordTES4("SCHR");
*/
		esm.startSubRecordTES4("SCHR");
		esm.writeT<uint32_t>(0); // unused byte * 4
		esm.writeT<uint32_t>(refCount);
		esm.writeT<uint32_t>(compiledSize);
		esm.writeT<uint32_t>(varCount);
		esm.writeT<uint32_t>(scriptType);
		esm.endSubRecordTES4("SCHR");

		// SCDA (compiled)
		esm.startSubRecordTES4("SCDA");
		esm.write(scriptConverter.GetCompiledByteBuffer(), scriptConverter.GetByteBufferSize());
		esm.endSubRecordTES4("SCDA");

		// SCTX (text)
/*
		convertedScriptText << "ScriptName " << strEDID << std::endl << std::endl;
		std::string inputLine;
		while (std::getline(inputScriptText, inputLine))
		{
			convertedScriptText << "; " << inputLine << std::endl;
		}
		esm.startSubRecordTES4("SCTX");
		esm.writeHCString(convertedScriptText.str());
		esm.endSubRecordTES4("SCTX");
*/
		esm.startSubRecordTES4("SCTX");
		esm.writeHCString(scriptConverter.GetConvertedScript());
		esm.endSubRecordTES4("SCTX");

		// local variables
		uint32_t localVarIndex=1;
		for (auto localVar = scriptConverter.mLocalVarList.begin(); localVar != scriptConverter.mLocalVarList.end(); localVar++)
		{
			uint32_t flagVarType = 0x0;
			if (Misc::StringUtils::lowerCase( localVar->first ) == "short" ||
				Misc::StringUtils::lowerCase( localVar->first ) == "long" )
				flagVarType = 0x1;

			// SLSD struct [index, unused x12, flags, unused]
			esm.startSubRecordTES4("SLSD");
			esm.writeT<uint32_t>(localVarIndex++);
			esm.writeT<uint32_t>(0); // unused byte * 12 
			esm.writeT<uint32_t>(0); // ...
			esm.writeT<uint32_t>(0); // ...
			esm.writeT<uint8_t>(flagVarType); // var type
			esm.writeT<uint32_t>(0); // unused byte * 4
			esm.endSubRecordTES4("SLSD");
			// SCVR string
			esm.startSubRecordTES4("SCVR");
			esm.writeHCString(localVar->second);
			esm.endSubRecordTES4("SCVR");
		}

		// SCROs
		for (auto refItem = scriptConverter.mReferenceList.begin(); refItem != scriptConverter.mReferenceList.end(); refItem++)
		{
			esm.startSubRecordTES4("SCRO");
			esm.writeT<uint32_t>(*refItem);
			esm.endSubRecordTES4("SCRO");
		}

	}

    void Script::blank()
    {
        mData.mNumShorts = mData.mNumLongs = mData.mNumFloats = 0;
        mData.mScriptDataSize = 0;
        mData.mStringTableSize = 0;

        mVarNames.clear();
        mScriptData.clear();

        if (mId.find ("::")!=std::string::npos)
            mScriptText = "Begin \"" + mId + "\"\n\nEnd " + mId + "\n";
        else
            mScriptText = "Begin " + mId + "\n\nEnd " + mId + "\n";
    }

}
