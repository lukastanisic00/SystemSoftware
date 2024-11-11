#include "../inc/assembler.hpp"
#include"../misc/regex.hpp"



Assembler::Assembler(string input, string output) : inputPath(input), outputPath(output){

  SectionDefinition undefinedSection = {"UNDEFINED", 0, 0};
  SectionDefinition absSection = {"ABSOLUTE", 0, 0};
  SectionTable.insert(SectionTable.begin() + currentSectionIndex++, undefinedSection);
  SectionTable.insert(SectionTable.begin() + currentSectionIndex++, absSection);
  SymbolDefinition undefSymbol = {"UND", 0, 0, false, false, true };
  SymbolTable.push_back(undefSymbol);
  currentSymbolIndex++;
  SymbolDefinition absSymbol = {"ABS", 1, 0x0, false, false, true};
  SymbolTable.insert(SymbolTable.begin() + currentSymbolIndex++, absSymbol);

  currentSection = 1;
  currentSymbol = 0;
  errorDetected = false;
  locationCounter = 0;

  cout<<"Input file: "<<this->inputPath<<endl;
}

void Assembler::assemble(){
   
processCode();  
  
//first pass
if(firstPass()) //if successful, proceed
  if(secondPass()){

    cout<< "Sucessfully compiled."<< endl;
    
    // generates .o and .txt file upon successful compilation
    generateBinary();
    generateOutput();
    return;
  }

cout<< "Error during compilation."<<endl;   
cout<<endl;

}


bool Assembler::firstPass(){

  currentLine = 0;
  for(auto line = 0; line < inputCode.size(); line++){

    smatch atoms;
    string code = inputCode[line];

    if(regex_match(code, regNewline)) continue;

    code = regex_replace(code, regComment, "");
    code = regex_replace(code, regTabs, "");
    code = regex_replace(code, regEmptySpace, "");

    if(code.length() == 0) continue;

    if(regex_search(code, atoms, regLabel)){
      string label = atoms.str(1);

      processLabelDeclaration(label);

      code = atoms.str(2); //additional code will be further processed

      if(code.length() == 0)
        continue;
      
    }
    if(regex_search(code, atoms, regGlobal)){

      string syms = atoms.str(1);
      stringstream ss(syms);

      string symbol;
      syms ="";
      
      while (getline(ss, symbol, ',')) {
                    
        for(auto i =0; i< symbol.length(); i++){
          if (symbol[i] != ' '){
            syms+= symbol[i];
          }
        }
        processGlobalDeclaration(syms);
        syms ="";            
        }
    }

    else if(regex_search(code, atoms, regExtern)){
      string syms = atoms.str(1);
      stringstream ss(syms);

      string symbol;
      syms ="";
      while (getline(ss, symbol, ',')) {              
        for(auto i =0; i< symbol.length(); i++){
          if (symbol[i] != ' '){
            syms+= symbol[i];
          }
        }
        processExternDeclaration(syms);
        syms ="";            
        }
    }
    else if(regex_search(code, atoms, regSection)){
      
      string label = atoms.str(1);
      processSectionDeclaration(label);
    }
    else if(regex_search(code, atoms, regWord)){
      processWordDeclaration();    
    }
    else if(regex_search(code, atoms, regSkip)){
      string literal = atoms.str(1);
      processSectionDeclaration(literal);    
    }
    else if(regex_search(code, atoms, regAscii)){
      string str = atoms.str(1);
      processASCIIDeclaration(str);    
    }
    else if(regex_search(code, atoms, regEqu)){
      string label = atoms.str(1);
      string value = atoms.str(2);

      processEquDeclaration(label, value);    
    }

    else if(regex_search(code, atoms, regEnd)){
      processEndDeclaration();    
      break;
    }
    else if(regex_search(code, atoms, regComment)){
    }
    else{
      processInstruction(code);
    }

    if(errorDetected) {
      line++;
      cout<<"Error at line " << line << "." << endl;
      
      return false;
    }
  }
  
    updatePool();
    return true;
    
}

bool Assembler::secondPass(){
  currentLine = 0;

  for(auto line = 0; line < inputCode.size(); line++){
    
    smatch atoms;
    string code = inputCode[line];

    if(regex_match(code, regNewline)) continue;

    code = regex_replace(code, regComment, "");
    code = regex_replace(code, regTabs, "");
    code = regex_replace(code, regEmptySpace, "");
    
    if(code.length() == 0) continue;


    if(regex_search(code, atoms, regLabel)){

      if(atoms.str(2).length() == 0)
        continue;
      code = atoms.str(2); //additional code will be further processed    
    }
    if(regex_search(code, atoms, regSection)){
      string label = atoms.str(1);
      processSectionDeclarationSecondPass(label);
    }
    else if(regex_search(code, atoms, regWord)){
      string firstSymbol = atoms.str(1);
      processWordDeclarationSecondPass(firstSymbol);

      string additionalSymbols = atoms.str(2);
      string sym ="";
      for(auto i = 0; i < additionalSymbols.size();i++ ){
        if(additionalSymbols[i] == ',' && sym != ""){
          processWordDeclarationSecondPass(sym);
          sym ="";
        }         
        if(additionalSymbols[i] == ' ') continue;
        else sym = sym + additionalSymbols[i];  
      }    
    }
    else if(regex_search(code, atoms, regSkip)){
      string literal = atoms.str(1);
      processSectionDeclarationSecondPass(literal);    
    }
    else if(regex_search(code, atoms, regAscii)){
      string str = atoms.str(1);
      processASCIIDeclarationSecondPass(str);    
    }
     else if(regex_search(code, atoms, regEqu)){
      string label = atoms.str(1);
      string value = atoms.str(2);
  
      processEquDeclarationSecondPass(label, value);    
    }
    else if(regex_search(code, atoms, regEnd)){ 
        
      if(currentSection != -1){
        SectionTable[currentSection].length = locationCounter;    
        processSectionEnding();
      }
      locationCounter = 0;
      break;
    }
    else if(regex_search(code, atoms, regGlobal)){ 
    }
    else if(regex_search(code, atoms, regExtern)){ 
    }
    else{
      processInstructionSecondPass(code);
    }
    if(errorDetected) {
      line++;
      cout<<"Error at line " << line << "." << endl;

      return false;
    }
  }

  unloadLiteralsPool();
  return true;
}

void Assembler::processGlobalDeclaration(string label){

  for (auto i = 0; i< SymbolTable.size(); i++){
    if (SymbolTable[i].label == label ){
      SymbolTable[i].global = true;
      return;
    }
  }

  SymbolDefinition newSymbol = {label, UndefinedSectionIndex, 0, true, false, false};
  currentSymbolIndex++;
  SymbolTable.push_back(newSymbol);

}

