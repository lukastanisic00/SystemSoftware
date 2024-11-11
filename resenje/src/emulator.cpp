#include "../inc/emulator.hpp"


Emulator::Emulator(string input):inputFile(input),registers(GPR_NUMBER, 0x0),
  csrRegisters(CSR_NUMBER,0x0)
{
  errorDetected = false;
  running = false;
  ldMem = false;
  iret = false;
  terminalError ="";
  memory[TIM_CFG] = 0;
}


bool Emulator::processInput(){

  if(inputFile.substr(inputFile.length()-4, 4) != ".hex"){
    errorDetected = true;
    cout<< "Inappropriate format for input file."<<endl;

    return false;
  }
  ifstream inputFileReader(inputFile, ios::binary);
  if (inputFileReader.fail()){
    cout<<"Input file "<< inputFile<< " cannot be opened."<<endl;
    errorDetected = true; 
    return false;
  }

  uint32_t segments = 0;
  inputFileReader.read((char *)&segments, sizeof(segments));
  
  for (auto i= 0; i < segments; i++){
    Segment readSegment;

    inputFileReader.read((char *)&readSegment.virtualAddress, sizeof(readSegment.virtualAddress));
    inputFileReader.read((char *)&readSegment.size, sizeof(readSegment.size));
   
    for (auto j= 0; j< readSegment.size; j++){
      int8_t val;
      inputFileReader.read((char *)(&val), sizeof(val));
      readSegment.data.push_back(val);
    }

  this->segments.push_back(readSegment);
  }

  inputFileReader.close();
  return true;
}

bool Emulator::loadData(){

  for(auto i = 0; i < segments.size(); i++){
    if (segments[i].virtualAddress + segments.size() >= MEMORY_MAPPED_REGISTERS_ADDRESS){
        errorDetected = true;
        cout<< "Segment to load into inaccessable area."<< endl;
        return false;
    }
    for(auto j = 0; j< segments[i].data.size(); j++){
      memory[segments[i].virtualAddress + j] = segments[i].data[j];
    }
  }

  return true;
}

void Emulator::emulate(){

  regPC= START;
  regSP = MEMORY_MAPPED_REGISTERS_ADDRESS;

  csrStatus = 0x0;
  resetTimer(0); 
  if (configureTerminal() == false){
    cout<< "Error configuring terminal. Emulation not initialized:"<<terminalError<<endl;
    return;
  }

  running = true;

  while (running){
    currentPC = regPC;
    if(fetchAndDecodeInstruction())
      executeInstruction();

    if(!running) return; // hard fault, immediate exit

    timerTick();
    readTerminal();
    handleInterrupt();
  }
  resetTerminal();
}

int8_t Emulator::readMem(uint32_t address){

  if(memory.find(address) == memory.end()){
    cout<<"Access out of allocated space at " << hex << address << ".";
    handleFault();
    return 0;
  }

  return memory[address] & 0xff;
}

int32_t Emulator::readMemWord(uint32_t address){

  if(address+3 >= MEMORY_SIZE) return 0x0;
  
  uint32_t byte0 = (uint32_t)readMem(address);
  uint32_t byte1 = (uint32_t)readMem(address+ 1);
  uint32_t byte2 = (uint32_t)readMem(address+ 2);
  uint32_t byte3 = (uint32_t)readMem(address+ 3);
  
  return (int32_t)((byte0 & 0xff)  | (byte1<<8 & 0xff00) | (byte2<<16 & 0xff0000) | (byte3<<24));
}

void Emulator::writeMem(int8_t value, uint32_t address){

  memory[address] = value;
  if(address == TERM_OUT) cout<<(char)value<<flush;  // if there is data written inside 
  // memory mapped terminal output register, display it
  if(address == TIMER_CFG) resetTimer(value); // configure timer immediately resets it with a newly
  //set period
}

void Emulator::writeMemWord(int32_t value, uint32_t address){

  int8_t byte0 = value & 0xff; 
  int8_t byte1 = (value>>8) & 0xff;
  int8_t byte2 = (value>>16) & 0xff;
  int8_t byte3 = (value>>24) & 0xff;

  if(address >= MEMORY_SIZE - 3){
    cout<<"Out of bounds write at "<< regPC <<"."<< endl;
    return; // out of bounds write
  }
    
  writeMem(byte0, address);
  writeMem(byte1, address+ 1);
  writeMem(byte2, address+ 2);
  writeMem(byte3, address+ 3);
}

