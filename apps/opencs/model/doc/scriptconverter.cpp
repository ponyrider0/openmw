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
			oldCode += "; " + scriptline + "\n";
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

	uint32_t ScriptConverter::GetByteBufferSize()
	{
		return mCompiledByteBuffer.size();
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
			if ( (lineBuffer[mLinePosition] >= '0' && lineBuffer[mLinePosition] <= '9') ||
				(lineBuffer[mLinePosition] == '.') )
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

	void ScriptConverter::parse_statement(std::vector<struct Token>::iterator & tokenItem)
	{
		if (tokenItem->type == TokenType::endlineT)
		{
			bUseCommandReference = false;
			mCommandReference = "";
		}

		if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
		{
			std::string possibleRef = tokenItem->str;
			tokenItem++;
			if (tokenItem->type == TokenType::operatorT && tokenItem->str == "->")
			{
				bUseCommandReference = true;
				mCommandReference = mESM.generateEDIDTES4(possibleRef, 0, "");
				return;
			}
			// putback
			tokenItem--;

			std::stringstream errMesg;
			errMesg << "Unhandled Identifier Statement: [" << possibleRef << "] " << tokenItem->str << std::endl;
			abort(errMesg.str());
			return;
		}

		if (tokenItem->type == TokenType::keywordT)
		{
			parse_keyword(tokenItem);
			return;
		} // keyword

		std::stringstream errMesg;
		errMesg << "Parser: Unhandled Token: <" << tokenItem->type << "> '" << tokenItem->str << "'" << std::endl;
		abort(errMesg.str());
		return;

	}

	void ScriptConverter::parse_expression(std::vector<struct Token>::iterator & tokenItem)
	{
		if (tokenItem->type == TokenType::endlineT)
		{
			// unload expression stack to compiled buffer
			while (mOperatorExpressionStack.empty() != true)
			{
				auto fromStack = mOperatorExpressionStack.back();
				std::vector<uint8_t> OpData;
				uint8_t OpCode_Push = 0x20;
				OpData.push_back(OpCode_Push);
				for (int i=0; i < fromStack.str.size(); i++) OpData.push_back(fromStack.str[i]);
				mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), OpData.begin(), OpData.end());
				mOperatorExpressionStack.pop_back();
			}

			// update statement/expression lengths...
			if (bSetCmd)
			{
				// 15 00 [Length(2)]
				int offset = mSetCmd_StartPosition + 2;
				uint16_t statementLength = mCurrentContext.compiledCode.size() - offset - 2; // -2: do not count offset length in length
				for (int i=0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&statementLength)[i];

				// 15 00 [Length(2)] [Var(N)] [ExpLength(2)]
				offset += (2 + mSetCmd_VarSize); 
				uint16_t expressionLength = statementLength - mSetCmd_VarSize - 2; // -2: do not count offset length in length
				for (int i = 0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&expressionLength)[i];
			}
			else
			{
				// TODO: if command
			}
			mParseMode = 0;
			return;
		}

		if (tokenItem->type == TokenType::operatorT)
		{
			parse_operator(tokenItem);
			return;
		}

		if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
		{
			std::string possibleRef = tokenItem->str;
			tokenItem++;
			// check if it's part of a reference command
			if (tokenItem->type == TokenType::operatorT && tokenItem->str == "->")
			{
				bUseCommandReference = true;
				mCommandReference = mESM.generateEDIDTES4(possibleRef, 0, "");
				return;
			}
			// putback
			tokenItem--;
			// if not, treat as local/global variable
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + tokenItem->str;
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size()-1] = expressionLine;
			// compile
			auto varData = compile_param_varname(tokenItem->str);
			mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), varData.begin(), varData.end());
			return;
		}

		if (tokenItem->type == TokenType::number_literalT)
		{
			// translate and compile to buffer
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + tokenItem->str;
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size()-1] = expressionLine;
			// compile to buffer
			uint8_t OpCode_Push = 0x20;
			mCurrentContext.compiledCode.push_back(OpCode_Push);
			for (int i=0; i < tokenItem->str.size(); i++) mCurrentContext.compiledCode.push_back(tokenItem->str[i]);

			return;
		}

		// translate and compile to buffer
		if (tokenItem->type == TokenType::keywordT)
		{
			parse_keyword(tokenItem);
			return;
		}

		std::stringstream errMesg;
		errMesg << "Unhandled Token: <" << tokenItem->type << "> '" << tokenItem->str << "'" << std::endl;
		abort(errMesg.str());
		return;

	}

	void ScriptConverter::parse_operator(std::vector<struct Token>::iterator& tokenItem)
	{
		uint8_t OpCode_Push = 0x20;

		// translate
		std::string expressionLine = mCurrentContext.convertedStatements.back();
		expressionLine += " " + tokenItem->str;
		mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size() - 1] = expressionLine;

		// place on stack or directly compile
		// if "(" encountered, then push back context and start fresh
		if (tokenItem->str == "(")
		{
			mOperatorExpressionStack.push_back(*tokenItem);
			return;
		}

		// if ")" encountered, then pop off operators until "(" reached
		if (tokenItem->str == ")")
		{
			while (mOperatorExpressionStack.empty() != true)
			{
				std::string operatorStr = mOperatorExpressionStack.back().str;
				mOperatorExpressionStack.pop_back();
				if (operatorStr == "(")
					break;
				mCurrentContext.compiledCode.push_back(OpCode_Push);
				for (int i = 0; i < operatorStr.size(); i++) mCurrentContext.compiledCode.push_back(operatorStr[i]);
			}
			return;
		}

		if ((tokenItem->str == "+") ||
			(tokenItem->str == "-") ||
			(tokenItem->str == "*") ||
			(tokenItem->str == "==") ||
			(tokenItem->str == ">") ||
			(tokenItem->str == ">=") ||
			(tokenItem->str == "<") ||
			(tokenItem->str == "<=") ||
			(tokenItem->str == "!="))
		{
			while (mOperatorExpressionStack.empty() != true &&
				mOperatorExpressionStack.back().IsHigherPrecedence(tokenItem))
			{
				struct Token *fromStack = &mOperatorExpressionStack.back();
				mCurrentContext.compiledCode.push_back(OpCode_Push);
				for (int i = 0; i < fromStack->str.size(); i++) mCurrentContext.compiledCode.push_back(fromStack->str[i]);

				mOperatorExpressionStack.pop_back();
			}
			mOperatorExpressionStack.push_back(*tokenItem);
			return;
		}

		// ERROR message
		std::stringstream ErrMsg;
		ErrMsg << "unhandled operator token: " << tokenItem->str << std::endl;
		abort(ErrMsg.str());
		return;

	}

	// Set [varName] To [Expression]
	void ScriptConverter::parse_set(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string varName;
		bool bIsLocalVar=false;
		int varIndex;
		std::vector<uint8_t> varData;

		// start building expression
		mOperatorExpressionStack.clear();

		tokenItem++;
		// variable token
		if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
		{
			// Read variable
			varName = tokenItem->str;

			// compile to varData
			varData = compile_param_varname(varName);
			mSetCmd_VarSize = varData.size();
		}
		else
		{
			std::stringstream errStr;
			errStr << "unexpected token type in set statement (expected variable name): [" << tokenItem->type << "] " << tokenItem->str << std::endl;
			abort(errStr.str());
			return;
		}

		tokenItem++;
		// read "to" (sanity check)
		if (Misc::StringUtils::lowerCase(tokenItem->str) != "to")
		{
			std::stringstream errStr;
			errStr << "unexpected token type in set statement (expected \"to\" operator): [" << tokenItem->type << "] " << tokenItem->str << std::endl;
			abort(errStr.str());
			return;
		}

		// Output translation
		std::stringstream cmdString;
		cmdString << "Set" << " " << varName << " " << "To" << " ";
		mCurrentContext.convertedStatements.push_back(cmdString.str());

		// Output byte-compiled code: 15 00 [Length(2)] [Var(N)] [ExpressionLength(2)] [Expression(N)]
		uint16_t OpCode = 0x15;
		uint16_t statementLen = varData.size(); // Var(N) + ExpressionLength(2) + Expression(N)
		uint16_t expressionLen = 0x00;

		mSetCmd_StartPosition = mCurrentContext.compiledCode.size(); // bookmark start of command
		compile_command(OpCode, statementLen);
		mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), varData.begin(), varData.end());
		for (int i=0; i<2; i++) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&expressionLen)[i]);

		// change parsemode to expression build mode
		mParseMode = 1;
		bSetCmd = true;

		return;
	}

	void ScriptConverter::parse_unarycmd(std::vector<struct Token>::iterator & tokenItem)
	{
		// output translated script text
		std::stringstream convertedStatement;
		std::string command;
		command = tokenItem->str;
		if (bUseCommandReference)
		{
			command = mCommandReference + "." + command;
		}
		convertedStatement << command;
		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters

		return;

	}

	void ScriptConverter::parse_localvar(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string varType = tokenItem->str;

		tokenItem++;
		if (tokenItem->type == TokenType::identifierT)
		{
			std::string varName = tokenItem->str;

			// sanity check -- make sure local var is not already defined
			for (auto searchItem = mLocalVarList.begin(); searchItem != mLocalVarList.end(); searchItem++)
			{
				if (Misc::StringUtils::lowerCase(varName) == Misc::StringUtils::lowerCase(searchItem->second))
				{
					// warning: skip var redefinition, don't abort
					std::stringstream errMesg;
					errMesg << "attempted local var redefinition: [" << tokenItem->type << "] " << tokenItem->str << std::endl;
					error_mesg(errMesg.str());
					return;
				}
			}
			// create new local var
			mLocalVarList.push_back(std::make_pair(varType, varName));
		}
		else
		{
			std::stringstream errMesg;
			errMesg << "Parser: expected identifier after short/float: [" << tokenItem->type << "] " << tokenItem->str << std::endl;
			abort(errMesg.str());
		}
		
		return;
	}

	void ScriptConverter::parse_if(std::vector<struct Token>::iterator & tokenItem)
	{
		// start building expression
		mOperatorExpressionStack.clear();

		tokenItem++;
		// check for bIsCallBack mode
		if (tokenItem->type == TokenType::operatorT && tokenItem->str == "(")
		{
			mOperatorExpressionStack.push_back(*tokenItem);
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
				mOperatorExpressionStack.clear();
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
				return;
			}
		}
		// put back for further processing
		tokenItem--;

		// TODO: begin if statement??

		// TODO: Compile IF statement header, then record byte position to update with JumpOffsets and ExpressionLength
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

		mParseMode = 1;
		return;
	}

	void ScriptConverter::parse_endif(std::vector<struct Token>::iterator & tokenItem)
	{
		// sanity check
		std::stringstream blockName;
		blockName << "if" << mCurrentContext.codeBlockDepth;

		if (mCurrentContext.blockName != blockName.str())
		{
			std::stringstream errMesg;
			errMesg << "mismatched code block end at code depth: " << mCurrentContext.codeBlockDepth << std::endl;
			abort(errMesg.str());
			return;
		}

		// pop environment off stack
		struct ScriptContext fromStack;
		fromStack.blockName = mContextStack.back().blockName;
		fromStack.codeBlockDepth = mContextStack.back().codeBlockDepth;
		fromStack.bIsCallBack = mContextStack.back().bIsCallBack;
		fromStack.convertedStatements = mContextStack.back().convertedStatements;
		fromStack.compiledCode = mContextStack.back().compiledCode;
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
			// ....?
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

		return;

	}

	void ScriptConverter::parse_end(std::vector<struct Token>::iterator & tokenItem)
	{
		int offset;

		// sanity check
		if (mCurrentContext.blockName != mScriptName ||
			mCurrentContext.codeBlockDepth != 0)
		{
			std::stringstream errMesg;
			errMesg << "ERROR: Early termination of script at blockdepth " << mCurrentContext.codeBlockDepth << std::endl;
			abort(errMesg.str());
		}

		// translate statements
		// output localvars list
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

		// output "Begin GameMode" block
		auto statementItem = mCurrentContext.convertedStatements.begin();
		mConvertedStatementList.push_back(*statementItem);
		statementItem++;
		// body of GameMode block
		while (statementItem != mCurrentContext.convertedStatements.end())
		{
			std::stringstream statementStream;
			statementStream << '\t' << *statementItem;
			mConvertedStatementList.push_back(statementStream.str());
			statementItem++;
		}
		mCurrentContext.convertedStatements.clear();
		// End statement for Gamemode block
		std::string endString = "End";
		mConvertedStatementList.push_back(endString);

		// Add header to start of global buffer
		mConvertedStatementList.insert(mConvertedStatementList.begin(), mConvertedHeader.begin(), mConvertedHeader.end());

		// byte compile
		// 2. update block length: 10 00 06 00 00 00 [BlockLen(4)]
		offset = 6;
		uint32_t blockLength = mCurrentContext.compiledCode.size() - offset; // do not count offset length in length
		for (int i=0; i < 4; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&blockLength)[i];

		// 1. write end byte-statement to context buffer
		uint16_t OpCode = 0x11;
		for (int i = 0; i<4; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<const char *> (&OpCode)[i]);

		// 3. write compiled context buffer to global buffer
		mCompiledByteBuffer.insert( mCompiledByteBuffer.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end() );
		mCurrentContext.compiledCode.clear();

		// 4. write compiled header to start of global buffer
		mCompiledByteBuffer.insert(mCompiledByteBuffer.begin(), mCompiledByteHeader.begin(), mCompiledByteHeader.end());

		nextLine(tokenItem);
		return;

	}

	void ScriptConverter::parse_begin(std::vector<struct Token>::iterator & tokenItem)
	{
		if (bIsFullScript == true || mScriptName != "")
		{
			std::stringstream errMesg;
			errMesg << "WARNING: Begin statement encountered twice in script" << std::endl;
			error_mesg(errMesg.str());
			return;
		}

		bIsFullScript = true;

		tokenItem++;
		if (tokenItem->type == TokenType::identifierT)
		{
			mScriptName = mESM.generateEDIDTES4(tokenItem->str, 0, "SCPT");
		}
		else
		{
			std::stringstream errMesg;
			errMesg << "error parsing script name" << std::endl;
			abort(errMesg.str());
			return;
		}

		// Re-Initialize Script Context
		if (mContextStack.empty() != true)
		{
			std::stringstream errMesg;
			errMesg << "WARNING: Stack was not empty prior to start of script." << std::endl;
			error_mesg(errMesg.str());
			mContextStack.clear();
		}
		mCurrentContext.blockName = mScriptName;
		mCurrentContext.codeBlockDepth = 0;

		// translate script statement: Header=>"ScriptName...", CurrentContext=>"Begin GameMode"
		std::stringstream headerStream;
		headerStream << "ScriptName" << " " << mScriptName;
		mConvertedHeader.push_back(headerStream.str());

		std::stringstream convertedStatement;
		convertedStatement << "Begin GameMode";
		mCurrentContext.convertedStatements.push_back(convertedStatement.str() );

		// bytecompile
		// Script Header: 1D 00 00 00
		uint16_t ScriptNameCode = 0x1D;
		uint16_t UnusedData = 0x00;
		for (int i = 0; i<2; ++i) mCompiledByteHeader.push_back(reinterpret_cast<uint8_t *> (&ScriptNameCode)[i]);
		for (int i = 0; i<2; ++i) mCompiledByteHeader.push_back(reinterpret_cast<uint8_t *> (&UnusedData)[i]);

		// compiled block for Begin: 10 00 06 00 00 00 [BlockLen(4)]
		uint16_t GameModeCode = 0x10;
		uint16_t GameModeData = 0x06;
		uint16_t OtherData = 0x00;
		std::vector<uint8_t> blockLen;
		for (int i=0; i<4; i++) blockLen.push_back(0);
		compile_command(GameModeCode, GameModeData, OtherData, blockLen);

		return;
	}

	void ScriptConverter::parse_keyword(std::vector<struct Token>::iterator & tokenItem)
	{
		if (Misc::StringUtils::lowerCase(tokenItem->str) == "choice")
		{
			parse_choice(tokenItem);

		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "journal")
		{
			parse_journal(tokenItem);
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "goodbye")
		{
			bGoodbye = true;
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "additem")
		{
			parse_addremoveitem(tokenItem, false);
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "removeitem")
		{
			parse_addremoveitem(tokenItem, true);
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "begin")
		{
			parse_begin(tokenItem);
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "end")
		{
			parse_end(tokenItem);
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "if")
		{
			parse_if(tokenItem);
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "endif")
		{
			parse_endif(tokenItem);
		}
		else if (Misc::StringUtils::lowerCase(tokenItem->str) == "set")
		{
			parse_set(tokenItem);
		}
		else if ((Misc::StringUtils::lowerCase(tokenItem->str) == "short") ||
			(Misc::StringUtils::lowerCase(tokenItem->str) == "long") ||
			(Misc::StringUtils::lowerCase(tokenItem->str) == "float"))
		{
			parse_localvar(tokenItem);
		}
		else if ((Misc::StringUtils::lowerCase(tokenItem->str) == "disable") ||
			(Misc::StringUtils::lowerCase(tokenItem->str) == "enable"))
		{
			parse_unarycmd(tokenItem);
		}
		else
		{
			// Default: unhandled keyword, skip to next tokenStatement
			std::stringstream errMesg;
			errMesg << "Parser: Unhandled Keyword: [" << tokenItem->str << "]" << std::endl;
			error_mesg(errMesg.str());
			return;
		}
		// common instructions
		if (mParseMode == 0)
			nextLine(tokenItem);
		return;

	}

	void ScriptConverter::parse_addremoveitem(std::vector<struct Token>::iterator& tokenItem, bool bRemove)
	{
		std::string itemEDID, itemCountString;
		bool bEvalItemCountVar=false;

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
			errorMesg << "ERROR: ScriptConverter:parse_keyword() - unexpected token type, expected string_literal:  [" << tokenItem->type << "] token=" << tokenItem->str << std::endl;
			abort(errorMesg.str());
			return;
		}

		tokenItem++;
		// item count
		if (tokenItem->type == TokenType::number_literalT)
		{
			itemCountString = tokenItem->str;
		}
		else if (tokenItem->type == TokenType::identifierT)
		{
			itemCountString = tokenItem->str;
			bEvalItemCountVar = true;
		}
		else if (tokenItem->type == TokenType::endlineT)
		{
			itemCountString = "1";
			tokenItem--; // put EOL back
		}
		else
		{
			std::stringstream errorMesg;
			errorMesg << "ERROR: ScriptConverter:parse_keyword() - unexpected token type, expected number_literalT: [" << tokenItem->type << "] token=" << tokenItem->str << std::endl;
			abort(errorMesg.str());
//			std::cout << std::endl << "DEBUG OUTPUT: " << std::endl << mScriptText << std::endl << std::endl;
			return;
		}

		// translate statement
		std::stringstream convertedStatement;
		std::string command;
		command = "AddItem";
		if (bRemove)
			command = "RemoveItem";
		if (bUseCommandReference)
		{
			command = mCommandReference + "." + command;
		}
		convertedStatement << command << " " << itemEDID << " " << itemCountString;
		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 0x1002; // Additem
		if (bRemove)
			OpCode = 0x1052;
		uint16_t sizeParams = 10;
		uint16_t countParams = 2;
		uint32_t nItemCount;
		if (bEvalItemCountVar)
		{
			compile_command(OpCode, sizeParams, countParams, compile_param_varname(itemEDID), compile_param_varname(itemCountString));
		}
		else
		{
			nItemCount = atoi(itemCountString.c_str());;
			compile_command(OpCode, sizeParams, countParams, compile_param_varname(itemEDID), compile_param_long(nItemCount));
		}

		return;

	}

	void ScriptConverter::parse_choice(std::vector<struct Token>::iterator& tokenItem)
	{
		std::string choiceText;
		int choiceNum;

		// skip tokenItem "choice"
		tokenItem++;

		// stop processing if detecting endofline
		while (tokenItem->type != TokenType::endlineT && tokenItem != mTokenList.end())
		{

			// process string literal + number literal pairs
			if (tokenItem->type == TokenType::string_literalT)
				choiceText = tokenItem->str;
			else
			{
				// error parsing choice statement
				std::stringstream errorMesg;
				errorMesg << "ERROR: ScriptConverter:parse_choice() - unexpected token type, expected string_literal: " << tokenItem->type << std::endl;
				abort(errorMesg.str());
				return;
			}
			tokenItem++; // advance to next token

			if (tokenItem->type == TokenType::number_literalT)
				choiceNum = atoi(tokenItem->str.c_str());
			else
			{
				std::stringstream errorMesg;
				errorMesg << "ERROR: ScriptConverter:parse_choice() - unexpected token type, expected number_literal: " << tokenItem->type << std::endl;
				abort(errorMesg.str());
				return;
			}
			tokenItem++; // advance to next token

			// add Choice Text:Number pair to choicetopicnames list
			mChoicesList.push_back(std::make_pair(choiceNum, choiceText));
		}

	}

	void ScriptConverter::parse_journal(std::vector<struct Token>::iterator& tokenItem)
	{
		std::string questEDID, questStageStr;
		std::vector<uint8_t> questStageData;
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
			errorMesg << "parse_journal(): unexpected token type, expected string_literal: [" << tokenItem->type << "] " << tokenItem->str << std::endl;
			error_mesg(errorMesg.str());
			return;
		}
		tokenItem++;

		// quest stage
		questStageStr = tokenItem->str;
		if (tokenItem->type == TokenType::number_literalT)
		{
			int questStageInt = atoi(questStageStr.c_str());
			questStageData = compile_param_long(questStageInt);
		}
		else if (tokenItem->type == TokenType::string_literalT || 
			tokenItem->type == TokenType::identifierT)
		{
			questStageData = compile_param_varname(questStageStr);
		}
		else
		{
			// error parsing journal
			std::stringstream errorMesg;
			errorMesg << "parse_journal(): unexpected token type, expected number or string literal: [" << tokenItem->type << "] " << tokenItem->str << std::endl;
			error_mesg(errorMesg.str());
			return;
		}
		tokenItem++;

		// translate SetStage statement
		std::stringstream convertedStatement;
		convertedStatement << "SetStage " << questEDID << " " << questStageStr;
		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 1039; // Additem
		uint16_t sizeParams = 10;
		uint16_t countParams = 2;
		compile_command(OpCode, sizeParams, countParams, compile_param_varname(questEDID), questStageData);
		return;

	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams)
	{
		if (mParseMode == 1)
		{
			uint8_t OpCode_Push = 0x20;
			mCurrentContext.compiledCode.push_back(OpCode_Push);
		}
		if (bUseCommandReference)
		{
			uint16_t RefData = prepare_reference(mCommandReference);
			if (RefData != 0)
			{
				uint16_t RefCode = 0x1C;
				if (mParseMode == 1)
				{
					RefCode = 0x72;
				}
				for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&RefCode)[i]);
				for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);
			}
			else
			{
				// ERROR
				std::stringstream errMsg;
				errMsg << "could not resolve reference for referenced command: " << mCommandReference << std::endl;
				error_mesg(errMsg.str());
				return;
			}
			bUseCommandReference = false;
			mCommandReference = "";
		}
		else
		{
			if (mParseMode == 1)
			{
				uint8_t OpCode_ExpressionFunction = 0x58;
				mCurrentContext.compiledCode.push_back(OpCode_ExpressionFunction);
			}
		}
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&OpCode)[i]);
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&sizeParams)[i]);
	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, std::vector<uint8_t> param1data)
	{
		compile_command(OpCode, sizeParams);
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&countParams)[i]);
		mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), param1data.begin(), param1data.end());
	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, std::vector<uint8_t> param1data, std::vector<uint8_t> param2data)
	{
		compile_command(OpCode, sizeParams, countParams, param1data);
		mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), param2data.begin(), param2data.end());
	}

	std::vector<uint8_t> ScriptConverter::compile_param_long(int int_val)
	{
		uint8_t RefCode = 0x6E;
		uint32_t RefData = int_val;
		std::vector<uint8_t> resultData;
		
		if (mParseMode == 1)
		{
			uint8_t OpCode_Push = 0x20;
			resultData.push_back(OpCode_Push);
		}
		resultData.push_back(RefCode);
		for (int i = 0; i<4; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);

		return resultData;
	}

	std::vector<uint8_t> ScriptConverter::compile_param_float(double float_val)
	{
		uint8_t RefCode = 0x6E;
		uint64_t RefData = reinterpret_cast<uint64_t *>(&float_val)[0];
		std::vector<uint8_t> resultData;

		if (mParseMode == 1)
		{
			uint8_t OpCode_Push = 0x20;
			resultData.push_back(OpCode_Push);
		}
		resultData.push_back(RefCode);
		for (int i = 0; i<8; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);

		return resultData;
	}

	std::vector<uint8_t> ScriptConverter::compile_param_varname(const std::string & varName)
	{
		uint8_t RefCode = 0;
		uint16_t RefData = 0;
		std::vector<uint8_t> resultData;

		// do localvar lookup first, 
		RefData = prepare_localvar(varName);
		if (RefData != 0)
		{
			int i = RefData - 1;
			if (Misc::StringUtils::lowerCase(mLocalVarList[i].first) == "short" ||
				Misc::StringUtils::lowerCase(mLocalVarList[i].first) == "long")
			{
				RefCode = 0x73; // short/long localvar
			}
			else if (Misc::StringUtils::lowerCase(mLocalVarList[i].first) == "float")
			{
				RefCode = 0x66; // float (or ref) localvar
			}
			else
			{
				abort("mLocalVarList contains invalid datatype\n");
				resultData.clear();
				return resultData;
			}
		}

		if (RefData == 0)
		{
			RefData = prepare_reference(varName);
			if (RefData != 0)
			{
				if (mParseMode == 1)
				{
					RefCode = 0x47; // global formID
				}
				else
				{
					RefCode = 0x72; // param reference
				}
			}
		}

		if (RefData != 0)
		{
			if (mParseMode == 1)
			{
				uint8_t OpCode_Push = 0x20;
				resultData.push_back(OpCode_Push);
			}
			if (bUseCommandReference)
			{
				uint8_t cmdRefCode = 0x72;
				uint16_t cmdRefIndex = prepare_reference(mCommandReference);
				resultData.push_back(cmdRefCode);
				for (int i = 0; i<2; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&cmdRefIndex)[i]);
				bUseCommandReference = false;
				mCommandReference = "";
			}
			resultData.push_back(RefCode);
			for (int i = 0; i<2; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);
		}

		return resultData;
	}

	uint16_t ScriptConverter::prepare_localvar(const std::string& varName)
	{
		uint16_t RefData = 0;

		// do localvar lookup first, 
		for (int i = 0; i < mLocalVarList.size(); i++)
		{
			if (Misc::StringUtils::lowerCase(varName) == Misc::StringUtils::lowerCase(mLocalVarList[i].second))
			{
				return RefData = i + 1;
			}
		}

		return RefData;
	}

	uint16_t ScriptConverter::prepare_reference(const std::string & refName)
	{
		uint16_t RefData = 0;

		uint32_t formID;
		if (Misc::StringUtils::lowerCase(refName) == "player" || Misc::StringUtils::lowerCase(refName) == "playerref")
			formID = 0x14;
		else
			formID = mESM.crossRefStringID(refName, "");

		if (formID != 0)
		{
			// search mReferenceList for crossReferenced formID, then add if missing
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
			RefData = refListIndex;
		}

		return RefData;
	}

	void ScriptConverter::nextLine(std::vector<struct Token>::iterator & tokenItem)
	{
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT && tokenItem != mTokenList.end())
			tokenItem++;
		// leave endline token on queue for parser
		return;
	}

	void ScriptConverter::error_mesg(std::string errString)
	{
		if (mScriptName != "")
			std::cout << "Script Converter Error (" << mScriptName << "): " << errString;
		else
			std::cout << "Script Converter Error <result script>: " << errString;
	}

	void ScriptConverter::abort(std::string error_string)
	{
		error_mesg(error_string);
		if (mFailureCode == 0)
			mFailureCode = 1;
	}

	bool ScriptConverter::HasFailed()
	{
		if (mFailureCode != 0)
			return true;

		return false;
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
			if (HasFailed()) return;
			std::istringstream preLineBufferB(preLineBufferA);
			std::string lineBuffer;
			while (std::getline(preLineBufferB, lineBuffer, '\r'))
			{
				if (HasFailed()) return;
				mLinePosition = 0;
				mReadMode = 0;
				read_line(lineBuffer);
			}
		}
	}

	void ScriptConverter::parser()
	{
		if (HasFailed()) return;
		mParseMode = 0;

		for (auto tokenItem = mTokenList.begin(); tokenItem != mTokenList.end(); tokenItem++)
		{
			if (HasFailed()) return;

			// TODO: keywords = moddisposition, goodbye, setfight, additem, showmap, "set control to 1", getJournalIndex, getdisposition, addtopic, 
			//			modpcfacrep, journal, pcraiserank, pcclearexpelled, messagebox, addspell
			// mParseMode: 0 -- start new expression at endofine
			// mPraseMode: 1 -- start new expression at close parenthesis

			if (mParseMode == 0) // Ready-Mode to begin new statement
			{
				parse_statement(tokenItem);
				continue;
			}

			if (mParseMode == 1) // Build expression
			{
				while (tokenItem->type != TokenType::endlineT && tokenItem != mTokenList.end())
				{
					parse_expression(tokenItem);
					tokenItem++;
				}
				// TODO: do specific expression completion for IF or SET
				if (tokenItem != mTokenList.end())
					parse_expression(tokenItem);
				continue;
			}

			// unhandled mode, reset mode and skip to new line
			mParseMode = 0;
			nextLine(tokenItem);
			continue;

		}

	}

	bool ScriptConverter::Token::IsHigherPrecedence(const std::vector<struct Token>::iterator & tokenItem)
	{
		int precedenceA=0, precedenceB=0;

		if (str[0] == '*') precedenceA = 3;
		else if (str[0] == '+') precedenceA = 2;
		else if (str[0] == '-') precedenceA = 2;
		else if (str[0] == '=') precedenceA = 1;
		else if (str[0] == '>') precedenceA = 1;
		else if (str[0] == '<') precedenceA = 1;

		if (tokenItem->str[0] == '*') precedenceB = 3;
		else if (tokenItem->str[0] == '+') precedenceB = 2;
		else if (tokenItem->str[0] == '-') precedenceB = 2;
		else if (tokenItem->str[0] == '=') precedenceB = 1;
		else if (tokenItem->str[0] == '>') precedenceB = 1;
		else if (tokenItem->str[0] == '<') precedenceB = 1;

		if (precedenceA > precedenceB)
			return true;

		return false;
	}

}

