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

#include <components/esm/esmwriter.hpp>
#include <components/misc/stringops.hpp>
#include <components/to_utf8/to_utf8.hpp>
#include <apps/opencs/model/doc/document.hpp>

#define IS_CMD(A) Misc::StringUtils::lowerCase(cmdString) == Misc::StringUtils::lowerCase(A)
#define IS_SUBCMD(A) Misc::StringUtils::lowerCase(cmdString).find( Misc::StringUtils::lowerCase(A) ) != std::string::npos

namespace ESM
{

	ScriptConverter::ScriptConverter(const std::string & scriptText, ESM::ESMWriter& esm, CSMDoc::Document& doc) : mScriptText(scriptText),
		mESM(esm), mDoc(doc)
	{
		// Initialize ScriptContext
		mCurrentContext.blockName = "";
		mCurrentContext.codeBlockDepth = 0;

		setup_dictionary();
		lexer();

	}

	void ScriptConverter::processScript()
	{
		parser();

		// finalize result script translation
		if (bIsFullScript != true && mCurrentContext.convertedStatements.empty() != true)
		{
			// append script line: "Call mwDialogHelperFinalizeFunc"
			std::vector<uint8_t> argdata;
			uint16_t sizeParams = 2;
			uint16_t countParams = 1;
			std::string argString = "mwDialogHelperFinalizeFunc";
			std::string cmdString = "Call";
			std::string scriptSIG = "SCPT";
			uint16_t funcRef = prepare_reference(argString, scriptSIG, 100);
			// compile argstring
			// 01 05 ...
			countParams = 0x0501;
			// 00 52 +(global ref index)
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
			// number of arguments
			argdata.push_back(0x00);
			std::stringstream convertedStatement;
			convertedStatement << "\r\n" << cmdString << " " << argString;
			mCurrentContext.convertedStatements.push_back(convertedStatement.str());
			// compile
			uint16_t OpCode = getOpCode(cmdString);
			sizeParams += argdata.size();
			compile_command(OpCode, sizeParams, countParams, argdata);
		}

		// Finalize Byte Buffer
		if (bIsFullScript != true && mCurrentContext.compiledCode.empty() != true)
		{
			mCompiledByteBuffer.insert(mCompiledByteBuffer.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end());
			mCurrentContext.compiledCode.clear();
		}

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
//			exportStr << *statement << std::endl;
			exportStr << *statement << "\r\n";
		}
//		exportStr << std::endl << oldCode;
		exportStr << "\r\n" << oldCode;

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
		if (HasFailed() == true)
			return 0;
		else
			return mCompiledByteBuffer.size();
	}

	uint16_t ScriptConverter::LookupLocalVarIndex(const std::string & varName)
	{
		uint16_t retData = 0;
		retData = prepare_localvar(varName);
		return retData;
	}

	std::string ScriptConverter::lookupLocalVarType(const std::string & varName)
	{
		int retData = 0;
		retData = prepare_localvar(varName);
		if (retData == 0)
			return "";
		return mLocalVarList[retData-1].first;
	}

	std::string ScriptConverter::lookupLocalVarType(int varIndex)
	{
		if (varIndex == 0 || mLocalVarList.empty())
			return "";
		return mLocalVarList[varIndex-1].first;
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
					lineBuffer[mLinePosition] == '*' ||
					lineBuffer[mLinePosition] == '/' ||
					lineBuffer[mLinePosition] == '&' ||
					lineBuffer[mLinePosition] == '|' ||
					lineBuffer[mLinePosition] == '.' )
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
//		bool done = false;
		bool bMemberOperator=false;
		int tokenType = TokenType::number_literalT;
		std::string tokenStr;
		while (mLinePosition < lineBuffer.size())
		{
			if ( (tokenType == TokenType::identifierT) && (lineBuffer[mLinePosition] == '.') )
			{
				bMemberOperator=true;
				break;
			}
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

			// add member operator if present
			if (bMemberOperator)
			{
				mTokenList.push_back(Token(TokenType::operatorT, "."));
				mLinePosition++;
			}

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

		case '/':
			// /=
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

		case '&':
			if (lineBuffer[mLinePosition] == '&')
				tokenStr += lineBuffer[mLinePosition++];
			break;

		case '|':
			if (lineBuffer[mLinePosition] == '|')
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
		Debug_CurrentScriptLine << tokenItem->str << " ";
		if (tokenItem->type == TokenType::endlineT)
		{
			bUseCommandReference = false;
			bUseVarReference = false;
			mCommandRef_EDID = "";
			mVarRef_mID= "";
			mNewStatement = true;
			Debug_CurrentScriptLine.str(""); Debug_CurrentScriptLine.clear();
		}
		else if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
		{
			std::string possibleRef = tokenItem->str;
			std::string refEDID = "";
			std::string refSIG = "";
			std::string refVal = "";
			lookup_reference(possibleRef, refEDID, refSIG, refVal);

			tokenItem++;
			if (tokenItem->str == "->")
			{
				bUseCommandReference = true;
				if (refEDID != "")
				{
					mCommandRef_EDID = refEDID;
				}
				else
				{
					mCommandRef_EDID = mESM.generateEDIDTES4(possibleRef, 0, "");
					abort("ERROR: could not find appropriate reference for: " + mCommandRef_EDID + " (" + possibleRef + ")\n");
				}
				return;
			}
			else if (tokenItem->str == ".")
			{
				// unless this is an if/set expression, then treat as a referenced command
				if (mParseMode == 1)
				{
					error_mesg("parse_statement(): unexpected expression mode encountered.\n");
					bUseVarReference = true;
					if (refEDID != "")
						mVarRef_mID = possibleRef;
				}
				else
				{
					error_mesg("parse_statement(): unexpected \".\" operator in a non-expression line.\n");
					bUseCommandReference = true;
					if (refEDID != "")
					{
						mCommandRef_EDID = refEDID;
					}
					else
					{
						mCommandRef_EDID = mESM.generateEDIDTES4(possibleRef, 0, "");
						abort("ERROR: could not find appropriate reference for: " + mCommandRef_EDID + " (" + possibleRef + ")\n");
					}
					prepare_reference(mCommandRef_EDID, refSIG, 100);
				}
				return;
			}
			// putback
			tokenItem--;

			if (mNewStatement == true)
			{
				std::stringstream errMesg;
				errMesg << "parse_statement(): Unhandled Identifier Command: <" << tokenItem->TypeToString() << "> " << tokenItem->str << std::endl << "DEBUG LINE: " << Debug_CurrentScriptLine.str() << std::endl;
				abort(errMesg.str());
			}
			mNewStatement = false;
			return;
		}
		else if (tokenItem->type == TokenType::keywordT)
		{
			parse_keyword(tokenItem);
			mNewStatement = false;
			return;
		} // keyword
		else
		{
			if (mNewStatement == true)
			{
				std::stringstream errMesg;
				errMesg << "Unhandled Token: <" << tokenItem->TypeToString() << "> '" << tokenItem->str << "'" << std::endl << "DEBUG LINE: " << Debug_CurrentScriptLine.str() << std::endl;
				abort(errMesg.str());
			}
			mNewStatement = false;
		}
		return;

	}

	bool ScriptConverter::parse_expression_generate_eval(std::vector<struct Token>::iterator & tokenItem)
	{
//		return false;

		// DEBUG OUTPUT
		std::cout << "\n\n=================================================\nPARSE_EXPRESSION_GENERATE_EVAL STARTING:\n=====================================================\n";

		// Sanity-checks, previous token must be IF keyword
		--tokenItem;
		if (tokenItem->str == "(") --tokenItem;
		if ( tokenItem->str != "if" || mParseMode != 1)
		{
			std::cout << "SANITY CHECKS: if / mParseMode, FAILED, token= " << tokenItem->str << ", mParseMode=" << mParseMode << "\n";
			abort("parse_expression_generate_eval: unsupported script context");
			tokenItem++;
			if (tokenItem->str == "(") tokenItem++;
			return false;
		}
		tokenItem++;
		if (tokenItem->str == "(") tokenItem++;

		std::cout << "SANITY CHECKS: if ( statement, PASSED\n";

/**
		// framework for full command support
		while (tokenItem->type != ESM::ScriptConverter::TokenType::endlineT)
		{
			// grab first token and parse sub expression
			if (tokenItem->type == ESM::ScriptConverter::TokenType::keywordT)
			{
				// ....
			}
			else if (tokenItem->type == ESM::ScriptConverter::TokenType::operatorT)
			{
				// ...
			}
			else if (tokenItem->type == ESM::ScriptConverter::TokenType::number_literalT)
			{
				// ...
			}
			else
			{
				abort("parse_expression_generate_eval: unsupported expression, expected keyword token");
				return false;
			}
			tokenItem++;
		}
**/
		// build eval script line
		// ...
		// compile
		// ...

		// Hardcoded getsoundplaying
		if (Misc::StringUtils::lowerCase(tokenItem->str) != "getsoundplaying")
		{
			std::cout << "SANITY CHECKS: getsoundplaying, FAILED, token=" << tokenItem->str << "\n";
			abort("parse_expression_generate_eval: unsupported keyword in expression:" + tokenItem->str);
			return false;
		}
		std::cout << "SANITY CHECKS: getsoundplaying, PASSED\n";

		// 1. parse getsoundplaying command
		// 2. parse comparison operator
		// 3. parse operand2

		std::string scriptFuncName = "mwGetSoundPlaying";
		std::string scriptSIG = "SCPT";
		uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
		tokenItem++;
		std::string argString = tokenItem->str;
		std::string soundSIG = "SOUN";
		bool bEvalArgString = false;
		bool bNeedsDialogHelper = false;
		if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper, soundSIG, true) == false)
		{
			std::cout << "sub_parse_arg(" << argString << "), FAILED\n";
			abort("parse_expression_generate_eval: getsoundplaying: sub_parse_arg() failed: argString=[" + argString + "] tokenItem=<" + tokenItem->TypeToString() + "> [" + tokenItem->str + "]");
			return false;
		}
		std::cout << "sub_parse_arg(" << argString << "), PASSED\n";
		uint16_t soundRef = prepare_reference(argString, soundSIG, 100);
		// compose command string
		std::string commandString = "(Call " + scriptFuncName + " " + argString + ")";
		std::vector<uint8_t> argdata;
		// compile subfunction command
		argdata.push_back(0x58);
		argdata.push_back(0x00);
		argdata.push_back(0x00);
		// 'Call' Function
		argdata.push_back(0x83);
		argdata.push_back(0x18);
		// sizeparams
		argdata.push_back(0x0E);
		argdata.push_back(0x00);
		// compile argstring
		// 01 05 00 52 +(global ref index)
		argdata.push_back(0x01);
		argdata.push_back(0x05);
		argdata.push_back(0x00);
		argdata.push_back(0x52);
		for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
		argdata.push_back(0x01);
		argdata.push_back(0x05);
		argdata.push_back(0x00);
		argdata.push_back(0x52);
		for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&soundRef)[i]);

		// parse remainder of exression
		tokenItem++;
		if (tokenItem->type != ESM::ScriptConverter::TokenType::operatorT)
		{
			std::cout << "SANITY CHECKS: operator, FAILED\n";
			abort("parse_expression_generate_eval: expected operator, instead: " + tokenItem->str);
			return false;
		}
		std::cout << "SANITY CHECKS: operator, PASSED\n";
		std::string operatorString = tokenItem->str;

		tokenItem++;
		if (tokenItem->type != ESM::ScriptConverter::TokenType::number_literalT)
		{
			std::cout << "SANITY CHECKS: operand, FAILED\n";
			abort("parse_expression_generate_eval: expected number literal, instead: " + tokenItem->str);
			return false;
		}
		std::cout << "SANITY CHECKS: operand, PASSED\n";
		std::string operandString = tokenItem->str;

		if (operandString != "0")
		{
			std::cout << "SANITY CHECKS: '0', FAILED\n";
			abort("parse_expression_generate_eval: expected operand=0, instead: operand=" + operandString);
			return false;
		}
		std::cout << "SANITY CHECKS: '0', PASSED\n";
		if (operatorString != "==")
		{
			std::cout << "SANITY CHECKS: '==', FAILED\n";
			abort("parse_expression_generate_eval: expected operator='==', instead: operator=" + operatorString);
			return false;
		}
		std::cout << "SANITY CHECKS: '==', PASSED\n";

		// compile operand and operator
		argdata.push_back(0x42);
		argdata.push_back(0x00); // hardcoded zero operand
		argdata.push_back(0x04); // hardcoded '==' operator


		std::stringstream convertedStatement;
		convertedStatement << "Eval ( " << commandString << " " << operatorString  << " " << operandString << ")";
		if (mParseMode != 1)
		{
			std::cout << "SANITY CHECKS: mParseMode, FAILED\n";
			abort("parse_expression_generate_eval: expected mParseMode == 1");
			return false;
		}
		else // mParseMode == 1 aka parse_expression
		{
			std::cout << "SANITY CHECKS: mParseMode, PASSED\n";
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			// remove '(' if that is the last statement 
			if (expressionLine.back() == '(')
			{
				expressionLine.pop_back();
				// get rid of space before '(' too
				expressionLine.pop_back();
			}
			expressionLine += " " + convertedStatement.str();
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size() - 1] = expressionLine;
			mOperatorExpressionStack.clear();
			gotoEOL(tokenItem);
		}

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 0;
		uint16_t sizeParams = 3; // start out with 3 extra bytes, per EVAL structure
		sizeParams += argdata.size();
		uint8_t countParams = 0x01;

//		OpCode = getOpCode("eval");
		OpCode = 0x1769;
		if (OpCode == 0)
		{
			std::cout << "SANITY CHECKS: OpCode, FAILED\n";
			abort ("parse_expression_generate_eval: OpCode not found or invalid.");
			return false;
		}
		std::cout << "SANITY CHECKS: OpCode, PASSED\n";

		compile_command(OpCode, sizeParams); 
		// countparams is only 1 byte, per EVAL structure
		mCurrentContext.compiledCode.push_back(countParams);
		// decrement argsize then rewrite after countParams, per EVAL structure
		--sizeParams;
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&sizeParams)[i]);
		mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), argdata.begin(), argdata.end());

		std::cout << "==============================================\nPARSE_EXPRESSION_GENERATE_EVAL COMPLETED SUCCESSFULLY:\n========================================\n";

		return true;

	}

	void ScriptConverter::parse_expression(std::vector<struct Token>::iterator & tokenItem)
	{
		std::vector<uint8_t> varData;

		Debug_CurrentScriptLine << tokenItem->str << " ";
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
				int offset = mSetCmd_StartPosition + 2; // +2 bytes for if/set opcode
				uint16_t statementLength = mCurrentContext.compiledCode.size() - offset - 2; // -2: do not count offset length in length
				for (int i=0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&statementLength)[i];

				// 15 00 [Length(2)] [Var(N)] [ExpLength(2)]
				offset += (2 + mSetCmd_VarSize); 
				uint16_t expressionLength = statementLength - mSetCmd_VarSize - 2; // -2: do not count offset length in length
				for (int i = 0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&expressionLength)[i];
			}
			else
			{
				// Assume all offsets are from 0 (start of new context block)
				// 16 00 [CompLen(2)]
				int offset = 2; // +2 bytes for if/set opcode
				uint16_t statementLength = mCurrentContext.compiledCode.size() - offset - 2; // -2: do not count offset length in length
				for (int i = 0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&statementLength)[i];

				// 16 00 [CompLen(2)] [JumpOps(2)] [ExpressionLength(2)]
				offset += (2 + 2);
				uint16_t expressionLength = statementLength - 2 - 2; // -2: do not count offset length in length
				for (int i = 0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&expressionLength)[i];
			}
			mParseMode = 0;
			bSetCmd = false;
			bUseCommandReference = false;
			bUseVarReference = false;
			mCommandRef_EDID = "";
			mVarRef_mID = "";
			return;
		}

		if (tokenItem->type == TokenType::operatorT)
		{
			parse_operator(tokenItem);
			bUseVarReference = false;
			mVarRef_mID = "";
			return;
		}

		if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
		{
			std::string possibleRef = tokenItem->str;
			std::string refEDID = "";
			std::string refSIG = "";
			std::string refVal = "";
			lookup_reference(possibleRef, refEDID, refSIG, refVal);

			tokenItem++;
			// check if it's part of a reference command or reference variable
			if (tokenItem->str == "->" || tokenItem->str == ".")
			{
				Debug_CurrentScriptLine << possibleRef << tokenItem->str;
				
				if (tokenItem->str == "->")
				{
					bUseCommandReference = true;
					if (refEDID != "")
					{
						mCommandRef_EDID = refEDID;
					}
					else
					{
						mCommandRef_EDID = mESM.generateEDIDTES4(possibleRef, 0, "");
						abort("ERROR: could not find appropriate reference for: " + mCommandRef_EDID + " (" + possibleRef + ")\n");
					}
					prepare_reference(mCommandRef_EDID, refSIG, 100);
				}
				else if (tokenItem->str == ".")
				{
					bUseVarReference = true;
					mVarRef_mID = possibleRef;
				}
				return;
			}
			// putback
			tokenItem--;

			// if not, treat as local/global variable
			// check if localvar, if not then check if reference
			std::string varString = tokenItem->str;
			bool bLookupFailed = false;
			if (bUseVarReference == false)
			{
				if (prepare_localvar(varString) != 0)
				{
					if (bIsFullScript == false)
					{
						bUseVarReference = true;
						mVarRef_mID = "mwDialogHelper";
					}
				}
				else
				{
					if (prepare_reference(refEDID, refSIG, 100) == 0)
						bLookupFailed = true;
					else
						varString = refEDID;
				}
			}
			if (bLookupFailed)
			{
				std::stringstream errMesg;
				errMesg << "identifier not found: (" << refSIG << ") " << varString << " or " << mESM.generateEDIDTES4(tokenItem->str, 0) << std::endl;
				abort(errMesg.str());
				return;
			}

			std::string expressionLine = mCurrentContext.convertedStatements.back();
			if (bUseVarReference)
			{
				std::string tokenPrefix = "";
				if (bNegationOperator)
					tokenPrefix = "-";
				
				refEDID=""; refSIG=""; refVal="";
				// lookup appropriate refEDID
				lookup_reference(mVarRef_mID, refEDID, refSIG, refVal);
				if (refEDID == "")
				{
					std::stringstream errMesg;
					errMesg << "could not resolve reference: (" << refSIG << ") " << mVarRef_mID << std::endl;
					abort(errMesg.str());
					bUseVarReference = false;
					mVarRef_mID = "";
					return;
				}
				expressionLine += " " + tokenPrefix + refEDID + "." + varString;
			}
			else
			{
				std::string tokenPrefix = "";
				if (bNegationOperator)
					tokenPrefix = "-";
				expressionLine += " " + tokenPrefix + varString;
			}
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size()-1] = expressionLine;

			// compile (includes external_local_var)
			varData = compile_param_varname(tokenItem->str, refSIG, 1);

			// Push OpCode only gets applied during expression parsing, should not be done in any other compiler function
			uint8_t OpCode_Push = 0x20;
			mCurrentContext.compiledCode.push_back(OpCode_Push);
			mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), varData.begin(), varData.end());
			if (bNegationOperator)
			{
				// push ~
				mCurrentContext.compiledCode.push_back(OpCode_Push);
				mCurrentContext.compiledCode.push_back('~');
				bNegationOperator = false;
			}
			// already reset in compile_param
