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

	ScriptConverter::ScriptConverter(const std::string & scriptText, ESM::ESMWriter& esm) : mESM(esm), mScriptText(scriptText)
	{
		// Initialize ScriptContext
		mCurrentContext.blockName = "";
		mCurrentContext.codeBlockDepth = 0;

		setup_dictionary();
		lexer();
		parser();

	}

	std::string ScriptConverter::GetConvertedScript()
	{
		// flush context data first, then output
		mConvertedStatementList.insert(mConvertedStatementList.end(), mCurrentContext.convertedStatements.begin(), mCurrentContext.convertedStatements.end() );
		mCurrentContext.convertedStatements.clear();

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

	const char* ScriptConverter::GetCompiledByteBuffer()
	{
		// flush context data then output
		mCompiledByteBuffer.insert(mCompiledByteBuffer.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end());
		mCurrentContext.compiledCode.clear();

		return mCompiledByteBuffer.data();
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
				// operator
				if (lineBuffer[mLinePosition] == '(' ||
					lineBuffer[mLinePosition] == ')' ||
					lineBuffer[mLinePosition] == '+' ||
					lineBuffer[mLinePosition] == '=' ||
					lineBuffer[mLinePosition] == '!' ||
					lineBuffer[mLinePosition] == '-' ||
					lineBuffer[mLinePosition] == '>' ||
					lineBuffer[mLinePosition] == '<' ||
					lineBuffer[mLinePosition] == '*' )
				{
					mReadMode = 3;
					continue;
				}
				// skip everything else
				mLinePosition++;
			}
			if (mReadMode == 1)
				read_identifier(lineBuffer);
			if (mReadMode == 2)
				read_quotedliteral(lineBuffer);
			if (mReadMode == 3)
				read_operator(lineBuffer);
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
				if ( (mLinePosition+1 < lineBuffer.size()) && 
					( (lineBuffer[mLinePosition+1] >= '0' && lineBuffer[mLinePosition+1] <= '9') ||
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
			// match to keyword
			for (auto keyItem = mKeywords.begin(); keyItem != mKeywords.end(); keyItem++)
			{
				std::string searchTerm = Misc::StringUtils::lowerCase(*keyItem);
				if (Misc::StringUtils::lowerCase(tokenStr) == searchTerm)
				{
					tokenType = TokenType::keywordT;
				}
			}

			mTokenList.push_back(Token(tokenType, tokenStr));
		}
		mReadMode = 0;

	}

	void ScriptConverter::read_operator(const std::string & lineBuffer)
	{
		int tokenType = TokenType::operatorT;
		std::string tokenStr;
		tokenStr = lineBuffer[mLinePosition++];

		if (mLinePosition == lineBuffer.size())
		{
			// add single char operator token, then return
			mTokenList.push_back(Token(tokenType, tokenStr));
			mReadMode = 0;
			return;
		}

		// check for two-char operators
		switch (tokenStr[0])
		{
		case '=':
			if (lineBuffer[mLinePosition] == '=')
				tokenStr += lineBuffer[mLinePosition++];
			break;

		case '+':
			if (lineBuffer[mLinePosition] == '=' ||
				lineBuffer[mLinePosition] == '+')
				tokenStr += lineBuffer[mLinePosition++];
			break;

		case '-':
			if (lineBuffer[mLinePosition] == '-' ||
				lineBuffer[mLinePosition] == '=' ||
				lineBuffer[mLinePosition] == '>')
				tokenStr += lineBuffer[mLinePosition++];
			break;

		case '*':
			// *=
			if (lineBuffer[mLinePosition] == '=')
				tokenStr += lineBuffer[mLinePosition++];
			break;
		
		case '!':
			if (lineBuffer[mLinePosition] == '=')
				tokenStr += lineBuffer[mLinePosition++];
			break;

		case '>':
			if (lineBuffer[mLinePosition] == '=')
				tokenStr += lineBuffer[mLinePosition++];
			break;

		case '<':
			if (lineBuffer[mLinePosition] == '=')
				tokenStr += lineBuffer[mLinePosition++];
			break;

		}

		mReadMode = 0;
		if (tokenStr != "")
			mTokenList.push_back(Token(tokenType, tokenStr));

		return;
	}

	void ScriptConverter::parse_unarycmd(std::vector<struct Token>::iterator & tokenItem)
	{
		// output translated script text
		std::stringstream convertedStatement;
		std::string command;
		command = tokenItem->str;
		if (bUseComdRef)
		{
			command = mCmdRef + "." + command;
		}
		convertedStatement << command;
		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters

		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;

	}

	void ScriptConverter::parse_localvar(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string varType = tokenItem->str;

		tokenItem++;
		std::string varName = tokenItem->str;

		if (tokenItem->type == TokenType::identifierT)
		{
			// create new local var
			mLocalVarList.push_back(std::make_pair(varType, varName));
		}
		else
		{
			std::cout << "Parser: expected identifier after short: [" << tokenItem->type << "] " << tokenItem->str << std::endl;
		}
		
		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;
	}

	void ScriptConverter::parse_if(std::vector<struct Token>::iterator & tokenItem)
	{

		// start building expression
		bBuildExpression = true;
		mExpressionStack.clear();
		mExpressionStack.push_back(*tokenItem);

		// check for bIsCallBack mode
		tokenItem++;
		if (tokenItem->type == TokenType::operatorT && tokenItem->str == "(")
		{
			mExpressionStack.push_back(*tokenItem);
			tokenItem++;
		}
		if (tokenItem->type == TokenType::keywordT)
		{
			if ( (Misc::StringUtils::lowerCase(tokenItem->str) == "onactivate") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "ondeath") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onknockout") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onmurder") || 
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcadd") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcdrop") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcequip") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpchitme") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcsoulgemuse") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onrepair") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "menumode") )
			{
				// reset expression
				bBuildExpression = false;
				mExpressionStack.clear();
				// setup callback context and return
				// push context to stack
				mContextStack.push_back(mCurrentContext);
				// re-initialize current context
				mCallBackName = tokenItem->str;
				mCurrentContext.bIsCallBack = true;
				mCurrentContext.codeBlockDepth++;
				std::stringstream blockName;
				blockName << "if" << mCurrentContext.codeBlockDepth;
				mCurrentContext.blockName = blockName.str();
				mCurrentContext.compiledCode.clear();
				mCurrentContext.convertedStatements.clear();
				mParseMode = 0;
				nextLine(tokenItem);
				return;
			}
		}
		// put back for further processing
		tokenItem--;

		// *** calculate and compile after if-block is completed ***
		// compiled code
		// if [Expression]...
		// uint16_t OpCode = 0x16;
		// complen (2 bytes)
		// jump operations (2 bytes) --number of operations to next conditional: elseif/else/endif
		// exp len ( 2 bytes)
		// exp byte buffer (n)


		// push context to stack
		mContextStack.push_back(mCurrentContext);
		// re-initialize current context
		mCurrentContext.bIsCallBack = false;
		mCurrentContext.codeBlockDepth++;
		std::stringstream blockName;
		blockName << "if" << mCurrentContext.codeBlockDepth;
		mCurrentContext.blockName = blockName.str();
		mCurrentContext.compiledCode.clear();
		mCurrentContext.convertedStatements.clear();
		mCurrentContext.ifExpression = "";

		mParseMode = 0;
		return;
	}

	void ScriptConverter::parse_endif(std::vector<struct Token>::iterator & tokenItem)
	{
		// sanity check
		std::stringstream blockName;
		blockName << "if" << mCurrentContext.codeBlockDepth;

		if (mCurrentContext.blockName != blockName.str())
		{
			std::cout << "ERROR: mismatched code block end at code depth: " << mCurrentContext.codeBlockDepth << std::endl;

			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}

		// pop environment off stack
		struct ScriptContext fromStack;
		fromStack.blockName = mContextStack.back().blockName;
		fromStack.codeBlockDepth = mContextStack.back().codeBlockDepth;
		fromStack.bIsCallBack = mContextStack.back().bIsCallBack;
		fromStack.convertedStatements = mContextStack.back().convertedStatements;
		fromStack.compiledCode = mContextStack.back().compiledCode;
		fromStack.ifExpression = mContextStack.back().ifExpression;
		mContextStack.pop_back();

		if (mCurrentContext.bIsCallBack)
		{
			// output callback begin
			std::stringstream callBackHeader;
			callBackHeader << "Begin" << " " << mCallBackName;
			mConvertedStatementList.push_back(callBackHeader.str());
			mCallBackName = "";
		}
		else
		{
			// add directly to fromStack to avoid extra tab from next block
			fromStack.convertedStatements.push_back(mCurrentContext.ifExpression);
		}
		for (auto statementItem = mCurrentContext.convertedStatements.begin(); statementItem != mCurrentContext.convertedStatements.end(); statementItem++)
		{
			std::stringstream statementStream;		
			statementStream << '\t' << *statementItem;
			if (mCurrentContext.bIsCallBack)
				mConvertedStatementList.push_back(statementStream.str());
			else
				fromStack.convertedStatements.push_back(statementStream.str());
		}
		mCurrentContext.convertedStatements.clear();
		mCurrentContext.convertedStatements = fromStack.convertedStatements;
		//if CallBack, insert "end" callback command
		std::stringstream endStatement;
		if (mCurrentContext.bIsCallBack)
		{
			endStatement << "End";
			std::string emptyLine = "";
			mConvertedStatementList.push_back(endStatement.str());
			mConvertedStatementList.push_back(emptyLine);
		}
		else
		{
			endStatement << "endif";
			mCurrentContext.convertedStatements.push_back(endStatement.str());
		}

		// merge with current environment
//		mCurrentContext.blockName = fromStack.blockName;
//		mCurrentContext.codeBlockDepth = fromStack.codeBlockDepth;
//		mCurrentContext.bIsCallBack = fromStack.bIsCallBack;

		// byte compile
		// write end byte-statement to current context
		// write current context to stack or global buffer
/*
		for (auto codeItem = mCurrentContext.compiledCode.begin(); codeItem != mCurrentContext.compiledCode.end(); codeItem++)
		{
			// if bIsCallBack, then write directly to global buffer, otherwise append to previous context buffer
			if (mCurrentContext.bIsCallBack)
				mCompiledByteBuffer.push_back(*codeItem);
			else
				fromStack.compiledCode.push_back(*codeItem);
		}
*/

		if (mCurrentContext.bIsCallBack)
		{
			// begin activate modelen  8
			// begin gamemode modelen 6

			// begin callback block
			uint16_t BeginIfCode = 0x10;
			uint16_t ModeLen = 0x6;
			if (Misc::StringUtils::lowerCase(mCallBackName) == "onactivate")
				ModeLen = 0x8;
			uint32_t BlockLen = 0;
			// insert prior to inserting if-block
			for (int i = 0; i<2; ++i) fromStack.compiledCode.push_back(reinterpret_cast<const char *> (&BeginIfCode)[i]);
			for (int i = 0; i<2; ++i) fromStack.compiledCode.push_back(reinterpret_cast<const char *> (&ModeLen)[i]);
			for (int i = 0; i<4; ++i) fromStack.compiledCode.push_back(reinterpret_cast<const char *> (&BlockLen)[i]);
			for (int i = 0; i<2; ++i) fromStack.compiledCode.push_back(0);
		}
		else
		{
			// compile if expression and insert proper lengths
			uint16_t BeginIfCode = 0x16;
			uint16_t comparisonLen = 0;
			uint16_t jumpOperationOffset = 0;
			uint16_t expressionLen = 0;
			// insert prior to inserting if-block
			for (int i = 0; i<2; ++i) fromStack.compiledCode.push_back(reinterpret_cast<const char *> (&BeginIfCode)[i]);
		}

		// inserting if-block
		if (mCurrentContext.bIsCallBack)
			mCompiledByteBuffer.insert(mCompiledByteBuffer.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end() );
		else
			fromStack.compiledCode.insert(fromStack.compiledCode.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end() );

		if (mCurrentContext.bIsCallBack)
		{
			uint32_t EndCode = 0x11;
			for (int i = 0; i<4; ++i) mCompiledByteBuffer.push_back(reinterpret_cast<const char *> (&EndCode)[i]);
		}
		else
		{
			uint32_t EndCode = 0x11;
			for (int i = 0; i<4; ++i) fromStack.compiledCode.push_back(reinterpret_cast<const char *> (&EndCode)[i]);
		}

		mCurrentContext.compiledCode.clear();
		mCurrentContext.compiledCode = fromStack.compiledCode;


		// reset context
		mCurrentContext.blockName = fromStack.blockName;
		mCurrentContext.codeBlockDepth = fromStack.codeBlockDepth;
		mCurrentContext.bIsCallBack = fromStack.bIsCallBack;
		mCurrentContext.ifExpression = fromStack.ifExpression;

		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;

	}

	void ScriptConverter::parse_end(std::vector<struct Token>::iterator & tokenItem)
	{
		// sanity check
		if (mCurrentContext.blockName != mScriptName ||
			mCurrentContext.codeBlockDepth != 0)
		{
			std::cout << "ERROR: Early termination of script at blockdepth " << mCurrentContext.codeBlockDepth << std::endl;
		}

		// converted statement
		// output localvars list
		// output converted statements
		// no converted equivalent to end statement
		std::string emptyLine = "";
		mConvertedHeader.push_back(emptyLine);
		if (mLocalVarList.empty() == false)
		{
			for (auto varItem = mLocalVarList.begin(); varItem != mLocalVarList.end(); varItem++)
			{
				std::stringstream varListText;
				varListText << varItem->first << " " << varItem->second;
				mConvertedHeader.push_back( varListText.str() );
			}
			mConvertedHeader.push_back(emptyLine);
		}

		//////////////////// Change context zero into GameMode ///////////////////////////
//		mConvertedStatementList.insert(mConvertedStatementList.end(), mCurrentContext.convertedStatements.begin(), mCurrentContext.convertedStatements.end() );

		auto statementItem = mCurrentContext.convertedStatements.begin();
		mConvertedStatementList.push_back(*statementItem);
		for (statementItem++; statementItem != mCurrentContext.convertedStatements.end(); statementItem++)
		{
			std::stringstream statementStream;
			statementStream << '\t' << *statementItem;
			mConvertedStatementList.push_back(statementStream.str());
		}
		mCurrentContext.convertedStatements.clear();
		// translate "end" command to GameMode end
		std::string endString = "End";
		mConvertedStatementList.push_back(endString);

		// prepend header to global buffer
		mConvertedStatementList.insert(mConvertedStatementList.begin(), mConvertedHeader.begin(), mConvertedHeader.end());

		// byte compile
		// 1. skip: localVars are handled by SCRV subrecords
		// 2. write context byte queue
		// 3. write end byte-statement

		//////////////////// Change context zero into GameMode ///////////////////////////

		uint32_t OpCode = 0x11;
		for (int i = 0; i<4; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&OpCode)[i]);

		mCompiledByteBuffer.insert( mCompiledByteBuffer.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end() );
		mCurrentContext.compiledCode.clear();

		// prepend header to global buffer
		mCompiledByteBuffer.insert(mCompiledByteBuffer.begin(), mCompiledByteHeader.begin(), mCompiledByteHeader.end());

		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;

	}

	void ScriptConverter::parse_begin(std::vector<struct Token>::iterator & tokenItem)
	{
		if (bIsFullScript == true || mScriptName != "")
		{
			std::cout << "WARNING: Begin statement encountered twice in script" << std::endl;
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}

		bIsFullScript = true;

		// keyword begin
		tokenItem++;

		if (tokenItem->type == TokenType::identifierT)
		{
			mScriptName = mESM.generateEDIDTES4(tokenItem->str, 0, "SCPT");
		}
		else
		{
			std::cout << "Parser: error parsing script name" << std::endl;
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}

		// Re-Initialize Script Context
		if (mContextStack.empty() != true)
		{
			std::cout << "WARNING: Stack was not empty prior to start of script." << std::endl;
		}
		mContextStack.clear();
		mCurrentContext.blockName = mScriptName;
		mCurrentContext.codeBlockDepth = 0;

		// translate script statement
		std::stringstream headerStream;
		headerStream << "ScriptName" << " " << mScriptName;
		mConvertedHeader.push_back(headerStream.str());

		// convert global context to GameMode
		std::stringstream convertedStatement;
		convertedStatement << "Begin GameMode";
		mCurrentContext.convertedStatements.push_back(convertedStatement.str() );

		// compile statement
		uint32_t OpCode = 0x1D;
		for (int i = 0; i<4; ++i) mCompiledByteHeader.push_back(reinterpret_cast<const char *> (&OpCode)[i]);

		// TODO: add compiled block for Begin

		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;
	}

	void ScriptConverter::parse_addremoveitem(std::vector<struct Token>::iterator& tokenItem, bool bRemove)
	{
		std::string itemEDID, itemCount;
		bool bEvalArg2=false;

		// check keyword
		tokenItem++;

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
			errorMesg << "ERROR: ScriptConverter:parse_keyword() - unexpected token type, expected string_literal:  [" << tokenItem->type << "] token=" << tokenItem->str;
			std::cout << errorMesg.str() << std::endl;
			// throw std::runtime_error(errorMesg.str());
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}
		tokenItem++;

		// item count
		if (tokenItem->type == TokenType::number_literalT)
		{
			itemCount = tokenItem->str;
		}
		else if (tokenItem->type == TokenType::identifierT)
		{
			itemCount = tokenItem->str;
			bEvalArg2 = true;
		}
		else if (tokenItem->type == TokenType::endlineT)
		{
			itemCount = "1";
			tokenItem--; // put EOL back
		}
		else
		{
			std::stringstream errorMesg;
			errorMesg << "ERROR: ScriptConverter:parse_keyword() - unexpected token type, expected number_literalT: [" << tokenItem->type << "] token=" << tokenItem->str;
			std::cout << errorMesg.str() << std::endl;
			std::cout << std::endl << "DEBUG OUTPUT: " << std::endl << mScriptText << std::endl << std::endl;
			// throw std::runtime_error(errorMesg.str());
			mParseMode = 0;
			// advance to endofline in stream
			while (tokenItem->type != TokenType::endlineT)
				tokenItem++;
			return;
		}
		tokenItem++;

		// output translated script text
		std::stringstream convertedStatement;
		std::string command;
		command = "AddItem";
		if (bRemove)
			command = "RemoveItem";
		if (bUseComdRef)
		{
			command = mCmdRef + "." + command;
		}
		convertedStatement << command << " " << itemEDID << " " << itemCount;
		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 0x1002; // Additem
		if (mParseMode == 6)
			OpCode = 0x1052;
		uint16_t sizeParams = 10;
		uint16_t countParams = 2;
		uint32_t arg2;
		if (bEvalArg2)
		{
			compile_command(OpCode, sizeParams, countParams, itemEDID, itemCount);
		}
		else
		{
			arg2 = atoi(itemCount.c_str());;
			compile_command(OpCode, sizeParams, countParams, itemEDID, arg2);
		}

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
				errorMesg << "ERROR: ScriptConverter:parse_choice() - unexpected token type, expected string_literal: " << tokenItem->type;
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
				errorMesg << "ERROR: ScriptConverter:parse_choice() - unexpected token type, expected number_literal: " << tokenItem->type;
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
		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 1039; // Additem
		uint16_t sizeParams = 10;
		uint16_t countParams = 2;
		uint32_t arg2 = atoi(questStage.c_str());;
		compile_command(OpCode, sizeParams, countParams, questEDID, arg2);


		mParseMode = 0;
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		return;

	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, uint32_t arg1, uint32_t arg2)
	{
		if (bUseComdRef)
		{
			uint32_t refFormID;
			if (Misc::StringUtils::lowerCase( mCmdRef) == "player" || Misc::StringUtils::lowerCase(mCmdRef) == "playerref")
				refFormID = 0x14;
			else
				uint32_t refFormID = mESM.crossRefStringID(mCmdRef, "");
			int refListIndex = -1;
			for (int i = 0; i < mReferenceList.size(); i++)
			{
				if (mReferenceList[i] == refFormID)
				{
					refListIndex = i + 1;
					break;
				}
			}
			if (refListIndex == -1)
			{
				mReferenceList.push_back(refFormID);
				refListIndex = mReferenceList.size();
			}
			uint16_t OpRef = 0x1C | (refListIndex << 16);
			for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&OpRef)[i]);
		}

		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&OpCode)[i]);
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&sizeParams)[i]);
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&countParams)[i]);
		for (int i = 0; i<4; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&arg1)[i]);
		for (int i = 0; i<4; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&arg2)[i]);
	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, const std::string & edid, uint32_t arg2)
	{
		uint32_t formID = mESM.crossRefStringID(edid, "", false);
		int refListIndex = -1;
		for (int i = 0; i < mReferenceList.size(); i++)
		{
			if (mReferenceList[i] == formID)
			{
				refListIndex = i + 1;
				break;
			}
		}
		if (refListIndex == -1)
		{
			mReferenceList.push_back(formID);
			refListIndex = mReferenceList.size();
		}

		uint8_t refTag = 0x72;
		uint8_t unknownChar = 0x6E;

		uint32_t arg1FormID = refTag | (refListIndex << 8) | (unknownChar << 24);

		compile_command(OpCode, sizeParams, countParams, arg1FormID, arg2);

	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, const std::string & edid, const std::string & expression)
	{
		uint32_t formID = mESM.crossRefStringID(edid, "", false);
		if (formID != 0)
		{
			int refListIndex = -1;
			for (int i = 0; i < mReferenceList.size(); i++)
			{
				if (mReferenceList[i] == formID)
				{
					refListIndex = i + 1;
					break;
				}
			}
			if (refListIndex == -1)
			{
				mReferenceList.push_back(formID);
				refListIndex = mReferenceList.size();
			}
			uint8_t refTag = 0x72;
			uint8_t unknownChar = 0x6E;
			uint32_t arg2formID = refTag | (refListIndex << 8) | (unknownChar << 24);

			compile_command(OpCode, sizeParams, countParams, edid, arg2formID);
			return;
		}
		else
		{
			// TODO: lookup local var
			// TODO: implement function
			std::cout << "ERROR: Byte compiler inline expression evaluation not yet implemented." << std::endl;
			std::cout << std::endl << "DEBUG OUTPUT:" << std::endl << mScriptText << std::endl << std::endl;
			return;
		}

	}

	void ScriptConverter::nextLine(std::vector<struct Token>::iterator & tokenItem)
	{
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT)
			tokenItem++;
		// leave endline token on queue for parser
		return;
	}

	void ScriptConverter::setup_dictionary()
	{
		mKeywords.push_back("scriptname");
		mKeywords.push_back("short");
		mKeywords.push_back("long");
		mKeywords.push_back("float");
		mKeywords.push_back("begin");
		mKeywords.push_back("end");
		mKeywords.push_back("set");
		mKeywords.push_back("to");
		mKeywords.push_back("if");
		mKeywords.push_back("elseif");
		mKeywords.push_back("else");
		mKeywords.push_back("endif");
		mKeywords.push_back("while");
		mKeywords.push_back("endwhile");
		mKeywords.push_back("journal");
		mKeywords.push_back("choice");
		mKeywords.push_back("goodbye");
		mKeywords.push_back("additem");
		mKeywords.push_back("removeitem");
		mKeywords.push_back("setfight");
		mKeywords.push_back("moddisposition");
		mKeywords.push_back("getdisposition");
		mKeywords.push_back("modpcfacrep");
		mKeywords.push_back("pcraiserank");
		mKeywords.push_back("messagebox");
		mKeywords.push_back("addspell");
		mKeywords.push_back("enable");
		mKeywords.push_back("disable");

		mKeywords.push_back("onactivate");
		mKeywords.push_back("ondeath");
		mKeywords.push_back("onknockout");
		mKeywords.push_back("onmurder");
		mKeywords.push_back("onpcadd");
		mKeywords.push_back("onpcdrop");
		mKeywords.push_back("onpcequip");
		mKeywords.push_back("onpchitme");
		mKeywords.push_back("onpcsoulgemuse");
		mKeywords.push_back("onrepair");
		mKeywords.push_back("menumode");

		mKeywords.push_back("addtopic");
		mKeywords.push_back("stopscript");
		mKeywords.push_back("playsound");
		mKeywords.push_back("centeroncell");
		mKeywords.push_back("return");
		mKeywords.push_back("positioncell");
		mKeywords.push_back("activate");
		mKeywords.push_back("cast");
		mKeywords.push_back("resurrect");
		mKeywords.push_back("stopcombat");
		mKeywords.push_back("sethealth");
		mKeywords.push_back("rotate");
		mKeywords.push_back("startscript");
		mKeywords.push_back("equip");

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

			if (mParseMode == 0) // Ready-Mode to begin new statement/expression
			{

				if (tokenItem->type == TokenType::endlineT)
				{
					bUseComdRef = false;
					mCmdRef = "";
					bBuildExpression = false;
					for (auto expToken = mExpressionStack.begin(); expToken != mExpressionStack.end(); expToken++)
						mCurrentContext.ifExpression += expToken->str + " ";
					continue;
				}
				if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
				{
					std::string possibleRef = tokenItem->str;
					tokenItem++;
					if (tokenItem->type == TokenType::operatorT && tokenItem->str == "->")
					{
						mCmdRef = mESM.generateEDIDTES4(possibleRef, 0, "");
						bUseComdRef = true;
						continue;
					}
					if (bBuildExpression)
					{
						tokenItem--;
						mExpressionStack.push_back(*tokenItem);
						tokenItem++;
						mExpressionStack.push_back(*tokenItem);
					}

					std::cout << "Parser: Unhandled Identifier Statement: [" << possibleRef << "] " << tokenItem->str << std::endl;
					while (tokenItem->type != TokenType::endlineT)
						tokenItem++;
					continue;
				}
				if (tokenItem->type == TokenType::keywordT)
				{
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "choice")
					{
						parse_choice(tokenItem);
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "journal")
					{
						parse_journal(tokenItem);
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "goodbye")
					{
						bGoodbye = true;
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "additem")
					{
						parse_addremoveitem(tokenItem, false);
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "removeitem")
					{
						parse_addremoveitem(tokenItem, true);
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "begin")
					{
						parse_begin(tokenItem);
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "end")
					{
						parse_end(tokenItem);
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "if")
					{
						parse_if(tokenItem);
						continue;
					}
					if (Misc::StringUtils::lowerCase(tokenItem->str) == "endif")
					{
						parse_endif(tokenItem);
						continue;
					}
					if ( (Misc::StringUtils::lowerCase(tokenItem->str) == "short") ||
						(Misc::StringUtils::lowerCase(tokenItem->str) == "long") ||
						(Misc::StringUtils::lowerCase(tokenItem->str) == "float") )
					{
						parse_localvar(tokenItem);
						continue;
					}
					if ( (Misc::StringUtils::lowerCase(tokenItem->str) == "disable") ||
						(Misc::StringUtils::lowerCase(tokenItem->str) == "enable") )
					{
						parse_unarycmd(tokenItem);
						continue;
					}

					// catch any unhandled tokens when building expression
					if (bBuildExpression)
					{
						if (bUseComdRef)
						{
							tokenItem->str = mCmdRef + "." + tokenItem->str;
						}
						mExpressionStack.push_back(*tokenItem);
						continue;
					}

					// Default: unhandled keyword, skip to next tokenStatement
					std::cout << "Parser: Unhandled Keyword: [" << tokenItem->str << "]" << std::endl;
					while (tokenItem->type != TokenType::endlineT)
						tokenItem++;
					continue;
				} // keyword

			    // catch any unhandled tokens when building expression
				if (bBuildExpression)
				{
					mExpressionStack.push_back(*tokenItem);
					continue;
				}

				std::cout << "Parser: Unhandled Token: <" << tokenItem->type << "> '" << tokenItem->str << "'" << std::endl;
				while (tokenItem->type != TokenType::endlineT)
					tokenItem++;
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

/*
if (tokenItem->type == TokenType::operatorT)
{
if (bBuildExpression != true)
{
mParseMode = 0;
while (tokenItem->type != TokenType::endlineT)
tokenItem++;
continue;
}
if (tokenItem->str == "(")
{
// push on stack
continue;
}
if (tokenItem->str == ")")
{
// complete inside expression
continue;
}
if ( (tokenItem->str == "+") ||
(tokenItem->str == "-") ||
(tokenItem->str == "*") )
{
// math operation
continue;
}
if ( (tokenItem->str == "==") ||
(tokenItem->str == ">") ||
(tokenItem->str == ">=") ||
(tokenItem->str == "<") ||
(tokenItem->str == "<=") ||
(tokenItem->str == "!=") )
{
// comparison
// pop off stack
continue;
}

std::cout << "Parser: Error building expression " << tokenItem->str << std::endl;
bBuildExpression = false;
mParseMode = 0;
while (tokenItem->type != TokenType::endlineT)
tokenItem++;
continue;
}
*/