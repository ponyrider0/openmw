#include "savingstate.hpp"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#include <boost/filesystem/fstream.hpp>

#include "operation.hpp"
#include "document.hpp"

CSMDoc::SavingState::SavingState (Operation& operation, const boost::filesystem::path& projectPath,
    ToUTF8::FromType encoding)
: mOperation (operation), mEncoder (encoding),  mProjectPath (projectPath), mProjectFile (false)
{
    mWriter.setEncoder (&mEncoder);
}

bool CSMDoc::SavingState::hasError() const
{
    return mOperation.hasError();
}

void CSMDoc::SavingState::start (Document& document, bool project)
{
    mProjectFile = project;

    if (mStream.is_open())
        mStream.close();

    mStream.clear();

    mSubRecords.clear();

    if (project)
        mPath = mProjectPath;
    else
        mPath = document.getSavePath();

    boost::filesystem::path file (mPath.filename().string() + ".tmp");

    mTmpPath = mPath.parent_path();

    mTmpPath /= file;
}

const boost::filesystem::path& CSMDoc::SavingState::getPath() const
{
    return mPath;
}

const boost::filesystem::path& CSMDoc::SavingState::getTmpPath() const
{
    return mTmpPath;
}

boost::filesystem::ofstream& CSMDoc::SavingState::getStream()
{
    return mStream;
}

ESM::ESMWriter& CSMDoc::SavingState::getWriter()
{
    return mWriter;
}

bool CSMDoc::SavingState::isProjectFile() const
{
    return mProjectFile;
}

std::map<std::string, std::deque<int> >& CSMDoc::SavingState::getSubRecords()
{
    return mSubRecords;
}

int CSMDoc::SavingState::loadEDIDmap(std::string filename)
{
	int errorcode=0;
	bool broken_mapping = false;

	if (filename == "tr_mainlandEDIDmap.csv")
		broken_mapping = true;

	// read and parse each line
	std::ifstream inputFile(filename);
	std::string inputLine;
	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);		
		std::string strRecordType, strModFilename, strHexFormID, strEDID;
		uint32_t formID;

		for (int i=0; i < 4; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');
			// remove double-quotes
			token.erase(0,1); token.erase(token.size()-1, 1);

			// assign token to string
			switch (i)
			{
			case 0:
				strRecordType = Misc::StringUtils::lowerCase(token);
				break;
			case 1:
				strModFilename = Misc::StringUtils::lowerCase(token);
				break;
			case 2:
				strHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			case 3:
				strEDID = Misc::StringUtils::lowerCase(token);
				break;
			}
		}

		// convert hex to integer
		if (strHexFormID.find("0x", 0) == 0)
		{
			std::istringstream hexToInt{ strHexFormID };
			hexToInt>> std::hex >> formID;
		}
		else
		{
			// error
			continue;
		}

		// calculate offset of FormID based on Modfilename
		uint32_t formIDOffset = 0;
		if (strModFilename == "oblivion.esm")
			formIDOffset = 0;
		else if (strModFilename == "morrowind_ob.esm")
			formIDOffset = 0x01000000;
		else if (strModFilename == "morrowind_ob - ucwus.esp")
			formIDOffset = 0x02000000;
		else if (strModFilename == "morroblivion_unofficial_patch.esp")
			formIDOffset = 0x03000000;
		else if (strModFilename == "tr_mainland")
			formIDOffset = 0x04000000;
		else if (strModFilename == "tr_preview")
			formIDOffset = 0x04000000;

		if (broken_mapping)
			formIDOffset = 0x04000000;

		formID |= formIDOffset;

		// make final double check, then add to FormID map
		std::string searchStr = mWriter.crossRefFormID(formID);
		uint32_t searchInt = mWriter.crossRefStringID(strEDID, "", false, true);

		if ( (formID != 0 && searchStr.size() == 0) && (strEDID != "" && searchInt == 0) )
		{
			// add to formID map
			uint32_t reserveResult = mWriter.reserveFormID(formID, strEDID, "", true);
			if (reserveResult != formID)
			{
				// error: formID couldn't be reserved??
				errorcode = -2;
				std::cout << "FormID Import ERROR: Unable to import [" << strEDID <<
					 "] to " << formID << ", assigned to " << reserveResult <<
					" instead." << std::endl;
				continue;
			}
		}
		else
		{
			// substition values were 0 or null OR...
			// formID found, make sure this is an override
			// TODO: detect if searchResults were positive and update a modified EDID...
			errorcode = -1;
			continue;
		}

	}

