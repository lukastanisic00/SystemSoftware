#ifndef REGEX_HPP
#define REGEX_HPP
#include <regex>
#include <string>

using namespace std;

//directives
string symbol = "[a-zA-Z_][a-zA-Z0-9_]*";
string decLiteral = "-?[0-9]+";
string hexLiteral = "0x[0-9a-fA-F]+";
string expr = "(" + symbol + "|" + hexLiteral + "|"+ decLiteral +")(\\s*(\\+|\\-|\\*|\\/)\\s*(" + symbol + "|" + hexLiteral + "|"+ decLiteral +"))*$";
string str = "\"(.*)\"";

regex regSym(symbol+"$");
regex regExpr(expr);

regex regGlobal("^\\s*\\.global \\s*(" + symbol + "(,\\s*" + symbol + ")*)\\s*$");
regex regExtern("^\\s*\\.extern \\s*(" + symbol + "(,\\s*" + symbol + ")*)\\s*$");
regex regSection("\\s*\\.section \\s*(" + symbol + ")\\s*$");
regex regWord("^\\s*\\.word \\s*(" + symbol +"|"+ decLiteral +"(," + symbol +"|"+ decLiteral +")*)\\s*$");
regex regSkip("^\\s*\\.skip \\s*(" + decLiteral + ")\\s*$");
regex regAscii("^\\s*\\.ascii \\s*(" + str + ")\\s*$");
regex regEqu("^\\s*\\.equ \\s*(" + symbol + ")\\s*,\\s*(" + expr + ")\\s*$");
regex regEnd("\\s*^\\.end\\s*$");

//instructions
string gpr = "%(r[0-9]|sp|pc|r1[0-5])";
string csr = "%(status|handler|cause)";

//simple instructions 
regex regHalt("^\\s*halt\\s*$");
regex regIret("^\\s*iret\\s*$");
regex regInt("^\\s*int\\s*$");
regex regRet("^\\s*ret\\s*$");

//single gpr operand instructions
regex regPush("^\\s*push \\s*(" + gpr + ")\\s*$");
regex regPop("^\\s*pop \\s*(" + gpr + ")\\s*$");
regex regNot("^\\s*not \\s*(" + gpr + ")\\s*$");

//two gpr and csr operands instructions
regex regAdd("^\\s*add \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regSub("^\\s*sub \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regMul("^\\s*mul \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regDiv("^\\s*div \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$");
regex regXchg("^\\s*xchg \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$");
regex regAnd("^\\s*and \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regOr("^\\s*or \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regXor("^\\s*xor \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regShl("^\\s*shl \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regShr("^\\s*shr \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*$"); 
regex regCsrrd("^\\s*csrrd\\s*("+ csr + ")\\s*,\\s*(" + gpr+ ")\\s*$");
regex regCsrwr("^\\s*csrwr\\s*("+ gpr + ")\\s*,\\s*(" + csr+ ")\\s*$"); 

//jump instructions
regex regJmp("^\\s*jmp \\s*("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ ")\\s*$");
regex regBeq("^\\s*beq \\s*("+ gpr + ")\\s*,\\s*(" + gpr + ")\\s*,\\s*("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ ")\\s*$");
regex regBne("^\\s*bne \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*,\\s*("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ ")\\s*$");
regex regBgt("^\\s*bgt \\s*("+ gpr + ")\\s*,\\s*(" + gpr+ ")\\s*,\\s*("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ ")\\s*$");
regex regCall("^\\s*call \\s*("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ ")\\s*$");

//load and store instructions
string mem = "("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ ")";
string immed = "\\$("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ ")";
regex regMem(mem);
regex regImmed(immed);
string regInd ="\\[(" + gpr + ")((\\s*\\+\\s*)("+ symbol + "|"+ hexLiteral + "|"+ decLiteral+ "))?"+ "\\s*\\]";
regex regRegInd(regInd);
regex regLd("^\\s*ld\\s*(" + mem + "|" + immed + "|" + gpr + "|"+ regInd + ")\\s*,\\s*(" + gpr + ")\\s*$");
regex regSt("^\\s*st\\s*("+ gpr + ")\\s*,\\s*(" + mem + "|" + gpr + "|"+ regInd + ")\\s*$");

//labels
regex regLabel("\\s*(" + symbol + ")\\s*:\\s*(.*)");

//auxiliaries
regex regComment("(#)+.*");
regex regEmptySpace("^\\s*");
regex regTabs("\\t+");
regex regNewline("^[[:space:]]*$");











#endif