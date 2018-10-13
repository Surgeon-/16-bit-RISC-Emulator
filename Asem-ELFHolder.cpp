
#include "Asem-ELFHolder.hpp"
#include "Asem-Enumeration.hpp"
#include "Asem-SymTab.hpp"
#include "Asem-FuncEH.hpp"
#include "Asem-Func.hpp"
#include "StringUtil.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#define UCHAR unsigned char

namespace asem {

void BinaryVector::addByte(unsigned char  val) {

    data.push_back(val);

}

void BinaryVector::addWord(unsigned short val) {

    UCHAR arr[2];

    std::memcpy(arr, &val, 2);

    data.push_back( arr[0] );
    data.push_back( arr[1] );

}

void BinaryVector::addLong(unsigned int   val) {

    UCHAR arr[4];

    std::memcpy(arr, &val, 4);

    data.push_back( arr[0] );
    data.push_back( arr[1] );
    data.push_back( arr[2] );
    data.push_back( arr[3] );

}

////////////////////////////////////////////////////////////////////////////////

void ELFHolder::clear() {

    for (size_t i = 0; i < 4; i += 1) {
        relocations[i].clear();
        sections[i].data.clear();
        skip[0] = 0u;
    }
    
    symtab.data.clear();

    curr_section = Section::Undefined;

}
    
#define ST_UNDEFINED    0
#define ST_RD_SYMTAB    1
#define ST_RD_RR_TEXT   2
#define ST_RD_RR_BSS    3
#define ST_RD_RR_DATA   4
#define ST_RD_RR_RODATA 5
#define ST_RD_TEXT      6
#define ST_RD_BSS       7
#define ST_RD_DATA      8
#define ST_RD_RODATA    9
    
void ELFHolder::loadFromFile(const char * path) {

    clear();

    std::ifstream file{path, std::ifstream::in};

    if (!file.is_open()) 
        throw std::runtime_error(std::string{"Could not open file ["} + path + "] for reading.");

    //list.clear();
    //list.emplace_back();

    int state = ST_UNDEFINED;

    size_t line_ord = 1;

    for (std::string line; std::getline(file, line); line_ord += 1) {

        line = gen::string_crop( gen::string_replace_all(line, "\t", " "), " ");

        if (line.empty()) continue;

        // Process comment (does continue):
        if (line[0] == '#') {

            if (line == "#.symtab") {
                state = ST_RD_SYMTAB; continue;
            }

            if (line == "#.ret.text") {
                state = ST_RD_RR_TEXT; continue;
            }

            if (line == "#.ret.bss") {
                state = ST_RD_RR_BSS; continue;
            }

            if (line == "#.ret.data") {
                state = ST_RD_RR_DATA; continue;
            }

            if (line == "#.ret.rodata") {
                state = ST_RD_RR_RODATA; continue;
            }

            if (line == "#.text") {
                state = ST_RD_TEXT; continue;
            }

            if (line == "#.bss") {
                state = ST_RD_BSS; continue;
            }

            if (line == "#.data") {
                state = ST_RD_DATA; continue;
            }

            if (line == "#.rodata") {
                state = ST_RD_RODATA; continue;
            }

            // Otherwise it's a regular comment:
            continue;

        }

        // PROCESS LINE (not a comment) ////////////////////////////////////////
        
        switch (state) {
            
            // Symbol table:
            case ST_RD_SYMTAB:
                ElfHldProcSymTab(*this, line);
                break;
                
            // Relocation records:
            case ST_RD_RR_TEXT:   ElfHldProcRR(*this, line, Section::Text  ); break;
            case ST_RD_RR_BSS:    ElfHldProcRR(*this, line, Section::BSS   ); break;
            case ST_RD_RR_DATA:   ElfHldProcRR(*this, line, Section::Data  ); break;
            case ST_RD_RR_RODATA: ElfHldProcRR(*this, line, Section::ROData); break;
                
            // Sections:
            case ST_RD_TEXT:   ElfHldProcSec(*this, line, Section::Text  ); break;
            case ST_RD_BSS:    ElfHldProcSec(*this, line, Section::BSS   ); break;
            case ST_RD_DATA:   ElfHldProcSec(*this, line, Section::Data  ); break;
            case ST_RD_RODATA: ElfHldProcSec(*this, line, Section::ROData); break;
                break;
            
        }

        // END PROCESS LINE ////////////////////////////////////////////////////

    }

}

#undef ST_UNDEFINED
#undef ST_RD_SYMTAB
#undef ST_RD_RR_TEXT
#undef ST_RD_RR_BSS
#undef ST_RD_RR_DATA
#undef ST_RD_RR_RODATA
#undef ST_RD_TEXT
#undef ST_RD_BSS
#undef ST_RD_DATA
#undef ST_RD_RODATA

std::string  ELFHolder::rrToString(Section::Enum sec) const {
        
    if (relocations[sec].empty()) return "";

    std::string rv{"#.ret"};

    switch (sec) {
        case Section::Text:   rv += ".text\n";   break;
        case Section::Data:   rv += ".data\n";   break;
        case Section::BSS:    rv += ".bss\n";    break;
        case Section::ROData: rv += ".rodata\n"; break;
    }

    char buffer[128];

    snprintf( buffer, sizeof(buffer)
            , "%-9s  %-10s  %6s\n"
            , "#Offset"
            , "Type"
            , "SymOrd"
            ) ;

    rv += buffer;

    for (const RelocRecord & rr : relocations[sec]) {

        snprintf( buffer, sizeof(buffer)
                , "0x%07X  %-10s  0x%04X\n"
                , (int)rr.offset
                , RelocTypeToString(rr.type)
                , (int)rr.value
                ) ;

        rv += buffer;

    }

    rv.pop_back();

    return rv;

}

std::string ELFHolder::secToString(Section::Enum sec) const {

    if (sections[sec].data.empty()) return "";

    std::string rv{"#"};

    switch (sec) {
        case Section::Text:   rv += ".text\n";   break;
        case Section::Data:   rv += ".data\n";   break;
        case Section::BSS:    rv += ".bss\n";    break;
        case Section::ROData: rv += ".rodata\n"; break;
    }

    rv += gen::buffer_to_hex( &sections[sec].data[0] 
                            ,  sections[sec].data.size() // sizeof(data[0]) = 1
                            ,  ' '
                            ) ;

    return rv;

}
    
}

#undef UCHAR