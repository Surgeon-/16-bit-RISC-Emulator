
#include "VM87-Runtime.hpp"
#include "Asem-Enumeration.hpp"
#include "Asem-ELFHolder.hpp"
#include "Asem-SymTab.hpp"
#include "Asem-Func.hpp"
#include "Punning.hpp"
#include "CPrint.hpp"

#include <iostream>
#include <cstring>
#include <chrono>
#include <curses.h>

#define USHORT_RANGE 65536

using namespace asem;

static bool InRange(unsigned val, unsigned min, unsigned len) {
        
    return (val >= min && val < (min + len));

}

namespace vm87 {
    
    InstructionDesc::InstructionDesc(unsigned short e) {
        
        pred = static_cast<Predicate::Enum>((e >> 14) & 0x3);
        id   = static_cast<Command::Enum>(((e >> 10) & 0xF) + Command::Add);
        
        dst_am = static_cast<AddrMode::Enum>((e >> 8) & 0x3);
        src_am = static_cast<AddrMode::Enum>((e >> 3) & 0x3);
        
        dst_reg_no = (e >> 5) & 0x7;
        src_reg_no = (e >> 0) & 0x7;
        
    }
    
    InstructionDesc::InstructionDesc()
        : InstructionDesc(0u)
        { }
    
    ProcessorState::ProcessorState() {

        for (size_t i = 0; i < 8; i += 1)
            regs[i] = 0;
        
        psw = 0;
        
    }
    
    bool ProcessorState::getZF() const {
        
        return ((psw & (1 << 0)) != 0);
        
    }
    
    bool ProcessorState::getOF() const {
        
        return ((psw & (1 << 1)) != 0);
        
    }
    
    bool ProcessorState::getCF() const {
        
        return ((psw & (1 << 2)) != 0);
        
    }
    
    bool ProcessorState::getNF() const {
        
        return ((psw & (1 << 3)) != 0);
        
    }
    
    bool ProcessorState::getMF() const {
        
        return ((psw & (1 << 15)) != 0);
        
    }
    
    bool ProcessorState::getTF() const {
        
        return ((psw & (1 << 13)) != 0);
        
    }
    
    void ProcessorState::setZF(bool v) {
        
        if (v)
            psw |=  ((USHORT)1 << 0);
        else
            psw &= ~((USHORT)1 << 0);
        
    }
    
    void ProcessorState::setOF(bool v) {
        
        if (v)
            psw |=  ((USHORT)1 << 1);
        else
            psw &= ~((USHORT)1 << 1);
        
    }
    
    void ProcessorState::setCF(bool v) {
        
        if (v)
            psw |=  ((USHORT)1 << 2);
        else
            psw &= ~((USHORT)1 << 2);
        
    }
    
    void ProcessorState::setNF(bool v) {
        
        if (v)
            psw |=  ((USHORT)1 << 3);
        else
            psw &= ~((USHORT)1 << 3);
        
    }
    
    void ProcessorState::setMF(bool v) {
        
        if (v)
            psw |=  ((USHORT)1 << 15);
        else
            psw &= ~((USHORT)1 << 15);
        
    }
    
