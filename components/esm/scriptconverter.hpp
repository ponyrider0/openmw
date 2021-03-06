#ifndef OPENMW_SCRIPT_CONVERTER_H
#define OPENMW_SCRIPT_CONVERTER_H

#include <string>
#include <vector>
#include <sstream>
#include <components/esm/esmwriter.hpp>

namespace CSMDoc
{
	class Document;
}

namespace ESM {

	class ScriptConverter
	{
		char* actorvalueStrings[39] =
		{
			"strength",
			"intelligence",
			"willpower",
			"agility",
			"speed",
			"endurance",
			"personality",
			"luck",
			"health",
			"magicka",
			"fatigue",
			"encumbrance",
			"armorer",
			"athletics",
			"blade",
			"block",
			"blunt",
			"handtohand",
			"heavyarmor",
			"alchemy",
			"alteration",
			"conjuration",
			"destruction",
			"illusion",
			"mysticism",
			"restoration",
			"acrobatics",
			"lightarmor",
			"marksman",
			"mercantile",
			"security",
			"sneak",
			"speechcraft", // 33
			"mediumarmor",
			"enchantment",
			"axe",
			"spear", // 37
			"shortblade", // 38
			"longblade"
		};

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
			bool IsHigherPrecedence(const std::vector<struct Token>::iterator& tokenItem);
			std::string TypeToString();
		};

		struct ScriptContext
		{
			std::string blockName="";
			int codeBlockDepth=0;
			bool bIsCallBack=false;
			int JumpOps = 0;
			std::vector< std::string > convertedStatements;
			std::vector< uint8_t > compiledCode;
		};

		std::string mScriptText;
		ESM::ESMWriter& mESM;
		CSMDoc::Document& mDoc;
		std::vector<std::string> mKeywords;
		std::vector<struct Token> mTokenList;
		std::vector< std::string > mConvertedStatementList;
		std::vector< char > mCompiledByteBuffer;
		std::vector< char > mCompiledByteHeader;
		std::vector< std::string > mConvertedHeader;

		std::stringstream Debug_CurrentScriptLine;
		bool mNewStatement=true;
		int mFailureCode=0;
		int mLinePosition = 0;
		int mReadMode = 0; // 0=ready, 1=identifier, 2=quoted_literal
		int mParseMode = 0; // 0=ready, 1=choice
		bool bGoodbye = false;
		bool bIsFullScript = false;
		std::string mScriptName;
		struct ScriptContext mCurrentContext;
		std::vector < struct ScriptContext > mContextStack;
		std::string mCallBackName = "";

		bool bUseCommandReference = false;
		bool bUseVarReference = false;
		std::string mCommandRef_EDID = "";
		std::string mVarRef_mID = "";
//		int mDialogHelperIndex=0;
		bool bSetCmd = false;
		bool bIfCmd = false;
		int mSetCmd_StartPosition=0;
		int mSetCmd_VarSize=0;
		std::vector< struct Token > mOperatorExpressionStack;
		bool bNegationOperator = false;

		void setup_dictionary();
		void lexer();
		void parser();
		void read_line(const std::string& lineBuffer);
		void read_quotedliteral(const std::string& lineBuffer);
		void read_identifier(const std::string& lineBuffer);
		void read_operator(const std::string& lineBuffer);

		void parse_statement(std::vector<struct Token>::iterator& tokenItem);
		void parse_expression(std::vector<struct Token>::iterator& tokenItem);
		void parse_operator(std::vector<struct Token>::iterator& tokenItem);
		bool parse_expression_generate_eval(std::vector<struct Token>::iterator& tokenItem);

		uint16_t getOpCode(std::string OpCodeString);
		void parse_placeatme(std::vector<struct Token>::iterator& tokenItem);
		void parse_positionCW(std::vector<struct Token>::iterator& tokenItem);
		void parse_modfactionrep(std::vector<struct Token>::iterator& tokenItem);
		void parse_set(std::vector<struct Token>::iterator& tokenItem);
		void parse_0arg(std::vector<struct Token>::iterator& tokenItem);
		bool sub_parse_arg(std::vector<struct Token>::iterator& tokenItem, std::string& argString, bool& bEvalArgString, bool& bNeedsDialogHelper, std::string argSIG="", bool bReturnBase=false);
		void parse_1arg(std::vector<struct Token>::iterator& tokenItem);
		void parse_2arg(std::vector<struct Token>::iterator& tokenItem);
		void parse_messagebox(std::vector<struct Token>::iterator& tokenItem);
		void parse_localvar(std::vector<struct Token>::iterator& tokenItem);
		void parse_if(std::vector<struct Token>::iterator& tokenItem);
		void parse_else(std::vector<struct Token>::iterator& tokenItem);
		void parse_endif(std::vector<struct Token>::iterator& tokenItem);
		void parse_end(std::vector<struct Token>::iterator& tokenItem);
		void parse_begin(std::vector<struct Token>::iterator& tokenItem);
		void parse_keyword(std::vector<struct Token>::iterator& tokenItem);
		void parse_addremoveitem(std::vector<struct Token>::iterator& tokenItem, bool bRemove);
		void parse_choice(std::vector<struct Token>::iterator& tokenItem);
		void parse_journal(std::vector<struct Token>::iterator& tokenItem);
		void compile_command(uint16_t OpCode, uint16_t sizeParams);
		void compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, std::vector<uint8_t> param1data);
		void compile_command(uint16_t OpCode, uint16_t sizeParams, uint16_t countParams, std::vector<uint8_t> param1data, std::vector<uint8_t> param2data);
		std::vector<uint8_t> compile_param_long(int int_val);
		std::vector<uint8_t> compile_param_float(double float_val);
		std::vector<uint8_t> compile_param_varname(const std::string& varName, const std::string &sSIG="", int modeflag=0);
		uint16_t prepare_localvar(const std::string& varName, int mode=0);
		uint16_t prepare_reference(const std::string& refName, std::string& refSIG, int mode=0 );
		std::vector<uint8_t> compile_external_localvar(const std::string& refName, const std::string& varName);
		bool lookup_reference(const std::string &baseName, std::string &refEDID, std::string &refSIG, std::string &refValString);
		void gotoEOL(std::vector<struct Token>::iterator & tokenItem);
		void pop_context_conditional(std::vector<struct Token>::iterator& tokenItem);
		int check_get_set_mod_AV_command(const std::string &commandString);
		bool sub_parse_get_set_mod_AV_command(std::vector<struct Token>::iterator &tokenItem, int actorValue, std::string &cmdString, std::string &arg1String, std::vector<uint8_t> &arg1data);

		void error_mesg(std::string);
		void abort(std::string);
		bool HasFailed();

	public:
		std::vector< std::pair <int, std::string > > mChoicesList;
		std::vector< uint32_t > mReferenceList;
		std::vector< std::pair<std::string, std::string> > mLocalVarList;

		ScriptConverter(const std::string& scriptText, ESM::ESMWriter& esm, CSMDoc::Document &doc);
		void processScript();
		std::string GetConvertedScript();
		bool PerformGoodbye();
		const char* GetCompiledByteBuffer();
		uint32_t GetByteBufferSize();
		void ExtractLocalVars();
		uint16_t LookupLocalVarIndex(const std::string& varName);
		std::string lookupLocalVarType(const std::string& varName);
		std::string lookupLocalVarType(int varIndex);
		void ExtractChoices();

	};
}

#endif