void Assembler::processExternDeclaration(string label){

  for (auto i = 0; i< SymbolTable.size(); i++){
    if (SymbolTable[i].label == label){
      if(SymbolTable[i].defined == true){
        errorDetected = true;
        cout<< "Redefinition of external symbol "<< label <<"."<< endl;
        return;  
      }   
    }
  }  

    SymbolDefinition newSymbol = {label, UndefinedSectionIndex, 0, true, true, false};
    currentSymbolIndex++;
    SymbolTable.push_back(newSymbol);

}

void Assembler::processSectionDeclaration(string label){
  
  if (currentSection != -1)
    SectionTable[currentSection].length = locationCounter;
  
  locationCounter = 0;

  for(auto i = 0; i< SymbolTable.size(); i++){
    if (SymbolTable[i].label == label){
      currentSection = SymbolTable[i].section;
      return;
    }
  }

  SectionDefinition newSection = {label, 0, 0, map<uint32_t,uint8_t>()}; 
  currentSection = SectionTable.size();
  SectionTable.push_back(newSection);

  SymbolDefinition newSymbol = {label, (uint32_t)currentSection, 0, false, false, true };
  SymbolTable.push_back(newSymbol);
  currentSymbolIndex++;
}

void Assembler::processWordDeclaration(){

  if (currentSection == -1){
    errorDetected = true;
    cout<< ".word directive can only be used in a section." << endl;
    return;
  }

  locationCounter+= 4;
}

void Assembler::processSkipDeclaration(string value){

  if (currentSection == -1){
    errorDetected = true;
    cout<< ".skip directive can only be used in a section." << endl;
    return;
  }

  locationCounter+= getValue(value);
}

void Assembler::processASCIIDeclaration(string str)
{
  if (currentSection == -1){
    errorDetected = true;
    cout<< ".string directive can only be used in a section." << endl;
    return;
  }

  uint8_t stringSize = str.length();
  
  locationCounter+=stringSize - 2;

  uint8_t pad = 4 - locationCounter % 4;
  if(pad != 4) locationCounter+=pad;
}

void Assembler::processEquDeclaration(string label, string value){

  int16_t symIndex = -1;
  for (auto i = 0; i< SymbolTable.size(); i++){
    if (SymbolTable[i].label == label){
      symIndex = i;
      if(SymbolTable[i].external){
        errorDetected = true;
        cout<<".equ can not define an external symbol."<<endl;
        return;
      }
      if(SymbolTable[i].defined == true){
        errorDetected = true;
        cout<<".equ can only define a previously undefined symbol."<<endl;
        return;
      }
      
      SymbolTable[i].defined = true;
      SymbolTable[i].section = AbsoluteSectionIndex; // the symbol is now a constant and therefore
      // it's in the absolute section    
    }
  }
  if (symIndex == -1){
    SymbolDefinition newSymbol = {label, AbsoluteSectionIndex, 0, false, 
      false, true};
    SymbolTable.push_back(newSymbol);
  }  
}

void Assembler::processEndDeclaration()
{
  //indicates the ending of input file, any content after is disregarded
  if (currentSection != -1 || currentSection != 0)
    SectionTable[currentSection].length = locationCounter;

  currentSymbol = -1;
  currentSection = -1;
  fileEnd = true;
}

void Assembler::processLabelDeclaration(string label)
{
  if(currentSection == -1){
    errorDetected = true;
    cout<< "Labels can only be used in a section." << endl;
    return;  
  }

  for (auto i = 0; i< SymbolTable.size(); i++){
    if(SymbolTable[i].label == label){
      if(SymbolTable[i].defined || SymbolTable[i].external)
        {
          errorDetected = true;
          cout<<"Redefinition of symbol"<< label <<"."<<endl;
          return;
        } 

      SymbolTable[i].value = locationCounter;
      SymbolTable[i].defined = true; 
      SymbolTable[i].section = currentSection; 
      return;
    }
  }

    SymbolDefinition newSymbol = {label, (uint32_t)currentSection, (int32_t)locationCounter, false, false, true};
    currentSymbolIndex++;
    SymbolTable.push_back(newSymbol); 
}

void Assembler::processInstruction(string line){

  if(currentSection == -1){
    errorDetected = true;
    cout<< "Instructions can only be inside a section." << endl;
    return;
  }

  smatch atoms;
  if(regex_search(line, atoms, regIret ))
      locationCounter+=8; // iret uses two instructions
  
  else if(regex_search(line, atoms, regLd )){
    smatch subatoms;
    smatch subatoms3;
    string op = atoms.str(1);

    if((regex_search(op, subatoms, regMem ) || regex_search(op, subatoms, regImmed ))
    && !regex_search(op, subatoms3, regRegInd )){
      
      locationCounter+=4; // ld memind uses two instructions, first loads the address, 
      // then loads the content from the address
      string symbol = subatoms.str(1);
      if(regex_search(op, subatoms, regMem ) && !regex_search(op, subatoms, regImmed 
        )&& !regex_search(op, subatoms, regRegInd)){

        locationCounter+=4;  
      }
      
      int32_t val = 0;

      if (symbol[0] >= '0' && symbol[0] <= '9'){
        
        val = getValue(symbol);
        if(symbol[0] =='$') symbol.substr(1, symbol.length()-1); 
      
      // check if the literal is already in the pool, if not insert it
        for(auto j = 0; j < SectionTable[currentSection].literalPool.size(); j++ ){
          if(symbol == SectionTable[currentSection].literalPool[j].symbol){
            return;
          }
        }
      
        SectionTable[currentSection].literalPool.push_back({symbol, val, 4, locationCounter});       
     }
    }
    else{

      locationCounter+=4;
    }
  }
    // jump instruction use absolute address that needs to be inserted into the literal pool
  else if(regex_search(line, atoms, regBeq)
      || regex_search(line, atoms, regBne) || regex_search(line, atoms, regBgt)){
      
      locationCounter+=4;
      string op = atoms.str(5);
      int32_t val = 0;
     
      if (op[0] >= '0' && op[0] <= '9'){
        val = getValue(op);
      for(auto j = 0; j < SectionTable[currentSection].literalPool.size(); j++ ){
          if(op == SectionTable[currentSection].literalPool[j].symbol){
            return;
          }
        }
        
      SectionTable[currentSection].literalPool.push_back({op, val, 4, locationCounter});
      }  
    }
  else if(regex_search(line, atoms, regCall) || regex_search(line, atoms, regJmp)){
    string op = atoms.str(1);
    locationCounter+=4;
    int32_t val = 0;
    
    if (op[0] >= '0' && op[0] <= '9'){
      val = getValue(op);

      for(auto j = 0; j < SectionTable[currentSection].literalPool.size(); j++ ){
        if(op == SectionTable[currentSection].literalPool[j].symbol){
          return;
        }
      }

      SectionTable[currentSection].literalPool.push_back({op, val, 4, locationCounter});
    }       
  }
  else if(regex_search(line, atoms, regSt)){

    locationCounter+=4;
    smatch subatoms;
    string op = atoms.str(3);

    if (regex_search(op, subatoms, regMem)){
      string symbol = subatoms.str(1);
      int32_t val = 0;

      if (symbol[0] >= '0' && symbol[0] <= '9'){
        val = getValue(symbol);
        if(symbol[0] =='$') symbol.substr(1, symbol.length()-1); 
      
      // check if the literal is already in the pool, if not insert it
        for(auto j = 0; j < SectionTable[currentSection].literalPool.size(); j++ ){
          if(symbol == SectionTable[currentSection].literalPool[j].symbol){
            return;
          }
        }

        SectionTable[currentSection].literalPool.push_back({symbol, val, 4, locationCounter});       
    }
  }
}
  else
    locationCounter+=4;  // instructions are of fixed size of 4 bytes  
}

