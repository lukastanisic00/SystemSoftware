#include "../inc/linker.hpp"


Linker::Linker(){

  this->relocatable = false;
  this->outputFile = "linkerOut.o";
  this->fileEnd = false;
  this->errorDetected =  false;
}

void Linker::link(){

  cout<<"Processing files..."<< endl;
  
  if(!processInputFiles()) {
    cout<<"Linking unsuccessful: error reading input files."<< endl;
    return;
  }

  cout<<"Linking Sections"<< endl;
  aggregateSectionTables();
  if(errorDetected){
    cout<<"Linking unsuccessful: error allocating sections."<<endl;
    return;
  }

  cout<<"Linking Symbols"<< endl;
  aggregateSymbolTables();
  if(errorDetected){
    cout<<"Linking unsuccessful: error allocating symbols."<<endl;
    return;
  }

  cout<<"Linking Relocations"<< endl;
  aggregateRelocationTables();
  
  cout<<"Linking Data"<< endl;
  aggregateDataTables();

  cout<<"Resolving Relocations"<< endl;
  resolveRelocations();
  
  if(errorDetected){
    cout<<"Linking unsuccessful: error resolving symbols."<<endl;
    return;
  }

  printContent();

  if(relocatable){
    generateObj();
    generateObjTxt();
    cout<<"Linking successful: relocatable file generated."<<endl;
    return;
  }

  generateExe();
  generateBinaryExe();
  cout<<"Linking successful: executable file generated."<<endl;
  
  cout<<"************"<<endl;

  return;  
}

void Linker::printContent(){

  cout<<endl;
  cout<<"*********"<<endl;
  cout<<"PROGRAM CONTENT"<<endl;
  cout<<endl;

  for (auto i= 0; i< AggregatedSectionTable.size(); i++){
    cout<< "Section data <"<< AggregatedSectionTable[i].name<< ">:"<< endl;
    uint32_t words = AggregatedSectionTable[i].length - AggregatedSectionTable[i].length % 4;
    for (auto j = 0; j< words; j+=4){
      cout<< hex<<setw(8) << setfill('0') << (AggregatedSectionTable[i].virtualAddress+j);

      cout<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].virtualAddress+j];
      cout<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].virtualAddress+j+1];
      cout<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].virtualAddress+j+2];
      cout<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].virtualAddress+j+3];
      cout<< endl;
    }
    if(words!= AggregatedSectionTable[i].length){
      cout<< hex<<setw(8) << setfill('0') << (AggregatedSectionTable[i].base+words);
      for(auto k = 0; k<AggregatedSectionTable[i].length -words; k++  )
        cout<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].virtualAddress+k+words];
    }
  }

  cout<<"***********"<<endl;
  cout<<endl;
}

void Linker::aggregateDataTables(){

  for(auto i = 0; i < inputFiles.size(); i++){
    for(auto j = 0; j < SectionTables[i].size() ; j++){
      uint32_t aggregatedSection = SectionTables[i][j].aggregateIndex;
      uint32_t vaddr = AggregatedSectionTable[SectionTables[i][j].aggregateIndex].virtualAddress;
      vaddr+= SectionTables[i][j].base;
      for(uint32_t k = 0; k < SectionTables[i][j].length; k++){
        uint32_t newAddr = vaddr + k;
        AggregatedSectionTable[aggregatedSection].data[newAddr]= SectionTables[i][j].data[k]; 
      }
    }
  }
}

bool Linker::overlapingSections(MappedSection s1, MappedSection s2){

  uint32_t endAdrr1 = s1.startAddress + s1.size;
  uint32_t endAdrr2 = s2.startAddress + s2.size;
  
  if(s1.startAddress <= s2.startAddress && endAdrr1 <= s2.startAddress) return false;
  
  if(s1.startAddress >= s2.startAddress && s1.startAddress >= endAdrr2) return false;
  
  return true;
}