void Emulator::push(int32_t value){

  regSP -= 4;

  if((uint32_t) regSP <= 0){
    errorDetected = true;
    cout<<"Stack limit breached at "<<hex<< regPC<< " on address " << regSP << "."<< endl;
    handleFault();
    return;
  }

  writeMemWord(value, regSP);
}

int32_t Emulator::pop(){

  int32_t value = readMemWord(regSP);
  regSP += 4;

  return value;
}

bool Emulator::fetchAndDecodeInstruction(){ // check if the operands are appropriately set and proceed

  op = readMem(regPC);
  ++regPC;

  uint8_t Byte1 = readMem(regPC); // readMem(regPC++) is an undefined behavior 
  //that could cause issues
  ++regPC; 

  regA = (Byte1>>4) & 0xf;
  regB = Byte1 & 0xf;

  uint8_t Byte2 = readMem(regPC);
  ++regPC; 

  regC = (Byte2>>4) & 0xf;
  displacement = Byte2 & 0xf;
  displacement<<=8;

  uint8_t Byte3 = readMem(regPC);
  ++regPC; 
  displacement|=Byte3;

  if((Byte2 & 0x08) != 0x00)
    displacement |= 0xf000;

  switch (op){
    case HALT:case INT:{
      //halt
      if(Byte1 != 0x0 || Byte2 != 0x0 || Byte3 != 0x0){
        cout<<"Illegal operand(s) in operation at "<<regPC<<"."<< endl;
        handleFault();
        return false;    
      }
      return true;
    }
    case CALL:{
      // CALL
      if((regC != 0x0)||(regA != pc) || (regB != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;
    }
    case BEQ: case BNE: case BGT: case JMP:{
      if((regC > 15) ||(regA != pc) || (regB > 15)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true; // all operands should be set
    }
    case PUSH:{
      if((regB != 0) || (regA != sp) || (regC > 15) || (displacement != -4)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;
    }
    case POP:{ // neccessary restriction to ensure that load/store call is indeed PUSH/POP
      if((regB != sp) ||(regA > 15) || (regC != 0)||(displacement != 4)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      if(regA == pc){
        if((readMem(regPC) & 0xff)==  0x97){ // pop csr
          iret = true;
          
          regPC +=4; // instruction is processed
          // since this is the only way this instruction is called it is safe to assume that the operands,
          // have expected values
        }
      }
      return true;
    }
    case ADD:case SUB:case DIV:case MUL: case AND: case OR: case XOR: case SHL: case SHR:{
      if((regC > 15) || (regA > 15) || (regB > 15) ||(displacement != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;     
    }
    case XCHG:{
      if((regC > 15) || (regA != 0) || (regB > 15) ||(displacement != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;
    }
    case NOT:{
      if((regC != 0) || (regA > 15) || (regB > 15) ||(displacement != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        return false;
      }
      return true;      
    }
    case LD_REG_IND: {
      if((regC > 15) || (regA > 15) || (regB > 15)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        handleFault();
        return false;
      }
      if((regB == pc) && (regC == 0) && (readMem(regPC) == 0x92) && ((readMem(regPC)& 0xf) == regA)){
        ldMem = true;
        regPC+=4;
        // in a similar fashion to POP case, we identify the expected next instruction
        // since the previous content of destination register is overwritten it's safe
        // to assume that instructions in this order are called for this very purpose
      }
      return true;
    }
    case LD_REG:{
      if((regC != 0) || (regA > 15) || (regB > 15)||(displacement != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;   
    }
    case ST_REG_MEM:{
      if((regC > 15) || (regA > 15) || (regB !=0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;
    }
    case ST_REG_IND:{
      if((regC > 15) || (regA > 15) || (regB != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;
    }
    case CSRRD:{
      if((regC != 0) || (regA > 3) || (regB > 3) ||(displacement != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;      
    }
    case CSRWR:{
      if((regC != 0) || (regA > 3) || (regB > 3) ||(displacement != 0)){
        cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
      }
      return true;
    }
    default:{
      cout<<"Illegal operand(s) in operation at "<<hex<<regPC<<"."<< endl;
        cout<<dec;
      handleFault();
      return false;
    }
    }
}

bool Emulator::executeInstruction(){
  
  switch (op){
    case HALT:{
      running = false;
      return true;
    }
    case INT:{
      // interruption of this kind is synchronously called
      // status<=status&(~0x1); pc<=handle;
      csrCause = software_interrupt;
      processSubroutine();
      return true;
    }
    case CALL:{
      // CALL
      push(regPC);
      regPC = readMemWord(regPC + displacement);
      return true;    
    }
    case BEQ:{
      if(registers[regB] == registers[regC])
        regPC = readMemWord(registers[regA] + displacement);
      return true;  
    } 
    case BNE:{
      if(registers[regB] != registers[regC])
        regPC = readMemWord(registers[regA] + displacement);
      return true; 
    } 
    case BGT:{
      if(registers[regB] > registers[regC])
        regPC = readMemWord(registers[regA] + displacement);
      return true;  
    } 
    case JMP:{
      regPC = readMemWord(registers[regA] + displacement);
      return true;
    }
    case PUSH:{
      registers[regA]+= displacement;
      writeMemWord(registers[regC], registers[regA]);
      return true;
    } 
    case POP:{ 
      registers[regA] = readMemWord(registers[regB]);
      registers[regB]+= displacement;
      if(iret == true){
        csrStatus = readMemWord(registers[regB]);
        registers[regB]+= displacement;
        iret = false;
        csrCause = 0;
      }
      return true;
    }
    case ADD:{
      registers[regA] = registers[regB] + registers[regC];
      return true;
    }
    case SUB:{
      registers[regA] = registers[regB] - registers[regC];
      return true;
    }
    case MUL:{
      registers[regA] = registers[regB] * registers[regC];
      return true;
    } 
    case DIV:{
      if(registers[regC] == 0){
        cout<<"Illegal zero divisor at "<< hex<<to_string(regPC) << "." <<endl;
        cout<<dec;
        handleFault();
        return false;
      }
      registers[regA] = registers[regB] / registers[regC];
      return true;
    }
    case AND:{
      registers[regA] = registers[regB] & registers[regC];
      return true;
    } 
    case OR:{
      registers[regA] = registers[regB] | registers[regC];
      return true;
    } 
    case XOR:{
      registers[regA] = registers[regB] ^ registers[regC];
      return true;
    }
    case SHL:{
      registers[regA] = registers[regB] << registers[regC];
      return true;
    }
    case SHR:{
      registers[regA] = registers[regB] >> registers[regC];
      return true;
    } 
    case XCHG:{
      int32_t temp = registers[regB];
      registers[regB] = registers[regC];
      registers[regC] = temp;
      return true;
    }
    case NOT:{
      registers[regA] = ~registers[regB];
      return true;         
    }
    case LD_REG_IND:{ // also for immediate and register-memory
      registers[regA] = readMemWord(registers[regB]+ registers[regC] + displacement);
      if(ldMem){
        ldMem = false;
        registers[regA] = readMemWord(registers[regA]);
      }
      return true;    
    }
    case LD_REG:{
      registers[regA] =registers[regB] + displacement;
      return true;       
    }
    case ST_REG_MEM:{
      uint32_t address = readMemWord(registers[regA]+ registers[regB]+ displacement);
      writeMemWord(registers[regC], address);
      return true;      
    }
    case ST_REG_IND:{
      writeMemWord(registers[regC], registers[regA]+registers[regB]+displacement);
      return true;
    }
    case CSRRD:{ 
      registers[regA] = csrRegisters[regB]; 
      return true;   
    }
    case CSRWR:{
      csrRegisters[regA] = registers[regB];
      return true;
    }
    default:
        cout<< "Illegal operation code at "<< hex<<to_string(regPC)<<"."<< endl;
        cout<<dec;
        handleFault();
        return false;
    }
}

void Emulator::setFlag(uint8_t flag){

  csrStatus |= flag;
}

void Emulator::resetFlag(uint8_t flag){

  csrStatus &= ~flag;
}

bool Emulator::getFlag(uint8_t flag){

  return csrStatus & flag;
}


void Emulator::handleFault(){

  csrCause = fault;
  handleInterrupt();
}

void Emulator::setInterupt(uint8_t interrupt){
  // previous interrupt is overwritten
  if(interrupt < 4){
    csrCause = interrupt;
  } 
  
}

void Emulator::handleInterrupt(){ 

  if(csrCause == fault){
    csrCause = 0; // clears interrupt flag
    running = false;
    cout<< "Emulator stopped due to a fatal error."<<endl;
    errorDetected = true;
  }  

  else if (getFlag(interrupt_flag) == 0){
    if(csrCause == timer_interrupt && !getFlag(timer_flag))
      processSubroutine();
        
    if(csrCause == terminal_interrupt && !getFlag(terminal_flag))
      processSubroutine();  
  }        
}

void Emulator::processSubroutine(){ 

  push(csrStatus);
  push(regPC);

  regPC = csrHandler;

  // disabling any other interrupts while processing current
  setFlag(interrupt_flag);
  setFlag(timer_flag);
  setFlag(terminal_flag);
}

void Emulator::timerTick(){

  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
  currentTime = ms.count();
  
  if (timerActive == true){
    auto timeElapsed =currentTime - previousTime;
    if (currentTime - previousTime >= period){
      setInterupt(timer_interrupt);
      previousTime = currentTime;
    }
  } 
}

void Emulator::resetTimer(uint32_t value){

  timerActive =  true;
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
  currentTime = ms.count();
  previousTime = currentTime;

  switch(value){
    case 0x0: period = 500; break;
    case 0x1: period = 1000; break;
    case 0x2: period = 1500; break;
    case 0x3: period = 2000; break;
    case 0x4: period = 5000; break;
    case 0x5: period = 10000; break;
    case 0x6: period = 30000; break;
    case 0x7: period = 60000; break;
    default: timerActive = false; return;
  }    
}

// Terminal Misc

// intial terminal configurations, should be restored at the end of use
struct termios initialConfig;
void restoreConfig(){
  // restores original config
  tcsetattr(STDIN_FILENO, TCSANOW, &initialConfig);
}

bool Emulator::configureTerminal(){
  // getting current configuration
  if (tcgetattr(STDIN_FILENO, &initialConfig) < 0){
    terminalError = "Couldn't retrieve configurations";
    return false;
  }
	
  // modifying attributes accordingly
  static struct termios newConfig = initialConfig;
    
  newConfig.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN); // disables all echos, line buffering( will not wait for enter)
  // and special character processing
  newConfig.c_cc[VMIN] = 0;  // returns even if no characters are received
  newConfig.c_cc[VTIME] = 0; // reads exactly one character

  newConfig.c_cflag &= ~(CSIZE | PARENB); // disables parity bit and sets output to char size(8 bits)
  newConfig.c_cflag |= CS8; // transmitted using 8 bits per byte


  // setting modified attributes
  if (tcsetattr(STDIN_FILENO, TCSANOW, &newConfig)){
    terminalError = "Failed to set new configuration.";
    return false;
  }

  //  when program ends, calls a function to restore original configurations
  if (atexit(restoreConfig) != 0){
    terminalError = "Failed to set restoration point.";
    return false;
  }
  
  return true;
}

void Emulator::resetTerminal(){

  restoreConfig();
}

void Emulator::readTerminal(){

  char inputChar;
  if (read(STDIN_FILENO, &inputChar, 1)){
    writeMemWord((uint32_t)inputChar, TERM_IN);
    setInterupt(terminal_interrupt);    
  }
}

void Emulator::generateOutput(){  
  cout << "----------------------------------------------------------------" << std::endl;
  cout << "Emulated processor executed halt instruction" << std::endl;
  cout << "Emulated processor state:" << std::endl;

  for (auto i = 0; i < 16; i++) {
    cout << "r" << i << "=" << std::hex << std::setw(16) << std::setfill('0') << registers[i] << std::endl;
  }

  cout<<endl;
}

int main(int argc, const char *argv[]){
   
   if (argc != 2){
        cout << "Only one file for execution needs to be passed to emulator." << endl;
        return -1;
    }
    
    string inputFile = argv[1];
    Emulator emulator(inputFile);

    if (!emulator.processInput()) return -1;

    if (!emulator.loadData()) return -2;
    
    emulator.emulate();

    emulator.generateOutput();

    return 0;
}