/*
	// hex-string to integer
	std::string your_string_rep{ "0x10FD7F04" };
	std::istringstream buffer{ your_string_rep };
	int value = 0;
	buffer >> std::hex >> value;

	// read comma separated buffer
	std::string input = "abc,def,ghi";
	std::istringstream ss(input);
	std::string token;
	while (std::getline(ss, token, ',')) {
		// std::cout << token << '\n';
	}

	// read input separated by white-space or /
	std::ifstream ifs("Your file.txt");
	std::vector<int> numbers;
	while (ifs)
	{
		while (ifs.peek() == ' ' || ifs.peek() == '/')
			ifs.get();
		int number;
		if (ifs >> number)
			numbers.push_back(number);
	}
*/

	return errorcode;
}

int CSMDoc::SavingState::loadCellIDmap(std::string filename)
{
	int errorcode = 0;

	// read and parse each line
	std::ifstream inputFile(filename);
	std::string inputLine;
	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strWorldspace, strX, strY, strHexFormID, strEDID, strFULL;
		uint32_t formID;
		int CellX, CellY;

		for (int i=0; i < 6; i++)
		{
			std::string token;
			std::getline(parserStream, token, '\t');
			// remove double-quotes
			token.erase(0, 1); token.erase(token.size()-1, 1);

			// assign token to string
			switch (i)
			{
			case 0:
				strWorldspace = Misc::StringUtils::lowerCase(token);
				break;
			case 1:
				strX = Misc::StringUtils::lowerCase(token);
				break;
			case 2:
				strY = Misc::StringUtils::lowerCase(token);
				break;
			case 3:
				strHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			case 4:
				strEDID = Misc::StringUtils::lowerCase(token);
				break;
			case 5:
				strFULL = Misc::StringUtils::lowerCase(token);
				break;
			}
		}

		// convert hex to integer
		if (strHexFormID.find("(", 0) == 0)
		{
			// remove parentheses
			strHexFormID.erase(0,1); strHexFormID.erase(strHexFormID.size()-1, 1);
			std::istringstream hexToInt{ strHexFormID };
			hexToInt>> std::hex >> formID;
		}
		else
		{
			// error
			continue;
		}

		// convert CellX, CellY
		CellX = atoi(strX.c_str());
		CellY = atoi(strY.c_str());

		if (strEDID == "wilderness")
			strEDID = "";

		if (strWorldspace == "wrldmorrowind" && formID != 0)
		{
			formID |= 0x01000000;
			// create 2D mapping...
            mExteriorCellMap[CellX][CellY].cellID = formID;
		}

	} // while getline(inputFile, inputLine)

	return errorcode;
}

uint32_t CSMDoc::SavingState::crossRefCellXY(int cellX, int cellY)
{
	uint32_t cellID=0;

	if ( mExteriorCellMap.find(cellX) != mExteriorCellMap.end() )
	{
        auto mapping = mExteriorCellMap[cellX].find(cellY);
		if ( mapping != mExteriorCellMap[cellX].end() )
		{
			cellID = mExteriorCellMap[cellX][cellY].cellID;
		}
	}

	return cellID;
}

uint32_t CSMDoc::SavingState::crossRefLandXY(int cellX, int cellY)
{
	uint32_t landscapeID=0;

	if (mExteriorCellMap.find(cellX) != mExteriorCellMap.end())
	{
        auto mapping = mExteriorCellMap[cellX].find(cellY);
		if (mapping != mExteriorCellMap[cellX].end())
		{
            landscapeID = mExteriorCellMap[cellX][cellY].landID;
		}
	}

	return landscapeID;
}