bool Linker::processInput(int numParam, const char *params[]){

  regex regPlace("^-place=([a-zA-Z_][a-zA-Z_0-9]*)@(0[xX][0-9a-fA-F]+)$");
  smatch placement;

  bool outputSet = false;
  this->outputFile = "";
  bool reloc = false;
  bool hex = false;

  for (auto i = 1; i < numParam; i++){
    string currentParam = params[i];

    if (currentParam == "-o"){
      outputSet = true;
    }
    else if (currentParam == "-relocatable"){
      reloc = true;
      this->relocatable = true;
    }
    else if (currentParam == "-hex"){
      hex = true;
    }
    else if (regex_search(currentParam, placement, regPlace)){
      string sectionLabel = placement.str(1);
      uint32_t address = stoul(placement.str(2), nullptr, 16);
      mappedSections.push_back({sectionLabel, address, 0});
    }
    else if (outputSet){
      if (outputFile.size() != 0){
        cout<<"Error: Multiple output files set."<<endl;
        return false;
      }

      outputFile = currentParam;
      outputSet = false;
    }
    else{
      inputFiles.push_back(currentParam);
    }
  }

  if ((reloc && hex) || (!reloc && !hex)){
    cout << "File may only have either -relocatable or -hex option set" << endl;
    errorDetected = true;
    return false;
  }

  if(outputFile.length() <= 5){
    outputFile = "program.hex"; 
    if(this->relocatable)
      outputFile = "relocProgram.o"; 
  }


  string outputExtension = outputFile.substr(outputFile.length() -2);
  if(this->relocatable){
    if (outputExtension != ".o"){
      cout<<"Inappropriate extension for an object file(.o)"<<endl;
      errorDetected = true;
    } 
  }
  outputExtension = outputFile.substr(outputFile.length() -4);
  if (outputExtension != ".hex"){
    cout<<"Inappropriate extension for a hex file(.hex)"<<endl;
    errorDetected = true;
  } 


  return true;    
}

