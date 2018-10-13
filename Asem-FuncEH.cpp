
#include "Asem-Enumeration.hpp"
#include "Asem-ELFHolder.hpp"
#include "Asem-SymTab.hpp"
#include "StringUtil.hpp"

#include <string>
#include <sstream>
#include <stdexcept>

namespace asem {

void ElfHldProcSymTab(ELFHolder & eh, const std::string & line) {

    std::stringstream ss{line};

    std::string temp;
    int ord;
    std::string name;
    int val;
    Section::Enum sect = Section::Undefined;
    Scope::Enum scope  = Scope::Undefined;
    bool def = true;

    // Ord, Name:
    ss >> ord >> name;

    // Val:
    ss >> temp;
    if (gen::string_is_integer(temp)) {
        val = std::stoi(temp);
    } else {
        if (temp == "?")
            def = false;
        else
            throw std::logic_error("asem::ElfHldProcSymTab(...) - Integer "
                    "expected but [" + temp + "] provided.");
    }

    // Section:
    ss >> temp;
    if (temp == "Text") {
        sect = Section::Text;
    }
    else if (temp == "Data") {
        sect = Section::Data;
    }
    else if (temp == "BSS") {
        sect = Section::BSS;
    }
    else if (temp == "ROData") {
        sect = Section::ROData;
    }

    // Scope:
    ss >> temp;
    if (temp == "Local") {
        scope = Scope::Local;
    }
    else if (temp == "Global") {
        scope = Scope::Global;
    }
    
    // Write to SymTab:
    eh.symtab.data.emplace( name,
        SymbolTableEntry(ord, scope, sect, Section::Undefined, val, def) );

}

void ElfHldProcRR(ELFHolder & eh, const std::string & line, Section::Enum sec) {

    std::stringstream ss{line};

    int off;
    std::string temp;
    RelocType::Enum type = RelocType::Undefined;
    int ord;

    // Off:
    ss >> std::hex >> off;

    // Type:
    ss >> temp;
    if (temp == "R_ABS_08") {
        type = RelocType::Abs_08;
    }
    else if (temp == "R_ABS_16") {
        type = RelocType::Abs_16;
    }
    else if (temp == "R_ABS_32") {
        type = RelocType::Abs_32;
    }
    else if (temp == "R_PCR_16") {
        type = RelocType::PCRel_16;
    }

    // Ord:
    ss >> std::hex >> ord;

    // Add to EH:
    eh.relocations[sec].emplace_back(type, off, ord);

}

void ElfHldProcSec(ELFHolder & eh, const std::string & line, Section::Enum sec) {

    gen::hex_to_buffer(line, eh.sections[sec].data, ' ');

}
    
}