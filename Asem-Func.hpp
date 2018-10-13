
#ifndef ASEM_FUNC_HPP
#define ASEM_FUNC_HPP

#include "Asem-Enumeration.hpp"

namespace asem {

    const char * SectionToString(Section::Enum section);

    const char * ScopeToString(Scope::Enum scope);

    const char * RelocTypeToString(RelocType::Enum mode);

    AddrMode::Enum OperandAddrMode(Operand::Enum type);
       
    void InstructionOperands(Command::Enum instr, bool & dst, bool & src);
    
}

#endif /* ASEM_FUNC_HPP */