uint32_t CSMDoc::SavingState::crossRefPathgridXY(int cellX, int cellY)
{
	uint32_t pathgridID=0;

	if (mExteriorCellMap.find(cellX) != mExteriorCellMap.end())
	{
        auto mapping = mExteriorCellMap[cellX].find(cellY);
		if (mapping != mExteriorCellMap[cellX].end())
		{
            pathgridID = mExteriorCellMap[cellX][cellY].pathID;
		}
	}

	return pathgridID;
}

int CSMDoc::SavingState::loadEDIDmap2(std::string filename)
{
	int errorcode = 0;

	std::ifstream inputFile(filename);
	std::string inputLine;
	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strHexFormID, strEDID;
		uint32_t formID;

		for (int i=0; i < 2; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');

			// assign token to string
			switch (i)
			{
			case 0:
				strHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			case 1:
				strEDID = Misc::StringUtils::lowerCase(token);
				break;
			}
		}

		std::istringstream hexToInt{ strHexFormID };
		hexToInt>> std::hex >> formID;

		// make final double check, then add to FormID map
		std::string searchStr = mWriter.crossRefFormID(formID);
		uint32_t searchInt = mWriter.crossRefStringID(strEDID, "", false, true);

		if ((formID != 0 && searchStr.size() == 0) && (strEDID != "" && searchInt == 0))
		{
			// add to formID map
			uint32_t reserveResult = mWriter.reserveFormID(formID, strEDID, "", true);
			if (reserveResult != formID)
			{
				// error: formID couldn't be reserved??
				errorcode = -2;
				std::cout << "FormID Import ERROR: Unable to import [" << strEDID <<
					"] to " << formID << ", assigned to " << reserveResult <<
					" instead." << std::endl;
				continue;
			}
		}
		else
		{
			// substition values were 0 or null OR...
			// formID found, make sure this is an override
			// TODO: detect if searchResults were positive and update a modified EDID...
			errorcode = -1;
			continue;
		}

	}

	inputFile.close();

	return 0;
}

int CSMDoc::SavingState::loadCellIDmap2(std::string filename)
{
	int errorcode = 0;

	// read and parse each line
	std::ifstream inputFile(filename);
	std::string inputLine;
	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strX, strY, strHexFormID, strLandHexFormID, strEDID;
		uint32_t formID, landFormID;
		int CellX, CellY;

		for (int i=0; i < 5; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');

			// assign token to string
			switch (i)
			{
			case 0:
				strHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			case 1:
				strX = Misc::StringUtils::lowerCase(token);
				break;
			case 2:
				strY = Misc::StringUtils::lowerCase(token);
				break;
			case 3:
				strEDID = Misc::StringUtils::lowerCase(token);
				break;
			case 4:
				strLandHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			}
		}

		// convert hex to integer
		std::istringstream hexToInt{ strHexFormID };
		hexToInt>> std::hex >> formID;

		hexToInt.clear(); hexToInt.str(strLandHexFormID);
		hexToInt>> std::hex >> landFormID;

		// convert CellX, CellY
		CellX = atoi(strX.c_str());
		CellY = atoi(strY.c_str());

		mExteriorCellMap[CellX][CellY].cellID = formID;
		mExteriorCellMap[CellX][CellY].landID = landFormID;

	} // while getline(inputFile, inputLine)

	inputFile.close();

	return errorcode;
}

int CSMDoc::SavingState::loadPNAMINFOSubstitutionMap(std::string filename)
{
	int errorcode = 0;

	std::ifstream inputFile(filename);
	std::string inputLine;
	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strINFOID, strPNAMID;
		uint32_t formID, pnamID;

		for (int i = 0; i < 2; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');

			// assign token to string
			switch (i)
			{
			case 0:
				strINFOID = token;
				break;
			case 1:
				strPNAMID = token;
				break;
			}
		}

		if (strINFOID == "" || strPNAMID == "")
			continue;

		std::istringstream hexToInt{ strINFOID };
		hexToInt >> std::hex >> formID;

		hexToInt.clear(); hexToInt.str(strPNAMID);
		hexToInt >> std::hex >> pnamID;

		mWriter.mPNAMINFOmap[formID] = pnamID;

	}
	inputFile.close();

	return errorcode;
}