void Linker::generateObj(){

  string outputBin = this->outputFile;
  
  ofstream outputFile(outputBin, ios::out | ios::binary);
  
  // Symbols
  uint32_t symbols = AggregatedSymbolTable.size();
  outputFile.write((char *)&symbols, sizeof(symbols));
  for (auto i = 0; i< AggregatedSymbolTable.size();i++){     
    outputFile.write((char *)(&i), sizeof(i));
    uint32_t labelLen = AggregatedSymbolTable[i].label.length();

    outputFile.write((char *)(&labelLen), sizeof(labelLen));
    outputFile.write(AggregatedSymbolTable[i].label.c_str(), labelLen);

    outputFile.write((char *)(&AggregatedSymbolTable[i].section), sizeof(AggregatedSymbolTable[i].section));
    outputFile.write((char *)(&AggregatedSymbolTable[i].defined), sizeof(AggregatedSymbolTable[i].defined));
    outputFile.write((char *)(&AggregatedSymbolTable[i].external), sizeof(AggregatedSymbolTable[i].external));
    outputFile.write((char *)(&AggregatedSymbolTable[i].global), sizeof(AggregatedSymbolTable[i].global));
    outputFile.write((char *)(&AggregatedSymbolTable[i].value), sizeof(AggregatedSymbolTable[i].value));        
  }
  // Sections
  uint32_t sections = AggregatedSectionTable.size();
  outputFile.write((char *)&sections, sizeof(sections));

  for (auto i = 0; i< AggregatedSectionTable.size(); i++){
    outputFile.write((char *)(&i), sizeof(i));

    uint32_t labelLen = AggregatedSectionTable[i].name.length();
    outputFile.write((char *)(&labelLen), sizeof(labelLen));
    outputFile.write(AggregatedSectionTable[i].name.c_str(), labelLen);

    outputFile.write((char *)(&AggregatedSectionTable[i].base), sizeof(AggregatedSectionTable[i].base));
    outputFile.write((char *)(&AggregatedSectionTable[i].length), sizeof(AggregatedSectionTable[i].length));

    // Data output
    uint32_t dataSize  = AggregatedSectionTable[i].length;
    outputFile.write((char *)&dataSize, sizeof(dataSize));
    for (auto j = 0; j< AggregatedSectionTable[i].length; j++){
      uint32_t adr0 = AggregatedSectionTable[i].base + j;
      outputFile.write((char *)(&adr0), sizeof(adr0));
      
      int8_t value = AggregatedSectionTable[i].data[AggregatedSectionTable[i].base+j]; 
      outputFile.write((char *)(&value), sizeof(value));
    }         
  }      

    // Relocations 
  uint32_t relocs = AggregatedRelocationTable.size();
  
  outputFile.write((char *)&relocs, sizeof(relocs));
  for (auto i = 0; i< AggregatedRelocationTable.size();i++){  
    outputFile.write((char *)(&i), sizeof(i));   
    outputFile.write((char *)(&AggregatedRelocationTable[i].addend), sizeof(AggregatedRelocationTable[i].addend));
    outputFile.write((char *)(&AggregatedRelocationTable[i].offset), sizeof(AggregatedRelocationTable[i].offset));
    outputFile.write((char *)(&AggregatedRelocationTable[i].section), sizeof(AggregatedRelocationTable[i].section));
    outputFile.write((char *)(&AggregatedRelocationTable[i].symbolIndex), sizeof(AggregatedRelocationTable[i].symbolIndex));
  }

    outputFile.close();
    cout<<"Binary generated in "<<outputBin<<endl;
}
void Linker::generateExe(){
  
  string outputTXT = outputFile.substr(0, outputFile.length()-3);
  outputTXT+="txt";
  ofstream hexTXT(outputTXT);

  // Data output for absolute section
 
  for(SectionDefinition section: AggregatedSectionTable){
    // Data output
    if (section.length == 0) continue;
    hexTXT<<"< "<<section.name<<" >"<<endl;
    int32_t cnt = section.length - (section.length % 8);
    uint32_t adr;
    for(auto i = 0; i < cnt; i+= 8){
      uint32_t adr=section.virtualAddress+i;
      hexTXT<< hex<<setfill('0') << setw(8)<< adr<< "\t";
      hexTXT<< hex<<setfill('0') << setw(2)<< +section.data[adr+0]<< "\t"<< hex<<setfill('0') << setw(2)<< +section.data[adr+1]
      << "\t"<< hex<<setfill('0') << setw(2)<< +section.data[adr+2]<< "\t"<< hex<<setfill('0') << setw(2)
      << +section.data[adr+3]<< "\t"<< hex<<setfill('0') << setw(2)<< +section.data[adr+4]
        << "\t"<< hex<<setfill('0') << setw(2)<< +section.data[adr+5]<< "\t"
        << hex<<setfill('0') << setw(2)<< +section.data[adr+6]<< "\t"
        << hex<<setfill('0') << setw(2)<< +section.data[adr+7];

      hexTXT<< endl;
    }  
    int32_t cnt1 = section.length % 8;
    adr =section.length-cnt1 + section.virtualAddress;
    hexTXT<< hex<<setfill('0') << setw(8)<< adr<< "\t";
    for(auto i = 0; i< 8; i++){
      if(i<cnt1)
        hexTXT<< hex<<setfill('0') << setw(2)<< +section.data[adr+0]<< "\t";
      else
        hexTXT<< hex<<setfill('0') << setw(2)<< +0<< "\t";     
    }
    hexTXT<< endl;
  } 
  hexTXT<< endl;

  hexTXT.close();
  cout<<"Text generated in "<< outputTXT<<endl; 
}