int32_t Assembler::calculateExpression(string expression){
  vector<SymbolDefinition> symbols;
  smatch atoms;

  int32_t res = 0;
  int32_t op = 0;
  uint8_t opr = 0; // let 1 be addition, 2 deduction, 3 multplication and 4 division

  if (regex_search(expression, atoms, regExpr)){
    for (string atom: atoms){
      if(atom.size() == 0) continue;
      if(atom[0] == '+') opr = 1;
      else if(atom[0] == '-') opr = 2;
      else if(atom[0] == '*') opr = 3;
      else if(atom[0] == '/') opr = 4;
      else if(atom[0] >= '0' && atom[0] <= '9') {
        op = getValue(atom);
        switch (opr)
        {
        case 0:
          res=op;
          break;
        case 1:
          res+=op;
          break;
        case 2:
          res-=op;
          break;  
        case 3:
          res*=op;
          break;
        case 4:
          res/=op;
          break;  
        };
      }
      else{
        int32_t symInd = -1;
        smatch subatoms;
        if(regex_match(atom, subatoms, regSym)){
          for(auto i =0; i< SymbolTable.size(); i++){
            if (SymbolTable[i].label ==  atom){
              symInd = i;
              if(!SymbolTable[i].defined){
                errorDetected = true;
                cout<<"Can not use an undefined symbol "<< SymbolTable[i].label <<" in a expression."<<endl;
                return -1;
              }
              if(SymbolTable[i].external){
                errorDetected = true;
                cout<<"Can not use an external symbol "<< SymbolTable[i].label <<" in a expression."<<endl;
                return -1;
              }
              op = SymbolTable[i].value;
              switch (opr)
        {
        case 0:
          res=op;
          break;
        case 1:
          res+=op;
          break;
        case 2:
          res-=op;
          break;  
        case 3:
          res*=op;
          break;
        case 4:
          res/=op;
          break;  
        };
          } 
          }
          if (symInd == -1){
            errorDetected = true;
            cout<<"Can not use an undeclared symbol "<< atom <<" in a expression."<<endl;
            return -1;
          }
        }
      }
    }
  }

  return res;
}


void Assembler::processSectionEnding(){

  SectionTable[currentSection].length =locationCounter;
}

int32_t Assembler::getValue(string literal){

  if(literal.substr(0,2) == "0x" || literal.substr(0,2) == "0X"){
    stringstream ss;
    uint32_t x;
    ss << hex << literal;
    ss >> x;
    return x;
  }

  return stoi(literal);
  
}

void Assembler::updatePool(){

  uint32_t offset =0;
  for(auto i=0; i< SectionTable.size(); i++){
    for(auto j =0; j< SectionTable[i].literalPool.size(); j++){
      SectionTable[i].literalPool[j].location = SectionTable[i].length + offset;
      offset+=4;
    }
  }
}
void Assembler::processSectionDeclarationSecondPass(string label){

  if(currentSection != -1){
    processSectionEnding();
  }

  locationCounter = 0;
  for (auto i = 0; i< SectionTable.size(); i++){
    if(SectionTable[i].name == label){
      currentSection = i;
      return;
    }
  }
  currentSection = -1; //reset
}

void Assembler::processWordDeclarationSecondPass(string parameter){

  // parameter is a number
  if(parameter[0] >= '0' && parameter[0] <= '9'){

    int32_t val = getValue(parameter);
    SectionTable[currentSection].data[locationCounter]= val & 0xff;
    SectionTable[currentSection].data[locationCounter+1]= (val>>8) & 0xff;
    SectionTable[currentSection].data[locationCounter+2]= (val>>16) & 0xff;
    SectionTable[currentSection].data[locationCounter+3]= (val>>24) & 0xff;
    locationCounter+=4;
    return;
  } 

  // parameter is a symbol
  else{
    currentSymbol = -1;
    for(auto i = 1; i< SymbolTable.size(); i++){
      if(SymbolTable[i].label == parameter){
        currentSymbol = i;
        break;
      }
      if (currentSymbol == -1){
        cout<< "Symbol not found: "<<parameter<<"."<<endl;
        errorDetected = true;
        return;
      }
      if(!SymbolTable[currentSymbol].defined){
        RelocationDefinition newReloc;
        newReloc.addend = 0;
        newReloc.type = "R_X86_64_32";
        newReloc.offset = locationCounter;
        newReloc.symbolIndex = currentSymbol;
        SectionTable[currentSection].data[locationCounter]= 0; 
        SectionTable[currentSection].data[locationCounter+1]= 0; 
        SectionTable[currentSection].data[locationCounter+2]= 0; 
        SectionTable[currentSection].data[locationCounter+3]= 0;
        locationCounter+=4;
        RelocationTable.push_back(newReloc);
        
        return;
    }

    SectionTable[currentSection].data[locationCounter]= 0; 
    SectionTable[currentSection].data[locationCounter+1]= 0; 
    SectionTable[currentSection].data[locationCounter+2]= 0; 
    SectionTable[currentSection].data[locationCounter+3]= 0; 

    // symbolic constants have absolute value and therefore relocation is redundant
    if(SymbolTable[currentSymbol].section != AbsoluteSectionIndex){   
      RelocationDefinition newReloc;
      newReloc.addend = 0;
      newReloc.type = "R_X86_64_32";
      newReloc.offset = locationCounter;
      newReloc.symbolIndex = currentSymbol;
      if(SymbolTable[currentSymbol].global == false) {//locally symbols are not visible to linker
        newReloc.addend = SymbolTable[currentSymbol].value;
      }
    
      RelocationTable.push_back(newReloc);

      SectionTable[currentSection].data[locationCounter]= 0; 
      SectionTable[currentSection].data[locationCounter+1]= 0;  
      SectionTable[currentSection].data[locationCounter+2]= 0;  
      SectionTable[currentSection].data[locationCounter+3]= 0; 
    }

    SectionTable[currentSection].data[locationCounter]= SymbolTable[currentSymbol].value & 0xff; 
    SectionTable[currentSection].data[locationCounter+1]= (SymbolTable[currentSymbol].value>>8) & 0xff;  
    SectionTable[currentSection].data[locationCounter+2]= (SymbolTable[currentSymbol].value>>16) & 0xff;  
    SectionTable[currentSection].data[locationCounter+3]= (SymbolTable[currentSymbol].value>>24) & 0xff; 
    }
  }
  
  locationCounter+= 4;
}

