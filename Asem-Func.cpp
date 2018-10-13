
#include "Asem-Enumeration.hpp"

namespace asem {

    const char * SectionToString(Section::Enum section) {
        
        switch (section) {
            
            case Section::Undefined: return "Undef.";
                
            case Section::ROData: return "ROData";
            case Section::Text:   return "Text";
            case Section::Data:   return "Data";
            case Section::BSS:    return "BSS";
               
            default: return "Error";
            
        }
        
    }

    const char * ScopeToString(Scope::Enum scope) {
        
        switch (scope) {
            
            case Scope::Undefined: return "Undef.";
            case Scope::Global:    return "Global";
            case Scope::Local:     return "Local";
               
            
        }
        
    }

    const char * RelocTypeToString(RelocType::Enum mode) {
        
        switch (mode) {
            
            
            case RelocType::Abs_08: return "R_ABS_08";
            case RelocType::Abs_16: return "R_ABS_16";
            case RelocType::Abs_32: return "R_ABS_32";
                
            case RelocType::PCRel_16: return "R_PCR_16";
                
            default: return "R_UNDEF.";
            
        }
        
    }

    AddrMode::Enum OperandAddrMode(Operand::Enum type) {
        
        switch (type) {
            
            case Operand::Literal:
            case Operand::SymbolValue:
            case Operand::PSW:
                return AddrMode::Imm;
                
            case Operand::SymbolDeref:
            case Operand::LiteralIndir:
                return AddrMode::MemDir;
                
            case Operand::RegDirect:
                return AddrMode::RegDir;
                
            case Operand::RegIndir:
            case Operand::RegIndirExp:
            case Operand::PCRel:
                return AddrMode::RegInd;
            
        }
        
    }
    
    void InstructionOperands(Command::Enum instr, bool & dst, bool & src) {
        
        switch (instr) {
            
            case Command::Add:
            case Command::Sub:
            case Command::Mul:
            case Command::Div:
            case Command::Cmp:
            case Command::And:
            case Command::Or:
            case Command::Not:
            case Command::Test:
            case Command::Mov:
            case Command::Shl:
            case Command::Shr:
                dst = true;
                src = true;
                break;
                
            case Command::Push:
            case Command::Call:
                dst = false;
                src = true;
                break;
                
            case Command::Pop:
                dst = true;
                src = false;
                break;
                
            case Command::Iret:
                dst = false;
                src = false;
                break;
            
        }
        
    }
    
}