int CSMDoc::SavingState::loadmwEDIDSubstitutionMap(std::string filename)
{
	int errorcode = 0;

	std::ifstream inputFile(filename);
	std::string inputLine;
	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strMwEDID, strGenEDID;
		uint32_t formID;

		for (int i=0; i < 2; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');

			// assign token to string
			switch (i)
			{
			case 0:
				strMwEDID = token;
				break;
			case 1:
				strGenEDID = token;
				break;
			}
		}

		if (strMwEDID == "" || strGenEDID == "")
			continue;

		mWriter.mMorroblivionEDIDmap[strGenEDID] = strMwEDID;

	}
	inputFile.close();

	return errorcode;
}

int CSMDoc::SavingState::loadEDIDmap3(std::string filename)
{
	// display a period for progress feedback
	std::cout << "Importing '" << filename << "'" << std::flush;

	int errorcode = 0;
	int lineNumber = 0;

	std::ifstream inputFile(filename, std::ios_base::in);
    if ( (inputFile.rdstate() & std::iostream::failbit) != 0 )
    {
        std::cout << "...ERROR: can not open file." << std::endl;
        return -1;
    }

	std::string inputLine;
	
	// skip header line
	std::getline(inputFile, inputLine);

	while (std::getline(inputFile, inputLine, '\n'))
	{
//        std::cout << "." << std::flush;
		lineNumber++;

        // work-around: remove all line-feeds '\r'
        int linefeedPos;
        while ( (linefeedPos = inputLine.find('\r')) != std::string::npos)
        {
            inputLine.erase(linefeedPos, 1);
        }

		std::istringstream parserStream(inputLine);
		std::string strRecordType="", strModFileName="", strHexFormID="", numReferences="", strEDID="";
		std::string strPosOffset="", strRotOffset="", strScale="", strRefEDID="";
		uint32_t formID=0;

		for (int i = 0; i < 10; i++)
		{
			std::string token;
            parserStream.clear();
			std::getline(parserStream, token, ',');
//            if (token.size() != 0)
//                std::cout << "TOKEN[" << i << "]: [" << token << "] (size=" << token.size() << ")" << std::endl;
			// check for double-quote in token, if there is one, then parse by another double quote
			if (token.size() != 0 && token[0] == '\"')
			{
                // complete double-quoted token -- keep looping until double-quotes are closed
                while (token[token.size()-1] != '\"')
                {
                    if ( ((parserStream.rdstate() & std::iostream::failbit) != 0) ||
                        ((parserStream.rdstate() & std::iostream::eofbit) != 0) )
                    {
                        parserStream.clear();
                        break;
                    }
                    // assume token parsing has interrupted by an in-line comma
                    // so, first add missing in-line comma
                    token = token + ',';
                    // then try to obtain second half of token
                    std::string token2;
                    std::getline(parserStream, token2, ',');
                    token = token + token2;
                    if ( ((parserStream.rdstate() & std::iostream::failbit) != 0) ||
                        ((parserStream.rdstate() & std::iostream::eofbit) != 0) )
                    {
                        parserStream.clear();
                        break;
                    }
                }
                if (token[token.size()-1] == '\"')
                {
                    token.erase(0, 1); token.erase(token.size()-1, 1);
                }
                else
                {
                    // error parsing quoted token, skip to next line
                    std::cout << "Import Error (line " << lineNumber << "): could not parse quoted token: " << token << std::endl;
                    continue;
                }
			}

			Misc::StringUtils::lowerCaseInPlace(token);
			// assign token to string
			switch (i)
			{
			case 0:
				strRecordType = token;
				break;
			case 1:
				strModFileName = token;
				break;
			case 2:
				strEDID = token;
				break;
			case 3:
				numReferences = atoi(token.c_str());
				break;
			case 4:
				strHexFormID = token;
				break;
			case 5:
				// comments
				break;
			case 6:
				// position offset
				strPosOffset = token;
				break;
			case 7:
				// rotation offset
				strRotOffset = token;
				break;
			case 8:
				// scale
				strScale = token;
				break;
			case 9:
				// scale
				strRefEDID = token;
				break;
			}
		}

		std::istringstream hexToInt{ strHexFormID };
		hexToInt >> std::hex >> formID;

		if (formID == 0 || strEDID == "")
			continue;

		// make final double check, then add to FormID map
		std::string searchStr = mWriter.crossRefFormID(formID);
		uint32_t searchInt = mWriter.crossRefStringID(strEDID, "", false, true);

		if (searchStr == strEDID && searchInt == formID)
			continue;

		if (searchStr == strEDID && searchInt != 0)
		{
			std::cout << "WARNING: EDID substitution conflict: " << strEDID << " already assigned to " 
				<< std::hex << searchInt << ", new FormID: " << formID << std::endl;
		}

		if (strRecordType.size() != 4)
			strRecordType = "UNKN";

		// add to formID map
		uint32_t reserveResult = mWriter.reserveFormID(formID, strEDID, strRecordType, true);
		if (reserveResult != formID && reserveResult != 0)
		{
			// error: formID couldn't be reserved??
			errorcode = -2;
			std::cout << "FormID Import ERROR: Unable to import [" << strEDID << "] to "
				<< formID << ", assigned to " << reserveResult << " instead." << std::endl;
			continue;
		}

		if (strPosOffset != "" ||  strRotOffset != "" || strScale != "")
		{
			std::stringstream opstream;
			std::string opstring="";

			opstream.str(strPosOffset);
			opstring=""; std::getline(opstream, opstring, ',');
			mWriter.mStringTransformMap[strEDID].op_x = opstring;
			opstring = ""; std::getline(opstream, opstring, ',');
			mWriter.mStringTransformMap[strEDID].op_y = opstring;
			opstring = ""; std::getline(opstream, opstring, ',');
			mWriter.mStringTransformMap[strEDID].op_z = opstring;

			opstream.str(""); opstream.clear();
			opstream.str(strRotOffset);
			opstring = ""; std::getline(opstream, opstring, ',');
			mWriter.mStringTransformMap[strEDID].op_rx = opstring;
			opstring = ""; std::getline(opstream, opstring, ',');
			mWriter.mStringTransformMap[strEDID].op_ry = opstring;
			opstring = ""; std::getline(opstream, opstring, ',');
			mWriter.mStringTransformMap[strEDID].op_rz = opstring;

			float fscale;
			fscale = atof(strScale.c_str());
			mWriter.mStringTransformMap[strEDID].fscale = fscale;
		}

		if (strRefEDID != "")
		{
			if (strEDID.find("ref#") != std::string::npos)
			{
				uint32_t searchInt = mWriter.crossRefStringID(strRefEDID, "", false, true);
				if (searchInt == formID)
				{
					// skip... 
//					std::string errMessage = "LoadEDIDmap3(" + filename + "): refEDID already set to requested formID: " + strRefEDID + "\n";
//					OutputDebugString(errMessage.c_str());
				}
				else
				{
					reserveResult = mWriter.reserveFormID(formID, strRefEDID, strRecordType, true);
				}
			}
		}

	}

	inputFile.close();
    std::cout << "...complete." << std::endl;

	return errorcode;
}

