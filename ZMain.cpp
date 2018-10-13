
#include <iostream>
#include <string>
#include <cstring>

#include <ncurses.h>
#include "CPrint.hpp"

#include "Asem-ELFHolder.hpp"
#include "Asem-SymTab.hpp"
#include "VM87-Runtime.hpp"

const asem::Section::Enum SECTIONS[4] = 
    { asem::Section::Text
    , asem::Section::Data
    , asem::Section::BSS
    , asem::Section::ROData
    } ;

void DisplayHelp() {
    
    std::cout << "\n";
    std::cout << "The calling conventions for this program are:\n";
    std::cout << "[1]  vm87 \"path_in\" [optional flags...]\n";
    std::cout << "   Where optional flags may be (in any order):\n";
    std::cout << "     netbeans - set only if running from netbeans.\n";
    std::cout << "     debug    - run in step-by-step debug mode.\n";
    std::cout << "[2]  vm87 info\n";
    std::cout << "The first option runs programs, the second one displays program info.\n";
    std::cout << "\n";
    
}

void DisplayInfo() {
    
    std::cout << "\n";
    std::cout << " < Elektrotehnicki fakultet Univerziteta u Beogradu >\n";
    std::cout << " - Projektni zadatak iz predmeta Sistemski Softver  -\n";
    std::cout << " - Emulator za hipoteticki 16-bitni procesor        -\n";
    std::cout << " - Student: Jovan Batnozic 0087/2015                -\n";
    std::cout << " -   Datum: Jun 2018. godine                        -\n";
    
    DisplayHelp();
    
}

// Unused functions:
std::string SymTabToString(const asem::SymbolTable & st) {
    
    return (st.toString() + "\n\n");
    
}

std::string RelocRecordsToString(const asem::ELFHolder & eh) {
    
    std::string rv;
    
    for (size_t i = 0; i < 4; i += 1) {
        
        std::string temp = eh.rrToString(SECTIONS[i]);
        
        if (temp.empty()) continue;
        
        rv += temp;
        rv += "\n\n";
         
    }
    
    return rv;
    
}

std::string SectionsToString(const asem::ELFHolder & eh) {
    
    std::string rv;
    
    for (size_t i = 0; i < 4; i += 1) {
        
        std::string temp = eh.secToString(SECTIONS[i]);
        
        if (temp.empty()) continue;
        
        rv += temp;
        rv += "\n\n";
         
    }
    
    return rv;
    
}

#define EXIT(val) do { rv = val; goto END_PROGRAM; } while (0)

int main(int argc, char** argv) {

    //const char * path = "/home/etf/Desktop/init_out.se";
    
    const char * path_in  = nullptr;
    
    bool flag_netbeans = false;
    bool flag_debug    = false;
    
    std::cout << argc << "\n";
    
    // Main arguments:
    if (argc < 2) {
        
        std::cout << "Too few arguments.\n";
        DisplayHelp();
        return 1;
        
    }

    if (argc == 2 && strcmp(argv[1], "info") == 0) {
        DisplayInfo();
        return 0;
    }
    
    path_in = argv[1];
    
    // Optional flags:
    for (int i = 2; i < argc; i += 1) {
        
        if (strcmp(argv[i], "netbeans") == 0) {
            flag_netbeans = true;
            continue;
        }
        
        if (strcmp(argv[i], "debug") == 0) {
            flag_debug = true;
            continue;
        }
        
        if (strcmp(argv[i], "info") == 0) {
            std::cout << "Flag [info] is ignored unless it's the first and only argument.";
            continue;
        }
        
        std::cout << "Unknown flag [" << argv[i] << "]\n.";
        
        return 1;
        
    }
    
    std::cout << "Running...\n";
    
    // Initialize NCURSES:
    initscr();
    cbreak();
    noecho();
    if (flag_netbeans) getch();
    scrollok(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    
    // Initialize other:
    int rv = 0;
    
    asem::ELFHolder eh{};
    
    vm87::Runtime rt{};
    
    try {
        
        eh.loadFromFile(path_in);
        
        rt.loadFromELF(eh, /* cs */ true); // CS must be true!!!
        
        rt.runProgram(/* do_debug */ flag_debug);
        
    } catch (vm87::LoadError & ex) {
        
        cprint("Loading error: %s\n\n", ex.what());
        
        EXIT(1);
        
    } catch (vm87::UnrecError & ex) {
        
        cprint("Unrecoverable error: %s\n\n", ex.what());
        
        EXIT(1);
        
    } catch (vm87::ViolationError & ex) {
        
        cprint("Unhandled violation error: %s\n\n", ex.what());
        
        EXIT(1);
        
    } catch (std::exception & ex) {
        
        cprint("Unnamed exception caught: %s\n\n", ex.what());
        
        EXIT(1);
        
    } catch (...) {
       
        cprint("Unknown exception caught: %s\n\n", "(no message available).");
        
        EXIT(1);
        
    }
    
    END_PROGRAM:
    
    if (rv == 0)
        cprint("\nProgram finished. Press ENTER to continue...\n");
    else
        cprint("\nProgram crashed. Press ENTER to continue...\n");
    
    while (getch() != '\n') continue;

    // Clean up NCURSES:
    endwin();
    
    return rv;
    
}

