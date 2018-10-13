
#ifndef ASEM_FUNCEH_HPP
#define ASEM_FUNCEH_HPP

#include "Asem-Enumeration.hpp"

#include <string>

namespace asem {
    
class ELFHolder;

void ElfHldProcSymTab(ELFHolder & eh, const std::string & line);

void ElfHldProcRR(ELFHolder & eh, const std::string & line, Section::Enum sec);

void ElfHldProcSec(ELFHolder & eh, const std::string & line, Section::Enum sec);
    
}

#endif /* ASEM_FUNCEH_HPP */