//			bUseVarReference = false;
//			mVarRef_mID = "";
			return;
		}

		if (tokenItem->type == TokenType::number_literalT)
		{
			// translate and compile to buffer
			std::string tokenPrefix = "";
			if (bNegationOperator)
				tokenPrefix = "-";
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + tokenPrefix + tokenItem->str;
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size()-1] = expressionLine;
			// compile to buffer
			// Push OpCode only gets applied during expression parsing, should not be done in any other compiler function
			uint8_t OpCode_Push = 0x20;
			mCurrentContext.compiledCode.push_back(OpCode_Push);
			for (int i=0; i < tokenItem->str.size(); i++) mCurrentContext.compiledCode.push_back(tokenItem->str[i]);
			if (bNegationOperator)
			{
				// push ~
				mCurrentContext.compiledCode.push_back(OpCode_Push);				
				mCurrentContext.compiledCode.push_back('~');
				bNegationOperator = false;
			}

			bUseVarReference = false;
			mVarRef_mID = "";
			return;
		}

		// translate and compile to buffer
		if (tokenItem->type == TokenType::keywordT)
		{
			// Push OpCode only gets applied during expression parsing, should not be done in any other compiler function
			uint8_t OpCode_Push = 0x20;
			mCurrentContext.compiledCode.push_back(OpCode_Push);
			parse_keyword(tokenItem);
			bUseVarReference = false;
			mVarRef_mID = "";
			return;
		}

		std::stringstream errMesg;
		errMesg << "parse_expression(): Unhandled Token: <" << tokenItem->TypeToString() << "> '" << tokenItem->str << "'" << std::endl << "DEBUG LINE: " << Debug_CurrentScriptLine.str() << std::endl;
		abort(errMesg.str());
		bUseVarReference = false;
		mVarRef_mID = "";
		return;

	}

	void ScriptConverter::parse_operator(std::vector<struct Token>::iterator& tokenItem)
	{
		uint8_t OpCode_Push = 0x20;

		// TODO: detect "negation" operator and treat different from "minus" operator
		if (tokenItem->str == "-")
		{
			tokenItem--;
			if ( (tokenItem->type != TokenType::identifierT) &&
				(tokenItem->type != TokenType::number_literalT) &&
				(tokenItem->type != TokenType::string_literalT) &&
				(tokenItem->str != ")") )
			{
				tokenItem++;
				bNegationOperator = true;
				tokenItem->str = "~";
				return;
			}
			tokenItem++;
		}

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

		if ( (tokenItem->str == "+") ||
			(tokenItem->str == "-") ||
			(tokenItem->str == "*") ||
			(tokenItem->str == "/") ||
			(tokenItem->str == "==") ||
			(tokenItem->str == ">") ||
			(tokenItem->str == ">=") ||
			(tokenItem->str == "<") ||
			(tokenItem->str == "<=") ||
			(tokenItem->str == "!=") ||
			(tokenItem->str == "=") ||
			(tokenItem->str == "*=") ||
			(tokenItem->str == "/=") ||
			(tokenItem->str == "&&") ||
			(tokenItem->str == "||") ||
			(tokenItem->str == ".") )
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
		ErrMsg << "parse_operator(): unhandled operator: " << tokenItem->str << std::endl << "DEBUG LINE: " << Debug_CurrentScriptLine.str() << std::endl;
		abort(ErrMsg.str());
		return;

	}

	uint16_t ScriptConverter::getOpCode(std::string OpCodeString)
	{
		uint16_t OpCode=0;
		std::string cmdString = Misc::StringUtils::lowerCase(OpCodeString);

		if (cmdString == "enable")
		{
			OpCode = 0x1021;
		}
		else if (cmdString == "disable")
		{
			OpCode = 0x1022;
		}
		else if (cmdString == "addtopic")
		{
			OpCode = 0x1058;
		}
		else if (cmdString == "moddisposition")
		{
			OpCode = 0x1053;
		}
		else if (cmdString == "messagebox")
		{
			OpCode = 0x1000;
		}
		else if (cmdString == "addspell")
		{
			OpCode = 0x101C;
		}
		else if (cmdString == "removespell")
		{
			OpCode = 0x101D;
		}
		else if (cmdString == "playsound")
		{
			OpCode = 0x1026;
		}
		else if (cmdString == "playsound3d")
		{
			OpCode = 0x10B2;
		}
		else if (cmdString == "centeroncell")
		{
			OpCode = 0x011B;
		}
		else if (cmdString == "positioncell")
		{
			OpCode = 0x1079;
		}
		else if (cmdString == "cast")
		{
			OpCode = 0x101E;
		}
		else if (cmdString == "resurrect")
		{
			OpCode = 0x108C;
		}
		else if (cmdString == "equipitem") // aka equip
		{
			OpCode = 0x10EE;
		}
		else if (cmdString == "startcombat")
		{
			OpCode = 0x1016;
		}
		else if (cmdString == "return")
		{
			OpCode = 0x001E;
		}
		else if (cmdString == "end")
		{
			OpCode = 0x0011;
		}
		else if (cmdString == "else")
		{
			OpCode = 0x0017;
		}
		else if (cmdString == "elseif")
		{
			OpCode = 0x0018;
		}
		else if (cmdString == "if")
		{
			OpCode = 0x0016;
		}
		else if (cmdString == "activate")
		{
			OpCode = 0x100D;
		}
		else if (cmdString == "getitemcount")
		{
			OpCode = 0x102F;
		}
		else if (cmdString == "getrandompercent" || cmdString == "random" ) // aka random100
		{
			OpCode = 0x104D;
		}
		else if (cmdString == "getactorvalue") // aka get<skill> or get<attribute>
		{
			OpCode = 0x100E;
		}
		else if (cmdString == "getinterior" || cmdString == "isininterior") // aka IsInInterior
		{
			OpCode = 0x112C;
		}
		else if (cmdString == "getdisabled")
		{
			OpCode = 0x1023;
		}
		else if (cmdString == "getsecondspassed")
		{
			OpCode = 0x100C;
		}
		else if (cmdString == "getdistance")
		{
			OpCode = 0x1001;
		}
		else if (cmdString == "getlevel")
		{
			OpCode = 0x1050;
		}
		else if (cmdString == "getpos")
		{
			OpCode = 0x1006;
		}
		else if (cmdString == "getstage") // aka getjournalindex
		{
			OpCode = 0x103A;
		}
		else if (cmdString == "rotate") // aka rotateworld
		{
			OpCode = 0x1004;
		}
		else if (cmdString == "setangle") 
		{
			OpCode = 0x1009;
		}
		else if (cmdString == "getangle")
		{
			OpCode = 0x1008;
		}
		else if (cmdString == "startquest")
		{
			OpCode = 0x1036;
		}
		else if (cmdString == "stopquest")
		{
			OpCode = 0x1037;
		}
		else if (cmdString == "getincell")
		{
			OpCode = 0x1043;
		}
		else if (cmdString == "getbuttonpressed")
		{
			OpCode = 0x101F;
		}
		else if (cmdString == "addscriptpackage")
		{
			OpCode = 0x1097;
		}
		else if (cmdString == "stopcombat")
		{
			OpCode = 0x1017;
		}
		else if (cmdString == "setactorvalue")
		{
			OpCode = 0x100F;
		}
		else if (cmdString == "modactorvalue")
		{
			OpCode = 0x1010;
		}
		else if (cmdString == "getdeadcount")
		{
			OpCode = 0x1054;
		}
		else if (cmdString == "modpcfame")
		{
			OpCode = 0x10F8;
		}
		else if (cmdString == "getpcexpelled")
		{
			OpCode = 0x10C1;
		}
		else if (cmdString == "setpcexpelled")
		{
			OpCode = 0x10C2;
		}
		else if (cmdString == "lock")
		{
			OpCode = 0x1072;
		}
		else if (cmdString == "unlock")
		{
			OpCode = 0x1073;
		}
		else if (cmdString == "additem")
		{
			OpCode = 0x1002;
		}
		else if (cmdString == "removeitem")
		{
			OpCode = 0x1052;
		}
		else if (cmdString == "drop")
		{
			OpCode = 0x1057;
		}
		else if (cmdString == "getdisease")
		{
			OpCode = 0x1027;
		}
		else if (cmdString == "startconversation")
		{
			OpCode = 0x1056;
		}
		else if (cmdString == "showmap")
		{
			OpCode = 0x1055;
		}
		else if (cmdString == "setpos")
		{
			OpCode = 0x1007;
		}
		else if (cmdString == "refreshtopiclist")
		{
			OpCode = 0x1145;
		}
		else if (cmdString == "getlocked")
		{
			OpCode = 0x1005;
		}
		else if (cmdString == "advancepclevel")
		{
			OpCode = 0x10D5;
		}
		else if (cmdString == "advancepcskill")
		{
			OpCode = 0x10D4;
		}
		else if (cmdString == "autosave")
		{
			OpCode = 0x115E;
		}
		else if (cmdString == "canpaycrimegold")
		{
			OpCode = 0x107F;
		}
		else if (cmdString == "disableplayercontrols")
		{
			OpCode = 0x1061;
		}
		else if (cmdString == "dispellallspells")
		{
			OpCode = 0x1148;
		}
		else if (cmdString == "dropme")
		{
			OpCode = 0x10A6;
		}
		else if (cmdString == "enableplayercontrols")
		{
			OpCode = 0x1060;
		}
		else if (cmdString == "getalarmed")
		{
			OpCode = 0x103D;
		}
		else if (cmdString == "getarmorrating")
		{
			OpCode = 0x1051;
		}
		else if (cmdString == "getattacked")
		{
			OpCode = 0x103F;
		}
		else if (cmdString == "getbartergold")
		{
			OpCode = 0x1108;
		}
		else if (cmdString == "getclothingvalue")
		{
			OpCode = 0x1029;
		}
		else if (cmdString == "getcrimegold")
		{
			OpCode = 0x1074;
		}
		else if (cmdString == "setcrimegold")
		{
			OpCode = 0x1075;
		}
		else if (cmdString == "getdisposition")
		{
			OpCode = 0x104C;
		}
		else if (cmdString == "setatstart")
		{
			OpCode = 0x104C;
		}
		else if (cmdString == "placeatme" || cmdString == "placeatpc")
		{
			OpCode = 0x1025;
		}
		else if (cmdString == "getscale")
		{
			OpCode = 0x1018;
		}
		else if (cmdString == "setscale")
		{
			OpCode = 0x113C;
		}
		else if (cmdString == "getfactionrank")
		{
			OpCode = 0x1049;
		}
		else if (cmdString == "getparentcellwaterheight")
		{
			OpCode = 0x15CC;
		}
		else if (cmdString == "hasspell")
		{
			OpCode = 0x1462;
		}
		else if (cmdString == "getcellchanged")
		{
			OpCode = 0x1952;
		}
		else if (cmdString == "rand")
		{
			OpCode = 0x1417;
		}
		else if (cmdString == "set")
		{
			OpCode = 0x0015;
		}
		else if (cmdString == "gotojail")
		{
			OpCode = 0x107E;
		}
		else if (cmdString == "getcurrentaipackage")
		{
			OpCode = 0x106E;
		}
		else if (cmdString == "call")
		{
			OpCode = 0x1883;
		}
		else if (cmdString == "hasmagiceffect")
		{
			OpCode = 0x10D6;
		}
		else if (cmdString == "isspelltarget")
		{
			OpCode = 0x10DF;
		}
		else if (cmdString == "modactorvalue2")
		{
			OpCode = 0x1468;
		}
		else if (cmdString == "getdetected")
		{
			OpCode = 0x102D;
		}
		else if (cmdString == "playgroup")
		{
			OpCode = 0x1013;
		}
		else if (cmdString == "getquestrunning")
		{
			OpCode = 0x1038;
		}
		
		return OpCode;
	}

	void ScriptConverter::parse_placeatme(std::vector<struct Token>::iterator & tokenItem)
	{
		// output translated script text
		std::string cmdString = tokenItem->str;

		if (Misc::StringUtils::lowerCase(cmdString) == "placeatpc")
		{
			cmdString = "PlaceAtMe";
			bUseCommandReference = true;
			mCommandRef_EDID = "player";
		}

		// record 4 float args
		std::string strCount="", strDist="", strDir="";
		std::string argString, argSIG;
		bool bEvalArgString, bNeedsDialogHelper;

		tokenItem++;
		if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper, argSIG, true) == false)
		{
			abort("parse_placeatme():: sub_parse_arg() failed - ");
			return;
		}

		tokenItem++;
		// Check for EOL
		if (tokenItem->type != TokenType::endlineT)
		{
			if (sub_parse_arg(tokenItem, strCount, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
			{
				abort("parse_placeatme():: sub_parse_arg() failed - ");
				return;
			}

			tokenItem++;
			if (sub_parse_arg(tokenItem, strDist, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
			{
				abort("parse_placeatme():: sub_parse_arg() failed - ");
				return;
			}

			tokenItem++;
			if (sub_parse_arg(tokenItem, strDir, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
			{
				abort("parse_placeatme():: sub_parse_arg() failed - ");
				return;
			}

		}

		// translate statement
		std::stringstream convertedStatement;
		std::string argPrefix = "", cmdPrefix = "";

		if (bUseCommandReference)
			cmdPrefix = mCommandRef_EDID + ".";

		if (bNeedsDialogHelper)
			argPrefix = "mwDialogHelper.";

		std::string arglist="";
		if (strCount != "")
		{
			arglist = " " + strCount + " " + strDist + " " + strDir;
		}

		convertedStatement << cmdPrefix << cmdString << " " << argPrefix << argString << arglist;
		if (mParseMode == 0)
		{
			mCurrentContext.convertedStatements.push_back(convertedStatement.str());
		}
		else // mParseMode == 1 aka parse_expression
		{
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + convertedStatement.str();
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size() - 1] = expressionLine;
		}

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 0;
		uint16_t sizeParams = 2;
		uint16_t countParams = 1; 

		OpCode = getOpCode(cmdString);
		if (OpCode == 0)
		{
			std::stringstream errorMesg;
			errorMesg << "parse_placeatme(): unhandled command=" << cmdString << std::endl;
			abort(errorMesg.str());
			return;
		}

		std::vector<uint8_t> arg1data, arg2data, arg3data, argObjData;
		if (arglist != "")
		{
			countParams += 3;
			arg1data = compile_param_long(atof(strCount.c_str()));
			arg2data = compile_param_long(atof(strDist.c_str()));
			arg3data = compile_param_long(atof(strDir.c_str()));
			// combine all above into one block
			arg1data.insert(arg1data.end(), arg2data.begin(), arg2data.end());
			arg1data.insert(arg1data.end(), arg3data.begin(), arg3data.end());
		}
		argObjData = compile_param_varname(argString, argSIG, 4);

		sizeParams += arg1data.size() + argObjData.size();
		compile_command(OpCode, sizeParams, countParams, argObjData, arg1data);

		return;

	}

	void ScriptConverter::parse_positionCW(std::vector<struct Token>::iterator & tokenItem)
	{
		// output translated script text
		std::string cmdString = tokenItem->str;

		// record 4 float args
		std::string strX, strY, strZ, strR;
		std::string argString, argSIG;
		bool bEvalArgString, bNeedsDialogHelper;

		tokenItem++;
		if (sub_parse_arg(tokenItem, strX, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
		{
			abort("parse_positionCW():: sub_parse_arg() failed - ");
			return;
		}

		tokenItem++;
		if (sub_parse_arg(tokenItem, strY, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
		{
			abort("parse_positionCW():: sub_parse_arg() failed - ");
			return;
		}

		tokenItem++;
		if (sub_parse_arg(tokenItem, strZ, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
		{
			abort("parse_positionCW():: sub_parse_arg() failed - ");
			return;
		}

		tokenItem++;
		if (sub_parse_arg(tokenItem, strR, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
		{
			abort("parse_positionCW():: sub_parse_arg() failed - ");
			return;
		}

		tokenItem++;
		if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper, argSIG) == false)
		{
			abort("parse_positionCW():: sub_parse_arg() failed - ");
			return;
		}

		// translate statement
		std::stringstream convertedStatement;
		std::string argPrefix = "", cmdPrefix = "";

		if (bUseCommandReference)
			cmdPrefix = mCommandRef_EDID + ".";

		if (bNeedsDialogHelper)
			argPrefix = "mwDialogHelper.";

		convertedStatement << cmdPrefix << cmdString << " " << strX << " " << strY << " " << strZ << " " << strR << " " << argPrefix << argString;
		if (mParseMode == 0)
		{
			mCurrentContext.convertedStatements.push_back(convertedStatement.str());
		}
		else // mParseMode == 1 aka parse_expression
		{
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + convertedStatement.str();
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size() - 1] = expressionLine;
		}

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 0;
		uint16_t sizeParams = 2;
		sizeParams += (1+8)*4; // (1 + 8 bytes) * 4 (64bit floats)
		sizeParams += 3; // 1 + 2 bytes (ref param)
		uint16_t countParams = 5;

		OpCode = getOpCode(cmdString);
		if (OpCode == 0)
		{
			std::stringstream errorMesg;
			errorMesg << "parse_positionCW(): unhandled command=" << cmdString << std::endl;
			abort(errorMesg.str());
			return;
		}

		std::vector<uint8_t> argXdata, argYdata, argZdata, argRdata, argDestData;
		argXdata = compile_param_float(atof(strX.c_str()));
		argYdata = compile_param_float(atof(strY.c_str()));
		argZdata = compile_param_float(atof(strZ.c_str()));
		argRdata = compile_param_float(atof(strR.c_str()));

		// combine all above into one block
		argXdata.insert(argXdata.end(), argYdata.begin(), argYdata.end());
		argXdata.insert(argXdata.end(), argZdata.begin(), argZdata.end());
		argXdata.insert(argXdata.end(), argRdata.begin(), argRdata.end());

		argDestData = compile_param_varname(argString, argSIG, 4);

		if (sizeParams != (2 + argXdata.size() + argDestData.size()) )
		{
			abort("Parse_PositionCW: error, unexpected data size.");
			return;
		}
		compile_command(OpCode, sizeParams, countParams, argXdata, argDestData);

		return;

	}

	void ScriptConverter::parse_modfactionrep(std::vector<struct Token>::iterator & tokenItem)
	{
		// ModPCFacRep 2 "Mages Guild"
		// Set fbmwMGAdvancement.MageReputation TO fbmwMGAdvancement.MageReputation + 2

		std::string cmdString = "Set", numberLiteral = "", factionEDID = "";
		bool bEvalArg = false;
		bool bNeedsDialogHelper = false;
		std::string factionQuestEDID = "", factionQuestVar = "";
		std::string factSIG = "QUST";

		// arg1: parse number literal
		tokenItem++;
		if (sub_parse_arg(tokenItem, numberLiteral, bEvalArg, bNeedsDialogHelper) == false)
			return;

		if (numberLiteral[0] != '+' && numberLiteral[0] != '-')
		{
			// insert '+'
			numberLiteral = "+" + numberLiteral;
		}

		// arg2: parse string literal
		tokenItem++;
		if (sub_parse_arg(tokenItem, factionEDID, bEvalArg, bNeedsDialogHelper, "FACT") == false)
			return;

		// match string literal to guild/faction
		// reference ==> faction advancement quest ==> questEDID
		// localvar ==> MageReputation/Faction Reputation
		uint32_t refIndex, refLocalVarIndex;
		std::string arg2_lowercase = Misc::StringUtils::lowerCase(factionEDID);
		// Factions: mages, fighters, thieves, tribunal, imperial, hlaalu, telvanni, redoran, blades
		if (arg2_lowercase == "fbmwmagesguild")
		{
			// fbmwMGAdvancement.MageReputation
			factionQuestEDID = "fbmwMGAdvancement";
			factionQuestVar = "MageReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "fbmwfightersguild")
		{
			factionQuestEDID = "fbmwFGAdvancement";
			factionQuestVar = "FighterReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "0eastsempirescompany")
		{
			factionQuestEDID = "fbmwCOAdvancement";
			factionQuestVar = "CompanyReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "0hlaalu")
		{
			factionQuestEDID = "fbmwHHAdvancement";
			factionQuestVar = "HlaaluReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "0redoran")
		{
			factionQuestEDID = "fbmwHRAdvancement";
			factionQuestVar = "RedoranReputation";
			refLocalVarIndex = 1;
		}
		else if (arg2_lowercase == "0telvanni")
		{
			factionQuestEDID = "fbmwHTAdvancement";
			factionQuestVar = "TelvanniReputation";
			refLocalVarIndex = 6;
		}
		else if (arg2_lowercase == "0imperialscult")
		{
			factionQuestEDID = "fbmwICAdvancement";
			factionQuestVar = "CultReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "0imperialslegion")
		{
			factionQuestEDID = "fbmwILAdvancement";
			factionQuestVar = "LegionReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "0moragstong")
		{
			factionQuestEDID = "fbmwMTAdvancement";
			factionQuestVar = "MTReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "0thievessguild")
		{
			factionQuestEDID = "fbmwTGAdvancement";
			factionQuestVar = "ThiefReputation";
			refLocalVarIndex = 2;
		}
		else if (arg2_lowercase == "0temple")
		{
			factionQuestEDID = "fbmwTTAdvancement";
			factionQuestVar = "TempleReputation";
			refLocalVarIndex = 2;
		}
		else
		{
			factionQuestEDID = factionEDID + "AdvancementQuest";
			factionQuestVar = factionEDID + "Reputation";
			refLocalVarIndex = 1;
		}
		refIndex = prepare_reference(factionQuestEDID, factSIG, 100); // prepare reference...

		// add translation
		// "Set fbmwMGAdvancement.MageReputation TO fbmwMGAdvancement.MageReputation " "+/-" "number literal"
		std::string expressionLine = cmdString + " " + factionQuestEDID + "." + factionQuestVar + " to " + factionQuestEDID + "." + factionQuestVar + " " + numberLiteral;
		mCurrentContext.convertedStatements.push_back(expressionLine);

		// compile factionEDID.factionVar set expression
		std::vector<uint8_t> varData;
		// better to just hardcode factionAdvancementQuest and questVar
		uint8_t cmdRefCode = 0x72;
		varData.push_back(cmdRefCode);
		for (int i = 0; i<2; ++i) varData.push_back(reinterpret_cast<uint8_t *> (&refIndex)[i]);
		uint8_t localvarRefCode = 0x73;
		varData.push_back(localvarRefCode);
		for (int i = 0; i<2; ++i) varData.push_back(reinterpret_cast<uint8_t *> (&refLocalVarIndex)[i]);
		mSetCmd_VarSize = varData.size();

		// compile code
		// hardcode expression statement
		// Output byte-compiled code: 15 00 [Length(2)] [Var(N)] [ExpressionLength(2)] [Expression(N)]
		uint16_t OpCode = 0x15;
		uint16_t statementLen = varData.size(); // Var(N) + ExpressionLength(2) + Expression(N)
		uint16_t expressionLen = 0x00;

		mSetCmd_StartPosition = mCurrentContext.compiledCode.size(); // bookmark start of command
		compile_command(OpCode, statementLen);
		mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), varData.begin(), varData.end());
		for (int i = 0; i<2; i++) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&expressionLen)[i]);

		// compile expression ( varData [+ number literal] )
		uint8_t pushCode = 0x20;
		// pushback Back...
		mCurrentContext.compiledCode.push_back(pushCode);
		mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), varData.begin(), varData.end());
		mCurrentContext.compiledCode.push_back(pushCode);
		for (int i=1; i < numberLiteral.size(); i++) mCurrentContext.compiledCode.push_back(numberLiteral[i]);
		mCurrentContext.compiledCode.push_back(pushCode);
		mCurrentContext.compiledCode.push_back(numberLiteral[0]);

		// update startposition
		// 15 00 [Length(2)]
		int offset = mSetCmd_StartPosition + 2; // +2 bytes for if/set opcode
		uint16_t statementLength = mCurrentContext.compiledCode.size() - offset - 2; // -2: do not count offset length in length
		for (int i = 0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&statementLength)[i];

		// 15 00 [Length(2)] [Var(N)] [ExpLength(2)]
		offset += (2 + mSetCmd_VarSize);
		uint16_t expressionLength = statementLength - mSetCmd_VarSize - 2; // -2: do not count offset length in length
		for (int i = 0; i<2; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&expressionLength)[i];

	}

	// Set [varName] To [Expression]
	void ScriptConverter::parse_set(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string varSIG = "";
		std::string varString = "";
		bool bIsLocalVar=false;
		int varIndex;
		std::vector<uint8_t> varData;
		bool bNeedsDialogHelper = false;

		// start building expression
		mOperatorExpressionStack.clear();

		tokenItem++;
		// variable token
		if (tokenItem->type == TokenType::keywordT)
		{
			if ((Misc::StringUtils::lowerCase(tokenItem->str) == "onactivate") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "ondeath") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onknockout") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onmurder") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcadd") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcdrop") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcequip") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpchitme") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onpcsoulgemuse") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "onrepair") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "menumode"))
			{
				// ignore callbacks
				gotoEOL(tokenItem);
				return;
			}
			else if (Misc::StringUtils::lowerCase(tokenItem->str) == "random100")
			{
				// issue error
				std::stringstream errStr;
				errStr << "WARNING: Random100 currently not supported as global var for set statement: [" << tokenItem->type << "] " << tokenItem->str << std::endl;
				error_mesg(errStr.str());
				gotoEOL(tokenItem);
				return;
			}
			else
			{
				// issue error
				std::stringstream errStr;
				errStr << "parse_set(): unexpected keyword (expected variable name): [" << tokenItem->type << "] " << tokenItem->str << std::endl;
				abort(errStr.str());
			}
		} 
		else if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
		{
			// Read variable
			std::string possibleRef = tokenItem->str;
			std::string refEDID = "";
			std::string refSIG = "";
			std::string refVal = "";
			lookup_reference(possibleRef, refEDID, refSIG, refVal);

			// check for member operator to see if we should treat as active variable or just parent reference of active variable
			tokenItem++;
			if (tokenItem->str == ".")
			{
				bUseVarReference = true;
				mVarRef_mID = possibleRef;
				tokenItem++;
				refEDID = ""; refSIG=""; refVal="";
				lookup_reference(tokenItem->str, refEDID, refSIG, refVal);
			}
			else
			{
				// putback
				tokenItem--;
			}

			varString = tokenItem->str;
			// check if localvar, if not then check if reference
			if (bUseVarReference == false)
			{
				if (prepare_localvar(varString) != 0)
				{
					if (bIsFullScript == false)
					{
						bNeedsDialogHelper = true;
					}
				}
				else
				{
					if (prepare_reference(refEDID, refSIG, 100) == 0)
					{
						std::stringstream errStr;
						errStr << "parse_set(): could not resolve reference name to refEDID [" << tokenItem->type << "] " << tokenItem->str << std::endl;
						abort(errStr.str());
					}
					else
					{
						varString = refEDID;
						varSIG = refSIG;
					}
				}
			}

		}
		else
		{
			std::stringstream errStr;
			errStr << "parse_set(): unexpected token type in set statement (expected variable name): [" << tokenItem->TypeToString() << "] " << tokenItem->str << std::endl;
			abort(errStr.str());
		}

		tokenItem++;
		// read "to" (sanity check)
		if (Misc::StringUtils::lowerCase(tokenItem->str) != "to")
		{
			std::stringstream errStr;
			errStr << "parse_set(): unexpected token type in set statement (expected \"to\" operator): [" << tokenItem->TypeToString() << "] " << tokenItem->str << std::endl;
			abort(errStr.str());
		}

		// Output translation
		std::stringstream cmdString;
		std::string varPrefix="";
		if (bUseVarReference)
		{
			std::string refEDID="", refSIG="", refVal="";
			lookup_reference(mVarRef_mID, refEDID, refSIG, refVal);
			
			cmdString << "Set" << " " << refEDID << "." << varString << " " << "To" << " ";
		}
		else
		{
			std::string varPrefix = "";
			if (bNeedsDialogHelper)
				varPrefix = "mwDialogHelper.";
			cmdString << "Set" << " " << varPrefix << varString << " " << "To" << " ";
		}
		mCurrentContext.convertedStatements.push_back(cmdString.str());

		// compile varData
		varData = compile_param_varname(varString, varSIG, (2 | 4));
		mSetCmd_VarSize = varData.size();

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

// already reset by compile_param_varname
//		bUseVarReference = false;
//		mVarRef_mID = "";

		return;
	}

	void ScriptConverter::parse_0arg(std::vector<struct Token>::iterator & tokenItem)
	{
		// output translated script text
		std::stringstream convertedStatement;
		std::string cmdString = tokenItem->str;
		bool bSkipCompile = false;
		bool bUseREF = false;

		if (Misc::StringUtils::lowerCase(cmdString) == "random100")
		{
			cmdString = "GetRandomPercent";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getcurrentweather")
		{
			bSkipCompile = true;
			bUseCommandReference = true;
			mCommandRef_EDID = "mwScriptHelper";
			auto varData = compile_external_localvar(mCommandRef_EDID, cmdString);
			mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), varData.begin(), varData.end());
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getinterior")
		{
			cmdString = "IsInInterior";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "cellchanged")
		{
			cmdString = "GetCellChanged";
/*
			bSkipCompile = true;
			bUseCommandReference = true;
			mCommandRef_EDID = "mwScriptHelper";
			auto varData = compile_external_localvar(mCommandRef_EDID, cmdString);
			mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), varData.begin(), varData.end());
*/
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getcommondisease")
		{
			cmdString = "GetDisease";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "clearinfoactor")
		{
//			cmdString = "refreshtopiclist";
			return;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getpccrimelevel")
		{
			cmdString = "GetCrimeGold";
			bUseCommandReference = true;
			mCommandRef_EDID = "player";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getwaterlevel")
		{
			cmdString = "GetParentCellWaterHeight";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getdisabled")
		{
			bUseREF = true;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setdelete")
		{
			// todo: check next token for 0 or 1 argument
			tokenItem++;
			if (tokenItem->str == "1")
			{
				cmdString = "DeleteReference";
				bUseREF = true;
			}
			else
			{
				// undelete??
				abort("parse_0arg(): setdelete 0, unhandled state: can not undelete a reference in Oblivion.");
			}
		}

		std::string cmdLine = cmdString;
		//----------------------------------------------
		if (bUseCommandReference)
		{
			cmdLine = mCommandRef_EDID + "." + cmdString;
		}

		convertedStatement << cmdLine;
		if (mParseMode == 0)
		{
			mCurrentContext.convertedStatements.push_back(convertedStatement.str());
		}
		else // mParseMode == 1 aka parse_expression
		{
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + convertedStatement.str();
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size() - 1] = expressionLine;
		}

		if (bSkipCompile)
		{
			bUseCommandReference = false;
			mCommandRef_EDID = "";
			return;
		}
		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 0;

		OpCode = getOpCode(cmdString);

		if (OpCode != 0)
		{
			compile_command(OpCode, 0);
		}
		else
		{
			abort("parse_0arg(): 0x0 opcode: " + cmdString + "\n");
		}

		return;

	}

	bool ScriptConverter::sub_parse_arg(std::vector<struct Token>::iterator &tokenItem, std::string &argString, bool &bEvalArgString, bool &bNeedsDialogHelper, std::string argSIG, bool bReturnBase)
	{
		bEvalArgString = false;
		bNeedsDialogHelper = false;

		if (tokenItem->type == TokenType::endlineT)
		{
			std::stringstream errorMesg;
			errorMesg << "sub_parse_arg(): unexpected end-of-line" << std::endl;
			error_mesg(errorMesg.str());
			return false;
		}

		if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
		{
			std::string possibleRef = tokenItem->str;
			std::string refEDID = "";
			std::string refSIG = argSIG;
			std::string refVal = "";
			if (bReturnBase == true) refVal += "+base";
			lookup_reference(possibleRef, refEDID, refSIG, refVal);

			tokenItem++;
			// check if it's part of a reference command or reference variable
			if (tokenItem->str == "->")
			{
				tokenItem++;
				if (bUseCommandReference == false)
				{
					bUseCommandReference = true;
				}
				else
				{
					abort("sub_parse_arg(): double command reference: possibleRef=" + possibleRef + ", mCommandRef=" + mCommandRef_EDID );
					return false;
				}
				if (refEDID != "")
				{
					mCommandRef_EDID = refEDID;
				}
				else
				{
					mCommandRef_EDID = mESM.generateEDIDTES4(possibleRef, 0, argSIG);
					abort("sub_parse_arg(): could not find appropriate reference for: " + mCommandRef_EDID + " (" + possibleRef + ")\n");
				}
			}
			else if (tokenItem->str == ".")
			{
				tokenItem++;
				if (tokenItem->type == TokenType::number_literalT)
				{
					// add decimal point to number literal and proceed
					std::string numLiteralString = tokenItem->str;
					auto deleteThisToken = tokenItem;
					tokenItem--;
					tokenItem->str = "0." + numLiteralString;
					mTokenList.erase(deleteThisToken);
					tokenItem--;
				}
				else if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
				{
					if (bUseVarReference == false)
					{
						bUseVarReference = true;
					}
					else
					{
						abort("sub_parse_arg(): double var reference: possibleRef=" + possibleRef + ", mVarRef=" + mVarRef_mID);
						return false;
					}
					mVarRef_mID = possibleRef;
				}
				else
				{
					// abort
					abort("sub_parse_arg(): unexpected token type following \".\" " + tokenItem->str + "\n");
					return false;
				}
			}
			else
			{
				// putback if not a reference phrase
				tokenItem--;
			}

//			mDialogHelperIndex = 0;
			if (prepare_localvar(possibleRef) != 0)
			{
				argString = possibleRef;
				if (bIsFullScript == false)
				{
					bNeedsDialogHelper = true;
				}
//				if (mDialogHelperIndex != 0)
//					bNeedsDialogHelper = true;
			}
			else
			{
				if (refEDID != "")
				{
					argString = refEDID;
				}
				else
				{
					std::stringstream errMesg;
					errMesg << "sub_parse_arg(): could not resolve arg into reference: " << possibleRef << "\n";
					abort(errMesg.str());
					return false;
				}
			}
			bEvalArgString = true;
		}
		else if (tokenItem->type == TokenType::number_literalT || tokenItem->str == "-" || tokenItem->str == "+")
		{
			if (tokenItem->str == "-" || tokenItem->str == "+")
			{
				argString = tokenItem->str;
				tokenItem++;
			}
			argString += tokenItem->str;
		}
		else
		{
			std::stringstream errorMesg;
			errorMesg << "sub_parse_arg(): unexpected token type: [" << tokenItem->type << "] token=" << tokenItem->str << std::endl;
			abort(errorMesg.str());
			return false;
		}

		return true;
	}

	void ScriptConverter::parse_1arg(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string cmdString = "", argString = "";
		bool bEvalArgString = false;
		bool bUseBinaryData = false;
		bool bNeedsDialogHelper = false;
		std::vector<uint8_t> argdata;
		std::string argSIG = "";
		bool bSkipArgParse = false;
		bool bReturnBase = false;

		int getAVresult = -1;
		bool argForceFloat = false;

		uint16_t OpCode = 0;
		uint16_t sizeParams = 2;
		uint16_t countParams = 1;

		cmdString = tokenItem->str;

		if (Misc::StringUtils::lowerCase(cmdString) == "equip")
		{
			cmdString = "EquipItem";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "pcexpelled")
		{
			cmdString = "GetPCExpelled";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "modreputation")
		{
			cmdString = "ModPCFame";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getresistdisease")
		{
			cmdString = "GetActorValue";
			argString = "ResistDisease";
			bSkipArgParse = true;
			bUseBinaryData = true;
			uint16_t actorvalue = 63; // hardcoded with TES4 AV for ResistDisease
			for (int i = 0; i < 2; i++) argdata.push_back(reinterpret_cast<uint8_t *> (&actorvalue)[i]);
		}
		else if ( (getAVresult=check_get_set_mod_AV_command(cmdString)) >= 0)
		{
			if (sub_parse_get_set_mod_AV_command(tokenItem, getAVresult, cmdString, argString, argdata) == false)
			{
				abort("parse_1arg(): getresistdisease: error processing Set/Mod AV command, tokenStr=" + tokenItem->str);
				return;
			}
			bSkipArgParse = true;
			bUseBinaryData = true;
		}
		else if ( (Misc::StringUtils::lowerCase(cmdString) == "getpos")
			|| (Misc::StringUtils::lowerCase(cmdString) == "getangle") )
		{
			bSkipArgParse = true;
			bUseBinaryData = true;
			tokenItem++;
			argString = tokenItem->str;
			std::string axisString = Misc::StringUtils::lowerCase(argString);
			if (axisString == "x")
			{
				axisString = "X";
			}
			else if (axisString == "y")
			{
				axisString = "Y";
			}
			else if (axisString == "z")
			{
				axisString = "Z";
			}
			else
			{
				abort("parse_1arg(): GetPos/GetAngle(): invalid axis\n");
				return;
			}
			uint8_t rawchar = axisString[0];
			argdata.push_back(rawchar);
			argString = axisString;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "addtopic")
		{
			argSIG = "DIAL";
		}
		else if ( (Misc::StringUtils::lowerCase(cmdString) == "playsoundvp")
			|| (Misc::StringUtils::lowerCase(cmdString) == "playsound3dvp")
			)
		{
			// remove 'vp' from command
			cmdString = cmdString.substr(0, cmdString.size()-2);
			// any need to remove extra argument data ??
		}
		else if ((Misc::StringUtils::lowerCase(cmdString) == "playloopsound3dvp")
			|| (Misc::StringUtils::lowerCase(cmdString) == "playloopsound3d")
			)
		{
			cmdString = "playsound3d";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getsoundplaying")
		{
			if (mParseMode == 1 && bSetCmd == false)
			{
				if (parse_expression_generate_eval(tokenItem) == false)
				{
					if (bUseCommandReference)
					{
						bUseCommandReference = false;
						mCommandRef_EDID = "";
					}
					abort("parse_1arg(): parse_expression_generate_eval failed.");
				}
				return;
			}
			std::string scriptFuncName = "mwGetSoundPlaying";
			std::string scriptSIG = "SCPT";
			uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
			tokenItem++;
			std::string soundSIG = "SOUN";
			if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper, soundSIG, true) == false)
			{
				if (bUseCommandReference)
				{
					bUseCommandReference = false;
					mCommandRef_EDID = "";
				}
				abort("parse_1arg(): getsoundplaying: sub_parse_arg() failed: argString=[" + argString + "] tokenItem=<" + tokenItem->TypeToString() + "> [" + tokenItem->str + "]");
				return;
			}
			std::string soundName = argString;
			uint16_t soundRef = prepare_reference(soundName, soundSIG, 100);
			// compose new arg string
			argString = scriptFuncName + " " + soundName;
			// compile argstring
			// 01 05 ...
			countParams = 0x0501;
			// 00 52 +(global ref index)
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
			// number of arguments
			argdata.push_back(0x01);
			// argument header: 05 00 52 +(refindex)
			argdata.push_back(0x05);
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&soundRef)[i]);
			bUseBinaryData = true;
			bEvalArgString = false;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "centeroncell")
		{
			argSIG = "CELL";
			bSkipArgParse = true;
			bUseBinaryData = true;
			tokenItem++;
			argString = mESM.generateEDIDTES4(tokenItem->str, 0, argSIG);
			uint16_t stringSize = argString.size();
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&stringSize)[i]);
			for (auto char_it = argString.begin(); char_it != argString.end(); char_it++)
			{
				uint8_t rawchar = *char_it;
				argdata.push_back(rawchar);
			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getjournalindex")
		{
			cmdString = "GetStage";
			argSIG = "QUST";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getpccell")
		{
			cmdString = "GetInCell";
			bUseCommandReference = true;
			mCommandRef_EDID = "player";
			argSIG = "CELL";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "startscript")
		{
			cmdString = "StartQuest";
			bSkipArgParse = true;
			argSIG = "QUST";
			tokenItem++;
			if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
			{
				// process exceptions
				std::string scriptID = tokenItem->str;
				if (Misc::StringUtils::lowerCase(scriptID) == "mhtransportscript")
				{
					// change to: set MWDestinationCode to 70
					// ...
					cmdString = "Set";
					argString = "MWDestinationCode to 70";
					argdata = compile_param_varname("MWDestinationCode", "GLOB", 4);
					argdata.push_back(0x03);
					argdata.push_back(0x00);
					argdata.push_back(0x20);
					argdata.push_back('7');
					argdata.push_back('0');
					bUseBinaryData = true;
				}
				else if (Misc::StringUtils::lowerCase(scriptID) == "gotojailwhendone")
				{
					// change to gotojail equivalent
					//...
					cmdString = "GoToJail";
					argString = "";
					countParams = 0;
					sizeParams = 0;
					argdata.clear();
					bUseBinaryData = true;
				}
				else
				{
					argString = mESM.generateEDIDTES4(scriptID, 0, "SQUST");
					mESM.RegisterScriptToQuest(scriptID);
					bEvalArgString = true;
				}
			}
			else
			{
				abort("parse_1arg(): startscript: error processing: " + cmdString + "\n");
				return;
			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "stopscript")
		{
			cmdString = "StopQuest";
			bSkipArgParse = true;
			argSIG = "QUST";
			tokenItem++;
			if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
			{
				std::string scriptID = tokenItem->str;
				argString = mESM.generateEDIDTES4(scriptID, 0, "SQUST");
				mESM.RegisterScriptToQuest(scriptID);
				bEvalArgString = true;
			}
			else
			{
				abort("parse_1arg(): stopscript: error processing: " + cmdString + "\n");
				return;
			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "aifollow")
		{
			tokenItem++;
			argString = tokenItem->str;
			// check if player, then hardcode to FollowPlayer package
			if (Misc::StringUtils::lowerCase(argString).find("player") != std::string::npos)
			{
				// check if dialog result script vs. object/actor script
				if (bIsFullScript)
				{
					// if object/actor script, then use hardcoded followplayer
					// cmdline: AddScriptPackage ref:FollowPlayer
					// FORMID: 0x0009828A 
					cmdString = "AddScriptPackage";
					argString = "FollowPlayer";
					argSIG = "PACK";
					bSkipArgParse = true;
					bEvalArgString = true;
				}
				else
				{
					// if dialog result script, then use "Call mwDialogHelperFollowPCFunction"
					std::string scriptFuncName = "mwDialogHelperFollowPCFunction";
					cmdString = "Call";
					std::string scriptSIG = "SCPT";
					uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
					// compose new arg string
					argString = scriptFuncName;
					// compile argstring
					// 01 05 ...
					countParams = 0x0501;
					// 00 52 +(global ref index)
					argdata.push_back(0x00);
					argdata.push_back(0x52);
					for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
					// number of arguments
					argdata.push_back(0x00);
					bUseBinaryData = true;
					bEvalArgString = false;
				}

			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "aiwander")
		{
			tokenItem++;
			std::string wanderRangeStr = tokenItem->str;
			int wanderRange = atoi(wanderRangeStr.c_str());

			if (bIsFullScript)
			{
				// default
				argString = "aaaDefaultExploreCurrentLoc256";
				if (wanderRange == 0)
				{
					argString = "aaaDefaultStayAtEditorLocation";
				}
				else if (wanderRange <= 256)
				{
					argString = "aaaDefaultExploreCurrentLoc256";
				}
				else if (wanderRange <= 512)
				{
					argString = "aaaDefaultExploreEditorLoc512"; // todo: make currentloc512
				}
				else if (wanderRange <= 1000)
				{
					argString = "aaaDefaultExploreEditorLoc1024"; // todo: make currentloc1024
				}
				else if (wanderRange > 1000)
				{
					argString = "aaaDefaultExploreCurrentLoc3000";
				}
				// hardcode
				// cmdline: AddScriptPackage ref:FollowPlayer
				// FORMID: 0x0009828A 
				cmdString = "AddScriptPackage";
				argSIG = "PACK";
				bSkipArgParse = true;
				bEvalArgString = true;
			}
			else
			{
				// if dialog result script, then use "Call mwDialogHelperAIWanderFunction"
				std::string scriptFuncName = "mwDialogHelperAIWanderFunction";
				cmdString = "Call";
				std::string scriptSIG = "SCPT";
				uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
				// compose new arg string
				argString = scriptFuncName;
				// compile argstring
				// 01 05 ...
				countParams = 0x0501;
				// 00 52 +(global ref index)
				argdata.push_back(0x00);
				argdata.push_back(0x52);
				for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
				// number of arguments
				argdata.push_back(0x00);
				bUseBinaryData = true;
				bEvalArgString = false;
			}

		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getdeadcount")
		{
			bReturnBase = true;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "unlock")
		{
			argString = "1";
			bSkipArgParse = true;
			bEvalArgString = false;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getpcrank")
		{
			cmdString = "GetFactionRank";
			tokenItem++;
			if (tokenItem->type == TokenType::string_literalT ||
				tokenItem->type == TokenType::identifierT)
			{
				// set player faction rank....
				if (bUseCommandReference == false)
				{
					bUseCommandReference = true;
					mCommandRef_EDID = "player";
					std::string refSIG="";
					prepare_reference(mCommandRef_EDID, refSIG, 100);
				}
				// reset token and continue
				tokenItem--;
			}
			else
			{
				// get NPC factionrank...
				// lookup NPC's faction-->
				// 1. lookup npc
				if (bUseCommandReference)
				{
					// retrieve original NPC mID
					tokenItem--; //cmd
					tokenItem--; // reference operator '>'
					tokenItem--; // reference operator '-'
					tokenItem--; // reference id
					std::string npcID = tokenItem->str;
					// 2. lookup npc->factionID
					int index = mDoc.getData().getReferenceables().getIndex(npcID);
					auto npcRec = mDoc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(index);
					// argString = npc->factionID
					argString = npcRec.get().mFaction;
					bSkipArgParse = true;
					bEvalArgString = true;
				}
				else
				{
					abort("parse_1arg(): no reference for <GetFactionRank>");
				}
			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "forcegreeting")
		{
//			cmdString = "StartConversation";
//			argString = "player";
//			bEvalArgString = true;
			std::string scriptFuncName = "mwStartConversationHelperFunction";
			cmdString = "Call";
			std::string scriptSIG = "SCPT";
			uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
			// compose new arg string
			argString = scriptFuncName;
			// compile argstring
			// 01 05 ...
			countParams = 0x0501;
			// 00 52 +(global ref index)
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
			// number of arguments
			argdata.push_back(0x00);
			bUseBinaryData = true;
			bEvalArgString = false;

		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setpccrimelevel")
		{
			cmdString = "SetCrimeGold";
			bUseCommandReference = true;
			mCommandRef_EDID = "player";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getdisposition")
		{
			argString = "player";
			bEvalArgString = true;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setscale")
		{
			argForceFloat = true;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getspell")
		{
			cmdString = "HasSpell";
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "activate")
		{
			tokenItem++;
			if (tokenItem->type != TokenType::identifierT && tokenItem->type != TokenType::string_literalT)
			{
				// no arg, skip processing
				tokenItem--;
				bSkipArgParse = true;
				bUseBinaryData = true;
				countParams = 0;
				argdata.clear();
			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getitemcount")
		{
			// force baserecord lookup
			bReturnBase = true;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getcurrentaipackage")
		{
			tokenItem++;
			if (tokenItem->type == TokenType::endlineT)
			{
				tokenItem--;
			}
			else if (tokenItem->str != "==")
			{
				std::stringstream errmesg;
				errmesg << "parse_1arg(): getcurrentaipackage: Unsupported operation/argument: [" << tokenItem->str << "]\n";
				abort(errmesg.str());
				tokenItem--;
				return;
			}
			else
			{
				tokenItem++;
				if (tokenItem->type == TokenType::number_literalT)
				{
					std::string numberString = tokenItem->str;
					int numberVal = atoi(numberString.c_str());
					int newVal = -1;
					switch (numberVal)
					{
					case -1: // none
						newVal = numberVal;
						break;
					case 0: // wander
						newVal = 13;
						break;
					case 1: // travel
						newVal = 14;
						break;
					case 2: // escort
						newVal = 2;
						break;
					case 3: // follow
						newVal = 1;
						break;
					case 4: // activate
						newVal = 7;
						break;
					case 5: // pursue
						newVal = 5; // combat
						break;
					default:
						// error message
						std::stringstream errmesg;
						errmesg << "parse_1arg(): Unsupported number comparison with 'GetCurrentAIPackage ==' [" << tokenItem->str << "]\n";
						abort(errmesg.str());
						tokenItem--; tokenItem--;
						return;
					}
					std::stringstream strconversion;
					strconversion << newVal;
					// modify argument
					tokenItem->str = strconversion.str();
					tokenItem--; tokenItem--;
					bSkipArgParse = true;
					bUseBinaryData = true;
					argdata.clear();
					sizeParams = 0;
					countParams = 0;
				}
				else if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::string_literalT)
				{
					std::stringstream errmesg;
					errmesg << "parse_1arg(): Unsupported variable comparison with 'GetCurrentAIPackage ==' [" << tokenItem->str << "]\n";
					abort(errmesg.str());
					tokenItem--; tokenItem--;
					return;
				}
				else
				{
					std::stringstream errmesg;
					errmesg << "parse_1arg(): Unexpected argument type with 'GetCurrentAIPackage ==' [" << tokenItem->str << "]\n";
					abort(errmesg.str());
					tokenItem--; tokenItem--;
					return;
				}
			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setdisposition")
		{
			tokenItem++;
			if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper, argSIG, bReturnBase) == false)
			{
				if (bUseCommandReference)
				{
					bUseCommandReference = false;
					mCommandRef_EDID = "";
				}
				abort("parse_1arg(): setdisposition: sub_parse_arg() failed - ");
				return;
			}
			std::string dispoString = argString;
			int dispoVal;
			if (bEvalArgString == false)
			{
				dispoVal = atoi(dispoString.c_str());
			}
			else
			{
				std::stringstream errmesg;
				errmesg << "parse_1arg(): setdisposition: unsupported variable argument: [" << dispoString << "]\n";
				abort(errmesg.str());
				return;
			}
			cmdString = "Call";
			std::string scriptFuncName = "mwSetDispositionFunction";
			std::string scriptSIG = "SCPT";
			uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
			// compose new arg string
			argString = scriptFuncName + " " + dispoString;
			// compile argstring
			// 01 05 ...
			countParams = 0x0501;
			// 00 52 +(global ref index)
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
			// number of arguments
			argdata.push_back(0x01);
			// argument header: 04 00 42 +(byte value)
			argdata.push_back(0x04);
			argdata.push_back(0x00);
			argdata.push_back(0x42);
			uint8_t byteval = dispoVal;
			argdata.push_back(byteval);
			bUseBinaryData = true;
			bEvalArgString = false;

		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "getspelleffects")
		{
//			cmdString = "HasMagicEffect";
			cmdString = "IsSpellTarget";
			argSIG = "SPEL";
		}
		else if ((Misc::StringUtils::lowerCase(cmdString) == "fadein") ||
			(Misc::StringUtils::lowerCase(cmdString) == "fadeout"))
		{
			std::string scriptFuncName ="";
			if (Misc::StringUtils::lowerCase(cmdString).find("in") != std::string::npos)
				scriptFuncName = "mwFadeInFunction";
			else
				scriptFuncName = "mwFadeOutFunction";
			cmdString = "Call";
			std::string scriptSIG = "SCPT";
			uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
			// compose new arg string
			argString = scriptFuncName;
			// compile argstring
			// 01 05 ...
			countParams = 0x0501;
			// 00 52 +(global ref index)
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
			// number of arguments
			argdata.push_back(0x00);
			bUseBinaryData = true;
			bEvalArgString = false;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "gethello")
		{
			std::string scriptFuncName = "mwGetHelloFunction";
			cmdString = "Call";
			std::string scriptSIG = "SCPT";
			uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
			// compose new arg string
			argString = scriptFuncName;
			// compile argstring
			// 01 05 ...
			countParams = 0x0501;
			// 00 52 +(global ref index)
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
			// number of arguments
			argdata.push_back(0x00);
			bUseBinaryData = true;
			bEvalArgString = false;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "sethello")
		{
			tokenItem++;
			if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper, argSIG, bReturnBase) == false)
			{
				if (bUseCommandReference)
				{
					bUseCommandReference = false;
					mCommandRef_EDID = "";
				}
				abort("parse_1arg(): sethello: sub_parse_arg() failed - ");
				return;
			}
			std::string helloString = argString;
			int helloVal;
			if (bEvalArgString == false)
			{
				helloVal = atoi(helloString.c_str());
			}
			else
			{
				std::stringstream errmesg;
				errmesg << "parse_1arg(): sethello: Unsupported variable argument: helloString=[" << helloString << "]\n";
				abort(errmesg.str());
				return;
			}
			cmdString = "Call";
			std::string scriptFuncName = "mwSetHelloFunction";
			std::string scriptSIG = "SCPT";
			uint16_t funcRef = prepare_reference(scriptFuncName, scriptSIG, 100);
			// compose new arg string
			argString = scriptFuncName + " " + helloString;
			// compile argstring
			// 01 05 ...
			countParams = 0x0501;
			// 00 52 +(global ref index)
			argdata.push_back(0x00);
			argdata.push_back(0x52);
			for (int i = 0; i<2; ++i) argdata.push_back(reinterpret_cast<uint8_t *> (&funcRef)[i]);
			// number of arguments
			argdata.push_back(0x01);
			// argument header: 04 00 42 +(byte value)
			argdata.push_back(0x04);
			argdata.push_back(0x00);
			argdata.push_back(0x42);
			uint8_t byteval = helloVal;
			argdata.push_back(byteval);
			bUseBinaryData = true;
			bEvalArgString = false;

		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "scriptrunning")
		{
			cmdString = "GetQuestRunning";
			argSIG = "SCPT";
		}


		//-------------------------------------------------
		if (bSkipArgParse == false && argString == "")
		{
			tokenItem++;
			if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper, argSIG, bReturnBase) == false)
			{
				if (bUseCommandReference)
				{
					bUseCommandReference = false;
					mCommandRef_EDID = "";
				}
				abort("parse_1arg(): sub_parse_arg() failed: argString=[" + argString + "] tokenItem=<" + tokenItem->TypeToString() + "> [" + tokenItem->str + "]");
				return;
			}
		}

		// translate statement
		std::stringstream convertedStatement;
		std::string argPrefix = "", cmdPrefix="";

		if (bUseCommandReference)
			cmdPrefix = mCommandRef_EDID + ".";

		if (bNeedsDialogHelper)
			argPrefix = "mwDialogHelper.";

		convertedStatement << cmdPrefix << cmdString << " " << argPrefix << argString;
		if (mParseMode == 0)
		{
			mCurrentContext.convertedStatements.push_back(convertedStatement.str());
		}
		else // mParseMode == 1 aka parse_expression
		{
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + convertedStatement.str();
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size() - 1] = expressionLine;
		}

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		if (bEvalArgString)
		{
			if (argString == "")
			{
				// abort error
				abort("parse_1arg(): empty argString");
				return;
			}
			argdata = compile_param_varname(argString, argSIG, 4);
		}
		else if (bUseBinaryData == false)
		{
			if ((argString.find(".") == std::string::npos)
				&& (argForceFloat == false))
				argdata = compile_param_long(atoi(argString.c_str()));
			else
				argdata = compile_param_float(atof(argString.c_str()));
		}

		OpCode = getOpCode(cmdString);
		if (OpCode == 0)
		{
			std::stringstream errorMesg;
			errorMesg << "parse_1arg(): unhandled command (No assigned OpCode)=" << cmdString << std::endl;
			abort(errorMesg.str());
			if (bUseCommandReference)
			{
				bUseCommandReference = false;
				mCommandRef_EDID = "";
			}
			return;
		}

		sizeParams += argdata.size();
		compile_command(OpCode, sizeParams, countParams, argdata);

		return;

	}

	void ScriptConverter::parse_2arg(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string cmdString = "", arg1String = "", arg2String = "";
		bool bEvalArg1 = false, bEvalArg2 = false;
		bool bUseBinaryData1 = false, bUseBinaryData2 = false;
		bool bNeedsDialogHelper1 = false, bNeedsDialogHelper2 = false;
		std::string arg1SIG = "", arg2SIG = "";
		std::vector<uint8_t> arg1data, arg2data;
		uint16_t sizeParams = 2;
		uint16_t countParams = 2;

		int getAVresult = -1;
		bool arg1forceFloat = false;
		bool arg2forceFloat = false;

		cmdString = tokenItem->str;

		if (Misc::StringUtils::lowerCase(cmdString) == "moddisposition")
		{
			arg1String = "player";
			bEvalArg1 = true;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setdisposition")
		{
			// todo: replace with getdisposition + moddisposition...
			abort("Unhandled command: SetDisposition\n");
			return;
		}
		else if ( (Misc::StringUtils::lowerCase(cmdString) == "rotateworld")
			|| (Misc::StringUtils::lowerCase(cmdString) == "rotate") 
			|| (Misc::StringUtils::lowerCase(cmdString) == "setangle")
			|| (Misc::StringUtils::lowerCase(cmdString) == "setpos")
			)
		{
			bEvalArg1 = false;
			bUseBinaryData1 = true;
			if (Misc::StringUtils::lowerCase(cmdString).find("rotate") != std::string::npos)
			{
				cmdString = "Rotate";
			}
			else if (Misc::StringUtils::lowerCase(cmdString).find("angle") != std::string::npos)
			{
				cmdString = "SetAngle";
				arg2forceFloat = true;
			}
			else
			{
				cmdString = "SetPos";
				arg2forceFloat = true;
			}
			tokenItem++;
			if (Misc::StringUtils::lowerCase(tokenItem->str) == "x")
				arg1String = "X";
			if (Misc::StringUtils::lowerCase(tokenItem->str) == "y")
				arg1String = "Y";
			if (Misc::StringUtils::lowerCase(tokenItem->str) == "z")
				arg1String = "Z";
			uint8_t rawchar = arg1String[0];
			arg1data.push_back(rawchar);
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setfight")
		{
			cmdString = "SetActorValue";
			arg1String = "Aggression";
			bUseBinaryData1 = true;
			uint16_t actorvalue = 33; // hardcoded with TES4 AV for Aggression
			for (int i = 0; i < 2; i++) arg1data.push_back(reinterpret_cast<uint8_t *> (&actorvalue)[i]);
			bUseBinaryData2 = true;			
			tokenItem++;
			if (sub_parse_arg(tokenItem, arg2String, bEvalArg2, bNeedsDialogHelper2) == false)
			{
				abort("parse_2arg(): error parse argument2.\n");
				return;
			}
			int aggressionVal = 0;
			int fightval = atoi(arg2String.c_str());
			if (fightval == 0) aggressionVal = 0;
			else if (fightval <= 30) aggressionVal = 5;
			else if (fightval >= 80) aggressionVal = 6;
			arg2data = compile_param_long(aggressionVal);
			std::stringstream newString;
			newString << aggressionVal;
			arg2String = newString.str();
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "modfight")
		{
			// skip ??
			cmdString = "ModActorValue";
			arg1String = "Aggression";
			bUseBinaryData1 = true;
			uint16_t actorvalue = 33; // hardcoded with TES4 AV for Aggression
			for (int i = 0; i < 2; i++) arg1data.push_back(reinterpret_cast<uint8_t *> (&actorvalue)[i]);
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setflee")
		{
			cmdString = "SetActorValue";
			arg1String = "Confidence";
			bUseBinaryData1 = true;
			uint16_t actorvalue = 34; // hardcoded with TES4 AV for Confidence
			for (int i = 0; i < 2; i++) arg1data.push_back(reinterpret_cast<uint8_t *> (&actorvalue)[i]);
			bUseBinaryData2 = true;
			tokenItem++;
			if (sub_parse_arg(tokenItem, arg2String, bEvalArg2, bNeedsDialogHelper2) == false)
			{
				abort("parse_2arg(): error parse argument2.\n");
				return;
			}
			int confidenceVal = 0;
			int fleeVal = atoi(arg2String.c_str());
			confidenceVal = 100 - fleeVal;
			arg2data = compile_param_long(confidenceVal);
			std::stringstream newString;
			newString << confidenceVal;
			arg2String = newString.str();
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "setalarm")
		{
			cmdString = "SetActorValue";
			arg1String = "Responsibility";
			bUseBinaryData1 = true;
			uint16_t actorvalue = 36; // hardcoded with TES4 AV for Responsibility
			for (int i = 0; i < 2; i++) arg1data.push_back(reinterpret_cast<uint8_t *> (&actorvalue)[i]);
			bUseBinaryData2 = true;
			tokenItem++;
			if (sub_parse_arg(tokenItem, arg2String, bEvalArg2, bNeedsDialogHelper2) == false)
			{
				abort("parse_2arg(): error parse argument2.\n");
				return;
			}
			int responsibilityVal = 0;
			int alarmVal = atoi(arg2String.c_str());
			// use 3 checks in case we want to modify curve mapping
			if (alarmVal == 0) responsibilityVal = 30;
			else if (alarmVal < 70) responsibilityVal = alarmVal + 30;
			else if (alarmVal >= 70) responsibilityVal = 100;
			arg2data = compile_param_long(responsibilityVal);
			std::stringstream newString;
			newString << responsibilityVal;
			arg2String = newString.str();
		}
		else if ( (getAVresult=check_get_set_mod_AV_command(cmdString)) >= 0)
		{
			if (sub_parse_get_set_mod_AV_command(tokenItem, getAVresult, cmdString, arg1String, arg1data) == false)
			{
				abort("error processing Set/Mod AV command\n");
				return;
			}
			bUseBinaryData1 = true;
		}
		else if ( (Misc::StringUtils::lowerCase(cmdString) == "modcurrenthealth")
			|| (Misc::StringUtils::lowerCase(cmdString) == "modcurrentfatigue")
			|| (Misc::StringUtils::lowerCase(cmdString) == "modcurrentmagicka") )
		{
			if (Misc::StringUtils::lowerCase(cmdString).find("health") != std::string::npos)
			{
				arg1String = "Health";
			}
			else if (Misc::StringUtils::lowerCase(cmdString).find("fatigue") != std::string::npos)
			{
				arg1String = "Fatigue";
			}
			else if (Misc::StringUtils::lowerCase(cmdString).find("magicka") != std::string::npos)
			{
				arg1String = "Magicka";
			}
			std::string compareString = Misc::StringUtils::lowerCase(arg1String);
			uint16_t byte_actorvalue=0;
			for (int i = 0; i < 39; i++)
			{
				if (compareString == actorvalueStrings[i])
				{
					byte_actorvalue = i;
					break;
				}
			}
			for (int i = 0; i < 2; i++) arg1data.push_back(reinterpret_cast<uint8_t *> (&byte_actorvalue)[i]);
			cmdString = "ModActorValue2";
			bUseBinaryData1 = true;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "pcclearexpelled")
		{
			cmdString = "SetPCExpelled";
			bEvalArg1 = false;
			arg2String = "0";
			bEvalArg2 = false;
			tokenItem++;
			if (tokenItem->type != TokenType::identifierT && tokenItem->type != TokenType::string_literalT)
			{
				// todo: lookup factionID based on current npc...
/*
				// 1. get npcID... but from where???? --> this is dynamic run-time data, not set at compile-time!! each TES3 NPC running script would need a separate script in TES4!
				std::string npcID = "";
				// 2. lookup npc->factionID
				int index = mDoc.getData().getReferenceables().getIndex(npcID);
				auto npcRec = mDoc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(index);
				// 3. argString = npc->factionID
				arg1String = npcRec.get().mFaction;
*/
				// abort script until above npcid resolution problem is solved
				abort("SetPCExpelled/PCClearExpelled: no faction specified\n");
				return;
			}
			else
			{
				tokenItem--;
			}
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "lock")
		{
			bEvalArg1 = false;
			arg2String = "1";
			bEvalArg2 = false;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "showmap")
		{
			tokenItem++;
			arg1String = "mw" + mESM.generateEDIDTES4(tokenItem->str, 2) + "MapMarker";
			arg2String = "0";
			bEvalArg2 = false;
		}
		else if (Misc::StringUtils::lowerCase(cmdString) == "random")
		{
			cmdString = "Rand";
			arg1String = "0";
			bEvalArg1 = false;
			arg1forceFloat = true;
			// read next argument, change to int, subtract one, switch back to ascii for text translation
			tokenItem++;
			if (tokenItem->type != number_literalT)
			{
				abort("ERROR: invalid Rand argument: " + tokenItem->str + "\n");
				return;
			}
			int tempRand = atoi(tokenItem->str.c_str());
			if (tempRand == 100)
			{
				// replace Rand command with GetRandomPercet
				cmdString = "GetRandomPercent";
				sizeParams = 0;
				countParams = 0;
				arg1String = " ";
				arg2String = " ";
				arg1data.clear();
				arg2data.clear();
				bEvalArg1 = false;
				bEvalArg2 = false;
				bUseBinaryData1 = true;
				bUseBinaryData2 = true;
			}
			else
			{
				tempRand--;
				std::stringstream randArg;
				randArg << tempRand;
				arg2String = randArg.str();
				arg2forceFloat = true;
				bEvalArg2 = false;
			}
		}


		if (arg1String == "")
		{
			tokenItem++;
			if (sub_parse_arg(tokenItem, arg1String, bEvalArg1, bNeedsDialogHelper1) == false)
			{
				abort("parse_2arg(): error parsing argument1.\n");
				return;
			}
		}

		if (arg2String == "")
		{
			tokenItem++;
			if (sub_parse_arg(tokenItem, arg2String, bEvalArg2, bNeedsDialogHelper2) == false)
			{
				abort("parse_2arg(): error parse argument2.\n");
				return;
			}
		}

		// translate statement
		std::stringstream convertedStatement;
		std::string cmdPrefix = "";
		std::string arg1Prefix = "";
		std::string arg2Prefix = "";
		if (bUseCommandReference)
			cmdPrefix = mCommandRef_EDID + ".";

		if (bNeedsDialogHelper1)
			arg1Prefix = "mwDialogHelper.";
		if (bNeedsDialogHelper2)
			arg2Prefix = "mwDialogHelper.";

		convertedStatement << cmdPrefix << cmdString << " " << arg1Prefix << arg1String << " " << arg2Prefix << arg2String;
		if (mParseMode == 0)
		{
			mCurrentContext.convertedStatements.push_back(convertedStatement.str());
		}
		else // mParseMode == 1 aka parse_expression
		{
			std::string expressionLine = mCurrentContext.convertedStatements.back();
			expressionLine += " " + convertedStatement.str();
			mCurrentContext.convertedStatements[mCurrentContext.convertedStatements.size() - 1] = expressionLine;
		}


		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		uint16_t OpCode = 0;

		OpCode = getOpCode(cmdString);
		if (OpCode == 0)
		{
			std::stringstream errorMesg;
			errorMesg << "parse_2arg(): unhandled command=" << cmdString << std::endl;
			abort(errorMesg.str());
			return;
		}

		if (bEvalArg1)
		{
			arg1data = compile_param_varname(arg1String, arg1SIG, 4);
		}
		else if (bUseBinaryData1 == false)
		{
			if ((arg1String.find(".") == std::string::npos)
				&& (arg1forceFloat == false))
				arg1data = compile_param_long(atoi(arg1String.c_str()));
			else
				arg1data = compile_param_float(atof(arg1String.c_str()));
		}

		if (bEvalArg2)
		{
			arg2data = compile_param_varname(arg2String, arg2SIG, 4);
		}
		else if (bUseBinaryData2 == false)
		{
			if ((arg1String.find(".") == std::string::npos)
				&& (arg2forceFloat == false))
				arg2data = compile_param_long(atoi(arg2String.c_str()));
			else
				arg2data = compile_param_float(atof(arg2String.c_str()));
		}

		sizeParams += arg1data.size() + arg2data.size();
		compile_command(OpCode, sizeParams, countParams, arg1data, arg2data);

		return;

	}

	void ScriptConverter::parse_messagebox(std::vector<struct Token>::iterator & tokenItem)
	{
		// parse variable count strings arglist
		std::string cmdString = tokenItem->str;
		std::vector<std::string> argList;
//		std::stringstream argList;
		int messageBoxMode = 0; // 0=single string, 1=multi-string/buttons, 2=variables

		// must have at least argSIG, argREFString ??? for each reference???

		uint16_t OpCode = 0;
		uint16_t sizeParams = 4;
		uint16_t countParams = 0;

//		tokenItem++;
//		if (sub_parse_arg(tokenItem, argString, bEvalArgString, bNeedsDialogHelper) == false)
//			return;

		tokenItem++;
//		std::string rawText = tokenItem->str;
		if (tokenItem->type == TokenType::string_literalT)
		{
			argList.push_back(tokenItem->str);
			sizeParams += (4 + tokenItem->str.size());
			// first text string not part of countParams
		}
		else
		{
			// through error, as first arg must be string literal...?
//			argList.push_back(tokenItem->str);
			abort("MessageBox Error: first argument is not string literal: '" + tokenItem->str + "'\n");
			return;
		}
		tokenItem++;
		while (tokenItem->type != TokenType::endlineT)
		{
			if (tokenItem->type == TokenType::string_literalT)
			{
				messageBoxMode = 1; // multi-string buttons
				argList.push_back(tokenItem->str);
				sizeParams += (4 + tokenItem->str.size());
				countParams++;
			}
			else
			{
				messageBoxMode = 2; // variables (references)
				sizeParams += 3;
				countParams++;

				// if mode == 2, resolve into references
				std::string argString;
				bool bEvalString, bNeedsDialogHelper;
				if (sub_parse_arg(tokenItem, argString, bEvalString, bNeedsDialogHelper) == false)
				{
					abort("parse_messagebox: could not resolve variable='" + tokenItem->str + "'\n");
					return;
				}
				if (bEvalString != true || bNeedsDialogHelper == true)
				{
					abort("parse_messagebox: unhandlded variable type='" + tokenItem->str + "'\n");
					return;
				}
				argList.push_back(argString);
			}
			tokenItem++;
		}

/*
		// debug search for script strings
		if (Misc::StringUtils::lowerCase(argList.str()).find("trial of") != std::string::npos)
		{
			std::string debugstr = "DEBUG: found string: " + argList.str() + "\n";
			OutputDebugString(debugstr.c_str());
		}
*/

		// translate statement
		std::stringstream convertedStatement;
		auto arg_item = argList.begin();
		convertedStatement << cmdString << " " << "\"" << *arg_item << "\"";
		arg_item++;
		while (arg_item != argList.end())
		{
			if (messageBoxMode == 1)
				convertedStatement << ", " << "\"" << *arg_item << "\"";
			else
				convertedStatement << ", " << *arg_item;
			arg_item++;
		}

		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
		OpCode = getOpCode(cmdString);
		if (OpCode == 0)
		{
			std::stringstream errorMesg;
			errorMesg << "parse_generic(): unhandled command=" << cmdString << std::endl;
			abort(errorMesg.str());
			return;
		}
		compile_command(OpCode, sizeParams);

		// message box text / first string literal
		arg_item = argList.begin();
		uint16_t argTextLen = arg_item->size();
		uint16_t stringSeparator = 0x01;
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&stringSeparator)[i]);
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&argTextLen)[i]);
		for (auto byte_it = arg_item->begin(); byte_it != arg_item->end(); byte_it++)
			mCurrentContext.compiledCode.push_back((uint8_t)*byte_it);

		arg_item++;
		// extra arguments
		if (messageBoxMode == 0)
		{
			for (int i = 0; i < 4; ++i) mCurrentContext.compiledCode.push_back(0);
		}
		else if (messageBoxMode == 1)
		{
			for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(0);
			for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&countParams)[i]);
			while (arg_item != argList.end())
			{
				argTextLen = arg_item->size();
				for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&stringSeparator)[i]);
				for (int i = 0; i < 2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&argTextLen)[i]);
				for (auto byte_it = arg_item->begin(); byte_it != arg_item->end(); byte_it++)
					mCurrentContext.compiledCode.push_back((uint8_t)*byte_it);
				arg_item++;
			}
		}
		else if (messageBoxMode == 2)
		{
			for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&countParams)[i]);
			while (arg_item != argList.end())
			{
				std::vector<uint8_t> argdata;
				std::string argSIG="";
				argdata = compile_param_varname(*arg_item, argSIG, 4);
				uint16_t dataSize = argdata.size();
				for (int i = 0; i < 2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&dataSize)[i]);
				// compile into var references...
				mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), argdata.begin(), argdata.end());
				arg_item++;
			}			
			for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(0);
		}

		return;

	}

	void ScriptConverter::parse_localvar(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string varType = tokenItem->str;

		tokenItem++;
		if (tokenItem->type == TokenType::identifierT || tokenItem->type == TokenType::keywordT)
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
		std::string ifString = tokenItem->str;
		bool bElseIf = false;
		mOperatorExpressionStack.clear();

		if (Misc::StringUtils::lowerCase(tokenItem->str) == "elseif")
		{
			bElseIf = true;
			pop_context_conditional(tokenItem);
		}

		tokenItem++;
		// check for bIsCallBack mode
		if (tokenItem->str == "(")
			tokenItem++;
		if (tokenItem->type == TokenType::keywordT && bElseIf == false)
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
				(Misc::StringUtils::lowerCase(tokenItem->str) == "menumode") ||
				(Misc::StringUtils::lowerCase(tokenItem->str) == "getaipackagedone") )
			{
				std::string tempCallbackName = tokenItem->str;
				// EXCEPTION CHECK for On<Event> == 0 reverse events... for now issue error and skip those events
				tokenItem++;
				if (tokenItem->str == "==" || tokenItem->str == ">=")
				{
					tokenItem++;
					if (tokenItem->str != "1")
					{
						abort("[" + tempCallbackName + "] unhandled callback condition: =='" + tokenItem->str + "'\n");
					}
				}
				else if (tokenItem->str == "!=")
				{
					tokenItem++;
					if (tokenItem->str != "0")
					{
						abort("[" + tempCallbackName + "] unhandled callback condition: =='" + tokenItem->str + "'\n");
					}
				}
				else if (tokenItem->str != ")" && tokenItem->type != TokenType::endlineT)
				{
					// unhandled On<Event> condition
					abort("[" + tempCallbackName + "] unhandled callback condition: '" + tokenItem->str + "'\n");
				}

				if (Misc::StringUtils::lowerCase(tempCallbackName) == "getaipackagedone")
				{
					tempCallbackName = "OnPackageDone";
				}
				else if (Misc::StringUtils::lowerCase(tempCallbackName) == "onpcequip")
				{
					tempCallbackName = "OnEquip";
				}
				else if (Misc::StringUtils::lowerCase(tempCallbackName) == "onpchitme")
				{
					tempCallbackName = "OnHit";
				}
				else if (Misc::StringUtils::lowerCase(tempCallbackName) == "onpcadd")
				{
					tempCallbackName = "OnAdd";
				}
				else if (Misc::StringUtils::lowerCase(tempCallbackName) == "onpcdrop")
				{
					tempCallbackName = "OnDrop";
				}

				// reset expression
				mOperatorExpressionStack.clear();
				// setup callback context and return
				// push context to stack
				mContextStack.push_back(mCurrentContext);
				// re-initialize current context
				mCurrentContext.bIsCallBack = true;
				mCurrentContext.codeBlockDepth++;
				mCurrentContext.JumpOps = 0;
				std::stringstream blockName;
				blockName << "if" << mCurrentContext.codeBlockDepth;
				mCurrentContext.blockName = blockName.str();
				mCurrentContext.compiledCode.clear();
				mCurrentContext.convertedStatements.clear();

				// output callback begin
				mCallBackName = tempCallbackName;
				std::stringstream callBackHeader;
				callBackHeader << "Begin" << " " << mCallBackName;
				mCurrentContext.convertedStatements.push_back(callBackHeader.str());

				// byte code: (OnActivate) 10 00 08 00 02 00 [BlockLen(4)] 00 00
				uint16_t BeginModeCode = 0x10;
				uint16_t ParamSize = 0x08;
				uint16_t CallbackType = 0x00;
				uint32_t BlockLen = 0;
				if (Misc::StringUtils::lowerCase(mCallBackName) == "onactivate")
				{
					CallbackType = 0x02;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "ondeath")
				{
					CallbackType = 0x0A;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "onequip")
				{
					CallbackType = 0x04;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "onunequip")
				{
					CallbackType = 0x05;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "menumode")
				{
					CallbackType = 0x01;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "onpackagedone")
				{
					CallbackType = 0x10;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "onmurder")
				{
					CallbackType = 0x0B;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "onhit")
				{
					CallbackType = 0x08;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "onknockout")
				{
					CallbackType = 0x0C;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "onadd")
				{
					CallbackType = 0x03;
				}
				else if (Misc::StringUtils::lowerCase(mCallBackName) == "ondrop")
				{
					CallbackType = 0x06;
				}
				else
				{
					abort("ERROR: unhandled callback event! " + mCallBackName + "\n");
				}

				compile_command(BeginModeCode, ParamSize);
				for (int i=0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *>(&CallbackType)[i]);
				for (int i=0; i<4; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *>(&BlockLen)[i]);
				for (int i=0; i<2; ++i) mCurrentContext.compiledCode.push_back(0);

				mParseMode = 0;
				return;
			}
		}
		// put back 1-2 tokens for further processing
		tokenItem--;
		if (tokenItem->str == "(")
			tokenItem--;

		// push context to stack
		mContextStack.push_back(mCurrentContext);
		// re-initialize current context
		mCurrentContext.bIsCallBack = false;
		mCurrentContext.codeBlockDepth++;
		mCurrentContext.JumpOps = 0;
		std::stringstream blockName;
		// the following "if" is just an ID for the block/context --> do not replace with ifString/elseif/variable
		blockName << "if" << mCurrentContext.codeBlockDepth;
		mCurrentContext.blockName = blockName.str();
		mCurrentContext.compiledCode.clear();
		mCurrentContext.convertedStatements.clear();

		// Output translation
		std::stringstream cmdString;
		cmdString << ifString << " ";
		mCurrentContext.convertedStatements.push_back(cmdString.str());

		// Compile IF statement header, then record byte position to update with JumpOffsets and ExpressionLength

		// 16 00 [CompLen(2)] [JumpOps(2)] [ExpressionLen(2)] [ExpressionBuffer(N)]
		uint16_t OpCode = getOpCode(ifString);
		uint16_t CompLen = 0x00; // complen (2 bytes)
		uint16_t JumpOps = 0x00; // jump operations (2 bytes) --number of operations to next conditional: elseif/else/endif
		uint16_t ExpressionLen = 0x00; // exp len ( 2 bytes)

		mSetCmd_StartPosition = mCurrentContext.compiledCode.size(); // bookmark start of command
		compile_command(OpCode, CompLen);
		for (int i=0; i<2; i++) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&JumpOps)[i]);
		for (int i=0; i<2; i++) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&ExpressionLen)[i]);

		// change parsemode to expression build mode
		mParseMode = 1;
		bSetCmd = false;

		return;
	}

	void ScriptConverter::parse_else(std::vector<struct Token>::iterator & tokenItem)
	{
		pop_context_conditional(tokenItem);

		// push context to stack
		mContextStack.push_back(mCurrentContext);
		// re-initialize current context
		mCurrentContext.bIsCallBack = false;
		mCurrentContext.codeBlockDepth++;
		mCurrentContext.JumpOps = 0;
		std::stringstream blockName;
		// the following "if" is just an ID for the block/context --> do not replace with ifString/elseif/variable
		blockName << "if" << mCurrentContext.codeBlockDepth;
		mCurrentContext.blockName = blockName.str();
		mCurrentContext.compiledCode.clear();
		mCurrentContext.convertedStatements.clear();

		// Output translation
		std::stringstream cmdString;
		cmdString << "else" << " ";
		mCurrentContext.convertedStatements.push_back(cmdString.str());

		// Compile IF statement header, then record byte position to update with JumpOffsets and ExpressionLength

		// 16 00 [CompLen(2)] [JumpOps(2)] [ExpressionLen(2)] [ExpressionBuffer(N)]
		uint16_t OpCode = getOpCode("else");
		uint16_t CompLen = 0x02; // complen=2 for else statement (2 bytes)
		uint16_t JumpOps = 0x00; // jump operations (2 bytes) --number of operations to next conditional: elseif/else/endif

		mSetCmd_StartPosition = mCurrentContext.compiledCode.size(); // bookmark start of command
		compile_command(OpCode, CompLen);
		for (int i = 0; i<2; i++) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&JumpOps)[i]);

		return;

	}

	void ScriptConverter::pop_context_conditional(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string endCmdString = tokenItem->str;

		// sanity check
		std::stringstream blockName;
		blockName << "if" << mCurrentContext.codeBlockDepth;

		if (mCurrentContext.blockName != blockName.str())
		{
			std::stringstream errMesg;
			errMesg << "pop_context_conditional(): mismatched code block end at code depth: " << mCurrentContext.codeBlockDepth;
			if (mCurrentContext.codeBlockDepth == 0)
			{
				error_mesg("WARNING:" + errMesg.str() + " - ignoring line.\n");
			}
			else
			{
				abort(errMesg.str() + " - aborting script compilation.\n");
			}
			return;
		}

		// pop environment off stack
		struct ScriptContext fromStack;
		fromStack.bIsCallBack = mContextStack.back().bIsCallBack;
		fromStack.blockName = mContextStack.back().blockName;
		fromStack.codeBlockDepth = mContextStack.back().codeBlockDepth;
		fromStack.compiledCode = mContextStack.back().compiledCode;
		fromStack.convertedStatements = mContextStack.back().convertedStatements;
		fromStack.JumpOps = mContextStack.back().JumpOps;
		mContextStack.pop_back();

		auto statementItem = mCurrentContext.convertedStatements.begin();
		if (statementItem != mCurrentContext.convertedStatements.end())
		{
			if (mCurrentContext.bIsCallBack)
			{
				mConvertedStatementList.push_back(*statementItem);
				mCallBackName = "";
			}
			else
			{
				fromStack.convertedStatements.push_back(*statementItem);
			}
			statementItem++;
		}
		while (statementItem != mCurrentContext.convertedStatements.end())
		{
			std::stringstream statementStream;
			statementStream << '\t' << *statementItem;
			if (mCurrentContext.bIsCallBack)
				mConvertedStatementList.push_back(statementStream.str());
			else
				fromStack.convertedStatements.push_back(statementStream.str());
			statementItem++;
		}

		std::stringstream endStatement;
		if (mCurrentContext.bIsCallBack)
		{
			endStatement << "End";
			std::string emptyLine = "";
			mConvertedStatementList.push_back(endStatement.str());
			mConvertedStatementList.push_back(emptyLine);
		}
		else if (Misc::StringUtils::lowerCase(endCmdString) == "endif")
		{
			endStatement << endCmdString;
			fromStack.convertedStatements.push_back(endStatement.str());
		}
		// do not put endstatement for else/elseif
		mCurrentContext.convertedStatements.clear();
		mCurrentContext.convertedStatements = fromStack.convertedStatements;

		// update BlockLength / JumpOffsets
		if (mCurrentContext.bIsCallBack)
		{
			int offset = 6;
			uint32_t blockLength = mCurrentContext.compiledCode.size() - offset - 2 - 4; // -4: additional correction for callbacks
			for (int i = 0; i<4; i++) mCurrentContext.compiledCode[offset + i] = reinterpret_cast<uint8_t *>(&blockLength)[i];
		}
		else
		{
			int jumpoffset = 4;
			uint16_t JumpOps = mCurrentContext.JumpOps-1;
			for (int i = 0; i<2; i++) mCurrentContext.compiledCode[jumpoffset + i] = reinterpret_cast<uint8_t *>(&JumpOps)[i];
		}

		// byte compile: insert compiled context block to global or current buffer
		if (mCurrentContext.bIsCallBack)
			mCompiledByteBuffer.insert(mCompiledByteBuffer.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end());
		else
			fromStack.compiledCode.insert(fromStack.compiledCode.end(), mCurrentContext.compiledCode.begin(), mCurrentContext.compiledCode.end());

		mCurrentContext.compiledCode.clear();
		mCurrentContext.compiledCode = fromStack.compiledCode;

		// reset context
		if (mCurrentContext.bIsCallBack == false)
		{
			mCurrentContext.JumpOps += fromStack.JumpOps; // for endif of inner context
		}
		else
		{
			mCurrentContext.JumpOps = fromStack.JumpOps;
		}
		mCurrentContext.blockName = fromStack.blockName;
		mCurrentContext.codeBlockDepth = fromStack.codeBlockDepth;
		mCurrentContext.bIsCallBack = fromStack.bIsCallBack;

		return;
	}

	int ScriptConverter::check_get_set_mod_AV_command(const std::string & commandString)
	{
		std::string compareString = Misc::StringUtils::lowerCase(commandString);

		if (compareString.size() < 3)
			return -1;

		std::string commandStem = compareString.substr(0, 3);
		if (commandStem != "get" && commandStem != "set" && commandStem != "mod")
			return -1;

		compareString = compareString.substr(3);

		for (int i = 0; i < 39; i++)
		{
//			if (compareString.compare(actorvalueStrings[i]) == 0)
			if (compareString == actorvalueStrings[i])
				return i;
		}

		return -1;
	}

	bool ScriptConverter::sub_parse_get_set_mod_AV_command(std::vector<struct Token>::iterator &tokenItem, int actorValue, std::string &cmdString, std::string &arg1String, std::vector<uint8_t> &arg1data)
	{
		cmdString = tokenItem->str;
		arg1String = cmdString.substr(3);

		if (IS_SUBCMD("get")) 
			cmdString = "GetActorValue";
		else if (IS_SUBCMD("set")) 
			cmdString = "SetActorValue";
		else if (IS_SUBCMD("mod")) 
			cmdString = "ModActorValue";

		if (actorValue > 32)
			return false;

		uint16_t byte_actorvalue = actorValue;
		for (int i = 0; i < 2; i++) arg1data.push_back(reinterpret_cast<uint8_t *> (&byte_actorvalue)[i]);

		return true;
	}

	void ScriptConverter::parse_endif(std::vector<struct Token>::iterator & tokenItem)
	{
		std::string endCmdString = tokenItem->str;
		bool bPrevContextWasCallBack = mCurrentContext.bIsCallBack;

		// set endcode prior to popping context
		uint32_t EndCode = 0x19; // endif code
		if (mCurrentContext.bIsCallBack)
		{
			EndCode = 0x11; // end begin-block
		}
		for (int i = 0; i<4; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&EndCode)[i]);

		pop_context_conditional(tokenItem);

		if (bPrevContextWasCallBack == false)
		{
			// increment 1 for "endif"
			mCurrentContext.JumpOps++;
		}

		return;

	}

	void ScriptConverter::parse_end(std::vector<struct Token>::iterator & tokenItem)
	{
		int offset;

		// sanity checks
		if (mCurrentContext.blockName != mScriptName ||
			mCurrentContext.codeBlockDepth > 0)
		{
			std::stringstream errMesg;
			errMesg << "ERROR: Early termination of script at blockdepth " << mCurrentContext.codeBlockDepth << std::endl;
			abort(errMesg.str());
			return;
		}
		else if (mCurrentContext.codeBlockDepth < 0)
		{
			std::stringstream errMesg;
			errMesg << "ERROR: multiple script ENDs encountered;" ;
			if (mCurrentContext.compiledCode.size() != 0)
				abort(errMesg.str() + " additional code detected after END, aborting.\n");
			else
				error_mesg(errMesg.str() + " no additional code detected, ignoring extra END.\n");
			return;
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

		mCurrentContext.codeBlockDepth--;
		gotoEOL(tokenItem);

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
		mCurrentContext.bIsCallBack = false;
		mCurrentContext.JumpOps = 0;

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
		std::string tokenString = Misc::StringUtils::lowerCase(tokenItem->str);

		if (tokenString == "choice")
		{
			parse_choice(tokenItem);
		}
		else if ((tokenString == "positioncell")
			|| (tokenString == "positionworld"))
		{
			parse_positionCW(tokenItem);
		}
		else if ((tokenString == "placeatme")
			|| (tokenString == "placeatpc"))
		{
			parse_placeatme(tokenItem);
		}
		else if (tokenString == "modpcfacrep")
		{
			parse_modfactionrep(tokenItem);
		}
		else if (tokenString == "messagebox")
		{
			parse_messagebox(tokenItem);
		}
		else if (tokenString == "journal")
		{
			parse_journal(tokenItem);
		}
		else if (tokenString == "goodbye")
		{
			bGoodbye = true;
		}
		else if (tokenString == "additem")
		{
			parse_addremoveitem(tokenItem, false);
		}
		else if (tokenString == "removeitem")
		{
			parse_addremoveitem(tokenItem, true);
		}
		else if (tokenString == "drop")
		{
			parse_addremoveitem(tokenItem, true);
		}
		else if (tokenString == "begin")
		{
			parse_begin(tokenItem);
		}
		else if (tokenString == "end")
		{
			parse_end(tokenItem);
		}
		else if ((tokenString == "if")
			|| (tokenString == "elseif"))
		{
			parse_if(tokenItem);
		}
		else if (tokenString == "else")
		{
			parse_else(tokenItem);
		}
		else if (tokenString == "endif")
		{
			parse_endif(tokenItem);
		}
		else if (tokenString == "set")
		{
			parse_set(tokenItem);
		}
		else if ((tokenString == "short")
			|| (tokenString == "long")
			|| (tokenString == "float")
			)
		{
			parse_localvar(tokenItem);
		}
		else if ((tokenString == "disable")
			|| (tokenString == "enable")
			|| (tokenString == "return")
			|| (tokenString == "random100")
			|| (tokenString == "getdisabled")
			|| (tokenString == "getsecondspassed")
			|| (tokenString == "getlevel")
			|| (tokenString == "getcurrentweather")
			|| (tokenString == "getinterior")
			|| (tokenString == "cellchanged")
			|| (tokenString == "getbuttonpressed")
			|| (tokenString == "stopcombat")
			|| (tokenString == "getcommondisease")
			|| (tokenString == "clearinfoactor")
			|| (tokenString == "getlocked")
			|| (tokenString == "advancepcskill")
			|| (tokenString == "advancepclevel")
			|| (tokenString == "autosave")
			|| (tokenString == "canpaycrimegold")
			|| (tokenString == "dispellallspells")
			|| (tokenString == "dropme")
			|| (tokenString == "enableplayercontrols")
			|| (tokenString == "disableplayercontrols")
			|| (tokenString == "getalarmed")
			|| (tokenString == "getarmorrating")
			|| (tokenString == "getattacked")
			|| (tokenString == "getbartergold")
			|| (tokenString == "getclothingvalue")
			|| (tokenString == "getpccrimelevel") // getcrimegold
			|| (tokenString == "setatstart")
			|| (tokenString == "getwaterlevel")
			|| (tokenString == "getscale")
			|| (tokenString == "gotojail")
//			|| (tokenString == "setdelete")
			)
		{
			parse_0arg(tokenItem);
		}
		else if ((tokenString == "moddisposition")
			|| (tokenString == "cast")
			|| (tokenString == "rotate")
			|| (tokenString == "rotateworld")
			|| (tokenString == "setangle")
			|| (tokenString == "modfight")
			|| (tokenString == "setfight")
			|| (tokenString == "setflee")
			|| (tokenString == "setalarm")
//			|| (tokenString == "sethealth")
			|| (tokenString == "pcclearexpelled")
			|| (tokenString == "lock")
			|| (tokenString == "showmap")
			|| (tokenString == "setpos")
//			|| (tokenString == "setstrength")
//			|| (tokenString == "modstrength")
			|| (tokenString == "sethealth") || (tokenString == "setfatigue") || (tokenString == "setmagicka")
			|| (tokenString == "setstrength") || (tokenString == "setintelligence") || (tokenString == "setwillpower") || (tokenString == "setagility") || (tokenString == "setspeed") || (tokenString == "setendurance") || (tokenString == "setpersonality") || (tokenString == "setluck")
			|| (tokenString == "setarmorer") || (tokenString == "setathletics") || (tokenString == "setshortblade") || (tokenString == "setlongblade") || (tokenString == "setblock")
			|| (tokenString == "setblunt") || (tokenString == "setaxe") || (tokenString == "setspear") || (tokenString == "sethandtohand")
			|| (tokenString == "setheavyarmor") || (tokenString == "setmediumarmor") || (tokenString == "setlightarmor")
			|| (tokenString == "setalchemy") || (tokenString == "setalteration") || (tokenString == "setconjuration") || (tokenString == "setenchantment")
			|| (tokenString == "setdestruction") || (tokenString == "setillusion") || (tokenString == "setmysticism") || (tokenString == "setrestoration")
			|| (tokenString == "setacrobatics") || (tokenString == "setmarksman") || (tokenString == "setmercantile") || (tokenString == "setsecurity") || (tokenString == "setsneak") || (tokenString == "setspeechcraft")
			|| (tokenString == "modhealth") || (tokenString == "modfatigue") || (tokenString == "modmagicka")
			|| (tokenString == "modcurrenthealth") || (tokenString == "modcurrentfatigue") || (tokenString == "modcurrentmagicka")
			|| (tokenString == "modstrength") || (tokenString == "modintelligence") || (tokenString == "modwillpower") || (tokenString == "modagility") || (tokenString == "modspeed") || (tokenString == "modendurance") || (tokenString == "modpersonality") || (tokenString == "modluck")
			|| (tokenString == "modarmorer") || (tokenString == "modathletics") || (tokenString == "modshortblade") || (tokenString == "modlongblade") || (tokenString == "modblock")
			|| (tokenString == "modblunt") || (tokenString == "modaxe") || (tokenString == "modspear") || (tokenString == "modhandtohand")
			|| (tokenString == "modheavyarmor") || (tokenString == "modmediumarmor") || (tokenString == "modlightarmor")
			|| (tokenString == "modalchemy") || (tokenString == "modalteration") || (tokenString == "modconjuration") || (tokenString == "modenchantment")
			|| (tokenString == "moddestruction") || (tokenString == "modillusion") || (tokenString == "modmysticism") || (tokenString == "modrestoration")
			|| (tokenString == "modacrobatics") || (tokenString == "modmarksman") || (tokenString == "modmercantile") || (tokenString == "modsecurity") || (tokenString == "modsneak") || (tokenString == "modspeechcraft")
			|| (tokenString == "random")
			)
		{
			parse_2arg(tokenItem);
		}
		else if ((tokenString == "addtopic")
			|| (tokenString == "startcombat")
			|| (tokenString == "addspell")
			|| (tokenString == "removespell")
			|| (tokenString == "getspell")
			|| (tokenString == "modpccrimelevel")
			|| (tokenString == "modreputation")
			|| (tokenString == "getitemcount")
			|| (tokenString == "playsound")
			|| (tokenString == "playsound3d")
			|| (tokenString == "playsoundvp")
			|| (tokenString == "playsound3dvp")
			|| (tokenString == "playloopsound3dvp")
			|| (tokenString == "playloopsound3d")
			|| (tokenString == "getsoundplaying")
			|| (tokenString == "centeroncell")
			|| (tokenString == "equip")
			|| (tokenString == "getjournalindex")
			|| (tokenString == "getdistance")
			|| (tokenString == "getpos")
			|| (tokenString == "getresistdisease")
			|| (tokenString == "startscript")
			|| (tokenString == "stopscript")
			|| (tokenString == "getpccell")
			|| (tokenString == "aifollow")
			|| (tokenString == "aiwander")
			|| (tokenString == "getdeadcount")
			|| (tokenString == "pcexpelled")
			|| (tokenString == "unlock")
			|| (tokenString == "getangle")
			|| (tokenString == "getpcrank")
			|| (tokenString == "forcegreeting")
			|| (tokenString == "getdisposition")
			|| (tokenString == "setpccrimelevel")
			|| (tokenString == "gethealth") || (tokenString == "getfatigue") || (tokenString == "getmagicka")
			|| (tokenString == "getstrength") || (tokenString == "getintelligence") || (tokenString == "getwillpower") || (tokenString == "getagility") || (tokenString == "getspeed") || (tokenString == "getendurance") || (tokenString == "getpersonality") || (tokenString == "getluck")
			|| (tokenString == "getarmorer") || (tokenString == "getathletics") || (tokenString == "getshortblade") || (tokenString == "getlongblade") || (tokenString == "getblock")
			|| (tokenString == "getblunt") || (tokenString == "getaxe") || (tokenString == "getspear") || (tokenString == "gethandtohand")
			|| (tokenString == "getheavyarmor") || (tokenString == "getmediumarmor") || (tokenString == "getlightarmor")
			|| (tokenString == "getalchemy") || (tokenString == "getalteration") || (tokenString == "getconjuration") || (tokenString == "getenchantment")
			|| (tokenString == "getdestruction") || (tokenString == "getillusion") || (tokenString == "getmysticism") || (tokenString == "getrestoration")
			|| (tokenString == "getacrobatics") || (tokenString == "getmarksman") || (tokenString == "getmercantile") || (tokenString == "getsecurity") || (tokenString == "getsneak") || (tokenString == "getspeechcraft")
			|| (tokenString == "setscale")
			|| (tokenString == "activate")
			|| (tokenString == "getcurrentaipackage")
			|| (tokenString == "setdisposition")
			|| (tokenString == "getspelleffects")
			|| (tokenString == "fadein")
			|| (tokenString == "fadeout")
			|| (tokenString == "gethello")
			|| (tokenString == "sethello")
			|| (tokenString == "getdetected")
			|| (tokenString == "playgroup")
			|| (tokenString == "scriptrunning")
			)
		{
			parse_1arg(tokenItem);
		}
		else
		{
			// reset bCommandReference if it is on
			if (bUseCommandReference)
			{
				bUseCommandReference = false;
				mCommandRef_EDID = "";
			}
			// Default: unhandled keyword, skip to next tokenStatement
			std::stringstream errMesg;
			errMesg << "WARNING: Unhandled Keyword: <" << tokenItem->TypeToString() << "> " << tokenItem->str << std::endl;
			error_mesg(errMesg.str());
//			OutputDebugString(errMesg.str().c_str());
			if (mParseMode == 1)
				mFailureCode = 1;
			return;
		}
		// common instructions
		if (mParseMode == 0)
			gotoEOL(tokenItem);
		return;

	}

	void ScriptConverter::parse_addremoveitem(std::vector<struct Token>::iterator& tokenItem, bool bRemove)
	{
		std::string cmdString = tokenItem->str;
		std::string itemCountString="";
		bool bEvalItemCountVar=false;

		std::string refEDID = "";
		std::string refSIG = "";
		std::string refVal = "";

		tokenItem++;
		// object EDID
		if (tokenItem->type == TokenType::string_literalT ||
			tokenItem->type == TokenType::identifierT)
		{
//			itemEDID = ESM::ESMWriter::generateEDIDTES4(tokenItem->str, 0, "");
			lookup_reference(tokenItem->str, refEDID, refSIG, refVal);
		}
		else
		{
			// error parsing journal
			std::stringstream errorMesg;
			errorMesg << "parse_addremoveitem(): unexpected token type, expected string_literal:  <" << tokenItem->type << "> " << tokenItem->str << std::endl;
			abort(errorMesg.str());
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
			errorMesg << "parse_addremoveitem(): unexpected token type, expected number_literalT: [" << tokenItem->type << "] token=" << tokenItem->str << std::endl;
			abort(errorMesg.str());
//			std::cout << std::endl << "DEBUG OUTPUT: " << std::endl << mScriptText << std::endl << std::endl;
		}

		// translate statement
		std::stringstream convertedStatement;
		std::string command, cmdPrefix="";

		command = cmdString;
//		command = "AddItem";
//		if (bRemove)
//			command = "RemoveItem";

		if (bUseCommandReference)
		{
			cmdPrefix = mCommandRef_EDID + ".";
		}
		convertedStatement << cmdPrefix << command << " " << refEDID << " " << itemCountString;
		mCurrentContext.convertedStatements.push_back(convertedStatement.str());

		// bytecompiled statement:
		// OpCode, ParamBytes, ParamCount, Parameters
//		uint16_t OpCode = 0x1002; // Additem
//		if (bRemove)
//			OpCode = 0x1052;
		uint16_t OpCode = getOpCode(cmdString);

		uint16_t sizeParams = 10;
		uint16_t countParams = 2;
		uint32_t nItemCount;
		if (bEvalItemCountVar)
		{
			compile_command(OpCode, sizeParams, countParams, compile_param_varname(refEDID, refSIG, 4), compile_param_varname(itemCountString));
		}
		else
		{
			nItemCount = atoi(itemCountString.c_str());;
			compile_command(OpCode, sizeParams, countParams, compile_param_varname(refEDID, refSIG, 4), compile_param_long(nItemCount));
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
				errorMesg << "parse_choice(): unexpected token type, expected string_literal: " << tokenItem->type << std::endl;
				abort(errorMesg.str());
			}
			tokenItem++; // advance to next token

			if (tokenItem->type == TokenType::number_literalT)
				choiceNum = atoi(tokenItem->str.c_str());
			else
			{
				std::stringstream errorMesg;
				errorMesg << "parse_choice(): unexpected token type, expected number_literal: " << tokenItem->type << std::endl;
				abort(errorMesg.str());
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
		uint16_t OpCode = 0x1039; // setstage
		uint16_t sizeParams = 10;
		uint16_t countParams = 2;
		compile_command(OpCode, sizeParams, countParams, compile_param_varname(questEDID, "QUST", 4), questStageData);
		return;

	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams)
	{
		// outside of if expression, increment jumpops
		if (mParseMode == 0)
		{
			mCurrentContext.JumpOps++;
		}

		if (mParseMode == 1)
		{
			uint8_t OpCode_Push = 0x20;
//			mCurrentContext.compiledCode.push_back(OpCode_Push);
		}
		if (bUseCommandReference)
		{
			std::string refSIG = "";
			uint16_t RefData = prepare_reference(mCommandRef_EDID, refSIG, 100);
			if (RefData != 0)
			{
				uint16_t RefCode;
				if (mParseMode == 1)
				{
					RefCode = 0x72;
					mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&RefCode)[0]);
				}
				else
				{
					RefCode = 0x1C;
					for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&RefCode)[i]);
				}
				for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);
			}
			else
			{
				// ERROR
				std::stringstream errMsg;
				errMsg << "could not resolve reference for referenced command: " << mCommandRef_EDID << std::endl;
				abort(errMsg.str());
				return;
			}
			bUseCommandReference = false;
			mCommandRef_EDID = "";
		}

		if (mParseMode == 1)
		{
			uint8_t OpCode_ExpressionFunction = 0x58;
			mCurrentContext.compiledCode.push_back(OpCode_ExpressionFunction);
		}
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&OpCode)[i]);
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&sizeParams)[i]);
	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, std::vector<uint8_t> param1data)
	{
		compile_command(OpCode, sizeParams);
		if (sizeParams == 0)
			return;
		for (int i = 0; i<2; ++i) mCurrentContext.compiledCode.push_back(reinterpret_cast<uint8_t *> (&countParams)[i]);
		mCurrentContext.compiledCode.insert(mCurrentContext.compiledCode.end(), param1data.begin(), param1data.end());
	}

	void ScriptConverter::compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, std::vector<uint8_t> param1data, std::vector<uint8_t> param2data)
	{
		compile_command(OpCode, sizeParams, countParams, param1data);
		if (sizeParams == 0)
			return;
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
//			resultData.push_back(OpCode_Push);
		}
		resultData.push_back(RefCode);
		for (int i = 0; i<4; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);

		return resultData;
	}

	std::vector<uint8_t> ScriptConverter::compile_param_float(double float_val)
	{
		uint8_t RefCode = 0x7A;
		uint64_t RefData = reinterpret_cast<uint64_t *>(&float_val)[0];
		std::vector<uint8_t> resultData;

		if (mParseMode == 1)
		{
			uint8_t OpCode_Push = 0x20;
//			resultData.push_back(OpCode_Push);
		}
		resultData.push_back(RefCode);
		for (int i = 0; i<8; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);

		return resultData;
	}

	// modeflag=0 (default), modeflag=1 (expression term), modeflag=2 (set cmd),modeflag=4 (no lookup?)
	std::vector<uint8_t> ScriptConverter::compile_param_varname(const std::string & varName, const std::string& sSIG, int modeflag)
	{
		uint8_t RefCode = 0;
		uint16_t RefData = 0;
		std::vector<uint8_t> resultData;
		bool bIsLocalVar = false;
		int dialghelperIndex=0;

		if (bUseVarReference)
		{
			if (mParseMode == 1)
			{
				uint8_t OpCode_Push = 0x20;
//				resultData.push_back(OpCode_Push);
			}
			resultData = compile_external_localvar(mVarRef_mID, varName);
			bUseVarReference = false;
			mVarRef_mID = "";
			return resultData;
		}

		if (bIsFullScript)
		{
			RefData = prepare_localvar(varName);
			if (RefData != 0) // local var matched
			{
				bIsLocalVar = true;
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
					bUseVarReference = false;
					mVarRef_mID = "";
					abort("mLocalVarList contains invalid datatype\n");
					resultData.clear();
					return resultData;
				}
			}
		}
		else
		{
			std::string varname_lowercase = Misc::StringUtils::lowerCase(varName);
			// if dialog script, then set up DialogHelper quest for localvar references...
			if (mESM.mLocalVarIndexmap.find(varname_lowercase) != mESM.mLocalVarIndexmap.end())
			{
				std::string refSIG = "QUST";
				dialghelperIndex = mESM.mLocalVarIndexmap[varname_lowercase];
				RefData = prepare_reference("mwDialogHelper", refSIG, 100);
				// sanity check
				if (RefData != 0)
				{
					RefCode = 0x72; // param reference
				}
				else
				{
					abort("compile_param_varname() error trying to set up mwDialogHelper reference for localVar: " + varName + "\n");
					resultData.clear();
					bUseVarReference = false;
					mVarRef_mID = "";
					return resultData;
				}
			}
			else
			{
				// localvar not found, pass through to check for reference below
//				error_mesg("compile_param_varname(): localvar not found for: " + varName + "\n");
			}

		}

		// if no local var matched, try to match with reference (global record or other base record)
		if (RefData == 0)
		{
			std::string refSIG = sSIG;
			int mode=0;
			if (modeflag & 4)
				mode = 100;
			RefData = prepare_reference(varName, refSIG, mode);
			if (RefData != 0) // reference matched
			{
				if (Misc::StringUtils::lowerCase(refSIG) == "glob")
					RefCode = 0x47; // global formID expression term (NOT function parameter)
				else
					RefCode = 0x72; // param reference
			}
		}

		if (RefData != 0)
		{
			if (mParseMode == 1)
			{
				uint8_t OpCode_Push = 0x20;
//				resultData.push_back(OpCode_Push);
			}
			resultData.push_back(RefCode);
			for (int i = 0; i<2; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&RefData)[i]);
			if (dialghelperIndex != 0)
			{
				if (dialghelperIndex > 0)
				{
					uint32_t dlgHelperIndexData = dialghelperIndex;
					// add DialogHelper Index
					for (int i = 0; i<3; ++i) resultData.push_back(reinterpret_cast<uint8_t *> (&dlgHelperIndexData)[i]);
				}
				else
				{
					bUseVarReference = false;
					mVarRef_mID = "";
					abort("compile_param_varname() error\n");
					resultData.clear();
					return resultData;
				}
			}
		}
		else
		{
			abort("Could not resolve varName=" + varName + "\n");
			resultData.clear();
		}
		// always clear at end of each compile param
		bUseVarReference = false;
		mVarRef_mID = "";

		return resultData;
	}

	uint16_t ScriptConverter::prepare_localvar(const std::string& varName, int mode)
	{
		uint16_t RefData = 0;
		std::string searchName = Misc::StringUtils::lowerCase(varName);

		if (bIsFullScript)
		{
			for (int i = 0; i < mLocalVarList.size(); i++)
			{
				if (searchName == Misc::StringUtils::lowerCase(mLocalVarList[i].second))
				{
					RefData = i + 1;
				}
			}
		}
		else
		{
			// try to lookup localvars with DlgHelper Quest
			if (mESM.mLocalVarIndexmap.find(searchName) != mESM.mLocalVarIndexmap.end())
			{
//				mDialogHelperIndex = mESM.mLocalVarIndexmap[searchName];
				RefData = 1;
			}

		}

		return RefData;
	}

	uint16_t ScriptConverter::prepare_reference(const std::string & baseName, std::string& refSIG, int mode)
	{
		uint16_t RefData = 0;
		std::string searchName;
		uint32_t formID;
		std::string refEDID = "";
		std::string refVal = "";

		if (baseName == "")
		{
			error_mesg("WARNING: prepare_reference(): baseName is empty!\n");
			return 0;
		}

		searchName = Misc::StringUtils::lowerCase(baseName);
		if (searchName == "player" || searchName == "playerref")
		{
			formID = 0x14;
		}
		else
		{
			if (mode == 100)
			{
				refEDID = baseName;
			}
			else
			{
//				searchName = mESM.generateEDIDTES4(searchName, 0, sSIG);
				lookup_reference(baseName, refEDID, refSIG, refVal);
			}
			formID = mESM.crossRefStringID(refEDID, refSIG, false);
		}

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
		else
		{
			// do not abort, just issue error and return 0 so scriptconverter can continue
			if (mode == 100)
				error_mesg("WARNING: prepare_reference(): could not resolve reference BaseObj EDID (" + refSIG + ") " + refEDID + "\n");
			else
				error_mesg("WARNING: prepare_reference(): could not resolve reference BaseObj stringID=" + baseName + "\n");
		}

		return RefData;
	}

	std::vector<uint8_t> ScriptConverter::compile_external_localvar(const std::string &refName, const std::string &varName)
	{
		std::vector<uint8_t> varData;
		std::string searchName = Misc::StringUtils::lowerCase(varName);
		std::string refScript = "script";
		std::string refEDID = "";
		std::string refSIG = "";
		uint16_t refIndex = 0;
		uint16_t refLocalVarIndex = 0;

		if (refName == "mwDialogHelper")
		{
			refEDID = refName;
			refSIG = "QUST";
			if (mESM.mLocalVarIndexmap.find(searchName) != mESM.mLocalVarIndexmap.end())
			{
				refLocalVarIndex = mESM.mLocalVarIndexmap[searchName];
			}
			else
			{
				abort("compile_external_localvar(): localvar not found\n");
				varData.clear();
				return varData;
			}
		}
		else if (refName == "mwScriptHelper")
		{
			refEDID = refName;
			refSIG = "QUST";
			if (mESM.mScriptHelperVarmap.find(searchName) != mESM.mScriptHelperVarmap.end())
			{
				refLocalVarIndex = mESM.mScriptHelperVarmap[searchName];
			}
			else
			{
				abort("compile_external_localvar(): localvar not found\n");
				varData.clear();
				return varData;
			}
		}
		else
		{
			if (lookup_reference(refName, refEDID, refSIG, refScript) == false)
			{
				abort("compile_external_localvar(): unable to lookup reference: " + refName + " (" + refEDID + ")\n");
				varData.clear();
				return varData;
			}
		}

		if (refScript == "")
		{
			abort("compile_external_localvar(): external reference script not found\n");
			varData.clear();
			return varData;
		}
		std::string refScriptText = mDoc.getData().getScripts().getRecord(refScript).get().mScriptText;
		ScriptConverter refScriptObj(refScriptText, mESM, mDoc);
		refScriptObj.ExtractLocalVars();

		if (refIndex == 0)
		{
			refIndex = prepare_reference(refEDID, refSIG, 100);
			if (refIndex == 0)
			{
				abort("compile_external_localvar(): external reference formID not found\n");
				varData.clear();
				return varData;
			}
		}

		if (refLocalVarIndex == 0)
		{
			refLocalVarIndex = refScriptObj.LookupLocalVarIndex(varName);
			if (refLocalVarIndex == 0)
			{
				abort("compile_external_localvar(): referenced.localvar not found\n");
				varData.clear();
				return varData;
			}
		}

		if (mParseMode == 1)
		{
			uint8_t OpCode_Push = 0x20;
//			varData.push_back(OpCode_Push);
		}
		uint8_t cmdRefCode = 0x72;
		varData.push_back(cmdRefCode);
		for (int i = 0; i<2; ++i) varData.push_back(reinterpret_cast<uint8_t *> (&refIndex)[i]);

		uint8_t localvarRefCode = 0x73;
		if ( (refScriptObj.lookupLocalVarType(refLocalVarIndex) == "float")
			|| (refScriptObj.lookupLocalVarType(refLocalVarIndex) == "ref") )
			localvarRefCode = 0x66;
		varData.push_back(localvarRefCode);
		for (int i = 0; i<2; ++i) varData.push_back(reinterpret_cast<uint8_t *> (&refLocalVarIndex)[i]);

		return varData;
	}

	bool ScriptConverter::lookup_reference(const std::string &baseName, std::string &refEDID, std::string &refSIG, std::string &refValString)
	{
		bool result=true;

		std::string refString = baseName;
		std::string refScript = "";
		std::string refFact = "";
		std::pair<int, CSMWorld::UniversalId::Type> refRecord;
		ESM::RecNameInts esmSIG = (ESM::RecNameInts) 0;

		if (refSIG != "")
		{
			esmSIG = reinterpret_cast<ESM::RecNameInts *>(&refSIG)[0];
			switch (esmSIG)
			{
			case ESM::REC_NPC_:
			case ESM::REC_BOOK:
			case ESM::REC_ACTI:
			case ESM::REC_ALCH:
			case ESM::REC_APPA:
			case ESM::REC_ARMO:
			case ESM::REC_CLOT:
			case ESM::REC_CONT:
			case ESM::REC_CREA:
			case ESM::REC_DOOR:
			case ESM::REC_INGR:
			case ESM::REC_LIGH:
			case ESM::REC_MISC:
			case ESM::REC_WEAP:
			case ESM::REC_STAT:
			case ESM::REC_LEVC:
				refRecord = mDoc.getData().getReferenceables().getDataSet().searchId(refString);
				break;

			case ESM::REC_CELL:
				refRecord.first = mDoc.getData().getCells().searchId(refString);
				break;

			case FourCC<'Q', 'U', 'S', 'T'>::value:
				refRecord.first = mDoc.getData().getJournals().searchId(refString);
				break;

			case ESM::REC_FACT:
				refRecord.first = mDoc.getData().getFactions().searchId(refString);
				break;

			case ESM::REC_GLOB:
				refRecord.first = mDoc.getData().getGlobals().searchId(refString);
				break;

			case ESM::REC_SCPT:
				refRecord.first = mDoc.getData().getScripts().searchId(refString);
				break;

			case ESM::REC_CLAS:
				refRecord.first = mDoc.getData().getClasses().searchId(refString);
				break;

			case ESM::REC_PGRD:
				refRecord.first = mDoc.getData().getPathgrids().searchId(refString);
				break;

			case ESM::REC_RACE:
				refRecord.first = mDoc.getData().getRaces().searchId(refString);
				break;

			case ESM::REC_REGN:
				refRecord.first = mDoc.getData().getRegions().searchId(refString);
				break;

			case ESM::REC_SKIL:
				refRecord.first = mDoc.getData().getSkills().searchId(refString);
				break;

			case ESM::REC_SOUN:
				refRecord.first = mDoc.getData().getSounds().searchId(refString);
				break;

			case ESM::REC_DIAL:
				refRecord.first = mDoc.getData().getTopics().searchId(refString);
				break;

			case ESM::REC_INFO:
				refRecord.first = mDoc.getData().getTopicInfos().searchId(refString);
				break;

			case ESM::REC_SPEL:
				refRecord.first = mDoc.getData().getSpells().searchId(refString);
				break;

			default:
				std::stringstream errStream;
				errStream << "ERROR: lookup_reference(): Directed reference search name not found: [" << refSIG << "]" << "\"" << baseName << "\"" << "\n";
				abort(errStream.str());
				return false;
			}
			refRecord.second = CSMWorld::UniversalId::Type_None;
		}
		else
		{
			// Prioritized Blind Search
			refRecord = mDoc.getData().getReferenceables().getDataSet().searchId(refString);
			// convert refRecord.second to refSIG
			if (refRecord.first != -1)
			{
//				for (int i = 0; i<4; i++)
//					refSIG[i] = reinterpret_cast<char *>(&refRecord.second)[i];
			}
			else
			{
				if (refRecord.first = mDoc.getData().getCells().searchId(refString) != -1)
				{
					refSIG = "CELL";
				}
				else if (refRecord.first = mDoc.getData().getJournals().searchId(refString) != -1)
				{
					refSIG = "QUST";
				}
				else if (refRecord.first = mDoc.getData().getFactions().searchId(refString) != -1)
				{
					refSIG = "FACT";
				}
				else if (refRecord.first = mDoc.getData().getGlobals().searchId(refString) != -1)
				{
					refSIG = "GLOB";
				}
				else if (refRecord.first = mDoc.getData().getScripts().searchId(refString) != -1)
				{
					refSIG = "SCPT";
				}
				else if (refRecord.first = mDoc.getData().getSpells().searchId(refString) != -1)
				{
					refSIG = "SPEL";
				}
				else if (refRecord.first = mDoc.getData().getClasses().searchId(refString) != -1)
				{
					refSIG = "CLAS";
				}
				else if (refRecord.first = mDoc.getData().getPathgrids().searchId(refString) != -1)
				{
					refSIG = "PGRD";
				}
				else if (refRecord.first = mDoc.getData().getRaces().searchId(refString) != -1)
				{
					refSIG = "RACE";
				}
				else if (refRecord.first = mDoc.getData().getRegions().searchId(refString) != -1)
				{
					refSIG = "REGN";
				}
				else if (refRecord.first = mDoc.getData().getSkills().searchId(refString) != -1)
				{
					refSIG = "SKIL";
				}
				else if (refRecord.first = mDoc.getData().getSounds().searchId(refString) != -1)
				{
					refSIG = "SOUN";
				}
				else if (refRecord.first = mDoc.getData().getTopics().searchId(refString) != -1)
				{
					refSIG = "DIAL";
				}
				else if (refRecord.first = mDoc.getData().getTopicInfos().searchId(refString) != -1)
				{
					refSIG = "INFO";
				}
				else if (refRecord.first = mDoc.getData().getSpells().searchId(refString) != -1)
				{
					refSIG = "SPEL";
				}
				else
				{
					std::stringstream errStream;
					errStream << "WARNING: lookup_reference(): Blind reference search name not found: " << "\"" << baseName << "\"" << "\n";
//					error_mesg(errStream.str());
					return false;
				}

				refRecord.second = CSMWorld::UniversalId::Type_None;
				esmSIG = reinterpret_cast<ESM::RecNameInts *>(&refSIG)[0];

			}

		}

		
		switch (refRecord.second)
		{
		case CSMWorld::UniversalId::Type_Npc:
			refScript = mDoc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(refRecord.first).get().mScript;
			refFact = mDoc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(refRecord.first).get().mFaction;
			refSIG = "NPC_";
			break;

		case CSMWorld::UniversalId::Type_Book:
			refScript = mDoc.getData().getReferenceables().getDataSet().getBooks().mContainer.at(refRecord.first).get().mScript;
			refSIG = "BOOK";
			break;

		case CSMWorld::UniversalId::Type_Activator:
			refScript = mDoc.getData().getReferenceables().getDataSet().getActivators().mContainer.at(refRecord.first).get().mScript;
			refSIG = "ACTI";
			break;

		case CSMWorld::UniversalId::Type_Potion:
			refScript = mDoc.getData().getReferenceables().getDataSet().getPotions().mContainer.at(refRecord.first).get().mScript;
			refSIG = "ALCH";
			break;

		case CSMWorld::UniversalId::Type_Apparatus:
			refScript = mDoc.getData().getReferenceables().getDataSet().getApparati().mContainer.at(refRecord.first).get().mScript;
			refSIG = "APPA";
			break;

		case CSMWorld::UniversalId::Type_Armor:
			refScript = mDoc.getData().getReferenceables().getDataSet().getArmors().mContainer.at(refRecord.first).get().mScript;
			refSIG = "ARMO";
			break;

		case CSMWorld::UniversalId::Type_Clothing:
			refScript = mDoc.getData().getReferenceables().getDataSet().getClothing().mContainer.at(refRecord.first).get().mScript;
			refSIG = "CLOT";
			break;

		case CSMWorld::UniversalId::Type_Container:
			refScript = mDoc.getData().getReferenceables().getDataSet().getContainers().mContainer.at(refRecord.first).get().mScript;
			refSIG = "CONT";
			break;

		case CSMWorld::UniversalId::Type_Creature:
			refScript = mDoc.getData().getReferenceables().getDataSet().getCreatures().mContainer.at(refRecord.first).get().mScript;
			refSIG = "CREA";
			break;

		case CSMWorld::UniversalId::Type_Door:
			refScript = mDoc.getData().getReferenceables().getDataSet().getDoors().mContainer.at(refRecord.first).get().mScript;
			refSIG = "DOOR";
			break;

		case CSMWorld::UniversalId::Type_Ingredient:
			refScript = mDoc.getData().getReferenceables().getDataSet().getIngredients().mContainer.at(refRecord.first).get().mScript;
			refSIG = "INGR";
			break;

		case CSMWorld::UniversalId::Type_Light:
			refScript = mDoc.getData().getReferenceables().getDataSet().getLights().mContainer.at(refRecord.first).get().mScript;
			refSIG = "LIGH";
			break;

		case CSMWorld::UniversalId::Type_Miscellaneous:
			refScript = mDoc.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(refRecord.first).get().mScript;
			refSIG = "MISC";
			break;

		case CSMWorld::UniversalId::Type_Weapon:
			refScript = mDoc.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(refRecord.first).get().mScript;
			refSIG = "WEAP";
			break;

		case CSMWorld::UniversalId::Type_Static:
			refSIG = "STAT";
			break;

		case CSMWorld::UniversalId::Type_CreatureLevelledList:
			refSIG = "LVLC";
			break;

		case CSMWorld::UniversalId::Type_None:
			// check esmSIG
			switch (esmSIG)
			{
			case ESM::REC_SCPT:
				refScript = refString;
				break;

			default:
				break;
			}
			break;

		default:
			std::stringstream errStream;
			errStream << "WARNING: Reference type not handled: [" << refRecord.second << "]" << "\"" << baseName << "\"" << "\n";
			error_mesg(errStream.str());
			break;
		}

		// sanity check
		if (refSIG == "")
		{
			std::stringstream errStream;
			errStream << "SCRIPT: WARNING! lookup_reference, refSIG failed to initialize: CSMWorld::UniversalId=[" << refRecord.second << "]" << "\"" << baseName << "\"" << "\n";
			error_mesg(errStream.str());
		}

		bool bReturnBase = true;

		// returnbase behavior defaults to OFF for following record types
		std::string refSIG_lowercase = Misc::StringUtils::lowerCase(refSIG);
		if (refSIG_lowercase == "npc_"
			|| refSIG_lowercase == "crea"
			|| refSIG_lowercase == "acti"
			|| refSIG_lowercase == "door"
			|| refSIG_lowercase == "stat"
			|| refSIG_lowercase == "cont"
//			|| refSIG_lowercase == "alch"
//			|| refSIG_lowercase == "book"
//			|| refSIG_lowercase == "appa"
//			|| refSIG_lowercase == "armo"
//			|| refSIG_lowercase == "clot"
//			|| refSIG_lowercase == "ingr"
//			|| refSIG_lowercase == "ligh"
//			|| refSIG_lowercase == "misc"
//			|| refSIG_lowercase == "weap"
//			|| refSIG_lowercase == "levc"
			)
		{
			bReturnBase = false;
		}

		// "base" string will override default record type behavior
		if (Misc::StringUtils::lowerCase(refValString).find("script") != std::string::npos)
			refValString = refScript;
		if (Misc::StringUtils::lowerCase(refValString).find("faction") != std::string::npos)
			refValString = refFact;
		if (Misc::StringUtils::lowerCase(refValString).find("+base") != std::string::npos)
			bReturnBase = true;

		// Generate Reference EDID to be used in TES4 Script
		//  For scriptable world objects, this is usually a persistent reference.
		//  For scripts, this is a quest reference.
		//  For inventory objects... ?
		//  For cells?
		// refEDID = mESM.generateEDIDTES4(refString, 0, refSIG);
		if ( bReturnBase == false && refSIG_lowercase != "scpt")
		{
			refEDID = mESM.generateEDIDTES4(baseName, 0, "PREF");
			// register baseobj for persistent ref creation
			mESM.RegisterBaseObjForScriptedREF(baseName, refSIG);
		}
		else if (refSIG_lowercase == "scpt")
		{
			refEDID = mESM.generateEDIDTES4(baseName, 0, "SQUST");
			// register script for Script_to_Quest creation
			mESM.RegisterScriptToQuest(baseName);
		}
		else
		{
			refEDID = mESM.generateEDIDTES4(baseName, 0, refSIG);
		}

		return result;
	}

	void ScriptConverter::gotoEOL(std::vector<struct Token>::iterator & tokenItem)
	{
		// advance to endofline in stream
		while (tokenItem->type != TokenType::endlineT && tokenItem != mTokenList.end())
			tokenItem++;
		tokenItem--; // put back one to account for tokenItem++ in for() statement
		return;
	}

	void ScriptConverter::error_mesg(std::string errString)
	{
		std::stringstream errStream;
		if (mScriptName != "")
			errStream << "ScriptConverter (" << mScriptName << "): " << errString;
		else
			errStream << "ScriptConverter <result script>: " << errString;

		OutputDebugString(errStream.str().c_str());
	}

	void ScriptConverter::abort(std::string error_string)
	{
		if (mFailureCode == 0)
		{
			mFailureCode = 1;
			error_mesg("ABORT: " + error_string);
		}
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
		mKeywords.push_back("return");
		mKeywords.push_back("startscript");
		mKeywords.push_back("stopscript");

		mKeywords.push_back("journal");
		mKeywords.push_back("choice");
		mKeywords.push_back("goodbye");
		mKeywords.push_back("additem");
		mKeywords.push_back("removeitem");
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
		mKeywords.push_back("getaipackagedone");

		mKeywords.push_back("getlevel");
		mKeywords.push_back("getpos");
		mKeywords.push_back("getinterior");
		mKeywords.push_back("getpccell");
		mKeywords.push_back("getangle");
		mKeywords.push_back("getcommondisease");
		mKeywords.push_back("getblightdisease");

		mKeywords.push_back("getbuttonpressed");
		mKeywords.push_back("getresistdisease");
		mKeywords.push_back("getdeadcount");

		// getactorvalues
		mKeywords.push_back("gethealth");
		mKeywords.push_back("getmagicka");
		mKeywords.push_back("getfatigue");

		mKeywords.push_back("getstrength");
		mKeywords.push_back("getendurance");
		mKeywords.push_back("getpersonallity");
		mKeywords.push_back("getluck");
		mKeywords.push_back("getintelligence");
		mKeywords.push_back("getwillpower");
		mKeywords.push_back("getagility");
		mKeywords.push_back("getspeed");

		mKeywords.push_back("getspeechcraft");
		mKeywords.push_back("getmercantile");
		mKeywords.push_back("getsneak");
		mKeywords.push_back("getsecurity");
		mKeywords.push_back("getacrobatics");
		mKeywords.push_back("getmarksman");
		mKeywords.push_back("gethandtohand");
		mKeywords.push_back("getshortblade");
		mKeywords.push_back("getlightarmor");
		mKeywords.push_back("getlongblade");
		mKeywords.push_back("getaxe");
		mKeywords.push_back("getbluntweapon");
		mKeywords.push_back("getblock");
		mKeywords.push_back("getarmorer");
		mKeywords.push_back("getathletics");
		mKeywords.push_back("getheavyarmor");
		mKeywords.push_back("getmediumarmor");
		mKeywords.push_back("getspear");
		mKeywords.push_back("getrestoration");
		mKeywords.push_back("getdestruction");
		mKeywords.push_back("getconjuration");
		mKeywords.push_back("getmysticism");
		mKeywords.push_back("getalteration");
		mKeywords.push_back("getillusion");
		mKeywords.push_back("getenchant");
		mKeywords.push_back("getalchemy");
		mKeywords.push_back("getunarmored");

		// setactorvalues
		mKeywords.push_back("sethealth");
		mKeywords.push_back("setmagicka");
		mKeywords.push_back("setfatigue");

		mKeywords.push_back("setstrength");
		mKeywords.push_back("setendurance");
		mKeywords.push_back("setagility");
		mKeywords.push_back("setspeed");
		mKeywords.push_back("setpersonality");
		mKeywords.push_back("setintelligence");
		mKeywords.push_back("setwillpower");
		mKeywords.push_back("setluck");

		mKeywords.push_back("setspeechcraft");
		mKeywords.push_back("setmercantile");
		mKeywords.push_back("setsneak");
		mKeywords.push_back("setsecurity");
		mKeywords.push_back("setacrobatics");
		mKeywords.push_back("setmarksman");
		mKeywords.push_back("sethandtohand");
		mKeywords.push_back("setshortblade");
		mKeywords.push_back("setlightarmor");
		mKeywords.push_back("setlongblade");
		mKeywords.push_back("setaxe");
		mKeywords.push_back("setbluntweapon");
		mKeywords.push_back("setblock");
		mKeywords.push_back("setarmorer");
		mKeywords.push_back("setathletics");
		mKeywords.push_back("setheavyarmor");
		mKeywords.push_back("setmediumarmor");
		mKeywords.push_back("setspear");
		mKeywords.push_back("setrestoration");
		mKeywords.push_back("setdestruction");
		mKeywords.push_back("setconjuration");
		mKeywords.push_back("setmysticism");
		mKeywords.push_back("setalteration");
		mKeywords.push_back("setillusion");
		mKeywords.push_back("setenchant");
		mKeywords.push_back("setalchemy");
		mKeywords.push_back("setunarmored");

		// modactorvalues
		mKeywords.push_back("modhealth");
		mKeywords.push_back("modcurrenthealth");
		mKeywords.push_back("modmagicka");
		mKeywords.push_back("modcurrentmagicka");
		mKeywords.push_back("modfatigue");
		mKeywords.push_back("modcurrentfatigue");

		mKeywords.push_back("modstrength");
		mKeywords.push_back("modendurance");
		mKeywords.push_back("modagility");
		mKeywords.push_back("modspeed");
		mKeywords.push_back("modpersonality");
		mKeywords.push_back("modintelligence");
		mKeywords.push_back("modwillpower");
		mKeywords.push_back("modluck");

		mKeywords.push_back("modspeechcraft");
		mKeywords.push_back("modmercantile");
		mKeywords.push_back("modsneak");
		mKeywords.push_back("modsecurity");
		mKeywords.push_back("modacrobatics");
		mKeywords.push_back("modmarksman");
		mKeywords.push_back("modhandtohand");
		mKeywords.push_back("modshortblade");
		mKeywords.push_back("modlightarmor");
		mKeywords.push_back("modlongblade");
		mKeywords.push_back("modaxe");
		mKeywords.push_back("modbluntweapon");
		mKeywords.push_back("modblock");
		mKeywords.push_back("modarmorer");
		mKeywords.push_back("modathletics");
		mKeywords.push_back("modheavyarmor");
		mKeywords.push_back("modmediumarmor");
		mKeywords.push_back("modspear");
		mKeywords.push_back("modrestoration");
		mKeywords.push_back("moddestruction");
		mKeywords.push_back("modconjuration");
		mKeywords.push_back("modmysticism");
		mKeywords.push_back("modalteration");
		mKeywords.push_back("modillusion");
		mKeywords.push_back("modenchant");
		mKeywords.push_back("modalchemy");
		mKeywords.push_back("modunarmored");

		mKeywords.push_back("getdisposition");
		mKeywords.push_back("setdisposition");
		mKeywords.push_back("moddisposition");
		mKeywords.push_back("setfight");
		mKeywords.push_back("setflee");

		mKeywords.push_back("setpos");
		mKeywords.push_back("setscale");
		mKeywords.push_back("setangle");
		mKeywords.push_back("rotate");
		mKeywords.push_back("rotateworld");

		mKeywords.push_back("sethello");
		mKeywords.push_back("setdelete");
		mKeywords.push_back("setatstart");

		mKeywords.push_back("addtopic");
		mKeywords.push_back("stopscript");
		mKeywords.push_back("playsound");
		mKeywords.push_back("centeroncell");
		mKeywords.push_back("positioncell");
		mKeywords.push_back("activate");
		mKeywords.push_back("cast");
		mKeywords.push_back("resurrect");
		mKeywords.push_back("stopcombat");
		mKeywords.push_back("startscript");
		mKeywords.push_back("equip");
		mKeywords.push_back("showmap");
		mKeywords.push_back("startcombat");
		mKeywords.push_back("modreputation");
		mKeywords.push_back("clearinfoactor");
		mKeywords.push_back("aifollow");
		mKeywords.push_back("aiwander");
		mKeywords.push_back("pclowerrank");
		mKeywords.push_back("lock");
		mKeywords.push_back("unlock");
		mKeywords.push_back("drop");
		mKeywords.push_back("modfight");
		mKeywords.push_back("modpccrimelevel");
		mKeywords.push_back("say");
		mKeywords.push_back("clearforcesneak");
		mKeywords.push_back("pcexpell");
		mKeywords.push_back("modaxe");
		mKeywords.push_back("playsound3d");
		mKeywords.push_back("placeatpc");
		mKeywords.push_back("modrestoration");
		mKeywords.push_back("removespell");
		mKeywords.push_back("playgroup");
		mKeywords.push_back("placeatme");
		mKeywords.push_back("playsoundvp");
		mKeywords.push_back("loopgroup");
		mKeywords.push_back("removeeffects");
		mKeywords.push_back("playsound3dvp");
		mKeywords.push_back("dontsaveobject");
		mKeywords.push_back("stopsound");
		mKeywords.push_back("playloopsound3dvp");
		mKeywords.push_back("playloopsound3d");
		mKeywords.push_back("forcegreeting");
		mKeywords.push_back("getsoundplaying");

		mKeywords.push_back("random");
		mKeywords.push_back("random100");
//		mKeywords.push_back("gamehour");
		mKeywords.push_back("cellchanged");
		mKeywords.push_back("getspell");
		mKeywords.push_back("getitemcount");
		mKeywords.push_back("getpccell");
		mKeywords.push_back("getcurrentaipackage");
		mKeywords.push_back("getjournalindex");
		mKeywords.push_back("getsecondspassed");
		mKeywords.push_back("getlocked");
		mKeywords.push_back("getdisabled");
		mKeywords.push_back("getpcrank");
		mKeywords.push_back("getbuttonpressed");
		mKeywords.push_back("getscale");
		mKeywords.push_back("getpcjumping");
		mKeywords.push_back("getcurrentweather");
		mKeywords.push_back("getdistance");

		mKeywords.push_back("getweapondrawn");
		mKeywords.push_back("getspellreadied");
		mKeywords.push_back("getpcrunning");
		mKeywords.push_back("pcexpelled");
		mKeywords.push_back("pcclearexpelled");
		mKeywords.push_back("getpccrimelevel"); // getcrimegold
//		mKeywords.push_back("getclothingvalue");
//		mKeywords.push_back("getbartergold");
		mKeywords.push_back("getattacked");
//		mKeywords.push_back("getarmorrating");
//		mKeywords.push_back("getalarmed");
		mKeywords.push_back("enableplayercontrols");
		mKeywords.push_back("disableplayercontrols");
		mKeywords.push_back("setatstart");
		mKeywords.push_back("setpccrimelevel"); // setcrimegold
		mKeywords.push_back("getwaterlevel");
		mKeywords.push_back("getspelleffects");
		mKeywords.push_back("fadein");
		mKeywords.push_back("fadeout");
		mKeywords.push_back("getdetected");
		mKeywords.push_back("scriptrunning");

		mKeywords.push_back("setalarm");

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
		bSetCmd = false;

		for (auto tokenItem = mTokenList.begin(); tokenItem != mTokenList.end(); tokenItem++)
		{
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
				if (tokenItem != mTokenList.end())
					parse_expression(tokenItem);
				continue;
			}

			// unhandled mode, reset mode and skip to new line
			mParseMode = 0;
			bSetCmd = false;
			gotoEOL(tokenItem);
			continue;

		}

	}

	void ScriptConverter::ExtractChoices()
	{
		mParseMode = 0;
		bSetCmd = false;

		for (auto tokenItem = mTokenList.begin(); tokenItem != mTokenList.end(); tokenItem++)
		{
			if (Misc::StringUtils::lowerCase(tokenItem->str) == "begin")
			{
				parse_begin(tokenItem);
			}
			else if (Misc::StringUtils::lowerCase(tokenItem->str) == "choice")
			{
				parse_choice(tokenItem);
			}
			if (tokenItem == mTokenList.end())
				return;
			gotoEOL(tokenItem); // goto EOL
			tokenItem++; // skip EOL
		}
	}

	void ScriptConverter::ExtractLocalVars()
	{
		mParseMode = 0;
		bSetCmd = false;

		for (auto tokenItem = mTokenList.begin(); tokenItem != mTokenList.end(); tokenItem++)
		{
			if (Misc::StringUtils::lowerCase(tokenItem->str) == "begin")
			{
				parse_begin(tokenItem);
			}
			else if ((Misc::StringUtils::lowerCase(tokenItem->str) == "short")
				|| (Misc::StringUtils::lowerCase(tokenItem->str) == "long")
				|| (Misc::StringUtils::lowerCase(tokenItem->str) == "float")
				|| (Misc::StringUtils::lowerCase(tokenItem->str) == "ref")
				)
			{
				parse_localvar(tokenItem);
			}
			gotoEOL(tokenItem); // goto EOL
			tokenItem++; // skip EOL
		}
	}

	bool ScriptConverter::Token::IsHigherPrecedence(const std::vector<struct Token>::iterator & tokenItem)
	{
		int precedenceA=0, precedenceB=0;

		if (str[0] == '*') precedenceA = 3;
		else if (str[0] == '/') precedenceA = 3;
		else if (str[0] == '+') precedenceA = 2;
		else if (str[0] == '-') precedenceA = 2;
		else if (str[0] == '=') precedenceA = 1;
		else if (str[0] == '>') precedenceA = 1;
		else if (str[0] == '<') precedenceA = 1;

		if (tokenItem->str[0] == '*') precedenceB = 3;
		else if (tokenItem->str[0] == '/') precedenceB = 3;
		else if (tokenItem->str[0] == '+') precedenceB = 2;
		else if (tokenItem->str[0] == '-') precedenceB = 2;
		else if (tokenItem->str[0] == '=') precedenceB = 1;
		else if (tokenItem->str[0] == '>') precedenceB = 1;
		else if (tokenItem->str[0] == '<') precedenceB = 1;

		if (precedenceA > precedenceB)
			return true;

		return false;
	}

	std::string ScriptConverter::Token::TypeToString()
	{
		std::string tokenTypeString = "";
		switch (type)
		{
		case TokenType::endlineT:
			tokenTypeString = "EOL";
			break;
		case TokenType::identifierT:
			tokenTypeString = "IDENTIFIER";
			break;
		case TokenType::keywordT:
			tokenTypeString = "KEYWORD";
			break;
		case TokenType::number_literalT:
			tokenTypeString = "NUMBER LITERAL";
			break;
		case TokenType::operatorT:
			tokenTypeString = "OPERATOR";
			break;
		case TokenType::string_literalT:
			tokenTypeString = "STRING LITERAL";
			break;
		}
		return tokenTypeString;

	}

}

