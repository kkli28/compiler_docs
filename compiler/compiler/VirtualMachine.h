#ifndef __VIRTUAL_MACHINE_H__
#define __VIRTUAL_MACHINE_H__

#include "stdafx.h"
#include "Instruction.h"

/*
�ļ�������������Ķ���
*/

namespace kkli {

	//========================================
	// �����
	//========================================
	class VirtualMachine {
	private:

		//ָ������
		const std::vector<std::string> INSTRUCTION_NAME = {
			"I_LEA ", "I_IMM ", "I_JMP ", "I_CALL", "I_JZ  ", "I_JNZ ", "I_ENT ", "I_ADJ ",  //��������ָ��
			"I_LEV ", "I_LI  ", "I_LC  ", "I_SI  ", "I_SC  ", "I_PUSH",
			"I_OR  ", "I_XOR ", "I_AND ", "I_EQ  ", "I_NE  ", "I_LT  ", "I_GT  ", "I_LE  ", "I_GE  ", "I_SHL ", "I_SHR ",
			"I_ADD ", "I_SUB ", "I_MUL ", "I_DIV ", "I_MOD ", //��������
			"I_PRTF", "I_MALC", "I_EXIT" //���ú���
		};
		//�ڴ��и��εĴ�С
		const int SEGMENT_SIZE = 256 * 1024;

	private:
		int* data;        //���ݶ�
		int* text;        //�����
		int* stack;       //ջ
		int* nextData;    //��һ��������ݵ�λ��
		int* nextText;    //��һ�����ָ���λ��

		//�����о���Ҫ�õ��Ĵ�������˽�������Ϊpublic
	public:
		int* sp;    //sp�Ĵ���
		int* bp;    //bp�Ĵ���
		int* pc;    //pc�Ĵ���
		int ax;     //ax�Ĵ�����ջ�����棩

		//��ֹ���졢���ơ���ֵ
		VirtualMachine(const VirtualMachine& rhs) = delete;
		const VirtualMachine& operator=(const VirtualMachine& rhs) = delete;
		VirtualMachine();
		
	public:

		//����
		static VirtualMachine* getInstance() {
			static VirtualMachine* vm = nullptr;
			if (vm == nullptr) {
				vm = new VirtualMachine();
			}
			return vm;
		}
		
		//�������ݡ�ָ�ָ��Ĳ�����
		void addData(int elem);
		void addInst(Instruction elem);
		void addInstData(int elem);
		
		//ɾ��������ָ��
		void deleteTopInst();

		//getter
		int* getDataSegment() const { return data; }
		int* getTextSegment() const { return text; }
		int* getStack() const { return stack; }

		//��ȡ��һ�����λ��
		int* getNextDataPos() const { return nextData; }
		int* getNextTextPos() const { return nextText; }

		//ջ����
		int pop();
		void push(int elem);

		//ִ��ָ��
		int run();

		//��ȡ�������Ϣ
		std::string getInfo() const;

		//��ȡָ����Ϣ
		const std::vector<std::string>& getInstructionInfo() const { return INSTRUCTION_NAME; }

		//��ȡָ������
		std::string getInstructionName(Instruction i) const { return INSTRUCTION_NAME[i]; }
	};
}

#endif