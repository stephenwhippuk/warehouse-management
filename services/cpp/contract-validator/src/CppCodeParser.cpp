#include "contract_validator/CppCodeParser.hpp"
#include "contract_validator/Logger.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>

namespace contract_validator {

std::vector<CppCodeParser::ClassInfo> CppCodeParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::error("Failed to open file for parsing: {}", filePath);
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseCode(buffer.str());
}

std::vector<CppCodeParser::ClassInfo> CppCodeParser::parseCode(const std::string& code) {
    std::vector<ClassInfo> classes;
    
    // Remove comments first
    std::string cleanCode = removeComments(code);
    
    // Find all class definitions
    std::vector<std::string> classDefinitions = findClassDefinitions(cleanCode);
    
    for (const auto& classDef : classDefinitions) {
        ClassInfo info;
        
        // Extract class name
        auto [className, fullName] = extractClassName(classDef);
        info.name = className;
        info.fullName = fullName;
        
        // Extract member variables
        info.members = extractMemberVariables(classDef);
        
        // Extract toJson method
        info.toJson = extractToJsonMethod(classDef);
        
        // Extract fromJson method
        info.fromJson = extractFromJsonMethod(classDef);
        
        classes.push_back(info);
    }
    
    return classes;
}

std::optional<CppCodeParser::ClassInfo> CppCodeParser::findClass(
    const std::vector<ClassInfo>& classes,
    const std::string& className) {
    
    for (const auto& cls : classes) {
        if (cls.name == className || cls.fullName == className) {
            return cls;
        }
    }
    return std::nullopt;
}

std::string CppCodeParser::removeComments(const std::string& code) {
    std::string result;
    result.reserve(code.size());
    
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inString = false;
    bool escaped = false;
    
    for (size_t i = 0; i < code.size(); ++i) {
        char c = code[i];
        char next = (i + 1 < code.size()) ? code[i + 1] : '\0';
        
        if (inString) {
            result += c;
            if (c == '"' && !escaped) {
                inString = false;
            }
            escaped = (c == '\\' && !escaped);
            continue;
        }
        
        if (inLineComment) {
            if (c == '\n') {
                inLineComment = false;
                result += c;
            }
            continue;
        }
        
        if (inBlockComment) {
            if (c == '*' && next == '/') {
                inBlockComment = false;
                ++i; // Skip the '/'
            }
            continue;
        }
        
        // Check for comment start
        if (c == '/' && next == '/') {
            inLineComment = true;
            ++i;
            continue;
        }
        
        if (c == '/' && next == '*') {
            inBlockComment = true;
            ++i;
            continue;
        }
        
        if (c == '"') {
            inString = true;
        }
        
        result += c;
    }
    
    return result;
}

std::vector<std::string> CppCodeParser::findClassDefinitions(const std::string& code) {
    std::vector<std::string> definitions;
    
    // Regex to match: class ClassName { ... }
    // This is a simplified approach - we'll extract the full class body
    std::regex classRegex(R"(class\s+(\w+)\s*(?::\s*[^{]*)?\{)");
    
    std::sregex_iterator iter(code.begin(), code.end(), classRegex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        size_t classStart = iter->position();
        size_t bracePos = iter->position() + iter->str().size() - 1; // Position of '{'
        
        size_t braceEnd = findMatchingBrace(code, bracePos);
        if (braceEnd != std::string::npos) {
            std::string classDef = code.substr(classStart, braceEnd - classStart + 1);
            definitions.push_back(classDef);
        }
    }
    
    return definitions;
}

size_t CppCodeParser::findMatchingBrace(const std::string& code, size_t openBracePos) {
    int braceCount = 1;
    bool inString = false;
    bool escaped = false;
    
    for (size_t i = openBracePos + 1; i < code.size(); ++i) {
        char c = code[i];
        
        if (inString) {
            if (c == '"' && !escaped) {
                inString = false;
            }
            escaped = (c == '\\' && !escaped);
            continue;
        }
        
        if (c == '"') {
            inString = true;
            continue;
        }
        
        if (c == '{') {
            ++braceCount;
        } else if (c == '}') {
            --braceCount;
            if (braceCount == 0) {
                return i;
            }
        }
    }
    
    return std::string::npos;
}

std::pair<std::string, std::string> CppCodeParser::extractClassName(const std::string& classDefinition) {
    // Extract: class ClassName [: inheritance] {
    std::regex classRegex(R"(class\s+(\w+))");
    std::smatch match;
    
    if (std::regex_search(classDefinition, match, classRegex)) {
        std::string className = match[1].str();
        
        // Try to find namespace (look backwards from class definition)
        // For now, just return className
        return {className, className};
    }
    
    return {"", ""};
}