void Linker::generateBinaryExe(){

  ofstream bin(this->outputFile, ios::out | ios::binary);
     
  uint32_t sections = AggregatedSectionTable.size();
  
  bin.write((char *)&sections, sizeof(sections));

  // Data output for absolute section
  for(SectionDefinition section: AggregatedSectionTable){
    // Data output   
    uint32_t dataSize = section.length;
    uint32_t vaddr = section.virtualAddress;

    bin.write((char *)(&vaddr), sizeof(vaddr));
    bin.write((char *)(&dataSize), sizeof(dataSize));

    uint32_t addr;
    for(auto i = 0; i < section.length; i++){
      uint32_t addr=section.virtualAddress+i;
      int8_t val = section.data[addr] & 0xff;
      
      bin.write((char *)(&val), sizeof(val));
    }    
  }
  
  bin.close();
  cout<< "Binary exe generated in "<< this->outputFile<<endl;  
}

void Linker::generateObjTxt(){

  string outputTXT = outputFile.substr(0, outputFile.length()-1);
  outputTXT+="txt";
  ofstream ObjTXT(outputTXT);

  ObjTXT << "Section table:" << endl;
  ObjTXT << "Id\tName\t\tSize" << endl;
  
  for (auto i = 0; i< AggregatedSectionTable.size(); i++){
    ObjTXT<< i << "\t"<< AggregatedSectionTable[i].name
      << "\t"<< hex<< setfill('0')<< setw(4)<< AggregatedSectionTable[i].length<< endl;
  }

  ObjTXT<< dec;
  ObjTXT<< endl;
  ObjTXT<< endl;

  ObjTXT << "Symbol table:" << endl;
  ObjTXT << "Value\tType\tSection\t\tName\t\tId" << endl;
  for (auto i = 0; i< AggregatedSymbolTable.size(); i++){
    ObjTXT<< hex<< setfill('0')<< setw(4)<< AggregatedSymbolTable[i].value<< "\t";
      
      if(AggregatedSymbolTable[i].global)
        ObjTXT<< "g\t";
      else 
        ObjTXT<< "l\t";
        
      if(AggregatedSymbolTable[i].defined)
        ObjTXT<< "d\t";
      else
        ObjTXT<< "u\t"; 
           
      if (AggregatedSymbolTable[i].external)
        ObjTXT<< "e\t";
      
      ObjTXT<< AggregatedSectionTable[AggregatedSymbolTable[i].section].name 
        <<"\t"<< AggregatedSymbolTable[i].label<< "\t"<< hex<< setfill('0')<< setw(4)<<i<< endl;
  }
  ObjTXT << dec;
  ObjTXT << endl;
  ObjTXT << endl;

  for (auto i= 0; i< AggregatedSectionTable.size(); i++){    
    ObjTXT<< "Relocation data<"<< AggregatedSectionTable[i].name<< ">:"<< endl;
    ObjTXT << "Offset\tType\tSymbol\tAddend" << endl;
    for (RelocationDefinition reloc : AggregatedRelocationTable){
      if (reloc.section == i){
        ObjTXT<< hex<< setfill('0')<< setw(8) 
          << reloc.offset<< "\t" <<reloc.type<< "\t" << AggregatedSymbolTable[reloc.symbolIndex].label
            << "\t"<<reloc.addend<< endl;
      }
    }
  } 

  ObjTXT<< dec<< endl;
  
  for (auto i= 0; i< AggregatedSectionTable.size(); i++){
    ObjTXT<< "Section data <"<< AggregatedSectionTable[i].name<< ">:"<< endl;
    uint32_t words = AggregatedSectionTable[i].length - AggregatedSectionTable[i].length % 4;
    for (auto j = 0; j< words; j+=4){
      ObjTXT<< hex<<setw(8) << setfill('0') << (AggregatedSectionTable[i].base+j);
      
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].base+j];
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].base+j+1];
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].base+j+2];
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].base+j+3];
      ObjTXT<< endl;
    }
    if(words!= AggregatedSectionTable[i].length){
      ObjTXT<< hex<<setw(8) << setfill('0') << (AggregatedSectionTable[i].base+words);
      for(auto k = 0; k<AggregatedSectionTable[i].length -words; k++  )
        ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +AggregatedSectionTable[i].data[AggregatedSectionTable[i].base+k+words];
    }
     
    ObjTXT << dec; 
    ObjTXT<< endl;
  }

  ObjTXT << endl;

  ObjTXT.close();
  cout<<"Text generated in "<< outputTXT<<endl;
}
    
