#include "savingstate.hpp"

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

int CSMDoc::SavingState::loadSubstitutionMap(std::string filename)
{
	int errorcode=0;
		
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
		else if (strModFilename == "tr_mainland.esp")
			formIDOffset = 0x04000000;
		else if (strModFilename == "tr_preview.esp")
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

int CSMDoc::SavingState::initializeSubstitutions()
{
	loadSubstitutionMap("oblivionEDIDmap.csv");
	loadSubstitutionMap("morrowindEDIDmap.csv");
	loadSubstitutionMap("morroblivion-ucwusEDIDmap.csv");
	loadSubstitutionMap("morroblivion-fixesEDIDmap.csv");

	return 0;
}