int CSMDoc::SavingState::loadCellIDmap3(std::string filename)
{
	// display a period for progress feedback
//	std::cout << ".";

	int errorcode = 0;

	// read and parse each line
	std::ifstream inputFile(filename);
	std::string inputLine;

	// skip header line
	std::getline(inputFile, inputLine);

	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strX, strY, strHexFormID, strLandHexFormID, strPathgridHexFormID,strEDID;
		uint32_t formID, landFormID, pathgridID;
		int CellX, CellY;

		for (int i = 0; i < 6; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');

			// assign token to string
			switch (i)
			{
			case 0:
				strHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			case 1:
				strX = Misc::StringUtils::lowerCase(token);
				break;
			case 2:
				strY = Misc::StringUtils::lowerCase(token);
				break;
			case 3:
				strEDID = token;
				break;
			case 4:
				strLandHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			case 5:
				strPathgridHexFormID = Misc::StringUtils::lowerCase(token);
				break;
			}
		}

		// convert hex to integer
		std::istringstream hexToInt{ strHexFormID };
		hexToInt >> std::hex >> formID;

		hexToInt.clear(); hexToInt.str(strLandHexFormID);
		hexToInt >> std::hex >> landFormID;

		hexToInt.clear(); hexToInt.str(strPathgridHexFormID);
		hexToInt >> std::hex >> pathgridID;

		// convert CellX, CellY
		CellX = atoi(strX.c_str());
		CellY = atoi(strY.c_str());

		mExteriorCellMap[CellX][CellY].cellID = formID;
		mExteriorCellMap[CellX][CellY].landID = landFormID;
		mExteriorCellMap[CellX][CellY].pathID = pathgridID;

		// create stringIDMap entries
		std::ostringstream generatedCellID;
		generatedCellID << "#" << CellX << " " << CellY;
		mWriter.reserveFormID(formID, generatedCellID.str(), "CELL", true);
		if (landFormID != 0)
		{
			mWriter.reserveFormID(formID, generatedCellID.str() + "-landscape", "LAND", true);
		}
		if (pathgridID != 0)
		{
			mWriter.reserveFormID(formID, generatedCellID.str() + "-pathgrid", "PGRD", true);
		}

	} // while getline(inputFile, inputLine)

	inputFile.close();

	return errorcode;
}

