#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

/*
�ļ����������ű�����¼��������еķ�����Ϣ
*/

#include "Token.h"

namespace kkli {

	//========================================
	// ���ű�
	//========================================

	/*
	tableΪ��������ɵķ���
	currTokenΪ�������Ƶķ��ţ�����Ϣ����д��Խ�ʷ��������﷨�������޷���
	һ���ھ���д��ɣ������Ҫ�����洢������Ϣ��д�������ٷ�����ű�
	*/
	class SymbolTable {
	private:
		std::vector<Token> table;
		int current;

	public:

		//����
		static SymbolTable* getInstance() {
			static SymbolTable* table = nullptr;
			if (table == nullptr) {
				table = new SymbolTable();
			}
			return table;
		}

		//�ж��Ƿ���ڷ���
		bool has(int hash, std::string name);

		//���ӷ�����Ϣ���ڴʷ��������ܹ���ȡ����Ϣ��
		void addToken(int type, std::string name, int hash);

		//���Ʒ�����Ϣ�����﷨�������ܻ�ȡ����Ϣ��
		void finishToken(int klass, int dataType, int value);

		//��ȡ��ǰ����
		Token& getCurrentToken() { return table[current]; }

		//��ȡ���ű���Ϣ
		std::string getSymbolTableInfo() const;

		//��ȡ�ڲ��ṹtable
		std::vector<Token>& getTable() { return table; }
	};
}

#endif