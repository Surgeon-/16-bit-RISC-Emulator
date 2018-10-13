
#ifndef ASEM_RUNTIME_HPP
#define ASEM_RUNTIME_HPP

#include <vector>
#include <stdexcept>
#include <chrono>

#include "Asem-Enumeration.hpp"
#include "Asem-ELFHolder.hpp"

namespace vm87 {
    
    typedef unsigned short           USHORT;
    typedef std::chrono::steady_clock CLOCK;
    typedef CLOCK::time_point    TIME_POINT;
    
    typedef std::invalid_argument LoadError; // (Unrecoverable) Stops the program
    typedef std::runtime_error   UnrecError; // (Unrecoverable) Stops the program
    typedef std::logic_error ViolationError; // Interrupts the program
    
    struct ProcessorState {
        
        USHORT regs[8];
        USHORT psw;
        // PSW:
        // [0] - Z
        // [1] - O
        // [2] - C
        // [3] - N
        
        ProcessorState();
        
        bool getZF() const;
        bool getOF() const;
        bool getCF() const;
        bool getNF() const;
        bool getMF() const; // mask (for interrupts)
        bool getTF() const; // timer
        
        void setZF(bool v);
        void setOF(bool v);
        void setCF(bool v);
        void setNF(bool v);
        void setMF(bool v);
        void setTF(bool v);
        
    };
    
    struct InstructionDesc {
        
        asem::Predicate::Enum pred;
        asem::Command::Enum   id;
        asem::AddrMode::Enum  dst_am;
        asem::AddrMode::Enum  src_am;
        
        unsigned dst_reg_no;
        unsigned src_reg_no;
        
        InstructionDesc();
        
        InstructionDesc(unsigned short encoded);
        
    };
    
    ////////////////////////////////////////////////////////////////////////////
    
    class Runtime {
        
    public:
        
        static const int PC = 7;
        static const int SP = 6;
        
        static const int READ    = 0;
        static const int WRITE   = 1;
        static const int EXECUTE = 2;
        
        static const USHORT SP_INIT = 1024u;
        
        static const bool DST = 0;
        static const bool SRC = 1;
        
        static const int INT_INIT      = 0;
        static const int INT_TIMER     = 1;
        static const int INT_VIOLATION = 2;
        static const int INT_KEYSTROKE = 3;
        
        ProcessorState state;
        
        std::vector<unsigned char> mem;
        
        size_t sec_addr[4];
        size_t sec_len[4];
        
        bool irq[8];
        int  irq_hand;
        
        bool debug;
        
        Runtime();
        
        size_t locateSections
            ( const asem::ELFHolder & eh
            , asem::Section::Enum sec[4]
            , size_t pos[4]
            , size_t len[4]
            , size_t * start_addr
            ) const;
        
        void loadFromELF(const asem::ELFHolder & eh, bool cs);
        
        void runProgram(bool do_debug);
        
        void fetchInstruction(InstructionDesc & desc, USHORT & data);
        
        void executeInstruction(const InstructionDesc & desc, USHORT data);
        
        void manageInterrupts(TIME_POINT & tp);
        
        // Printing:
        
        void printState() const;
        
        void printInstrDesc(const InstructionDesc & desc, USHORT data) const;
        
        // Execute helpers:
        
        bool callInterrupt(int ordinal);
        
        void accessAddress(USHORT address, int action) const;
        
        short loadValueSigned(const InstructionDesc & desc, USHORT data, bool place);
        
        USHORT loadValueUnsigned(const InstructionDesc & desc, USHORT data, bool place);
        
        void storeValueSigned(const InstructionDesc & desc, USHORT data, short value);
        
        void storeValueUnsigned(const InstructionDesc & desc, USHORT data, USHORT value);
        
        USHORT memLoad(USHORT address);
        
        void memStore(USHORT address, USHORT value);
        
        void setFlags
            ( const InstructionDesc & desc
            , short dst_s
            , short src_s
            , short res_s
            , USHORT dst_u
            , USHORT src_u
            , USHORT res_u
            ) ;
        
    };
    
}

#endif /* ASEM_RUNTIME_HPP */

