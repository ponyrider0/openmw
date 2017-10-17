#include "scriptconverter.hpp"

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#include <sstream>
#include <stdexcept>

#include <components\esm\esmwriter.hpp>
#include <components/misc/stringops.hpp>
#include <components/to_utf8/to_utf8.hpp>

namespace ESM
{

	ScriptConverter::ScriptConverter(const std::string & scriptText)
	{
		mScriptText = scriptText;
		lexer();
		parser();
	}

	std::string ScriptConverter::GetConvertedScript()
	{
		std::string oldCode, scriptline;
		std::stringstream scriptBuf(mScriptText);
		std::stringstream exportStr;

		while (std::getline(scriptBuf, scriptline))
		{
			oldCode += "; " + scriptline;
		}

		for (auto statement = mConvertedStatementList.begin(); statement != mConvertedStatementList.end(); statement++)
		{
			exportStr << *statement << std::endl;
		}
		exportStr << std::endl << oldCode;

		return std::string(exportStr.str());
	}

	bool ScriptConverter::PerformGoodbye()
	{
		return bGoodbye;
	}

	void ScriptConverter::read_line(const std::string& lineBuffer)
	{
		while (mLinePosition < lineBuffer.size())
		{
			// ready state
			if (mReadMode == 0)
			{
				// skip the rest of the line if commented out
				if (lineBuffer[mLinePosition] == ';')
				{
					break;
				}
				// switch to quoted string literal mode
				if (lineBuffer[mLinePosition] == '\"')
				{
					mReadMode = 2;
					continue;
				}
				// switch to token mode
				if ((lineBuffer[mLinePosition] >= '0' && lineBuffer[mLinePosition] <= '9') ||
					(lineBuffer[mLinePosition] >= 'a' && lineBuffer[mLinePosition] <= 'z') ||
					(lineBuffer[mLinePosition] >= 'A' && lineBuffer[mLinePosition] <= 'Z'))
				{
					mReadMode = 1;
					continue;
				}
				// skip everything else
				mLinePosition++;
			}
			if (mReadMode == 1)
				read_identifier(lineBuffer);
			if (mReadMode == 2)
				read_quotedliteral(lineBuffer);
		}
		mTokenList.push_back(Token(TokenType::endlineT, ""));

	}

	void ScriptConverter::read_quotedliteral(const std::string & lineBuffer)
	{
		bool done = false;
		bool checkQuote = false;
		int tokenType = TokenType::string_literalT;
		std::string stringLiteral;
		mLinePosition++; // skip first quote
		while (mLinePosition < lineBuffer.size())
		{
			if (checkQuote == false)
			{
				if (lineBuffer[mLinePosition] != '\"')
				{
					stringLiteral += lineBuffer[mLinePosition++];
					continue;
				}
				else
				{
					checkQuote = true;
					mLinePosition++;
					continue;
				}
			}
			else if (checkQuote == true)
			{
				if (lineBuffer[mLinePosition] == '\"')
				{
					stringLiteral += lineBuffer[mLinePosition++];
					checkQuote = false;
					continue;
				}
				else
				{
					// ignore probably bad double-quote
					if ((stringLiteral == "") &&
						(lineBuffer[mLinePosition] >= '0' && lineBuffer[mLinePosition] <= '9') ||
						(lineBuffer[mLinePosition] >= 'a' && lineBuffer[mLinePosition] <= 'z') ||
						(lineBuffer[mLinePosition] >= 'A' && lineBuffer[mLinePosition] <= 'Z'))
					{
						// issue warning
						std::cout << "WARNING: Script Reader: unexpected double-quote in string literal:[" << mLinePosition << "] " << lineBuffer << std::endl;
						mLinePosition++;
						checkQuote = false;
						continue;
					}
					// stringLiteral complete
					done = true;
					break;
				}
			}
		}
		if (done == true || (checkQuote == true && mLinePosition == lineBuffer.size()))
		{
			// add stringLiteral to tokenList and return to ready state
			mTokenList.push_back(Token(tokenType, stringLiteral));
			mReadMode = 0;
		}
		else
		{
			// error reading string literal
			std::stringstream errorMesg;
			errorMesg << "ERROR: Script Reader: unexpected end of line while reading string literal:[" << mLinePosition << "] " << lineBuffer;
			std::cout << errorMesg.str() << std::endl;
			//			throw std::runtime_error(errorMesg.str());
			mReadMode = 0;
			// skip to next line by advancing mLinePosition
			mLinePosition = lineBuffer.size();
		}

	}

