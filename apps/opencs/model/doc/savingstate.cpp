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
		else if (strModFilename == "morroblivion-fixes.esp")
			formIDOffset = 0x03000000;
		else if (strModFilename == "tr_mainland00.esp")
			formIDOffset = 0x04000000;
		else if (strModFilename == "tr_preview00.esp")
			formIDOffset = 0x04000000;

		if (broken_mapping)
			formIDOffset = 0x04000000;

		formID |= formIDOffset;

		// make final double check, then add to FormID map
		std::string searchStr = mWriter.crossRefFormID(formID);
		uint32_t searchInt = mWriter.crossRefStringID(strEDID);
		if ( (formID != 0 && searchStr.size() == 0) && (strEDID != "" && searchInt == 0) )
		{
			// add to formID map
			uint32_t reserveResult = mWriter.reserveFormID(formID, strEDID, true);
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
			mExteriorCellMap[CellX][CellY][0] = formID;
		}

	} // while getline(inputFile, inputLine)

	return errorcode;
}

uint32_t CSMDoc::SavingState::crossRefCellXY(int cellX, int cellY)
{
	uint32_t retVal=0;

	if ( mExteriorCellMap.find(cellX) != mExteriorCellMap.end() )
	{	
		if ( mExteriorCellMap[cellX].find(cellY) != mExteriorCellMap[cellX].end() )
		{
			retVal = mExteriorCellMap[cellX][cellY][0];
		}
	}

	return retVal;
}

uint32_t CSMDoc::SavingState::crossRefLandXY(int cellX, int cellY)
{
	uint32_t retVal=0;

	if (mExteriorCellMap.find(cellX) != mExteriorCellMap.end())
	{
		if (mExteriorCellMap[cellX].find(cellY) != mExteriorCellMap[cellX].end())
		{
			retVal = mExteriorCellMap[cellX][cellY][1];
		}
	}

	return retVal;
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
		uint32_t searchInt = mWriter.crossRefStringID(strEDID);

		if ((formID != 0 && searchStr.size() == 0) && (strEDID != "" && searchInt == 0))
		{
			// add to formID map
			uint32_t reserveResult = mWriter.reserveFormID(formID, strEDID, true);
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

//		mExteriorCellMap[CellX][CellY] = std::vector<uint32_t>();
		mExteriorCellMap[CellX][CellY][0] = formID;
		mExteriorCellMap[CellX][CellY][1] = landFormID;

	} // while getline(inputFile, inputLine)

	return 0;
}

int CSMDoc::SavingState::initializeSubstitutions()
{
/*
	loadEDIDmap("oblivionEDIDmap.csv");
	loadEDIDmap("morrowindEDIDmap.csv");
	loadEDIDmap("morroblivion-ucwusEDIDmap.csv");
	loadEDIDmap("morroblivion-fixesEDIDmap.csv");
	loadEDIDmap("tr_mainlandEDIDmap.csv");
	loadCellIDmap("morroblivionCellIDmap.tsv");
*/
	loadEDIDmap2("OblivionFormIDlist.csv");
	loadEDIDmap2("MorroblivionFormIDlist.csv");
	loadEDIDmap2("Morroblivion-UCWUSFormIDlist.csv");
	loadEDIDmap2("Morroblivion-FixesFormIDlist.csv");
	loadEDIDmap2("TR_MainlandFormIDlist.csv");
	loadCellIDmap2("MorroblivionCellIDmap.csv");
	loadCellIDmap2("TR_MainlandCellIDmap.csv");

	return 0;
}