bool Linker::processInputFiles(){

  for (auto i = 0; i < inputFiles.size(); i++){
    ifstream inputFile(inputFiles[i], ios::binary);
    if (inputFile.fail()){
      errorDetected = true;
      cout << "Failed to read input file " << inputFiles[i] << "." << endl;
      return false;
    }
    vector<SymbolDefinition> fileSymbolTable;
    uint32_t symbols = 0;
    inputFile.read((char *)&symbols, sizeof(symbols));

    for (auto i = 0; i < symbols; i++){
      SymbolDefinition readSymbol;

      uint32_t index;
      inputFile.read((char *)(&index), sizeof(index)); // redundant
      
      uint32_t stringLength;
      inputFile.read((char *)(&stringLength), sizeof(stringLength));
      readSymbol.label.resize(stringLength);
      inputFile.read((char *)readSymbol.label.c_str(), stringLength);

      inputFile.read((char *)(&readSymbol.section), sizeof(readSymbol.section));
      inputFile.read((char *)(&readSymbol.defined), sizeof(readSymbol.defined));
      inputFile.read((char *)(&readSymbol.external), sizeof(readSymbol.external));
      inputFile.read((char *)(&readSymbol.global), sizeof(readSymbol.global));
      inputFile.read((char *)(&readSymbol.value), sizeof(readSymbol.value));

      fileSymbolTable.push_back(readSymbol);
    }

    SymbolTables.push_back(fileSymbolTable);

    vector<SectionDefinition> fileSectionTable;
    uint32_t sections = 0;
    inputFile.read((char *)&sections, sizeof(sections));

    for (auto i = 0; i < sections; i++){
      struct SectionDefinition readSection;

      uint32_t index;
      inputFile.read((char *)(&index), sizeof(index)); // redundant
      
      uint32_t stringLength;
      inputFile.read((char *)(&stringLength), sizeof(stringLength));
      readSection.name.resize(stringLength);
      inputFile.read((char *)readSection.name.c_str(), stringLength);

      inputFile.read((char *)(&readSection.base), sizeof(readSection.base));
      inputFile.read((char *)(&readSection.length), sizeof(readSection.length));

      readSection.virtualAddress = 0;

      uint32_t dataSize;
      inputFile.read((char *)(&dataSize), sizeof(dataSize));

      for (auto j = 0; j < dataSize; j++){
        uint32_t adr;
        inputFile.read((char *)(&adr), sizeof(adr));

        int8_t val;
        inputFile.read((char *)(&val), sizeof(val));

        readSection.data[adr] = val;
      }
      fileSectionTable.push_back(readSection);
    }

    SectionTables.push_back(fileSectionTable);

    int relocs = 0;
    inputFile.read((char *)&relocs, sizeof(relocs));

    RelocationDefinition reloc;
    vector<RelocationDefinition> relocTab;
    
    for (auto i = 0; i < relocs; i++){
      inputFile.read((char *)(&i), sizeof(i));
      inputFile.read((char *)(&reloc.addend), sizeof(reloc.addend));
      inputFile.read((char *)(&reloc.offset), sizeof(reloc.offset));
      inputFile.read((char *)(&reloc.section), sizeof(reloc.section));
      inputFile.read((char *)(&reloc.symbolIndex), sizeof(reloc.symbolIndex));

      relocTab.push_back(reloc);   
    }
    
    RelocationTables.push_back(relocTab);

    inputFile.close();
  }

  return true;
}