int CSMDoc::SavingState::loadLocalVarIndexmap(std::string filename)
{
	int errorcode = 0;
	int lineNumber = 0;

	std::ifstream inputFile(filename);
	std::string inputLine;

	// skip header line
	std::getline(inputFile, inputLine);

	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strVarName, strVarIndex;
		int varIndex;

		for (int i = 0; i < 3; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');

			std::string dump;
			std::istringstream tokenStr;
			// assign token to string
			switch (i)
			{
			case 0:
//				strVarName = token;
				tokenStr.str(token);
				tokenStr >> dump >> strVarName; // dump var type (short, long, float)
				break;
			case 1:
				strVarIndex = token;
				break;
			}
		}

		if (strVarName == "" || strVarIndex == "")
			continue;

		varIndex = atoi( strVarIndex.c_str() );
		

		mWriter.mLocalVarIndexmap[Misc::StringUtils::lowerCase(strVarName)] = varIndex;
	}


	return errorcode;
}

int CSMDoc::SavingState::loadScriptHelperVarMap(std::string filename)
{
	int errorcode = 0;
	int lineNumber = 0;

	std::ifstream inputFile(filename);
	std::string inputLine;

	// skip header line
	std::getline(inputFile, inputLine);

	while (std::getline(inputFile, inputLine))
	{
		std::istringstream parserStream(inputLine);
		std::string strVarName, strVarIndex;
		int varIndex;

		for (int i = 0; i < 3; i++)
		{
			std::string token;
			std::getline(parserStream, token, ',');

			std::istringstream tokenStr;
			// assign token to string
			switch (i)
			{
			case 0:
				//				strVarName = token;
				tokenStr.str(token);
				tokenStr >> strVarName;
				break;
			case 1:
				strVarIndex = token;
				break;
			}
		}

		if (strVarName == "" || strVarIndex == "")
			continue;

		varIndex = atoi(strVarIndex.c_str());


		mWriter.mScriptHelperVarmap[Misc::StringUtils::lowerCase(strVarName)] = varIndex;
	}


	return errorcode;
}