void Assembler::processSkipDeclarationSecondPass(string literal){

  uint32_t bytes = getValue(literal);

  for(auto i = 0; i< bytes; i++){
    SectionTable[currentSection].data[locationCounter+i]= 0; 
  }

  locationCounter+=bytes;
}

void Assembler::processASCIIDeclarationSecondPass(string str){
  
  uint8_t stringSize = str.length();
  
  for(auto j = 1; j<stringSize-1; j++)
    SectionTable[currentSection].data[locationCounter+j - 1]= (uint8_t)str[j]; 
  
  locationCounter+=stringSize - 2;

  uint8_t pad = 4- locationCounter%4;
  if(pad != 4) locationCounter+=pad;

}

void Assembler::processInstructionSecondPass(string line){
  
  smatch atoms;
  uint8_t instructionBytes[4] ={ 0x0, 0x0, 0x0, 0x0};
  // b[0]|b[1]|b[2]|b[3]
  //no operand instructions
  if(regex_search(line, atoms, regHalt)){
    instructionBytes[0] = 0x00;
  }
  else if(regex_search(line, atoms, regInt)){
    instructionBytes[0] = 0x10;
  }
  else if(regex_search(line, atoms, regRet)){
    //pop PC
    instructionBytes[0] = 0x93;
    int8_t d = 0x04; 

    instructionBytes[1] = registerPC<<4;
    instructionBytes[1] |= registerSP;

    instructionBytes[2] = registerZero<<4;
    instructionBytes[2] |= 0;

    instructionBytes[3] = (d  & 0x00f0) >> 4;
    instructionBytes[3] |= d & 0x000f;
  }
  else if(regex_search(line, atoms, regIret)){
    //consists of two instructions

    // popPC
    instructionBytes[0] = 0x93;
    int8_t d = 0x04; 

    instructionBytes[1] = registerPC<<4;
    instructionBytes[1] |= registerSP;

    instructionBytes[2] = registerZero<<4;
    instructionBytes[2] |= 0;

    instructionBytes[3] = (d  & 0x00f0) >> 4;
    instructionBytes[3] |= d & 0x000f;

     
    SectionTable[currentSection].data[locationCounter]= instructionBytes[0]; 
    SectionTable[currentSection].data[locationCounter+1]= instructionBytes[1]; 
    SectionTable[currentSection].data[locationCounter+2]= instructionBytes[2]; 
    SectionTable[currentSection].data[locationCounter+3]= instructionBytes[3];
    
    locationCounter+=4;

    //reset
    instructionBytes[0] = 0x0;
    instructionBytes[1] = 0x0;
    instructionBytes[2] = 0x0;
    instructionBytes[3] = 0x0;

    //pop status
    instructionBytes[0] = 0x97;
    uint8_t csreg = 0x00;  // status register  
    d = 0x04; 

    instructionBytes[1] = csreg << 4;
    instructionBytes[1] |= registerSP;

    instructionBytes[2] = registerZero << 4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = (d  & 0x00f0) >> 4;
    instructionBytes[3] |= d & 0x000f;
  }
  //single operand isntructions
  else if(regex_search(line, atoms, regPop)){
    instructionBytes[0] = 0x93;
    uint8_t reg = fetchRegister(atoms.str(1));
    int8_t d = 0x04; 

    instructionBytes[1] = reg<<4;
    instructionBytes[1] |= registerSP;

    instructionBytes[2] = registerZero<<4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = (d  & 0x00f0) >> 4;
    instructionBytes[3] |= d & 0x000f;

  }
  else if(regex_search(line, atoms, regPush)){
    instructionBytes[0] = 0x81;
    string regS = atoms.str(1);
    uint8_t reg;
    if(regS == "%sp"){
      reg = 14;
    }
    else if(regS == "%pc"){
      reg = 15;
    }
    else{
      reg = getValue(regS.substr(2, regS.length()-2));
    }
    
    int16_t d = 0xfffc; // -4

    instructionBytes[1] = registerSP<<4;
    instructionBytes[1] |= 0x0;

    instructionBytes[2] = reg<<4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = d & 0x00f0;
    instructionBytes[3] |= d & 0x000f;

  }
  else if(regex_search(line, atoms, regNot)){
    instructionBytes[0] = 0x60;
    instructionBytes[1] = fetchRegister(atoms.str(1))<<4;
  }
  // arithmetic, logic, shift and swap operations
  else if(regex_search(line, atoms, regAdd)){
    instructionBytes[0] = 0x50;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(3));
    instructionBytes[2] = fetchRegister(atoms.str(1))<<4;
  }
  else if(regex_search(line, atoms, regSub)){
    instructionBytes[0] = 0x51;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(3));
    instructionBytes[2] = fetchRegister(atoms.str(1))<<4;
  }
  else if(regex_search(line, atoms, regMul)){
    instructionBytes[0] = 0x52;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(3));
    instructionBytes[2] = fetchRegister(atoms.str(1))<<4;
  }
  else if(regex_search(line, atoms, regDiv)){
    instructionBytes[0] = 0x53;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(3));
    instructionBytes[2] = fetchRegister(atoms.str(1))<<4;
  }
  else if(regex_search(line, atoms, regAnd)){
    instructionBytes[0] = 0x61;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(3));
    instructionBytes[2] = fetchRegister(atoms.str(1))<<4;
  }
  else if(regex_search(line, atoms, regOr)){
    instructionBytes[0] = 0x62;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(1));
    instructionBytes[2] = fetchRegister(atoms.str(3))<<4;
  }
  else if(regex_search(line, atoms, regXor)){
    instructionBytes[0] = 0x63;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(1));
    instructionBytes[2] = fetchRegister(atoms.str(3))<<4;
  }
  else if(regex_search(line, atoms, regShl)){
    instructionBytes[0] = 0x70;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(1));
    instructionBytes[2] = fetchRegister(atoms.str(3))<<4;
  }
  else if(regex_search(line, atoms, regShr)){
    instructionBytes[0] = 0x71;
    instructionBytes[1] = fetchRegister(atoms.str(3))<<4;
    instructionBytes[1] |= fetchRegister(atoms.str(1));
    instructionBytes[2] = fetchRegister(atoms.str(3))<<4;
  }
  else if(regex_search(line, atoms, regXchg)){
    instructionBytes[0] = 0x40;
    instructionBytes[1] = fetchRegister(atoms.str(1));
    instructionBytes[2] = fetchRegister(atoms.str(3))<<4;
  }
  //jump instructions
  else if(regex_search(line, atoms, regJmp)){
    string jumpAddr = atoms.str(1);
    int16_t d = setAbsoluteJump(jumpAddr);

    instructionBytes[0] = 0x38;

    instructionBytes[1] = registerPC<<4;
    instructionBytes[1] |= registerZero;
    
    instructionBytes[2] = registerZero<<4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = (d  & 0x00f0);
    instructionBytes[3] |= d & 0x000f;
  }
  else if(regex_search(line, atoms, regCall)){
    string jumpAddr = atoms.str(1);
    int16_t d = setAbsoluteJump(jumpAddr);
    instructionBytes[0] = 0x21;

    instructionBytes[1] = registerPC<<4;
    instructionBytes[1] |= registerZero;
    
    instructionBytes[2] = registerZero<<4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = (d  & 0x00f0);
    instructionBytes[3] |= d & 0x000f;
  }
  else if(regex_search(line, atoms, regBeq)){
    string jumpAddr = atoms.str(5);
  
    uint8_t reg1 = fetchRegister(atoms.str(1));
    uint8_t reg2 = fetchRegister(atoms.str(3));
    int16_t d = setAbsoluteJump(jumpAddr);
    
    instructionBytes[0] = 0x39;

    instructionBytes[1] = registerPC<<4;
    instructionBytes[1] |= reg1;
    
    instructionBytes[2] = reg2<<4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = (d  & 0x00f0);
    instructionBytes[3] |= d & 0x000f; 
  }
  else if(regex_search(line, atoms, regBne)){
    string jumpAddr = atoms.str(5);
    uint8_t reg1 = fetchRegister(atoms.str(1));
    uint8_t reg2 = fetchRegister(atoms.str(3));
    int16_t d = setAbsoluteJump(jumpAddr);

    instructionBytes[0] = 0x3a;

    instructionBytes[1] = registerPC<<4;
    instructionBytes[1] |= reg1;
    
    instructionBytes[2] = reg2<<4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = (d  & 0x00f0);
    instructionBytes[3] |= d & 0x000f; 
  }
  else if(regex_search(line, atoms, regBgt)){
    string jumpAddr = atoms.str(5);
    uint8_t reg1 = fetchRegister(atoms.str(1));
    uint8_t reg2 = fetchRegister(atoms.str(3));
    int16_t d = setAbsoluteJump(jumpAddr);
    
    instructionBytes[0] = 0x3b;

    instructionBytes[1] = registerPC<<4;
    instructionBytes[2] |= (d >> 8) & 0x0f;

    instructionBytes[3] = (d  & 0x00f0);
    instructionBytes[3] |= d & 0x000f;  
  }
  else if(regex_search(line, atoms, regCsrrd)){
    uint8_t reg = fetchRegister(atoms.str(3));
    char csrIndicator = atoms.str(1)[1];
    uint8_t csr = 0;

    switch(csrIndicator){
      case 's': csr =0x0; break;
      case 'h': csr =0x1; break;
      case 'c': csr =0x2; break;
    }
    
    instructionBytes[0] = 0x90;

    instructionBytes[1] = reg <<4;
    instructionBytes[1] |= csr;
  }
  else if(regex_search(line, atoms, regCsrwr)){
    
    uint8_t reg = fetchRegister(atoms.str(1));
    char csrIndicator = atoms.str(3)[1];
    uint8_t csr = 0;

    switch(csrIndicator){
      case 's': csr =0x0; break;
      case 'h': csr =0x1; break;
      case 'c': csr =0x2; break;
    }
    
    instructionBytes[0] = 0x94;

    instructionBytes[1] = csr << 4;
    instructionBytes[1] |= reg;
  }
  else if(regex_search(line, atoms, regLd)){ 
    string op1 = atoms.str(1);
    string regS = atoms.str(atoms.size()-1);
    uint8_t dest;

    if(regS == "sp"){
      dest = 14;
    }
    else if(regS == "pc"){
      dest = 15;
    }
    else{
      dest = getValue(regS.substr(1, regS.length()-1));
    }

    smatch subatoms;
    if(regex_search(op1, subatoms, regRegInd)){
      string subOp1 = subatoms.str(1);
      uint8_t src = fetchRegister(subatoms.str(1));
      string subOp2 = subatoms.str(5);
      int32_t value;

      if(subOp2.size() == 0){
        value = 0x0;
      }
      else if(subOp2[0]>='0' && subOp2[0]<= '9'){
        value = getValue(subOp2);  
      }
      else {
        for(auto i= 0; i< SymbolTable.size(); i++){
          if(SymbolTable[i].label == subOp2){
            if(SymbolTable[i].section != AbsoluteSectionIndex){
              errorDetected = true;
              cout << "Value of a loaded symbol must be determined."<< endl;
              return;
            }          
            value = SymbolTable[i].value;
          }
        }
      }
      if((value & 0xf000) != 0){
        errorDetected = true;
        cout << "Loaded value out of bounds."<< endl;
        return;
      }

      instructionBytes[0] = 0x92;

      instructionBytes[1] = dest <<4;
      instructionBytes[1] |= src;

      instructionBytes[2] = 0x0 << 4;
      instructionBytes[2] |= (value >> 8) & 0x0f;

      instructionBytes[3] = (value  & 0x00f0) >> 4;
      instructionBytes[3] |= value & 0x000f;
    }
    // loads immediate value
    else if(regex_search(op1, subatoms, regImmed)){
      string op = subatoms.str(1);
      int32_t value;
      int32_t symIndex = -1; 
      
      if(op[0]>='0' && op[0]<= '9'){
        value = getValue(op);
      }
      else {
        for(auto i= 0; i< SymbolTable.size(); i++){
          if(SymbolTable[i].label == op){
            value = SymbolTable[i].value;
            symIndex = i;
            break;
          }
        }
      }

      int32_t displacement = 0;
      displacement =  findSymbolLocationInLiteralPool(op) - locationCounter - 4;

      if(symIndex != -1){  
        if(SymbolTable[symIndex].section != AbsoluteSectionIndex){
          generateRelocation(symIndex, displacement + locationCounter + 4, currentSection);
        }
      }

      instructionBytes[0] = 0x92;

      instructionBytes[1] = dest <<4;
      instructionBytes[1] |= registerPC;

      instructionBytes[2] = 0x0 << 4;
      instructionBytes[2] |= (displacement >>8)& 0x000f;

      instructionBytes[3] = displacement  & 0x00f0;
      instructionBytes[3] |= displacement & 0x000f; 
    }
    // in order to load from address, mentioned address must first be loaded into a register
    else if(regex_search(op1, subatoms, regMem)){
      //therefore, this consists of immediate and then register indirect load 
      string op = subatoms.str(1);
      uint32_t value;
      int32_t symIndex = -1;

      if(op[0]>='0' && op[0]<= '9'){
        value = getValue(op);
      }
      else {
        for(auto i= 0; i< SymbolTable.size(); i++){
          if(SymbolTable[i].label == op){
            value = SymbolTable[i].value;
            symIndex = i;
            break;
          }
        }
      }

      int32_t displacement;
      displacement = findSymbolLocationInLiteralPool(op) - locationCounter - 4;

      if(symIndex != -1){
        if(SymbolTable[symIndex].section != AbsoluteSectionIndex)
          generateRelocation(symIndex, displacement + locationCounter +4, currentSection); 
      }  

      instructionBytes[0] = 0x92;

      instructionBytes[1] = dest <<4;
      instructionBytes[1] |= registerPC;

      instructionBytes[2] = 0x0 << 4;
      instructionBytes[2] |= (displacement >> 8) & 0x0f;

      instructionBytes[3] = displacement  & 0x00f0;
      instructionBytes[3] |= displacement & 0x000f; 

      stringstream ss;
      ss<<hex<<+instructionBytes[0]<<+instructionBytes[1]<<+instructionBytes[2]<<+instructionBytes[3];
      SectionTable[currentSection].data[locationCounter]= instructionBytes[0]; 
      SectionTable[currentSection].data[locationCounter+1]= instructionBytes[1]; 
      SectionTable[currentSection].data[locationCounter+2]= instructionBytes[2]; 
      SectionTable[currentSection].data[locationCounter+3]= instructionBytes[3];
    
      locationCounter+=4;

      //reset
      instructionBytes[0] = 0x0;
      instructionBytes[1] = 0x0;
      instructionBytes[2] = 0x0;
      instructionBytes[3] = 0x0;

      instructionBytes[0] = 0x92;

      instructionBytes[1] = dest <<4;
      instructionBytes[1] |= dest;

      instructionBytes[2] = 0x0 << 4;
      instructionBytes[2] |= 0x0;
    }    
    // register 
    else{
      instructionBytes[0] = 0x91;

      uint8_t src = getValue(op1.substr(1, op1.size()-1));

      instructionBytes[1] = dest <<4;
      instructionBytes[1] |= src;

      instructionBytes[2] = 0x0 << 4;
      instructionBytes[2] |= 0x0;
    }

  }
  // store
  else if (regex_search(line, atoms, regSt)){
    string op1 = atoms.str(1);
    string op2 = atoms.str(3);
    uint8_t src = getValue(op1.substr(2, op1.size() - 2));

    smatch subatoms;
    // register indirect
    if (regex_search(op2, subatoms, regRegInd)){   
      string subOp1 = subatoms.str(1);   
      uint8_t dest = getValue(subOp1.substr(2, subOp1.length() - 1));
      string subOp2 = subatoms.str(3);      
      int32_t value;

      if (subOp2.length() == 0){
        value = 0x0;
      }
      else if (subOp2[0] >= '0' && subOp2[0] <= '9'){
        value = getValue(subOp2);
      }
      else{
        for (auto i = 0; i < SymbolTable.size(); i++){
          if (SymbolTable[i].label == subOp2){
            if (SymbolTable[i].section != AbsoluteSectionIndex){
              errorDetected = true;
              cout << "Value of a loaded symbol must be determined." << endl;
              return;
            }
            value = SymbolTable[i].value;
          }
        }
      }
      if ((value & 0x0fff) != 0)
      {
        errorDetected = true;
        cout << "Loaded value out of bounds." << endl;
        return;
      }

      instructionBytes[0] = 0x80;

      instructionBytes[1] = dest << 4;
      instructionBytes[1] |= 0x0;

      instructionBytes[2] = src << 4;
      instructionBytes[2] |= (value >> 8) & 0x0f;

      instructionBytes[3] = (value & 0x00f0) >> 4;
      instructionBytes[3] |= value & 0x000f;
    }
    else if (regex_search(op2, subatoms, regMem)){
      string op = subatoms.str(1);
      uint32_t value;
      int32_t symIndex = -1;

      if (op[0] >= '0' && op[0] <= '9'){
        value = getValue(op);
      }
      else{
        for (auto i = 0; i < SymbolTable.size(); i++){
          if (SymbolTable[i].label == op){
            value = SymbolTable[i].value;
            symIndex = i;
            break;
          }
        }
      }

      uint16_t displacement = findSymbolLocationInLiteralPool(op) - locationCounter - 4;
      
      if (symIndex != -1){
        if (SymbolTable[symIndex].section != AbsoluteSectionIndex){
          generateRelocation(symIndex, displacement + locationCounter + 4, currentSection);
        }
      }
      
      instructionBytes[0] = 0x82;

      instructionBytes[1] = registerPC << 4;
      instructionBytes[1] |= 0x0;

      instructionBytes[2] = src << 4;
      instructionBytes[2] |= (displacement >> 8) & 0x0f;

      instructionBytes[3] = displacement & 0x00f0;
      instructionBytes[3] |= displacement & 0x000f;
    }
  }
  else{
    cout<< "Unrecognized command."<<endl;

    errorDetected = true;
    return;
  }

  stringstream ss;
  ss<<hex<<+instructionBytes[0]<<+instructionBytes[1]<<+instructionBytes[2]<<+instructionBytes[3];

  SectionTable[currentSection].data[locationCounter]= instructionBytes[0]; 
  SectionTable[currentSection].data[locationCounter+1]= instructionBytes[1]; 
  SectionTable[currentSection].data[locationCounter+2]= instructionBytes[2]; 
  SectionTable[currentSection].data[locationCounter+3]= instructionBytes[3];
  locationCounter+=4;

}