void Linker::aggregateSymbolTables(){

  uint32_t ai = 0;
  
  for(auto i =0; i< SymbolTables.size(); i++){
    for(auto j = 0; j< SymbolTables[i].size(); j++){
      if (extractedSymbols.find(SymbolTables[i][j].label) == extractedSymbols.end()){
        SymbolTables[i][j].aggregatedIndex = ai++;
        extractedSymbols[SymbolTables[i][j].label] = SymbolTables[i][j];
        extractedSymbols[SymbolTables[i][j].label].section
           = SectionTables[i][SymbolTables[i][j].section].aggregateIndex;
      }
      else{
        if (extractedSymbols[SymbolTables[i][j].label].global 
        && SymbolTables[i][j].global
        && SymbolTables[i][j].defined
        && extractedSymbols[SymbolTables[i][j].label].defined){
          // multiple strong symbols result in error
          errorDetected = true;
          cout<<"Multiple defintions of symbol "<< SymbolTables[i][j].label << "."<<endl;
          return;
        }
        else if(SymbolTables[i][j].defined && extractedSymbols[SymbolTables[i][j].label].global 
        && SymbolTables[i][j].global) {
          // if both symbols are strong, the one that is defined is chosen
            SymbolTables[i][j].aggregatedIndex = extractedSymbols[SymbolTables[i][j].label].aggregatedIndex;
            extractedSymbols[SymbolTables[i][j].label].section 
              = SectionTables[i][SymbolTables[i][j].section].aggregateIndex;
        }
        else if(!SymbolTables[i][j].global && !extractedSymbols[SymbolTables[i][j].label].global 
          && SymbolTables[i][j].label != "UND" && SymbolTables[i][j].label != "ABS"
          && SymbolTables[i][j].label != SectionTables[i][SymbolTables[i][j].section].name){
        // two local symbols of the same name can exist, they will just be labeled distinctively
          SymbolTables[i][j].aggregatedIndex = ai++;
          extractedSymbols[SymbolTables[i][j].label+to_string(ai)]
            = SymbolTables[i][j];
        }
        else{
          SymbolTables[i][j].aggregatedIndex = extractedSymbols[SymbolTables[i][j].label].aggregatedIndex;
        }

        extractedSymbols[SymbolTables[i][j].label].global|= SymbolTables[i][j].global;
        extractedSymbols[SymbolTables[i][j].label].defined|= SymbolTables[i][j].defined;
        extractedSymbols[SymbolTables[i][j].label].external|= SymbolTables[i][j].external;
        if (SymbolTables[i][j].defined)
          extractedSymbols[SymbolTables[i][j].label].value= SymbolTables[i][j].value;
      }
    }
  }

  uint32_t ind  = 0;
  for(map<string, SymbolDefinition>::iterator it= extractedSymbols.begin(); it!= extractedSymbols.end(); it++){
    if(!it->second.defined && it->second.external && !relocatable){ 
      // any externally used symbol must be defined
      cout<<"External symbol "<< it->first << " undefined." << endl;
        errorDetected = true;
        return;
    }
    it->second.aggregatedIndex = ind++;
    AggregatedSymbolTable.push_back(it->second);
    if(it->second.section != AbsoluteSectionIndex)
        AggregatedSymbolTable.back().value += AggregatedSectionTable[it->second.section].virtualAddress;
    it->second.value = AggregatedSymbolTable.back().value;
  }
  
  cout<<"***********"<<endl;
  cout<<"Combined Symbol Table"<<endl;
  cout<<endl;
  uint32_t k = 0;
  for(SymbolDefinition symbol:AggregatedSymbolTable){
    k++;
    cout<<"symbol: "<<symbol.label;
    cout<<"\tsection: "<<AggregatedSectionTable[symbol.section].name;
   
    if (symbol.global) cout<<"\tg";
    else cout<<"\tl";
   
    if (symbol.defined) cout<<"\td";
    else cout<<"\tu";
   
    if (symbol.external) cout<<"\te";
    else cout<<"\t";
    cout<<dec<<"\tind :"<<symbol.aggregatedIndex;
    cout<<"\tvalue :"<<hex<<symbol.value;
    cout<<endl;
  }
  cout<<"***********"<<endl;
  cout<<endl;
}