std::vector<CppCodeParser::MemberVariable> CppCodeParser::extractMemberVariables(const std::string& classCode) {
    std::vector<MemberVariable> members;
    
    // Find private section (most member variables are private)
    std::regex privateRegex(R"(private\s*:)");
    std::smatch match;
    
    std::string searchCode = classCode;
    size_t privateStart = 0;
    
    if (std::regex_search(classCode, match, privateRegex)) {
        privateStart = match.position() + match.str().size();
        searchCode = classCode.substr(privateStart);
    }
    
    // Match member variables: type name_;
    // Handles: std::string id_;
    //          std::optional<std::string> batchNumber_;
    //          int quantity_ = 0;
    std::regex memberRegex(R"((?:^|\n)\s*((?:std::)?(?:\w+(?:::\w+)*(?:<[^>]+>)?)\s+(\w+_)\s*(?:=\s*([^;]+))?\s*;))");
    
    std::sregex_iterator iter(searchCode.begin(), searchCode.end(), memberRegex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        MemberVariable member;
        std::string fullType = (*iter)[1].str();
        member.name = (*iter)[2].str();
        
        // Extract just the type part (before the variable name)
        size_t varNamePos = fullType.find(member.name);
        if (varNamePos != std::string::npos) {
            member.type = trim(fullType.substr(0, varNamePos));
        } else {
            member.type = fullType;
        }
        
        member.isOptional = isOptionalType(member.type);
        
        if (iter->size() > 3) {
            member.defaultValue = trim((*iter)[3].str());
        }
        
        members.push_back(member);
    }
    
    return members;
}

std::optional<CppCodeParser::ToJsonMethod> CppCodeParser::extractToJsonMethod(const std::string& classCode) {
    auto methodBody = extractMethodBody(classCode, "toJson");
    if (!methodBody) {
        return std::nullopt;
    }
    
    ToJsonMethod method;
    method.rawCode = *methodBody;
    method.fields = parseJsonFieldMappings(*methodBody);
    
    // Check if uses return { ... } style
    method.usesReturn = (methodBody->find("return") != std::string::npos &&
                         methodBody->find("json j") == std::string::npos);
    
    return method;
}

std::optional<CppCodeParser::FromJsonMethod> CppCodeParser::extractFromJsonMethod(const std::string& classCode) {
    auto methodBody = extractMethodBody(classCode, "fromJson");
    if (!methodBody) {
        return std::nullopt;
    }
    
    FromJsonMethod method;
    method.rawCode = *methodBody;
    // fromJson parsing can be added later if needed
    
    return method;
}

std::optional<std::string> CppCodeParser::extractMethodBody(const std::string& classCode,
                                                            const std::string& methodName) {
    // Match: json toJson() const {
    //        json toJson() {
    //        static Type fromJson(const json& j) {
    std::string pattern = methodName + R"(\s*\([^)]*\)\s*(?:const)?\s*\{)";
    std::regex methodRegex(pattern);
    std::smatch match;
    
    if (std::regex_search(classCode, match, methodRegex)) {
        size_t methodStart = match.position();
        size_t bracePos = match.position() + match.str().size() - 1;
        
        size_t braceEnd = findMatchingBrace(classCode, bracePos);
        if (braceEnd != std::string::npos) {
            // Extract just the body (without the opening brace)
            return classCode.substr(bracePos + 1, braceEnd - bracePos - 1);
        }
    }
    
    return std::nullopt;
}