void Assembler::processEquDeclarationSecondPass(string label, string value){

   for (auto i = 0; i< SymbolTable.size(); i++){
    if (SymbolTable[i].label == label){
      SymbolTable[i].value = calculateExpression(value);
      break;  
    }
  }
}

int32_t Assembler::setAbsoluteJump(string addr){
  
  int32_t displacement = findSymbolLocationInLiteralPool(addr) - locationCounter - 4;
  
  int32_t currSym = -1;
  for(auto i =0; i< SymbolTable.size(); i++){
    if (SymbolTable[i].label == addr) currSym = i;
  }
  // if address of the jump happens to be a symbol, this calls for a Relocation Tbale entry
  if(currSym != -1){
    // and symbol is not from the Absolute Section
    if(SymbolTable[currSym].section != AbsoluteSectionIndex){   
        for(auto i = 0; i< RelocationTable.size(); i++){
          if((currSym == RelocationTable[i].symbolIndex && RelocationTable[i].section == currentSection)
            || (SymbolTable[currSym].section == currentSection 
            && SymbolTable[currSym].value == RelocationTable[i].addend))
            
            return displacement;
        }
        RelocationDefinition newReloc;
        newReloc.addend = 0;
        newReloc.type = "R_X86_64_32";
        newReloc.offset = findSymbolLocationInLiteralPool(addr);
        newReloc.symbolIndex = currSym;
        newReloc.section = currentSection;

        if(SymbolTable[currSym].global == false){ //locally symbols are not visible to linker
          string symName = SectionTable[SymbolTable[currSym].section].name;
          for (auto i = 0; i< SymbolTable.size(); i++)
              if (SymbolTable[i].label == symName){
                newReloc.symbolIndex = i;
                break;
              }

              newReloc.addend = SymbolTable[currSym].value;
      }  

        RelocationTable.push_back(newReloc);
    }
         
  }   
      return displacement;
   
}

