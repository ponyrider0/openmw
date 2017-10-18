#ifndef OPENMW_SCRIPT_CONVERTER_H
#define OPENMW_SCRIPT_CONVERTER_H

#include <string>
#include <vector>
#include <components/esm/esmwriter.hpp>

namespace ESM {

	class ScriptConverter
	{
		enum TokenType
		{
			identifierT = 0,
			keywordT,
			operatorT,
			string_literalT,
			number_literalT,
			endlineT
		};

		struct Token
		{
			int type;
			std::string str;

			Token(int arg1, std::string arg2) { type = arg1; str = arg2; }
		};

		int mLinePosition = 0;
		int mReadMode = 0; // 0=ready, 1=identifier, 2=quoted_literal
		int mParseMode = 0; // 0=ready, 1=choice
		bool bGoodbye = false;
		bool bParserUsePlayerRef = false;

		void lexer();
		void parser();
		void read_line(const std::string& lineBuffer);
		void read_quotedliteral(const std::string& lineBuffer);
		void read_identifier(const std::string& lineBuffer);
		void parse_keyword(std::vector<struct Token>::iterator& tokenItem);
		void parse_choice(std::vector<struct Token>::iterator& tokenItem);
		void parse_journal(std::vector<struct Token>::iterator& tokenItem);
		void parse_operation(std::vector<struct Token>::iterator& tokenItem);
		void compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, uint32_t arg1, uint32_t arg2);
		void compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, const std::string& edid, uint32_t arg2);
		void compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, const std::string& edid, const std::string& expression);

	public:
		std::string mScriptText;
		ESM::ESMWriter& mESM;

		std::vector<struct Token> mTokenList;
		std::vector< std::pair <int, std::string > > mChoicesList;
		std::vector< std::string > mConvertedStatementList;
		std::vector< char > mCompiledByteBuffer;
		std::vector< uint32_t > mReferenceList;
		std::vector< std::string > mLocalVarList;

		ScriptConverter(const std::string& scriptText, ESM::ESMWriter& esm);
		std::string GetConvertedScript();
		bool PerformGoodbye();
		const char* GetCompiledByteBuffer();

	};
}

#endif