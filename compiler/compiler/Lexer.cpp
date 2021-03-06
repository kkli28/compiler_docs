#include "stdafx.h"
#include "Lexer.h"

//构造函数
kkli::Lexer::Lexer(std::string sourceFile, SymbolTable* tb, VirtualMachine* v) {
	table = tb;
	vm = v;

	index = 0;
	isDefinition = true;

	DEBUG_LEXER("Lexer::lexer(" + sourceFile + ")", "");
	std::string format = "";

	//判断文件是否可读取
	std::ifstream inFile(sourceFile);
	if (!inFile.good()) {
		throw Error("can't open file: " + sourceFile);
	}
	inFile >> std::noskipws;    //不跳过空白

	//处理内建符号
	DEBUG_LEXER("[add builtin] begin", FORMAT(format));
	source = "char else enum if int return sizeof while printf malloc exit scanf getchar putchar void main";
	source.push_back(END);
	
	//关键字
	int type = CHAR;
	while (type <= WHILE) {
		next(FORMAT(format));
		auto& tk = table->getCurrentToken(FORMAT(format));
		tk.type = type;
		tk.setScope({ KEY_WORD_SCOPE });
		++type;
	}

	//内建函数
	type = I_PRTF;
	while (type <= I_PUTC) {
		next(FORMAT(format));
		Token& tk = table->getCurrentToken(FORMAT(format));
		tk.klass = SYS_FUNC;
		tk.dataType = INT_TYPE;
		tk.value = type++;
		tk.setScope({ KEY_WORD_SCOPE });
	}

	//void类型识别为char类型
	next(FORMAT(format));
	Token& tk = table->getCurrentToken(FORMAT(format));
	tk.type = CHAR;
	tk.setScope({ KEY_WORD_SCOPE });

	//main函数
	next(FORMAT(format));
	table->setMainToken(FORMAT(format));
	table->getCurrentToken(FORMAT(format)).setScope({ KEY_WORD_SCOPE });  //main函数也是在关键字作用域中的

	DEBUG_LEXER("[add builtin] end", FORMAT(format));
	DEBUG_LEXER_SYMBOL_TABLE(table->getSymbolTableInfo(), FORMAT(format));
	//内建符号处理完毕

	char buff[1000000];
	//不对buff进行初始化，release模式下会出错
	//for (int i = 0; i < 1000000; ++i) {
//		buff[i] = END;
//	}

	int i = 0;
	while (!inFile.eof()) inFile >> buff[i++];

	source = std::move(std::string(buff, i));

	//添加末尾字符
	//source.push_back('\0');
	source.push_back(END);

	//备份一个
	sourceBackUp = source;

	DEBUG_LEXER_SOURCE("\n[source--begin]:\n" + source + "\n[source--end]\n", FORMAT(format));

	prevPrevIndex = 0;
	prevIndex = 0;
	index = 0;
	line = 1;
	gdBegIndex = 0;
	gdEndIndex = 0;
	isDefinition = false;
}

//更新索引
void kkli::Lexer::updateIndex(std::string format) {
	DEBUG_LEXER("Lexer::updateIndex()", format);
	prevPrevIndex = prevIndex;
	prevIndex = index;
}

//回退，为了局部变量定义
void kkli::Lexer::rollBack(std::string format) {
	DEBUG_LEXER("Lexer::rollBack()", format);
	//index = prevPrevIndex - 1;
	index = prevPrevIndex;
	prevPrevIndex = index;
	prevIndex = index;
	source[index] = ';';
}