uint32_t Assembler::findSymbolLocationInLiteralPool(string symbol){

  for(auto i = 0; i< SectionTable[currentSection].literalPool.size(); i++){
    if (SectionTable[currentSection].literalPool[i].symbol == symbol){ 
      return SectionTable[currentSection].literalPool[i].location;
    }
  }
  for(auto i = 0; i <SymbolTable.size(); i++){
    if(SymbolTable[i].label == symbol){
      int32_t val = 0;
      if (SymbolTable[i].section == AbsoluteSectionIndex) val = SymbolTable[i].value;
      uint32_t newLoc = SectionTable[currentSection].literalPool.size()*4 + SectionTable[currentSection].length;
      SectionTable[currentSection].literalPool.push_back({symbol, val, 4, newLoc});
      return newLoc;
    }
  }
  cout<<"Jump address symbol "<< symbol<<" undeclared."<<endl;
  errorDetected = true;
 
  return 0;
}

void Assembler::generateRelocation(uint32_t symbolIndex , uint32_t entry, uint16_t sectionIndex){

  for(auto i =0; i< RelocationTable.size(); i++){
    if (RelocationTable[i].symbolIndex == symbolIndex && 
        RelocationTable[i].section == sectionIndex ) 
          return;
  }

  RelocationDefinition newReloc;
  newReloc.addend = 0;
  newReloc.type = "R_X86_64_32";
  newReloc.offset = entry;
  newReloc.symbolIndex = symbolIndex;
  newReloc.section = sectionIndex;

  if(SymbolTable[symbolIndex].global == false) {//locally symbols are not visible to linker   
    string symName = SectionTable[sectionIndex].name;
    for (auto i = 0; i< SymbolTable.size(); i++)
      if (SymbolTable[i].label == symName){
        newReloc.symbolIndex = i;
        break;
      }

    newReloc.addend = SymbolTable[symbolIndex].value;
  }

  RelocationTable.push_back(newReloc);
}