std::vector<CppCodeParser::JsonFieldMapping> CppCodeParser::parseJsonFieldMappings(const std::string& methodBody) {
    std::vector<JsonFieldMapping> mappings;
    
    // Pattern 1: return { {"key", value}, ... }
    // Pattern 2: json j = { {"key", value}, ... }
    // Pattern 3: j["key"] = value;
    
    // Try to find return statement with initializer list
    std::regex returnRegex(R"regex(return\s*\{([^}]+)\})regex");
    std::smatch returnMatch;
    
    if (std::regex_search(methodBody, returnMatch, returnRegex)) {
        std::string initList = returnMatch[1].str();
        
        // Parse {"key", value} pairs
        std::regex pairRegex(R"regex(\{\s*"([^"]+)"\s*,\s*([^}]+)\s*\})regex");
        std::sregex_iterator iter(initList.begin(), initList.end(), pairRegex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            JsonFieldMapping mapping;
            mapping.jsonKey = (*iter)[1].str();
            mapping.expression = trim((*iter)[2].str());
            
            // Extract member variable name
            mapping.memberVar = extractMemberVarFromExpression(mapping.expression);
            mapping.isOptional = (mapping.expression.find("if") != std::string::npos ||
                                  mapping.expression.find("?") != std::string::npos);
            
            mappings.push_back(mapping);
        }
    } else {
        // Try j["key"] = value; pattern
        std::regex assignRegex(R"regex(j\s*\[\s*"([^"]+)"\s*\]\s*=\s*([^;]+)\s*;)regex");
        std::sregex_iterator iter(methodBody.begin(), methodBody.end(), assignRegex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            JsonFieldMapping mapping;
            mapping.jsonKey = (*iter)[1].str();
            mapping.expression = trim((*iter)[2].str());
            mapping.memberVar = extractMemberVarFromExpression(mapping.expression);
            mapping.isOptional = false;
            
            mappings.push_back(mapping);
        }
    }
    
    // Handle optional fields: if (field_) j["key"] = *field_;
    std::regex optionalRegex(R"regex(if\s*\(\s*(\w+_)\s*\)\s*(?:j\s*\[\s*"([^"]+)"\s*\]\s*=\s*\*(\w+_)|.*\{\s*"([^"]+)"\s*,\s*\*(\w+_)\s*\}))regex");
    std::sregex_iterator optIter(methodBody.begin(), methodBody.end(), optionalRegex);
    std::sregex_iterator optEnd;
    
    for (; optIter != optEnd; ++optIter) {
        JsonFieldMapping mapping;
        mapping.memberVar = (*optIter)[1].str();
        
        if ((*optIter)[2].matched) {
            // j["key"] = *field_ pattern
            mapping.jsonKey = (*optIter)[2].str();
        } else if ((*optIter)[4].matched) {
            // {"key", *field_} pattern
            mapping.jsonKey = (*optIter)[4].str();
        }
        
        mapping.expression = "*" + mapping.memberVar;
        mapping.isOptional = true;
        
        // Check if we already have this mapping
        bool found = false;
        for (auto& existing : mappings) {
            if (existing.jsonKey == mapping.jsonKey) {
                existing.isOptional = true;
                found = true;
                break;
            }
        }
        
        if (!found) {
            mappings.push_back(mapping);
        }
    }
    
    return mappings;
}

std::string CppCodeParser::extractMemberVarFromExpression(const std::string& expression) {
    // Handle: id_, *batchNumber_, getId(), toStatusString(status_)
    
    // Remove dereference
    std::string expr = expression;
    if (!expr.empty() && expr[0] == '*') {
        expr = expr.substr(1);
    }
    
    // Trim whitespace
    expr = trim(expr);
    
    // If it's a function call, extract the argument
    if (expr.find('(') != std::string::npos) {
        std::regex funcRegex(R"(\w+\s*\(\s*(\w+_)\s*\))");
        std::smatch match;
        if (std::regex_search(expr, match, funcRegex)) {
            return match[1].str();
        }
        return ""; // Unknown pattern
    }
    
    // Otherwise it should be a member variable
    if (expr.find('_') != std::string::npos) {
        return expr;
    }
    
    return "";
}

bool CppCodeParser::isOptionalType(const std::string& type) {
    return type.find("std::optional") != std::string::npos ||
           type.find("optional<") != std::string::npos;
}

std::string CppCodeParser::unwrapOptionalType(const std::string& type) {
    std::regex optionalRegex(R"((?:std::)?optional\s*<\s*([^>]+)\s*>)");
    std::smatch match;
    
    if (std::regex_search(type, match, optionalRegex)) {
        return trim(match[1].str());
    }
    
    return type;
}

std::string CppCodeParser::normalizeType(const std::string& type) {
    std::string normalized = type;
    
    // Remove const
    size_t constPos = normalized.find("const");
    while (constPos != std::string::npos) {
        normalized.erase(constPos, 5);
        constPos = normalized.find("const");
    }
    
    // Remove & and *
    normalized.erase(std::remove(normalized.begin(), normalized.end(), '&'), normalized.end());
    normalized.erase(std::remove(normalized.begin(), normalized.end(), '*'), normalized.end());
    
    // Trim whitespace
    return trim(normalized);
}

std::string CppCodeParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::vector<std::string> CppCodeParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    
    return tokens;
}

} // namespace contract_validator
