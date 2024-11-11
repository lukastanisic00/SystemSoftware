#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP
#include<iostream>
#include<string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <map>

using namespace std;

class Assembler{
  
  public:

    Assembler(string input, string output);
    
    void assemble();

  private:

    bool firstPass();
    bool secondPass();

    // first pass processing
    void processGlobalDeclaration(string label);
    void processExternDeclaration(string label);
    void processSectionDeclaration(string label);
    void processWordDeclaration();
    void processSkipDeclaration(string literal);
    void processASCIIDeclaration(string str);
    void processEquDeclaration(string label, string value);
    void processEndDeclaration();
    void processLabelDeclaration(string label);
    void processInstruction(string line);

    int32_t calculateExpression(string expression); // calculates the expression, returning a value
    // or leaving a relocation for linker to resolve
    void processSectionEnding(); // position the pool at the end of the section
    
    int32_t getValue(string literal);
    void updatePool();

    // second pass processing

    void processSectionDeclarationSecondPass(string label);
    void processWordDeclarationSecondPass(string parameter);
    void processSkipDeclarationSecondPass(string literal);
    void processASCIIDeclarationSecondPass(string str);
    void processInstructionSecondPass(string line);
    void processEquDeclarationSecondPass(string label, string value);
    
    int32_t setAbsoluteJump(string addr);
    uint32_t findSymbolLocationInLiteralPool(string symbol); // returns the symbol's address in the pool of literals
    
    uint8_t fetchRegister(string s);
    void unloadLiteralsPool();

    void generateRelocation(uint32_t symbolIndex, uint32_t entry, uint16_t sectionIndex);

    // register auxiliaries
    uint8_t const registerPC = 15;
    uint8_t const registerZero = 0;
    uint8_t const registerSP = 14;

    void processCode(); // processes set input file
    void generateOutput(); //generates a textual representation of object file
    void generateBinary();  //generates binary file for further processing

    uint32_t locationCounter;
    int32_t currentSection = -1;
    int32_t currentSymbol;

    uint8_t const AbsoluteSectionIndex = 1;
    uint8_t const UndefinedSectionIndex = 0;

    uint16_t currentSectionIndex = 0;
    uint32_t currentSymbolIndex  = 0;

    string inputPath;
    string outputPath;

    vector<string> inputCode;
    uint32_t currentLine;

    struct SymbolDefinition{
      string label;
      uint32_t section;
      int32_t value;
      bool global;   // global/~local
      bool external;
      bool defined; 
    };

    struct RelocationDefinition{
      uint32_t section;
      uint32_t offset;
      string type;
      uint32_t symbolIndex;
      int32_t addend;
    };

    struct LiteralsTable{
      string symbol;
      int32_t value;
      uint8_t size;   //in bytes
      uint32_t location;
    };

    struct SectionDefinition{
      string name;
      int32_t base;
      uint32_t length;
      map<uint32_t, uint8_t> data;
      vector<LiteralsTable> literalPool; 
    };

    vector<SymbolDefinition> SymbolTable;
    vector<SectionDefinition> SectionTable;
    vector<RelocationDefinition> RelocationTable;

    bool errorDetected;

    bool fileEnd = false;
};









#endif