	void ScriptConverter::read_identifier(const std::string & lineBuffer)
	{
		bool done = false;
		int tokenType = TokenType::number_literalT;
		std::string tokenStr;
		while (mLinePosition < lineBuffer.size())
		{
			if (lineBuffer[mLinePosition] >= '0' && lineBuffer[mLinePosition] <= '9')
			{
				tokenStr += lineBuffer[mLinePosition++];
			}
			else if ((lineBuffer[mLinePosition] >= 'a' && lineBuffer[mLinePosition] <= 'z') ||
				(lineBuffer[mLinePosition] >= 'A' && lineBuffer[mLinePosition] <= 'Z') ||
				(lineBuffer[mLinePosition] == '_') )
			{
				tokenType = TokenType::identifierT; // command/variable
				tokenStr += lineBuffer[mLinePosition++];
			}
			else if (lineBuffer[mLinePosition] == '-')
			{
				if ( (mLinePosition+1 < lineBuffer.size()) && (
					(lineBuffer[mLinePosition+1] >= 'a' && lineBuffer[mLinePosition+1] <= 'z') ||
					(lineBuffer[mLinePosition+1] >= 'A' && lineBuffer[mLinePosition+1] <= 'Z') ||
					(lineBuffer[mLinePosition+1] == '_') ) )
				{
					tokenType = TokenType::identifierT; // command/variable
					tokenStr += lineBuffer[mLinePosition++];
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		if (tokenStr != "")
		{
			mTokenList.push_back(Token(tokenType, tokenStr));
		}
		mReadMode = 0;

	}

	// Generic placeholder function
	void ScriptConverter::parse_keyword(std::vector<struct Token>::iterator& tokenItem)
	{
		std::string itemEDID, itemCount;

		// check keyword
		tokenItem++;

		if (mParseMode != 5 && mParseMode != 6)
		{
			// skip to end and return
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}

		// object EDID
		if (tokenItem->type == TokenType::string_literalT ||
			tokenItem->type == TokenType::identifierT)
		{
			itemEDID = ESM::ESMWriter::generateEDIDTES4(tokenItem->str, 0, "");
		}
		else
		{
			// error parsing journal
			std::stringstream errorMesg;
			errorMesg << "ERROR: ScriptConverter:parse_keyword - unexpected token type, expected string_literal: " << tokenItem->type;
			std::cout << errorMesg.str() << std::endl;
			// throw std::runtime_error(errorMesg.str());
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}
		tokenItem++;

		// quest stage
		if (tokenItem->type == TokenType::number_literalT)
		{
			itemCount = tokenItem->str;
		}
		else
		{
			// error parsing journal
			std::stringstream errorMesg;
			errorMesg << "ERROR: ScriptConverter:parse_keyword - unexpected token type, expected number_literalT: " << tokenItem->type;
			std::cout << errorMesg.str() << std::endl;
			// throw std::runtime_error(errorMesg.str());
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}
		tokenItem++;

		// record SetStage expression
		std::stringstream convertedStatement;
		std::string command;
		command = "AddItem";
		if (mParseMode == 6)
			command = "RemoveItem";
		if (bParserUsePlayerRef)
		{
			command = "PlayerRef." + command;
		}
		convertedStatement << command << " " << itemEDID << " " << itemCount;
		mConvertedStatementList.push_back(convertedStatement.str());

		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;

	}

	void ScriptConverter::parse_choice(std::vector<struct Token>::iterator& tokenItem)
	{
		std::string choiceText;
		int choiceNum;

		// skip tokenItem "choice"
		tokenItem++;

		// stop processing if detecting endofline
		while (tokenItem->type != TokenType::endlineT)
		{

			// process string literal + number literal pairs
			if (tokenItem->type == TokenType::string_literalT)
				choiceText = tokenItem->str;
			else
			{
				// error parsing choice statement
				std::stringstream errorMesg;
				errorMesg << "ERROR: ScriptConverter: Parser - unexpected token type, expected string_literal: " << tokenItem->type;
				std::cout << errorMesg.str() << std::endl;
				//			throw std::runtime_error(errorMesg.str());
				mParseMode = 0;
				// advance to endofline in stream
				while (tokenItem->type != TokenType::endlineT)
					tokenItem++;
				return;
			}
			tokenItem++; // advance to next token

			if (tokenItem->type == TokenType::number_literalT)
				choiceNum = atoi(tokenItem->str.c_str());
			else
			{
				std::stringstream errorMesg;
				errorMesg << "ERROR: ScriptConverter: Parser - unexpected token type, expected number_literal: " << tokenItem->type;
				std::cout << errorMesg.str() << std::endl;
				//			throw std::runtime_error(errorMesg.str());
				mParseMode = 0;
				// advance to endofline in stream
				while (tokenItem->type != TokenType::endlineT)
					tokenItem++;
				return;
			}
			tokenItem++; // advance to next token

						 // add Choice Text:Number pair to choicetopicnames list
			mChoicesList.push_back(std::make_pair(choiceNum, choiceText));
		}
		mParseMode = 0;

	}

	void ScriptConverter::parse_journal(std::vector<struct Token>::iterator& tokenItem)
	{
		std::string questEDID, questStage;
		// skip journal command
		tokenItem++;

		// quest EDID
		if (tokenItem->type == TokenType::string_literalT ||
			tokenItem->type == TokenType::identifierT)
		{
			questEDID = ESM::ESMWriter::generateEDIDTES4(tokenItem->str, 0, "QUST");
		}
		else
		{
			// error parsing journal
			std::stringstream errorMesg;
			errorMesg << "ERROR: ScriptConverter:parse_journal() - unexpected token type, expected string_literal: " << tokenItem->type;
			std::cout << errorMesg.str() << std::endl;
			// throw std::runtime_error(errorMesg.str());
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}
		tokenItem++;

		// quest stage
		if (tokenItem->type == TokenType::number_literalT)
		{
			questStage = tokenItem->str;
		}
		else
		{
			// error parsing journal
			std::stringstream errorMesg;
			errorMesg << "ERROR: ScriptConverter:parse_journal() - unexpected token type, expected number_literalT: " << tokenItem->type;
			std::cout << errorMesg.str() << std::endl;
			// throw std::runtime_error(errorMesg.str());
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}
		tokenItem++;

		// record SetStage expression
		std::stringstream convertedStatement;
		convertedStatement << "SetStage " << questEDID << " " << questStage;
		mConvertedStatementList.push_back(convertedStatement.str());

		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;

	}

	void ScriptConverter::lexer()
	{
		std::istringstream scriptBuffer(mScriptText);
		std::string preLineBufferA;
		while (std::getline(scriptBuffer, preLineBufferA, '\n'))
		{
			std::istringstream preLineBufferB(preLineBufferA);
			std::string lineBuffer;
			while (std::getline(preLineBufferB, lineBuffer, '\r'))
			{
				mLinePosition = 0;
				mReadMode = 0;
				read_line(lineBuffer);
			}
		}
	}

	void ScriptConverter::parser()
	{
		mParseMode = 0;

		for (auto tokenItem = mTokenList.begin(); tokenItem != mTokenList.end(); tokenItem++)
		{
			// TODO: keywords = moddisposition, goodbye, setfight, additem, showmap, "set control to 1", getJournalIndex, getdisposition, addtopic, 
			//			modpcfacrep, journal, pcraiserank, pcclearexpelled, messagebox, addspell
			if (mParseMode == 0 || mParseMode == 1)
			{
				if (mParseMode == 0)
					bParserUsePlayerRef = false;

				if (tokenItem->type == TokenType::identifierT && Misc::StringUtils::lowerCase(tokenItem->str) == "player")
				{
					bParserUsePlayerRef = true;
					mParseMode = 1;
					continue;
				}
				if (tokenItem->type == TokenType::identifierT && Misc::StringUtils::lowerCase(tokenItem->str) == "choice")
				{
					mParseMode = 2;
					// put back token
					tokenItem--;
					continue;
				}
				if (tokenItem->type == TokenType::identifierT && Misc::StringUtils::lowerCase(tokenItem->str) == "journal")
				{
					mParseMode = 3;
					// put back token
					tokenItem--;
					continue;
				}
				if (tokenItem->type == TokenType::identifierT && Misc::StringUtils::lowerCase(tokenItem->str) == "goodbye")
				{
					mParseMode = 4;
					// put back token
					tokenItem--;
					continue;
				}
				if (tokenItem->type == TokenType::identifierT && Misc::StringUtils::lowerCase(tokenItem->str) == "additem")
				{
					mParseMode = 5;
					// put back token
					tokenItem--;
					continue;
				}
				if (tokenItem->type == TokenType::identifierT && Misc::StringUtils::lowerCase(tokenItem->str) == "removeitem")
				{
					mParseMode = 6;
					// put back token
					tokenItem--;
					continue;
				}
				// Default: unhandled command, skip to next tokenStatement
				while (tokenItem->type != TokenType::endlineT)
					tokenItem++;
				continue;
			}
			if (mParseMode == 2)
			{
				parse_choice(tokenItem);
				continue;
			}
			if (mParseMode == 3)
			{
				parse_journal(tokenItem);
				continue;
			}
			if (mParseMode == 4)
			{
				bGoodbye = true;
				mParseMode = 0;
				continue;
			}
			if (mParseMode == 5 || mParseMode == 6)
			{
				parse_keyword(tokenItem);
				continue;
			}

			// Default: unhandled mode, reset mode and skip to new line
			mParseMode = 0;
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			continue;


		}

	}

}
