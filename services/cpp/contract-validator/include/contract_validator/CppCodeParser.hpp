#ifndef CONTRACT_VALIDATOR_CPPCODEPARSER_HPP
#define CONTRACT_VALIDATOR_CPPCODEPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace contract_validator {

/**
 * @brief Parses C++ code to extract model class information
 * 
 * Focuses on extracting:
 * - Class definitions and member variables
 * - toJson() method implementations
 * - Field mappings in JSON serialization
 * - fromJson() method implementations
 * 
 * This is a focused parser for contract validation, not a full C++ AST parser.
 */
class CppCodeParser {
public:
    struct MemberVariable {
        std::string name;
        std::string type;
        bool isOptional;  // std::optional<T>
        std::string defaultValue;
    };

    struct JsonFieldMapping {
        std::string jsonKey;      // The key in the JSON object
        std::string memberVar;    // The C++ member variable
        std::string expression;   // Full expression (e.g., "id_", "*optionalField_", "getMethod()")
        bool isOptional;          // If wrapped in optional check
    };

    struct ToJsonMethod {
        std::vector<JsonFieldMapping> fields;
        bool usesReturn;          // Uses return { ... } vs json j; j["x"] = ...
        std::string rawCode;      // Original method body
    };

    struct FromJsonMethod {
        std::vector<JsonFieldMapping> fields;
        std::string rawCode;
    };

    struct ClassInfo {
        std::string name;
        std::string fullName;     // With namespace
        std::vector<MemberVariable> members;
        std::optional<ToJsonMethod> toJson;
        std::optional<FromJsonMethod> fromJson;
        std::vector<std::string> inheritedClasses;
    };

    /**
     * @brief Parse a C++ source or header file
     * @param filePath Path to the file to parse
     * @return List of classes found in the file
     */
    static std::vector<ClassInfo> parseFile(const std::string& filePath);

    /**
     * @brief Parse C++ code from a string
     * @param code C++ source code
     * @return List of classes found
     */
    static std::vector<ClassInfo> parseCode(const std::string& code);

    /**
     * @brief Find a class by name in parsed results
     * @param classes List of parsed classes
     * @param className Name to search for
     * @return Class info if found
     */
    static std::optional<ClassInfo> findClass(const std::vector<ClassInfo>& classes,
                                              const std::string& className);

    /**
     * @brief Extract toJson method from a class definition
     * @param classCode Code containing the class
     * @return Parsed toJson method if found
     */
    static std::optional<ToJsonMethod> extractToJsonMethod(const std::string& classCode);

    /**
     * @brief Extract fromJson method from a class definition
     * @param classCode Code containing the class
     * @return Parsed fromJson method if found
     */
    static std::optional<FromJsonMethod> extractFromJsonMethod(const std::string& classCode);

    /**
     * @brief Extract member variables from a class
     * @param classCode Code containing the class
     * @return List of member variables
     */
    static std::vector<MemberVariable> extractMemberVariables(const std::string& classCode);

    /**
     * @brief Parse JSON field assignments in toJson method
     * @param methodBody Body of toJson method
     * @return Field mappings
     */
    static std::vector<JsonFieldMapping> parseJsonFieldMappings(const std::string& methodBody);

    /**
     * @brief Clean C++ code by removing comments
     * @param code C++ code
     * @return Code without comments
     */
    static std::string removeComments(const std::string& code);

    /**
     * @brief Extract class name and namespace
     * @param classDefinition Class definition string
     * @return Pair of (className, fullNameWithNamespace)
     */
    static std::pair<std::string, std::string> extractClassName(const std::string& classDefinition);

    /**
     * @brief Check if a member variable type is std::optional
     * @param type Type string
     * @return True if optional type
     */
    static bool isOptionalType(const std::string& type);

    /**
     * @brief Extract the inner type from std::optional<T>
     * @param type Optional type string
     * @return Inner type T
     */
    static std::string unwrapOptionalType(const std::string& type);

    /**
     * @brief Normalize type name (remove const, &, whitespace)
     * @param type Type string
     * @return Normalized type
     */
    static std::string normalizeType(const std::string& type);

private:
    /**
     * @brief Find all class definitions in code
     * @param code C++ code
     * @return List of class definition strings
     */
    static std::vector<std::string> findClassDefinitions(const std::string& code);

    /**
     * @brief Extract method body by name
     * @param classCode Class code
     * @param methodName Method name (e.g., "toJson")
     * @return Method body if found
     */
    static std::optional<std::string> extractMethodBody(const std::string& classCode,
                                                        const std::string& methodName);

    /**
     * @brief Find matching brace for a given open brace position
     * @param code Code string
     * @param openBracePos Position of opening brace
     * @return Position of closing brace
     */
    static size_t findMatchingBrace(const std::string& code, size_t openBracePos);

public:
    /**
     * @brief Trim whitespace from string
     */
    static std::string trim(const std::string& str);
    
private:

    /**
     * @brief Split string by delimiter
     */
    static std::vector<std::string> split(const std::string& str, char delimiter);

    /**
     * @brief Extract member variable name from an expression
     * @param expression C++ expression (e.g., "id_", "*batchNumber_", "getId()")
     * @return Member variable name
     */
    static std::string extractMemberVarFromExpression(const std::string& expression);
};

} // namespace contract_validator

#endif // CONTRACT_VALIDATOR_CPPCODEPARSER_HPP
