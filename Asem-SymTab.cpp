
#include "Asem-SymTab.hpp"
#include "Asem-Func.hpp"

#include <stdexcept>
#include <unordered_map>

#include "StringUtil.hpp"

using namespace gen;

namespace asem {
    
SymbolTable::SymbolTable() {

    reset();
    
    }

void SymbolTable::reset() {
    
    data.clear();
    counter[0] = counter[1] = counter[2] = counter[3] = size_t(0);
    curr_section = Section::Undefined;
    
}

int SymbolTable::check(const std::string & symbol_name) const {
    
    auto iter = data.find(symbol_name);
    
    if (iter == data.end()) return NOT_PRESENT;
    
    if ((*iter).second.defined == false) return UNDEFINED;
    
    return DEFINED;
    
}

void SymbolTable::toOrderedVector(std::vector<std::pair<std::string,SymbolTableEntry>> & vec) const {
    
    vec.resize(data.size());
    
    for (auto pair : data) {
        
        size_t index = pair.second.ordinal;
        
        vec[index].first = pair.first;
        
        vec[index].second = pair.second;
        
    }
    
}

ExpressionValue SymbolTable::eval(size_t line_ord, const std::string & expr) const {
    
    if (expr.empty()) throw std::logic_error("asem::AssemblyList::eval(...) - "
                                             "Expression is an empty string.");
    
    int offsets[4] = { 0, 0, 0, 0};
    int sign = 1;
    
    std::vector<std::string> tokens;
    std::vector<int> numbers;
    
    string_tokenize_vec_multi(expr, "+-", tokens, true);
    
    numbers.resize( tokens.size() );
    
    size_t cnt = 0;
    for (std::string & s : tokens) {
        
        s = string_crop(s);
        
        if (cnt % 2 == 0) { // Even: Should be number
            
            int i;
            bool is_num = false;
            
            if (string_is_integer(s)) {
            
                try {
                    i = std::stoi(s, 0, 0);
                    is_num = true;
                } catch (...) {
                    is_num = false;
                }
                
            }
            
            if (!is_num) {
                
                if  (!(string_is_identifier(s) && !char_is_digit(s[0])))
                    throw std::logic_error("Cannot evaluate token [" + s + "] in expression (not an identifier).");
                
                if (check(s) != DEFINED)
                    throw std::logic_error("Cannot evaluate token [" + s + "] in expression (undefined).");
                  
                i = data.at(s).value;
                
                offsets[ data.at(s).section ] += sign;
                
            }
            
            numbers[cnt] = i;
            
            //std::cout << "Eval: numbers[" << cnt << "] = " << i << "\n";
            
        } else { // Odd: Should be operator
            
            if (s != "+" && s != "-")
                throw std::logic_error("Operator expected, [" + s + "] found.");
            
            switch (s[0]) {
                
                case '+':
                    sign = 1;
                    break;                    
                    
                case '-':
                    sign = -1;
                    break;
                
            }
            
            //std::cout << "Eval: op[" << cnt << "] = " << s << "\n";
            
        }
        
        cnt += 1;
        
    }
    
    // Get value:
    int res = numbers[0];
    
    for (size_t i = 2; i < tokens.size(); i += 2) {
        
        int mul;
        
        if (tokens[i - 1] == "+")
            mul = 1;
        else
            mul = -1;
        
        res += numbers[i] * mul;
        
    }
    
    // Get relative_to:
    Section::Enum relative_to = Section::Undefined;
    
    if (!offsets[Section::Text] && !offsets[Section::Data] && // No relation
        !offsets[Section::BSS]  && !offsets[Section::ROData]) {
        
        return ExpressionValue(res, Section::Undefined, true);
        
    }
    
    if ( offsets[Section::Text] && !offsets[Section::Data] && // Text
        !offsets[Section::BSS]  && !offsets[Section::ROData]) {
        
        if (offsets[Section::Text] != 1) goto RELATIVE_TO_ERROR;
        
        return ExpressionValue(res, Section::Text);
        
    }
    
    if (!offsets[Section::Text] &&  offsets[Section::Data] && // Data
        !offsets[Section::BSS]  && !offsets[Section::ROData]) {
        
        if (offsets[Section::Data] != 1) goto RELATIVE_TO_ERROR;
        
        return ExpressionValue(res, Section::Data);
        
    }
    
    if (!offsets[Section::Text] && !offsets[Section::Data] && // BSS
         offsets[Section::BSS]  && !offsets[Section::ROData]) {
        
        if (offsets[Section::BSS] != 1) goto RELATIVE_TO_ERROR;
        
        return ExpressionValue(res, Section::BSS);
        
    }
    
    if (!offsets[Section::Text] && !offsets[Section::Data] && // ROData
        !offsets[Section::BSS]  &&  offsets[Section::ROData]) {
        
        if (offsets[Section::ROData] != 1) goto RELATIVE_TO_ERROR;
        
        return ExpressionValue(res, Section::ROData);
        
    }
    
    RELATIVE_TO_ERROR:
    
    throw std::logic_error( "Expression value must have a positive offset "
                            "to at most one section.");
    
}

void SymbolTable::verify() const {
    
    for (const std::pair<std::string, SymbolTableEntry> & pair : data) {
        
        if (pair.second.scope != Scope::Global && pair.second.defined == false) {
            
            if (pair.second.ordinal == 0) continue; // NULL Symbol
            
            throw std::logic_error("\b\b\b<unknown>: Symbol [" + pair.first + "] used but not defined or imported.");
            
        }
        
    }
    
}

std::string SymbolTable::toString() const {
    
    char buffer[200];
    
    std::vector<std::pair<std::string, asem::SymbolTableEntry>> st_vec;
    
    toOrderedVector(st_vec);
    
    std::string rv{"#.symtab\n"};
    
    snprintf(buffer, sizeof(buffer)
            , "#%3s   %-16s   %5s   %-7s   %-6s   %-7s \n"
            , "Ord"
            , "Symbol name"
            , "Value"
            , "Section"
            , "Scope"
            , "Rel.To"
            ) ;
    
    rv += buffer;
    
    rv += "#==========================================================\n";
    
    for (auto & pair : st_vec) {
        
        snprintf(buffer, sizeof(buffer)
                , " %3d   %-16s   %5s   %-7s   %-6s   %-7s \n"
                , pair.second.ordinal
                , pair.first.c_str()
                , pair.second.defined ? (std::to_string(pair.second.value).c_str()) : ("?")
                , SectionToString( pair.second.section )
                , ScopeToString( pair.second.scope )
                , SectionToString( pair.second.relative_to )
                ) ;
        
        rv += buffer;
        
    }
    
    rv += "#==========================================================";
    
    return rv;
    
}
    
}