//next
std::pair<int, int> kkli::Lexer::next(std::string format) {
	DEBUG_LEXER("Lexer::next()", format);
	
	int value = 0;
	char curr = source[index];

	updateIndex(FORMAT(format));
	while (curr != END) {

		//换行符
		if (curr == '\n') {
			curr = get();
			DEBUG_LEXER_NEXT("[\\n]", FORMAT(format));
			++line;
		}

		//宏 或 文件引用（直接跳过，不支持）
		else if (curr == '#') {
			DEBUG_LEXER_NEXT("[#...]", FORMAT(format));
			curr = get();
			while (curr != END && curr != '\n')  curr = get();
		}

		//标识符
		else if (isAlpha(curr) || curr == '_') {
			DEBUG_LEXER_NEXT("[ID]", FORMAT(format));
			int begIndex = index;      //该标识符名称的起始索引
			int hash = curr;           //该标识符的hash值
			curr = get();
			while (isAlpha(curr) || isNum(curr) || curr == '_') {
				hash = hash * 147 + curr;
				curr = get();
			}

			std::string name(source, begIndex, index - begIndex);

			//符号表中有该标识符
			if (table->has(isDefinition, hash, name, FORMAT(format))) {
				//定义模式下，在当前作用域下找到该符号，则表示符号重定义
				if (table->getCurrentToken(FORMAT(format)).scope[0] != KEY_WORD_SCOPE && isDefinition) {
					throw Error(line, "duplicate define symbol '" + name + "'");
				}
				else {
					return { table->getCurrentToken(FORMAT(format)).type, 0 };
				}
			}

			//符号表中没有该标识符，则向其中添加信息
			else {
				if (isDefinition) {
					Token& tk = table->getCurrentToken(FORMAT(format));
					tk.type = ID;
					tk.name = name;
					tk.hash = hash;

					DEBUG_LEXER_NEXT("add [type] ID, [name] " + name + ", [hash] " + std::to_string(hash), FORMAT(format));
					return { ID, 0 };
				}
				else {
					throw Error(line, "Undefined symbol '" + name + "'.");
				}
			}
		}

		//数字
		else if (isNum(curr)) {
			value = curr - '0';
			curr = get();

			//十进制数（八进制数与十六进制数以0及0x开始）
			if (value != 0) {
				while (isNum(curr)) {
					value = value * 10 + curr - '0';
					curr = get();
				}
				DEBUG_LEXER_NEXT("[dec] " + std::to_string(value), FORMAT(format));
			}

			else {
				//十六进制数
				if (curr == 'x' || curr == 'X') {
					curr = get();
					bool hasNum = false;
					while (isNum(curr)
						|| (curr >= 'a' && curr <= 'z')
						|| (curr >= 'A' && curr <= 'Z')) {
						hasNum = true;
						value = value * 16 + (curr & 15) + (curr >= 'A' ? 9 : 0);
						curr = get();
					}

					if (!hasNum) {
						throw Error("Line "+std::to_string(line)+". Invalid hex type, need 0~9 or a~z/A~Z after 0x.");
					}
					DEBUG_LEXER_NEXT("[hex] " + std::to_string(value), FORMAT(format));
				}

				//八进制数
				else {
					bool hasNum = false;
					while (curr >= '0' && curr <= '7') {
						hasNum = true;
						value = value * 8 + curr - '0';
						curr = get();
					}

					DEBUG_LEXER_NEXT("[oct] " + std::to_string(value), FORMAT(format));
				}
			}

			return { NUM_INT, value };
		}

		//注释 或 除号
		else if (curr == '/') {
			curr = get();

			//单行注释
			if (curr == '/') {
				while (curr != '\n' && curr != END) curr = get();
				DEBUG_LEXER_NEXT("[//]", FORMAT(format));
			}

			//多行注释
			else if (curr == '*') {
				curr = get();
				while (true) {
					if (curr == '*') {
						curr = get();
						if (curr == '/') {
							curr = get();
							break;
						}
					}
					else if (curr == END) break;
					else curr = get();
				}
				DEBUG_LEXER_NEXT("[/**/]", FORMAT(format));
			}

			//除号
			else {
				DEBUG_LEXER_NEXT("[/]", FORMAT(format));
				return { DIV, 0 };
			}
		}

		//字符
		else if (curr == '\'') {
			curr = get();
			if (curr == END) {
				throw Error("Line "+std::to_string(line)+". Invalid char type, need \'.");
			}

			//转义字符
			if (curr == '\\') {
				curr = get();
				if (curr == END) {
					throw Error("Line " + std::to_string(line) + ". Invalid char type, need escape charactor after \\.");
				}

				if (curr == 'n') value = '\n';
				else if (curr == '0') value = '\0';
				else value = curr;
			}
			else {
				value = curr;
			}
			curr = get();
			if (curr != '\'') {
				throw Error("Line "+std::to_string(line)+". Invalid char type, need \' to finish charactor");
			}
			curr = get();

			DEBUG_LEXER_NEXT("[char] " + std::to_string(value), FORMAT(format));
			return { NUM_CHAR, value };
		}

		//字符串
		else if (curr == '"') {
			curr = get();

			//记录字符串起始位置
			value = reinterpret_cast<int>(vm->getNextDataPos(CHAR_TYPE, FORMAT(format)));

			while (curr != '"' && curr != END) {

				//转义字符
				if (curr == '\\') {
					curr = get();
					if (curr == 'n') vm->addDataChar('\n', FORMAT(format));
					else if (curr == 't') vm->addDataChar('\t', FORMAT(format));
					else if (curr == END) {
						throw Error("Line " + std::to_string(line) + ". Invalid string type, need escape charactor after \\.");
					}
					else vm->addDataChar(curr, FORMAT(format));
					curr = get();
				}
				else {
					vm->addDataChar(curr, FORMAT(format));
					curr = get();
				}
			}

			//末尾添加字符 '\0'
			vm->addDataChar(0, FORMAT(format));

			if (curr == END) {
				throw Error("Line " +std::to_string(line)+". Invalid string type, need \" to finish string.");
			}
			curr = get();

			DEBUG_LEXER_NEXT("[string]", FORMAT(format));

			return { STRING, value };
		}

		// = 或 ==
		else if (curr == '=') {
			curr = get();

			// ==
			if (curr == '=') {
				curr = get();

				DEBUG_LEXER_NEXT("[EQ]", FORMAT(format));
				return { EQ, 0 };
			}

			// =
			else {
				DEBUG_LEXER_NEXT("[ASSIGN]", FORMAT(format));
				return { ASSIGN, 0 };
			}
		}

		// + 或 ++
		else if (curr == '+') {
			curr = get();

			// ++
			if (curr == '+') {
				curr = get();
				
				DEBUG_LEXER_NEXT("[INC]", FORMAT(format));
				return { INC, 0 };
			}

			// +
			else {
				DEBUG_LEXER_NEXT("[ADD]", FORMAT(format));
				return { ADD, 0 };
			}
		}

		// - 或 --
		else if (curr == '-') {
			curr = get();

			// --
			if (curr == '-') {
				curr = get();
				DEBUG_LEXER_NEXT("[DEC]", FORMAT(format));
				return { DEC, 0 };
			}

			// -
			else {
				DEBUG_LEXER_NEXT("[SUB]", FORMAT(format));
				return { SUB, 0 };
			}
		}

		// ! 或 !=
		else if (curr == '!') {
			curr = get();

			// !=
			if (curr == '=') {
				curr = get();
				DEBUG_LEXER_NEXT("[NE]", FORMAT(format));
				return { NE, 0 };
			}

			// !
			else {
				DEBUG_LEXER_NEXT("[NOT]", FORMAT(format));
				return { NOT, 0 };
			}
		}

		// < 或 <= 或 <<
		else if (curr == '<') {
			curr = get();

			// <=
			if (curr == '=') {
				curr = get();
				DEBUG_LEXER_NEXT("[LE]", FORMAT(format));
				return { LE, 0 };
			}

			// <<
			else if (curr == '<') {
				curr = get();
				DEBUG_LEXER_NEXT("[SHL]", FORMAT(format));
				return { SHL, 0 };
			}

			// <
			else {
				DEBUG_LEXER_NEXT("[LT]", FORMAT(format));
				return { LT, 0 };
			}
		}

		// > 或 >= 或 >>
		else if (curr == '>') {
			curr = get();

			// >=
			if (curr == '=') {
				curr = get();
				DEBUG_LEXER_NEXT("[GE]", FORMAT(format));
				return { GE, 0 };
			}

			// >>
			else if (curr == '>') {
				curr = get();
				DEBUG_LEXER_NEXT("[SHR]", FORMAT(format));
				return { SHR, 0 };
			}

			// >
			else {
				DEBUG_LEXER_NEXT("[GT]", FORMAT(format));
				return { GT, 0 };
			}
		}

		// | 或 ||
		else if (curr == '|') {
			curr = get();

			// ||
			if (curr == '|') {
				curr = get();
				DEBUG_LEXER_NEXT("[LOR]", FORMAT(format));
				return { LOR, 0 };
			}

			// |
			else {
				DEBUG_LEXER_NEXT("[OR]", FORMAT(format));
				return { OR, 0 };
			}
		}

		// & 或 &&
		else if (curr == '&') {
			curr = get();

			// &&
			if (curr == '&') {
				curr = get();
				DEBUG_LEXER_NEXT("[LAN]", FORMAT(format));
				return { LAN, 0 };
			}

			// &
			else {
				DEBUG_LEXER_NEXT("[AND]", FORMAT(format));
				return { AND, 0 };
			}
		}

		// ^
		else if (curr == '^') {
			curr = get();
			DEBUG_LEXER_NEXT("[XOR]", FORMAT(format));
			return { XOR, 0 };
		}

		// %
		else if (curr == '%') {
			curr = get();
			DEBUG_LEXER_NEXT("[MOD]", FORMAT(format));
			return { MOD, 0 };
		}

		// *
		else if (curr == '*') {
			curr = get();
			DEBUG_LEXER_NEXT("[MUL]", FORMAT(format));
			return { MUL, 0 };
		}

		// ?
		else if (curr == '?') {
			curr = get();
			DEBUG_LEXER_NEXT("[COND]", FORMAT(format));
			return { COND, 0 };
		}

		// ,
		else if (curr == ',') {
			curr = get();
			DEBUG_LEXER_NEXT("[COMMA]", FORMAT(format));
			return { COMMA, 0 };
		}

		// :
		else if (curr == ':') {
			curr = get();
			DEBUG_LEXER_NEXT("[COLON]", FORMAT(format));
			return { COLON, 0 };
		}

		// ;
		else if (curr == ';') {
			curr = get();
			DEBUG_LEXER_NEXT("[SEMICON]", FORMAT(format));
			return { SEMICON, 0 };
		}

		// (
		else if (curr == '(') {
			curr = get();
			DEBUG_LEXER_NEXT("[LPAREN]", FORMAT(format));
			return { LPAREN, 0 };
		}

		// )
		else if (curr == ')') {
			curr = get();
			DEBUG_LEXER_NEXT("[RPAREN]", FORMAT(format));
			return { RPAREN, 0 };
		}

		// [
		else if (curr == '[') {
			curr = get();
			DEBUG_LEXER_NEXT("[LBRACK]", FORMAT(format));
			return { LBRACK, 0 };
		}

		// ]
		else if (curr == ']') {
			curr = get();
			DEBUG_LEXER_NEXT("[RBRACK]", FORMAT(format));
			return { RBRACK, 0 };
		}

		// {
		else if (curr == '{'){
			curr = get();
			DEBUG_LEXER_NEXT("[LBRACE]", FORMAT(format));
			return { LBRACE, 0 };
		}

		// }
		else if (curr == '}'){
			curr = get();
			DEBUG_LEXER_NEXT("[RBRACE]", FORMAT(format));
			return { RBRACE, 0 };
		}

		// 空白符
		else {
			curr = get();
			DEBUG_LEXER_NEXT("[WS]", FORMAT(format));
		}
	}

	//扫描结束
	return { END, 0 };
}
