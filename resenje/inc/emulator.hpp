#ifndef EMULATOR_HPP
#define EUMULATOR_HPP
#include<iostream>
#include<string>
#include <regex>
#include <fstream>
#include <iomanip>
#include <termios.h>
#include <stdlib.h>
#include <chrono>
#include <unistd.h>
#include <signal.h>

using namespace std;

class Emulator{

  enum GPR{
    zero = 0,
    reg1 = 1,
    reg2 = 2,
    reg3 = 3,
    reg4 = 4,
    reg5 = 5,
    reg6 = 6,
    reg7 = 7,
    reg8 = 8,
    reg9 = 9,
    reg10 = 10,
    reg11 = 11,
    reg12 = 12,
    reg13 = 13,
    sp = 14,
    pc = 15
  };

  uint8_t const fault = 1;
  uint8_t const timer_interrupt = 2;
  uint8_t const terminal_interrupt = 3;
  uint8_t const software_interrupt = 4;
  
  enum CSR{
    status = 0,    //status word
    handler = 1,   // contains the address of the subroutine that's about to processed
    cause = 2  // 0x1 -fault; 0x2 timer interrupt; 0x3 terminal interrupt; 0x4 int
  };

  enum OpCode{
    HALT = 0x0,
    INT = 0x10,
    IRET = 0x93,
    CALL = 0x21,
    RET = 0x93,
    JMP = 0x38,
    BEQ = 0x39,
    BNE = 0x3a,
    BGT = 0x3b,
    PUSH = 0x81,
    POP = 0x93,
    XCHG = 0x40,
    ADD = 0x50,
    SUB = 0x51,
    MUL = 0x52,
    DIV = 0x53,
    NOT = 0x60,
    AND = 0x61,
    OR = 0x62,
    XOR = 0x63,
    SHL = 0x70,
    SHR = 0x71,
    LD_REG_IND = 0x92,
    LD_REG_MEM = 0x92,
    LD_IMMED = 0x92,
    LD_REG = 0x91,
    ST_REG_IND = 0x80,
    ST_REG_MEM = 0x82,
    CSRRD = 0x90,
    CSRWR = 0x94
  };

  // some instructions overlap but they have a unified approach regardless, their difference is based
  // solely on operand values

  // Memory
  map<uint32_t, int8_t> memory;   

  int8_t readMem(uint32_t address);  // reads a single addressible unit 
  int32_t readMemWord(uint32_t address); // reads a word(4 addressible units)     
  void writeMem(int8_t value, uint32_t address);  // writes value in a single addressible unit 
  void writeMemWord(int32_t value, uint32_t address); // writes a word
  
  const uint32_t MEMORY_SIZE = UINT32_MAX; 
  const uint32_t MEMORY_MAPPED_REGISTERS_ADDRESS = 0xFFFFFF00; // simultaneously the start of stack
  const uint16_t MEMORY_MAPPED_REGISTERS_SIZE = 256;

  struct Segment{
    uint32_t virtualAddress; 
    uint32_t size; // in bytes
    vector<int8_t> data;   
  };

  // Registers

  const uint8_t GPR_NUMBER = 16;   // number of general purpose registers 
  const uint8_t CSR_NUMBER = 3;   // number of specialized registers 

  vector<int32_t> registers;
  vector<int32_t> csrRegisters;

  int32_t &regZero = registers[zero];
  int32_t &regPC = registers[pc];
  int32_t &regSP = registers[sp];

  void push(int32_t value);
  int32_t pop();

  int32_t &csrStatus = csrRegisters[status];
  int32_t &csrHandler = csrRegisters[handler];
  int32_t &csrCause = csrRegisters[cause];


  // Instructions
  const uint8_t INSTRUCTION_SIZE = 4; // except for load mem and iret that use two instructions
  //which are parsed into base part instructions in assembler

  OpCode opCode; 
  uint8_t op;

  uint8_t regA;
  uint8_t regB;
  uint8_t regC;

  int16_t displacement; // 12 bits effectively

// loading from memory and iret consist of two instructions that still need to be executed atomically
  bool ldMem;
  bool iret;

  uint32_t currentPC; // holds the previous pcs value;

  bool fetchAndDecodeInstruction();
  bool executeInstruction();

  // Jump conditions

  enum StatusFlag
  {
    timer_flag = 1,// timer interrupt mask
    terminal_flag = 1<<1, //terminal interrupt mask
    interrupt_flag = 1<<2, // global external interrupt mask
  };

  void setFlag(uint8_t flag); // sets flag value to 0 in status
  void resetFlag(uint8_t flag); // sets flag value to 1 in status
  bool getFlag(uint8_t flag);

  vector<Segment> segments; 

  void handleFault(); // sets fault interrupt and calls handler to terminate execution

  void setInterupt(uint8_t interrupt); 
  void handleInterrupt();       
  void processSubroutine();   // processes interrupt request

  // Timer
  bool timerActive; 

  int64_t period; // all in milliseconds

  int64_t previousTime;
  int64_t currentTime;

  void resetTimer(uint32_t value);
  void timerTick(); // time passed between two instructions effectively

  const uint32_t TIM_CFG = 0xFFFFFF10; // mapped configuration register for timer

  // Terminal
  string terminalError;

  bool configureTerminal();
  void resetTerminal();
  void readTerminal();

  const uint32_t TERM_OUT = 0xFFFFFF00; // mapped output register for terminal
  const uint32_t TERM_IN = 0xFFFFFF04;  // mapped input register for terminal

  const uint32_t TIMER_CFG = 0xFFFFFF10;  // mapped register for timer configuration


  // Miscellaneous
  ofstream outputFile;
 
  string inputFile;
  bool running;
  bool errorDetected;

  const uint8_t ADDRESSABLE_UNIT = 1; // size of addressable unit in bytes
  const uint8_t WORD = 4; // in bytes
  const uint32_t START = 0x40000000; // initial address of the program

public:

  Emulator(string input);

  bool processInput();
  bool loadData();  // loads input file data into memory
    
  void emulate(); // starts the process of emulation

  void generateOutput();

};


#endif