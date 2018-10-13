
#ifndef ASEM_ELFHOLDER_HPP
#define ASEM_ELFHOLDER_HPP

#include "Asem-Enumeration.hpp"
#include "Asem-SymTab.hpp"

#include <list>
#include <vector>
#include <string>

namespace asem {
    
    using std::size_t;
    
    struct RelocRecord {
        
        RelocType::Enum type;
        size_t offset;
        size_t value;
        
        RelocRecord
            ( RelocType::Enum type = RelocType::Undefined
            , size_t offset = 0
            , size_t value = 0
            )
            : type(type)
            , offset(offset)
            , value(value)
            { }
        
    };
    
    ////////////////////////////////////////////////////////////////////////////
    
    struct BinaryVector {
        
        std::vector<unsigned char> data;
        
        void addByte(unsigned char  val);
        void addWord(unsigned short val);
        void addLong(unsigned int   val);
        
    };
    
    ////////////////////////////////////////////////////////////////////////////
    
    class ELFHolder {
        
    public:
        
        SymbolTable symtab;
        
        std::list<RelocRecord> relocations[4];
        
        BinaryVector sections[4];
        
        size_t skip[4];
        
        Section::Enum curr_section;
        
        void clear();
        
        void loadFromFile(const char * path);
        
        std::string  rrToString(Section::Enum sec) const;
        std::string secToString(Section::Enum sec) const;
        
        size_t locCnt(Section::Enum sec) const {
            
            return (sections[sec].data.size() + skip[sec]);
            
        }
        
    };
    
}

#endif /* ASEM_ELFHOLDER_HPP */