uint8_t Assembler::fetchRegister(string s){

  uint8_t reg;
    if(s == "%sp"){
      reg = 14;
    }
    else if(s == "%pc"){
      reg = 15;
    }
    else{
      reg= getValue(s.substr(2, s.length()-2));
    }

    return reg;
}

void Assembler::unloadLiteralsPool()
{
  for(auto i =0; i< SectionTable.size(); i++){
    if (SectionTable[i].literalPool.size() != 0){
      for(LiteralsTable pool: SectionTable[i].literalPool){
        SectionTable[i].data[pool.location] = pool.value & 0xff;
        SectionTable[i].data[pool.location+1] = (pool.value>>8) & 0xff;
        SectionTable[i].data[pool.location+2] = (pool.value>>16) & 0xff;
        SectionTable[i].data[pool.location+3] = (pool.value>>24) & 0xff;
      }
      SectionTable[i].length = 
        SectionTable[i].literalPool[SectionTable[i].literalPool.size()-1].location + 4;
    }
  }
}

void Assembler::processCode(){
  //opening input file
  string line;

  ifstream inputFileReader;
  inputFileReader.open(inputPath);
  if (inputFileReader.is_open() == false){
    cout<<"Could not open input file."<<endl;
    return;
  }

  while (getline(inputFileReader, line)){
    line = regex_replace(line, regEmptySpace, " ");
    line = regex_replace(line, regTabs, "");      
    
    inputCode.push_back(line);   
  }
}