void Linker::aggregateRelocationTables(){

  for(auto i = 0; i< inputFiles.size(); i++){
    for(auto j = 0; j< RelocationTables[i].size(); j++){
      SectionDefinition relocSection = SectionTables[i][RelocationTables[i][j].section];
      SymbolDefinition relocSymbol = 
        extractedSymbols[SymbolTables[i][RelocationTables[i][j].symbolIndex].label];
  
      int32_t addend = RelocationTables[i][j].addend + AggregatedSymbolTable[relocSymbol.aggregatedIndex].value;
      if(relocSection.name == relocSymbol.label) addend+= relocSection.base;
  
      if (relocSymbol.section == AbsoluteSectionIndex) addend = relocSymbol.value;
      uint32_t offset = RelocationTables[i][j].offset 
        + AggregatedSectionTable[relocSection.aggregateIndex].virtualAddress
        + SectionTables[i][RelocationTables[i][j].section].base;
        
      AggregatedRelocationTable.push_back({
        relocSection.aggregateIndex,
        offset,
        RelocationTables[i][j].type,
        relocSymbol.aggregatedIndex,
        addend
      });  
    }
  }
  cout<<"***********"<<endl;
  cout<<"Combined Relocations"<<endl;
  cout<<endl;
 
  for (RelocationDefinition reloc: AggregatedRelocationTable){
    cout << hex<<reloc.offset;
    cout << "\t"<< reloc.type;
    cout << "\t" <<AggregatedSymbolTable[reloc.symbolIndex].label;
    cout << "\t" <<AggregatedSectionTable[reloc.section].name;
    cout << "\t"<< hex<<reloc.addend;
    cout<<dec<<endl;
  }

  cout<<"***********"<<endl;
  cout<<endl;
}

void Linker::aggregateSectionTables(){

  // will be used for setting address of sections conjoined by name
  struct info{ int32_t addr = 0; uint32_t ind; };
  map<string, info> extractedSections;

  for(auto i = 0; i< inputFiles.size(); i++){
    // inializes each name
    for(auto j = 0; j< SectionTables[i].size(); j++){
      extractedSections[SectionTables[i][j].name].ind = 0;
      SectionTables[i][j].base = extractedSections[SectionTables[i][j].name].addr;
      extractedSections[SectionTables[i][j].name].addr +=SectionTables[i][j].length;
    }
  }
  // proceeding to create section definiton entries from extracted sections
  for(map<string, info>::iterator it= extractedSections.begin();it!= extractedSections.end(); it++){
    SectionDefinition newSection;
    newSection.name = it->first;
    newSection.length = it->second.addr;
    newSection.virtualAddress = 0;
    newSection.base = 0;

    if (it->first == "UNDEFINED"){
      it->second.ind = 0;
      AggregatedSectionTable.insert(AggregatedSectionTable.begin() + UndefinedSectionIndex, newSection);
    }
    else if (it->first == "ABSOLUTE"){
      AggregatedSectionTable.insert(AggregatedSectionTable.begin() + 0, newSection);
      it->second.ind =1;
    }
    else{
      it->second.ind = AggregatedSectionTable.size();
      AggregatedSectionTable.push_back(newSection);
    }
    newSection.aggregateIndex = it->second.ind;
  }  
    // updates value, setting virtual addresses and new section indices for sections of each file
  for(auto i = 0; i< inputFiles.size(); i++){
    for(auto j = 0; j< SectionTables[i].size(); j++){
      SectionTables[i][j].virtualAddress = extractedSections[SectionTables[i][j].name].addr;
      SectionTables[i][j].aggregateIndex = extractedSections[SectionTables[i][j].name].ind;
    } 
  }   
  for(auto i = 0; i< SymbolTables.size(); i++){
    for(auto j = 0; j< SymbolTables[i].size(); j++){
      if(SymbolTables[i][j].label != SectionTables[i][SymbolTables[i][j].section].name &&
      SymbolTables[i][j].section != AbsoluteSectionIndex)

        SymbolTables[i][j].value+= SectionTables[i][SymbolTables[i][j].section].base;     
    }
  }  
 
  if(this->relocatable == false){
    this->allocateSection();
  }   

}

