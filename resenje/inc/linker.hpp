#ifndef LINKER_HPP
#define LINKER_HPP
#include<iostream>
#include<string>
#include <regex>
#include <fstream>
#include <iomanip>
#include <map>

using namespace std;

class Linker{

  private:  
    
    struct SymbolDefinition{
      string label;
      uint32_t section;
      int32_t value;
      bool global;   // global/~local
      bool external;
      bool defined; 

      uint32_t aggregatedIndex;
    };

    struct RelocationDefinition{
      uint32_t section;
      uint32_t offset;
      string type;
      uint32_t symbolIndex;
      int32_t addend;
    };

    struct LiteralsTable{
      uint32_t value;
      uint8_t size;   //in bytes
      uint32_t location;
    };

    struct SectionDefinition
    {
      string name;
      int32_t base;
      uint32_t length; 
      map<uint32_t, uint8_t> data;
 
      uint32_t aggregateIndex; // used for navigation through aggregated sections
      uint32_t virtualAddress;
    };

    struct MappedSection{
      string sectionLabel;
      uint32_t startAddress;
      uint32_t size; // set after aggregation, for checking overlay
      uint32_t index;
    };

    vector<vector<SymbolDefinition>> SymbolTables;
    vector<vector<SectionDefinition>> SectionTables;
    vector<vector<RelocationDefinition>> RelocationTables;
    vector<MappedSection> mappedSections;
    map<string, SymbolDefinition > extractedSymbols;
    vector<string> inputFiles;

    vector<SymbolDefinition> AggregatedSymbolTable;
    vector<SectionDefinition> AggregatedSectionTable;
    vector<RelocationDefinition> AggregatedRelocationTable;

    bool relocatable;
    bool errorDetected;
    bool fileEnd;
    string outputFile;

    uint32_t const AbsoluteSectionIndex = 1;
    uint32_t const UndefinedSectionIndex = 0;

    bool processInputFiles(); // extracts data from input files

    // combines and connects symbols, sections, data and relocations of the same section
    void aggregateSectionTables(); 
    void aggregateSymbolTables();
    void aggregateRelocationTables();
    void aggregateDataTables();

    void allocateSection(); // sets the addresses of sections in memory

    bool overlapingSections(MappedSection s1, MappedSection s2);// checks if section spaces overlap

    void resolveRelocations(); // completes relocations and loads data

    // generates output files
    void generateObj(); // generates a binary object file available for further linking
    void generateBinaryExe(); // generates a binary exe for emulation
    void generateExe(); // generates an executable file in text format
    void generateObjTxt(); // generates an object file for further linking

    public:
    // processes the input command, if proper
    // sets approptiate flags, takes in input files and sets the output file, enabling linker to proceed 
    bool processInput(int argC, const char *argv[]);

    Linker();
    void link();

    void printContent(); //writes out aggregated tables


};




#endif