void Assembler::generateOutput(){

  string outputTXT = outputPath.substr(0, outputPath.length()-1);
  outputTXT+="txt";
  ofstream ObjTXT(outputTXT);

  ObjTXT<< "Object file content:" << endl;
  ObjTXT<< endl;
  ObjTXT<< endl;

  ObjTXT << "Section table:" << endl;
  ObjTXT << "Id\tName\t\tSize" << endl;
  
  for (auto i = 0; i< SectionTable.size(); i++){
    ObjTXT<< i << "\t"<< SectionTable[i].name
      << "\t"<< hex<< setfill('0')<< setw(4)<< SectionTable[i].length<< endl;
  }

  ObjTXT<< dec;
  ObjTXT<< endl;
  ObjTXT<< endl;

  ObjTXT << "Symbol table:" << endl;
  ObjTXT << "Value\tType\tSection\t\tName\t\tId" << endl;
  for (auto i = 0; i< SymbolTable.size(); i++){
    ObjTXT<< hex<< setfill('0')<< setw(4)<< SymbolTable[i].value<< "\t";   
      if(SymbolTable[i].global)
        ObjTXT<< "g\t";
      else 
        ObjTXT<< "l\t";

      if(SymbolTable[i].defined)
        ObjTXT<< "d\t";
      else
        ObjTXT<< "u\t"; 
           
      if (SymbolTable[i].external)
        ObjTXT<< "e\t";
      
      ObjTXT<< SectionTable[SymbolTable[i].section].name 
        <<"\t"<< SymbolTable[i].label<< "\t"<< hex<< setfill('0')<< setw(4)<<i<< endl;
  }

  ObjTXT << dec;
  ObjTXT << endl;
  ObjTXT << endl;

  for (auto i= 0; i< SectionTable.size(); i++){
       
    ObjTXT<< "Relocation data<"<< SectionTable[i].name<< ">:"<< endl;
    ObjTXT << "Offset\tType\tSymbol\tAddend" << endl;
    for (RelocationDefinition reloc : RelocationTable){
      if (reloc.section == i){
        ObjTXT<< hex<< setfill('0')<< setw(8) 
          << reloc.offset<< "\t" <<reloc.type<< "\t" << SymbolTable[reloc.symbolIndex].label
            << "\t"<<reloc.addend<< endl;
      }
    }
  }  

  ObjTXT<< dec<< endl; 
  for (auto i= 0; i< SectionTable.size(); i++){
    ObjTXT<< "Section data <"<< SectionTable[i].name<< ">:"<< endl;
    uint32_t words = SectionTable[i].length - SectionTable[i].length % 4;
    for (auto j = 0; j< words; j+=4){
      ObjTXT<< hex<<setw(8) << setfill('0') << (SectionTable[i].base+j);
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +SectionTable[i].data[SectionTable[i].base+j];
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +SectionTable[i].data[SectionTable[i].base+j+1];
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +SectionTable[i].data[SectionTable[i].base+j+2];
      ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +SectionTable[i].data[SectionTable[i].base+j+3];
      ObjTXT<< endl;
    }
    if(words!= SectionTable[i].length){
      ObjTXT<< hex<<setw(8) << setfill('0') << (SectionTable[i].base+words);
      for(auto k = 0; k<SectionTable[i].length -words; k++  )
        ObjTXT<< hex<<"\t"<<setw(2) << setfill('0')<< +SectionTable[i].data[SectionTable[i].base+k+words];
    }
     
    ObjTXT << dec; 
    ObjTXT<< endl;
  }
  ObjTXT << endl;

  ObjTXT.close();
  cout<<"Text file generated in "<<outputTXT<<endl;
  cout<<endl;
}

void Assembler::generateBinary(){
  
  string outputBin = this->outputPath;
  cout<<"Binary generated in "<<outputBin<<endl;

  ofstream outputFile(outputBin, ios::out | ios::binary);
  
  // Symbols
  uint32_t symbols = SymbolTable.size();
  outputFile.write((char *)&symbols, sizeof(symbols));
  for (auto i = 0; i< SymbolTable.size();i++){     
    outputFile.write((char *)(&i), sizeof(i));
    uint32_t labelLen = SymbolTable[i].label.length();

    outputFile.write((char *)(&labelLen), sizeof(labelLen));
    outputFile.write(SymbolTable[i].label.c_str(), labelLen);

    outputFile.write((char *)(&SymbolTable[i].section), sizeof(SymbolTable[i].section));
    outputFile.write((char *)(&SymbolTable[i].defined), sizeof(SymbolTable[i].defined));
    outputFile.write((char *)(&SymbolTable[i].external), sizeof(SymbolTable[i].external));
    outputFile.write((char *)(&SymbolTable[i].global), sizeof(SymbolTable[i].global));
    outputFile.write((char *)(&SymbolTable[i].value), sizeof(SymbolTable[i].value));   
       
  }
  // Sections
  uint32_t sections = SectionTable.size();
  outputFile.write((char *)&sections, sizeof(sections));

  for (auto i = 0; i< SectionTable.size(); i++){
    outputFile.write((char *)(&i), sizeof(i));

    uint32_t labelLen = SectionTable[i].name.length();
    outputFile.write((char *)(&labelLen), sizeof(labelLen));
    outputFile.write(SectionTable[i].name.c_str(), labelLen);

    outputFile.write((char *)(&SectionTable[i].base), sizeof(SectionTable[i].base));
    outputFile.write((char *)(&SectionTable[i].length), sizeof(SectionTable[i].length));
        
    // Data output
    uint32_t dataSize  = SectionTable[i].length;
    outputFile.write((char *)&dataSize, sizeof(dataSize));
    for (auto j = 0; j< SectionTable[i].length; j++){
      uint32_t adr0 = SectionTable[i].base + j;
      outputFile.write((char *)(&adr0), sizeof(adr0));
      int8_t value = SectionTable[i].data[SectionTable[i].base+j];
      
      outputFile.write((char *)(&value), sizeof(value));
    }         
  }      
    // Relocations 
  uint32_t relocs = RelocationTable.size();
  
  outputFile.write((char *)&relocs, sizeof(relocs));
  for (auto i = 0; i< RelocationTable.size();i++){  
    outputFile.write((char *)(&i), sizeof(i));   
    outputFile.write((char *)(&RelocationTable[i].addend), sizeof(RelocationTable[i].addend));
    outputFile.write((char *)(&RelocationTable[i].offset), sizeof(RelocationTable[i].offset));
    outputFile.write((char *)(&RelocationTable[i].section), sizeof(RelocationTable[i].section));
    outputFile.write((char *)(&RelocationTable[i].symbolIndex), sizeof(RelocationTable[i].symbolIndex));      

  }
    outputFile.close();
}

int main(int argc, const char *argv[]){

  string inputFile;
  string outputFile;

  if(argc == 1){
    cout<<"No input file to assemble."<<endl;
    return -2;
  }
  if (strcmp(argv[1], "-o")==0){
    if(argc != 4){
      cout<<"Inappropriate number of arguments for assembly."<<endl;
      return -2;
    }
    
    inputFile = argv[3];
    outputFile = argv[2];
  }

  else{
   inputFile = argv[1];    
  }

  string l = "ld value1, %pc";
  smatch atom;

  Assembler assembler(inputFile, outputFile);

  assembler.assemble();

  return 0;
}