    void ProcessorState::setTF(bool v) {
        
        if (v)
            psw |=  ((USHORT)1 << 13);
        else
            psw &= ~((USHORT)1 << 13);
        
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    Runtime::Runtime()
        : state()
        , mem(USHORT_RANGE, 0) {
        
        debug = false;
        
        for (size_t i = 0; i < 8; i += 1)
            irq[i] = false;
        
        irq_hand = 0;
        
    }
    
#define MIN(x, y) ((x>=y)?(y):(x))
    
    size_t Runtime::locateSections
        ( const ELFHolder &eh
        , Section::Enum sec[4]
        , size_t pos[4]
        , size_t len[4]
        , size_t * start_addr
        ) const {
        
        const SymbolTable & st = eh.symtab;

        size_t cnt = 0;
        
        size_t min_val = size_t(-1);
        
        // FETCH: (pos = ordinal in SymTab)
        
        if (st.check(".text") == SymbolTable::DEFINED) {
            sec[cnt] = Section::Text;
            pos[cnt] = st.data.at(".text").ordinal;
            len[cnt] = eh.sections[Section::Text].data.size();
            cnt += 1;
            int val  = st.data.at(".text").value;
            min_val  = size_t( MIN(min_val, size_t(val)) );
        }
        
        if (st.check(".bss") == SymbolTable::DEFINED) {
            sec[cnt] = Section::BSS;
            pos[cnt] = st.data.at(".bss").ordinal;
            len[cnt] = eh.sections[Section::BSS].data.size();
            cnt += 1;
            int val  = st.data.at(".bss").value;
            min_val  = size_t( MIN(min_val, size_t(val)) );
        }
        
        if (st.check(".data") == SymbolTable::DEFINED) {
            sec[cnt] = Section::Data;
            pos[cnt] = st.data.at(".data").ordinal;
            len[cnt] = eh.sections[Section::Data].data.size();
            cnt += 1;
            int val  = st.data.at(".data").value;
            min_val  = size_t( MIN(min_val, size_t(val)) );
        }
        
        if (st.check(".rodata") == SymbolTable::DEFINED) {
            sec[cnt] = Section::ROData;
            pos[cnt] = st.data.at(".rodata").ordinal;
            len[cnt] = eh.sections[Section::ROData].data.size();
            cnt += 1;
            int val  = st.data.at(".rodata").value;
            min_val  = size_t( MIN(min_val, size_t(val)) );
        }
        
        if (cnt == 0) return 0;
        
        // SORT (by pos, ascending):

        for (size_t i = 0; i < cnt; i += 1) {
            
            size_t min_pos = i;
            
            for (size_t t = i + 1; t < cnt; t += 1) {
                if (pos[t] < pos[min_pos]) min_pos = t;
            }
            
            if (min_pos != i) {
                std::swap(sec[i], sec[min_pos]);
                std::swap(pos[i], pos[min_pos]);
                std::swap(len[i], len[min_pos]);
            }
            
        }
        
        // ADJUST:
        
        *start_addr = min_val;
        
        pos[0] = min_val;
        
        for (size_t i = 1; i < cnt; i += 1) {
            
            pos[i] = pos[i - 1] + len[i - 1];
            
        }
        
        return cnt;
        
    }
    
    void Runtime::printState() const {
        
        cprint("| R 0 | R 1 | R 2 | R 3 | R 4 | R 5 | R 6 | R 7 | PSW |\n");
        cprint("|-----|-----|-----|-----|-----|-----|-----|-----|-----|\n");
        cprint("|%5d|%5d|%5d|%5d|%5d|%5d|%5d|%5d|%5X|\n"
            , (int)state.regs[0]
            , (int)state.regs[1]
            , (int)state.regs[2]
            , (int)state.regs[3]
            , (int)state.regs[4]
            , (int)state.regs[5]
            , (int)state.regs[6]
            , (int)state.regs[7]
            , (int)state.psw
            ) ;
        
    }
    
    void Runtime::printInstrDesc(const InstructionDesc & desc, USHORT data) const {
        
        cprint("OPCODE = %2d ; PRED = %d ; DSTAM = %d ; SRCAM = %d ; DATA = %d\n"
            , int(desc.id - Command::Add)
            , int(desc.pred)
            , int(desc.dst_am)
            , int(desc.src_am)
            , int(data)
            ) ;
        
    }
    
#undef MIN
    
    void Runtime::loadFromELF(const ELFHolder& eh, bool cs) {
        
        // Quick-fail:
        for (const auto & ste : eh.symtab.data) {
            
            if (ste.second.ordinal == 0u) continue;
            
            if (!ste.second.defined)
                throw LoadError( "Undefined symbol [" + ste.first + "] found "
                                 "in Symbol Table (only single-file programs "
                                 "are supported).");
            
        }
        
        Section::Enum sec[4];
        size_t        pos[4];
        size_t        len[4];
        size_t    start_addr;
        
        size_t cnt = locateSections(eh, sec, pos, len, &start_addr);
        
        if (cnt == 0)
            throw LoadError("Loaded ELF file has no defined sections.");
        
        if (start_addr < SP_INIT)
            throw LoadError("Starting address must be at least 1024.");
        
        if (pos[cnt - 1] + len[cnt - 1] >= (USHORT_RANGE - 128))
            throw LoadError("Program exceeds memory boundaries.");
        
        //std::cout << "start_addr = " << start_addr << "\n";
        
        for (size_t i = 0; i < cnt; i += 1) {
            
            /*std::cout << SectionToString(sec[i]) << " -> ";
            std::cout << "Pos = " << pos[i] << "\n";*/
            
            sec_addr[sec[i]] = pos[i];
            sec_len [sec[i]] = len[i];
            
            std::memcpy( &(mem[pos[i]]), &(eh.sections[sec[i]].data[0]), len[i] );
        }
        
        // RR: /////////////////////////////////////////////////////////////////
        
        std::vector<std::pair<std::string, SymbolTableEntry>> rr_vec;
        eh.symtab.toOrderedVector(rr_vec); // CREATE ORDERED SYMTAB VECTOR

        for (size_t i = 0; i < cnt; i += 1) {

            Section::Enum sect = sec[i];
            
            for (const RelocRecord & rr : eh.relocations[sect]) {

                //std::cout << "  At address " << /*pos[sect] +*/ rr.offset << "\n";
                
                void * raw_addr = &mem[/*pos[sect]  +*/ rr.offset]; // PEP

                int symval = (rr_vec[rr.value].second).value;

                switch (rr.type) {

                    case RelocType::Abs_08: {
                        char * addr = static_cast<char*>(raw_addr);
                        *addr += static_cast<char>(symval);
                    }
                        break;

                    case RelocType::Abs_16: {
                        short * addr = static_cast<short*>(raw_addr);
                        *addr += static_cast<short>(symval);
                    }
                        break;

                    case RelocType::Abs_32: {
                        int * addr = static_cast<int*>(raw_addr);
                        *addr += static_cast<int>(symval);
                    }
                        break;

                    case RelocType::PCRel_16: {
                        USHORT * addr = static_cast<USHORT*>(raw_addr);
                        *addr += static_cast<USHORT>(symval - rr.offset + 2);
                    }
                        break;

                } // end_switch

            } // end_for

        } // end_for
        
        // REGS (pc / sp): /////////////////////////////////////////////////////

        if (eh.symtab.check("_start") != SymbolTable::DEFINED)
            throw LoadError("Loaded ELF file does not define a '_start' symbol.");
        
        if (eh.symtab.data.at("_start").section != Section::Text)
            throw LoadError("Symbol '_start' must be in the .text section.");
        
        size_t text_ind = size_t(-1);
        for (size_t i = 0; i < cnt; i += 1)
            if (sec[i] == Section::Text) {
                text_ind = i;
                break;
            }
        
        //std::cout << "text_ind = " << text_ind << "\n";
        
        short _start = static_cast<short>(eh.symtab.data.at("_start").value);
        if (!cs) {
            _start += static_cast<short>(pos[text_ind]);
        }
        
        state.regs[PC] = _start;
        state.regs[SP] = SP_INIT;
        
        // INTERRUPTS: /////////////////////////////////////////////////////////
        static const char * routines[8] = 
            { "_interrupt0", "_interrupt1", "_interrupt2"
            , "_interrupt3", "_interrupt4", "_interrupt5"
            , "_interrupt6", "_interrupt7"} ;
        
        for (size_t i = 0; i < 8u; i += 1) {
            
            if (eh.symtab.check(routines[i]) == SymbolTable::DEFINED) {
                
                int symval = eh.symtab.data.at(routines[i]).value;
                
                USHORT sa = USHORT(symval);
                
                //std::cout << "Setting IVT " << i << " to " << sa << "\n";
                
                std::memcpy( &(mem[i * 2]), &sa, 2 );
                
            }
            
        }
        
    }

    void Runtime::runProgram(bool do_debug) {
        
        debug = do_debug;
        
        InstructionDesc desc{};
        USHORT data;
        
        TIME_POINT tp = CLOCK::now();
        
        callInterrupt(INT_INIT);
        
        while (true) {
        
            if (debug) printState();
            
            // FETCH:
            fetchInstruction(desc, data);

            if (debug) {
                
                printInstrDesc(desc, data);
                cprint("Press ENTER to Step ");
                while (1) {
                    
                    int choice = getch();
                    
                    if (choice == ERR) continue;
                    
                    if (choice == '\n') 
                        break;
                    else
                        return;
                    
                }
                cprint("\n");
                
            }
            
            // EXECUTE:
            try {
                
                executeInstruction(desc, data);
                
            } catch (ViolationError & ex) {
                
                if (!callInterrupt(INT_VIOLATION)) {
                    
                    throw;
                
                }
                
            }
            
            // INTERRUPTS:
            manageInterrupts(tp);
            
            // END OF PROGRAM (if psw & (1 << 10) != 0):
            if ( (state.psw & USHORT(1 << 10)) != 0 ) break;
            
        }
        
    }
    
    bool Runtime::callInterrupt(int ordinal) {
        
        USHORT ptr = memLoad(USHORT(ordinal * 2));
        
        if (ptr != 0) {

            if (state.regs[SP] <= 16)
                throw UnrecError("Stack overflow.");
            state.regs[SP] -= 2;
            memStore(state.regs[SP], state.regs[PC]);

            if (state.regs[SP] <= 16)
                throw UnrecError("Stack overflow.");
            state.regs[SP] -= 2;
            memStore(state.regs[SP], state.psw);

            // FLAGS - STUB
            
            state.regs[PC] = ptr;
            
            return true;

        }
        
        return false;
        
    }
    
    void Runtime::fetchInstruction(InstructionDesc & desc, USHORT & data) {
        
        const void * raw_addr;
        
        bool dst, src;
        
        accessAddress(state.regs[PC] + 0u, EXECUTE);
        accessAddress(state.regs[PC] + 1u, EXECUTE);
        
        raw_addr = &(mem[state.regs[PC]]);
        state.regs[PC] += 2;
        USHORT encoded = *(USHORT*)raw_addr;
        
        desc = InstructionDesc(encoded);
        
        InstructionOperands(desc.id, dst, src);
        
        if ((dst && (desc.dst_am != AddrMode::RegDir)) ||
            (src && (desc.src_am != AddrMode::RegDir))) {
            
            accessAddress(state.regs[PC] + 0u, EXECUTE);
            accessAddress(state.regs[PC] + 1u, EXECUTE);
            
            raw_addr = &(mem[state.regs[PC]]);
            state.regs[PC] += 2;
            data = *(USHORT*)raw_addr;
            
        }
        
    }
    
    void Runtime::executeInstruction(const InstructionDesc & desc, USHORT data) {
        
        // Check predicate:
        switch (desc.pred) {
            case Predicate::Eq:
                if (!state.getZF()) return;
                break;
            case Predicate::Ne:
                if (state.getZF()) return;
                break;
            case Predicate::Gt:
                if (state.getNF() || state.getZF()) return;
                break;
            default:
                break;
        }
        
         short dst_s, src_s, res_s;
        USHORT dst_u, src_u, res_u;
        
        switch (desc.id) {
            
            case Command::Add:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s + src_s;
                storeValueSigned(desc, data, res_s);
                break;
                
            case Command::Sub:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s - src_s;
                storeValueSigned(desc, data, res_s);
                break;
                
            case Command::Mul:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s * src_s;
                storeValueSigned(desc, data, res_s);
                break;
                
            case Command::Div:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                if (src_s == 0)
                    throw ViolationError("Division by zero.");
                res_s = dst_s / src_s;
                storeValueSigned(desc, data, res_s);
                break; 
                
            case Command::Cmp:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s - src_s;
                //storeValueSigned(desc, data, val_s);//
                break;
                
            case Command::And:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s & src_s;
                storeValueSigned(desc, data, res_s);
                break; 
                
            case Command::Or:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s | src_s;
                storeValueSigned(desc, data, res_s);
                break;
                
            case Command::Not:
                dst_s = ~loadValueSigned(desc, data, SRC);
                storeValueSigned(desc, data, dst_s);
                break; 
                
            case Command::Test:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s & src_s;
                //storeValueSigned(desc, data, val_s);//
                break;
                
            case Command::Push: // sp -= 2; mem[sp] = src
                if (state.regs[SP] <= 16)
                    throw UnrecError("Stack overflow.");
                state.regs[SP] -= 2;
                src_u = loadValueUnsigned(desc, data, SRC);
                memStore(state.regs[SP], src_u);
                break;
                
            case Command::Pop: // dst = mem[sp]; sp += 2
                if (state.regs[SP] >= SP_INIT)
                    throw UnrecError("Stack underflow.");
                dst_u = memLoad(state.regs[SP]);
                storeValueUnsigned(desc, data, dst_u);
                state.regs[SP] += 2;
                break;
                
            case Command::Call: // push pc; pc=src
                state.regs[SP] -= 2;
                memStore(state.regs[SP], state.regs[PC]);
                state.regs[PC] = loadValueUnsigned(desc, data, SRC);
                break;
                
            case Command::Iret: // pop psw; pop pc
                dst_u = memLoad(state.regs[SP]);
                state.regs[SP] += 2;
                state.psw = dst_u;
                dst_u = memLoad(state.regs[SP]);
                state.regs[SP] += 2;
                state.regs[PC] = dst_u;
                break;
                
            case Command::Mov:
                src_s = loadValueSigned(desc, data, SRC);
                storeValueSigned(desc, data, src_s); // !!!!!!!!!!!!!!!!!!
                break;
                
            case Command::Shl:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s << src_s;
                storeValueSigned(desc, data, res_s);
                break;
                
            case Command::Shr:
                dst_s = loadValueSigned(desc, data, DST);
                src_s = loadValueSigned(desc, data, SRC);
                res_s = dst_s >> src_s;
                storeValueSigned(desc, data, res_s);
                break;
                
        }
        
        // Update flags (unsigned values are redundant...);
        setFlags(desc, dst_s, src_s, res_s, dst_u, src_u, res_u);
        
    }
    
    void Runtime::manageInterrupts(TIME_POINT & tp) {
        
        // Initialization:
        // --not here
        
        // Timer:
        TIME_POINT tp2 = CLOCK::now();
        auto diff = tp2 - tp;
        
        int diff_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
        
        if (diff_ms >= 1000) {
            
            tp = tp2;
            
            if (state.getTF()) {
                
                irq[INT_TIMER] = true;
                
                //std::cout << "Timer\n";
                
            }
            
        }
        
        // Violation:
        // --not here
        
        // Key press:
        int ch;
        if ((ch = getch()) == ERR) {
            // No input
        }
        else {
            memStore(USHORT(0xFFFC), gen::pun_s_to_u<short>(short(ch)));
            irq[INT_KEYSTROKE] = true;
        }
        
        // Execute:
        for (size_t cnt = 0; cnt < 8; cnt += 1) {
            
            int i = irq_hand;
            irq_hand = (irq_hand + 1) % 8;
            
            if (irq[i] == false) continue;
            
            if (state.getMF() && i != INT_VIOLATION) continue;
                
            irq[i] = false;
            
            callInterrupt(i);
            
            break;
            
        }
        
    }
    
    // Execute helpers:
    
#define text   Section::Text
#define data   Section::Data
#define bss    Section::BSS
#define rodata Section::ROData
    
    void Runtime::accessAddress(USHORT address, int action) const {
        
        const USHORT a = address;
        
        switch (action) {
            
            case READ:
                if ( InRange(a, sec_addr[text], sec_len[text])     ||
                     InRange(a, sec_addr[data], sec_len[data])     ||
                     InRange(a, sec_addr[bss], sec_len[bss])       ||
                     InRange(a, sec_addr[rodata], sec_len[rodata]) ||
                     InRange(a, 0u, SP_INIT) || InRange(a, (65536u - 128u), 128) )
                    return;
                throw ViolationError( "Read access violation on "
                                      "address " + std::to_string(a));
                break;
                
            case WRITE:
                if ( InRange(a, sec_addr[data], sec_len[data]) ||
                     InRange(a, sec_addr[bss], sec_len[bss])   ||
                     InRange(a, 0u, SP_INIT) || InRange(a, (65536u - 128u), 128) )
                    return;
                throw ViolationError( "Write access violation on "
                                      "address " + std::to_string(a));
                break;
                
            case EXECUTE:
                if (InRange(a, sec_addr[text], sec_len[text])) return;
                throw ViolationError( "Execute access violation on "
                                      "address " + std::to_string(a));
                break;
                
            default:
                throw UnrecError("vm87::Runtime::accessAddress(...) - Unknown action.");
                break;
            
        }
        
    }
    
#undef text
#undef data
#undef bss
#undef rodata
    
    USHORT Runtime::memLoad(USHORT address) {
        
        accessAddress(address + 0u, READ);
        accessAddress(address + 1u, READ);
        
        USHORT rv;
        
        std::memcpy(&rv, &mem[address], sizeof(USHORT));
        
        return rv;
        
    }
        
    void Runtime::memStore(USHORT address, USHORT value) {
        
        accessAddress(address + 0u, WRITE);
        accessAddress(address + 1u, WRITE);
        
        std::memcpy(&mem[address], &value, sizeof(USHORT));
        
        if ((int)address == 0xFFFE) { // Console output
            
            char c;
            
            if (value == 10)
                c = '\n';
            else
                c = static_cast<char>(value);
            
            if (debug) cprint("CONSOLE OUTPUT: ");
            
            cprint("%c", c);
            //printf("%c", c);
            
            if (debug) cprint("\n");
            
        }
        
    }
    
    USHORT Runtime::loadValueUnsigned(const InstructionDesc & desc, USHORT data, bool place) {
        
        USHORT rv;
        
        AddrMode::Enum mode = ((place == DST)?(desc.dst_am):(desc.src_am));
        unsigned reg_no     = ((place == DST)?(desc.dst_reg_no):(desc.src_reg_no));
        
        switch (mode) {
            
            case AddrMode::Imm:
                if (reg_no == 0x7) return state.psw;
                return data;
                
            case AddrMode::MemDir:
                return memLoad(data);
                
            case AddrMode::RegDir:
                return state.regs[reg_no];
                
            case AddrMode::RegInd:
                return memLoad(state.regs[reg_no] + data);
            
        }
        
    }
    
    short Runtime::loadValueSigned(const InstructionDesc & desc, USHORT data, bool place) {
        
        USHORT temp = loadValueUnsigned(desc, data, place);
        
        return gen::pun_u_to_s<short>(temp);
        
    }
    
    void Runtime::storeValueUnsigned(const InstructionDesc & desc, USHORT data, USHORT value) {
        
        AddrMode::Enum mode = desc.dst_am;
        unsigned reg_no     = desc.dst_reg_no;
        
        switch (mode) {
            
            case AddrMode::Imm:
                if (reg_no == 0x7)
                    state.psw = value;
                else {
                    /* STUB - ERROR */
                }
                break;
                
            case AddrMode::MemDir:
                memStore(data, value);
                break;
                
            case AddrMode::RegDir:
                state.regs[reg_no] = value;
                break;
                
            case AddrMode::RegInd:
                memStore(state.regs[reg_no] + data, value);
                break;
            
        }
        
    }
    
    void Runtime::storeValueSigned(const InstructionDesc & desc, USHORT data, short value) {
        
        storeValueUnsigned(desc, data, gen::pun_s_to_u<short>(value));
        
    }
     
    void Runtime::setFlags
        ( const InstructionDesc & desc
        , short dst_s
        , short src_s
        , short res_s
        , USHORT dst_u
        , USHORT src_u
        , USHORT res_u
        ) {
        
        int temp;
        
        switch (desc.id) {
                
            case Command::Sub: // zocn
            case Command::Cmp:
                temp = int(dst_s) - int(src_s);
                if ((dst_s > 0 && src_s < 0 && res_s < 0) ||
                    (dst_s < 0 && src_s > 0 && res_s > 0))
                    state.setOF(true);
                else
                    state.setOF(false);
                
            case Command::Add: // zocn
                if (desc.id == Command::Add) {
                    temp = int(dst_s) + int(src_s);
                    if ((dst_s < 0 && src_s < 0 && res_s > 0) ||
                        (dst_s > 0 && src_s > 0 && res_s < 0))
                        state.setOF(true);
                    else
                        state.setOF(false);
                }
                
                state.setCF(temp & (1 << 16));
                
            case Command::Mul: // zn
            case Command::Div:
            case Command::And:
            case Command::Or:
            case Command::Not:
            case Command::Test:
                state.setZF(res_s == 0);
                state.setNF(res_s  < 0);
                break;
          
            case Command::Mov: // zn // !!!!!!!!!!!!!!!!!!!!!
                state.setZF(src_s == 0);
                state.setNF(src_s  < 0);
                break;
                
            case Command::Shl: // zcn
                state.setZF(res_s == 0);
                state.setNF(res_s  < 0);
                state.setCF(dst_s  & (1 << 15));
                break;
                
            case Command::Shr: // zcn
                state.setZF(res_s == 0);
                state.setNF(res_s  < 0);
                state.setCF(dst_s  & 1);
                break;
                
            case Command::Push:
            case Command::Pop:
            case Command::Call:
            case Command::Iret: 
                // No change
                break;
                
        }
        
    }
    
}

#undef USHORT_RANGE