void Linker::resolveRelocations(){
  // processes the only type of relocation present in current configuration
  // r_x84_64_32
  for(RelocationDefinition reloc: AggregatedRelocationTable){ 
   // corrects value in memory
    uint32_t sectionIndex = reloc.section;
   
    AggregatedSectionTable[sectionIndex].data[reloc.offset] = reloc.addend&0xff;
    AggregatedSectionTable[sectionIndex].data[reloc.offset+1] = (reloc.addend>>8)&0xff;
    AggregatedSectionTable[sectionIndex].data[reloc.offset+2] = (reloc.addend>>16)&0xff;
    AggregatedSectionTable[sectionIndex].data[reloc.offset+3] = (reloc.addend>>24)&0xff;   
  }
}

void Linker::allocateSection(){

  uint32_t initialFreeSpace = 0;

  for(auto i = 0; i< mappedSections.size(); i++){
    int32_t index = -1;
    for(auto j =0; j< AggregatedSectionTable.size(); j++ ){
      if(mappedSections[i].sectionLabel == AggregatedSectionTable[j].name){
        index = j; // retrieved the section index for use
        AggregatedSectionTable[j].virtualAddress = mappedSections[i].startAddress;
        mappedSections[i].size = AggregatedSectionTable[j].length;
        mappedSections[i].index = j;
        cout<<dec;
        break;
      }
    }
    // iterating thru files 
    uint32_t offset=0;
      for(auto k = 0; k < SectionTables.size(); k++){
        // iterating thru sections of a file
        for(auto l = 0; l< SectionTables[k].size(); l++){
          // offsetting section address with the start of aggregated section
          if(SectionTables[k][l].name == mappedSections[i].sectionLabel){
            SectionTables[k][l].virtualAddress = mappedSections[i].startAddress+ offset;
            offset+= SectionTables[k][l].length;
            // moving free space further
            if(SectionTables[k][l].virtualAddress + SectionTables[k][l].length > initialFreeSpace)
              initialFreeSpace = SectionTables[k][l].virtualAddress + SectionTables[k][l].length;
            break;
          }
        }    
    }
  }
  for(auto i = 0; i< mappedSections.size() -1 ; i++){
    for(auto j = i+1; j< mappedSections.size();j++){
      if (overlapingSections(mappedSections[i], mappedSections[j])){
        errorDetected = true;
        cout<<"Placed sections " << mappedSections[i].sectionLabel << " and " << 
          mappedSections[j].sectionLabel << " are overlapping." << endl;
          return;
      }
    }
  }
  for(auto i= 0; i< AggregatedSectionTable.size(); i++){
    int32_t index = -1;
    for(auto j = 0; j< mappedSections.size(); j++){
      if (mappedSections[j].sectionLabel == AggregatedSectionTable[i].name){
        index = i;
        break;
      }
    }
    if (index == -1){
      AggregatedSectionTable[i].virtualAddress = initialFreeSpace;
      initialFreeSpace+= AggregatedSectionTable[i].length;
    }
  }
}

int main(int argc, const char *argv[]){

  Linker linker;

  cout << "----------------------------------------------------------------" << std::endl;
  cout<<"LINKER"<<endl;
  cout<<endl;
  
  if(linker.processInput(argc, argv) == false){
    cout<<"Failed processing"<<endl;
    return -1;
  }
     
  linker.link();

  return 0;
}