int CSMDoc::SavingState::loadESMMastersMap(std::string filename)
{
	int errorcode = 0;
	int lineNumber = 0;

	std::ifstream inputFile(filename);
	std::string inputLine_noN;

	// skip header line
	std::getline(inputFile, inputLine_noN);

	while (std::getline(inputFile, inputLine_noN, '\n'))
	{
		std::istringstream inputLine_Str(inputLine_noN);
		std::string inputLine_noR;

        while (std::getline(inputLine_Str, inputLine_noR, '\r'))
        {
            std::istringstream parserStream(inputLine_noR);
            std::string strESMName;

            std::string token;
            std::istringstream tokenStr;

            std::getline(parserStream, token, ',');
            strESMName = token;

            std::vector<std::string> ESMmasterList;
            while (std::getline(parserStream, token, ',') )
            {
                if (token.size() > 5)
                    ESMmasterList.push_back(token);
            }
            
            mWriter.mESMMastersmap[Misc::StringUtils::lowerCase(strESMName)] = ESMmasterList;

        }

	}

	return errorcode;
}

int CSMDoc::SavingState::initializeSubstitutions(std::string esmName)
{
/*
	loadEDIDmap("oblivionEDIDmap.csv");
	loadEDIDmap("morrowindEDIDmap.csv");
	loadEDIDmap("morroblivion-ucwusEDIDmap.csv");
	loadEDIDmap("morroblivion-fixesEDIDmap.csv");
	loadEDIDmap("tr_mainlandEDIDmap.csv");
	loadCellIDmap("morroblivionCellIDmap.tsv");
*/
//	loadEDIDmap2("OblivionFormIDlist.csv");
//	loadEDIDmap2("MorroblivionFormIDlist.csv");
//	loadEDIDmap2("Morroblivion-UCWUSFormIDlist.csv");
//	loadEDIDmap2("Morroblivion-FixesFormIDlist.csv");

	std::cout << "Loading CSV files...." << std::endl;

#ifdef win32
    std::string csvRoot = "";
#else
    std::string csvRoot = getenv("HOME");
    csvRoot += "/";
#endif

	loadEDIDmap3(csvRoot + "OblivionFormIDlist4.csv");
	loadEDIDmap3(csvRoot + "MorroblivionFormIDlist4.csv");
//	loadEDIDmap3("Morroblivion-UCWUSFormIDlist4.csv");
//	loadEDIDmap3("Morroblivion-FixesFormIDlist4.csv");
	loadEDIDmap3(csvRoot + "Morrowind_Compatibility_LayerEDIDlist.csv");

//	loadCellIDmap2("MorroblivionCellIDmap.csv");
//	loadEDIDmap2("TR_MainlandFormIDlist.csv");
//	loadCellIDmap2("TR_MainlandCellIDmap.csv");
	loadCellIDmap3(csvRoot + "MorroblivionCellmap4.csv");

	loadmwEDIDSubstitutionMap(csvRoot + "GenericToMorroblivionEDIDmapLTEX.csv");
	loadmwEDIDSubstitutionMap(csvRoot + "GenericToMorroblivionEDIDmapCREA.csv");

	if (esmName.find("Tamriel_Data") != std::string::npos)
	{
		loadEDIDmap3(csvRoot + "TamrielDataEDIDlist.csv");
	}
	if (esmName.find("TR_Mainland") != std::string::npos) 
	{
		loadEDIDmap3(csvRoot + "TamrielDataEDIDlist.csv");
		loadEDIDmap3(csvRoot + "TRMainlandEDIDlist.csv");

//		loadPNAMINFOSubstitutionMap("TRMainlandGreetingPNAMINFOmap.csv");
		loadPNAMINFOSubstitutionMap(csvRoot + "TRMainland_Dialog_PNAMINFO.csv");
	}
	if (esmName.find("TR_Preview") != std::string::npos)
	{
		loadEDIDmap3(csvRoot + "TamrielDataEDIDlist.csv");
		loadEDIDmap3(csvRoot + "TRPreviewEDIDlist.csv");
	}

	loadEDIDmap3(csvRoot + "OverridesEDIDList.csv");
	loadEDIDmap3(csvRoot + "Overrides_" + esmName + ".csv");
	loadLocalVarIndexmap(csvRoot + "LocalVarMap.csv");
	loadScriptHelperVarMap(csvRoot + "ScriptHelperVarMap.csv");
	loadESMMastersMap(csvRoot + "ESMMastersMap.csv");

	std::cout << "Loading CSV files complete." << std::endl;

	return 0;
}
