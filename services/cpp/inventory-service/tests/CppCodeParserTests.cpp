#include <catch2/catch_all.hpp>
#include "contract_validator/CppCodeParser.hpp"
#include <iostream>

using namespace contract_validator;

TEST_CASE("CppCodeParser removes comments", "[parser][comments]") {
    SECTION("Remove line comments") {
        std::string code = R"(
            int x = 5; // This is a comment
            std::string s = "value"; // Another comment
        )";
        
        std::string clean = CppCodeParser::removeComments(code);
        REQUIRE(clean.find("This is a comment") == std::string::npos);
        REQUIRE(clean.find("int x = 5;") != std::string::npos);
    }
    
    SECTION("Remove block comments") {
        std::string code = R"(
            /* This is a
               block comment */
            int x = 5;
            /* Another block */
        )";
        
        std::string clean = CppCodeParser::removeComments(code);
        REQUIRE(clean.find("block comment") == std::string::npos);
        REQUIRE(clean.find("int x = 5;") != std::string::npos);
    }
    
    SECTION("Preserve strings with comment-like content") {
        std::string code = R"(
            std::string url = "http://example.com";
            std::string msg = "Use /* this */";
        )";
        
        std::string clean = CppCodeParser::removeComments(code);
        REQUIRE(clean.find("http://example.com") != std::string::npos);
        REQUIRE(clean.find("Use /* this */") != std::string::npos);
    }
}

TEST_CASE("CppCodeParser extracts member variables", "[parser][members]") {
    std::string classCode = R"(
        class Inventory {
        public:
            std::string getId() const { return id_; }
            
        private:
            std::string id_;
            std::string productId_;
            std::string warehouseId_;
            int quantity_ = 0;
            int reservedQuantity_ = 0;
            std::optional<std::string> batchNumber_;
            std::optional<json> metadata_;
        };
    )";
    
    auto members = CppCodeParser::extractMemberVariables(classCode);
    
    REQUIRE(members.size() >= 5);
    
    // Find specific members
    auto findMember = [&](const std::string& name) {
        for (const auto& m : members) {
            if (m.name == name) return m;
        }
        return CppCodeParser::MemberVariable{};
    };
    
    auto id = findMember("id_");
    REQUIRE(id.name == "id_");
    REQUIRE(id.type.find("string") != std::string::npos);
    REQUIRE_FALSE(id.isOptional);
    
    auto quantity = findMember("quantity_");
    REQUIRE(quantity.name == "quantity_");
    REQUIRE(quantity.type.find("int") != std::string::npos);
    REQUIRE(quantity.defaultValue == "0");
    
    auto batchNumber = findMember("batchNumber_");
    REQUIRE(batchNumber.name == "batchNumber_");
    REQUIRE(batchNumber.isOptional);
}

TEST_CASE("CppCodeParser extracts toJson method", "[parser][tojson]") {
    SECTION("Return style toJson") {
        std::string classCode = R"(
            class Inventory {
            public:
                json toJson() const {
                    return {
                        {"id", id_},
                        {"productId", productId_},
                        {"warehouseId", warehouseId_},
                        {"quantity", quantity_},
                        {"reservedQuantity", reservedQuantity_}
                    };
                }
            private:
                std::string id_;
                std::string productId_;
                std::string warehouseId_;
                int quantity_;
                int reservedQuantity_;
            };
        )";
        
        auto method = CppCodeParser::extractToJsonMethod(classCode);
        REQUIRE(method.has_value());
        REQUIRE(method->usesReturn);
        REQUIRE(method->fields.size() == 5);
        
        // Check field mappings
        REQUIRE(method->fields[0].jsonKey == "id");
        REQUIRE(method->fields[0].memberVar == "id_");
        
        REQUIRE(method->fields[1].jsonKey == "productId");
        REQUIRE(method->fields[1].memberVar == "productId_");
    }
    
    SECTION("Assignment style toJson") {
        std::string classCode = R"(
            class Inventory {
            public:
                json toJson() const {
                    json j;
                    j["id"] = id_;
                    j["productId"] = productId_;
                    j["quantity"] = quantity_;
                    return j;
                }
            private:
                std::string id_;
                std::string productId_;
                int quantity_;
            };
        )";
        
        auto method = CppCodeParser::extractToJsonMethod(classCode);
        REQUIRE(method.has_value());
        REQUIRE_FALSE(method->usesReturn);
        REQUIRE(method->fields.size() == 3);
    }
    
    SECTION("toJson with optional fields") {
        std::string classCode = R"(
            class Inventory {
            public:
                json toJson() const {
                    json j = {
                        {"id", id_},
                        {"quantity", quantity_}
                    };
                    
                    if (batchNumber_) {
                        j["batchNumber"] = *batchNumber_;
                    }
                    
                    if (metadata_) {
                        j["metadata"] = *metadata_;
                    }
                    
                    return j;
                }
            private:
                std::string id_;
                int quantity_;
                std::optional<std::string> batchNumber_;
                std::optional<json> metadata_;
            };
        )";
        
        auto method = CppCodeParser::extractToJsonMethod(classCode);
        REQUIRE(method.has_value());
        
        // Should find required and optional fields
        REQUIRE(method->fields.size() >= 2);
        
        // Find the optional fields
        bool foundBatchNumber = false;
        bool foundMetadata = false;
        
        for (const auto& field : method->fields) {
            if (field.jsonKey == "batchNumber") {
                foundBatchNumber = true;
                REQUIRE(field.isOptional);
                REQUIRE(field.memberVar == "batchNumber_");
            }
            if (field.jsonKey == "metadata") {
                foundMetadata = true;
                REQUIRE(field.isOptional);
                REQUIRE(field.memberVar == "metadata_");
            }
        }
        
        REQUIRE(foundBatchNumber);
        REQUIRE(foundMetadata);
    }
}

TEST_CASE("CppCodeParser parses full class", "[parser][class]") {
    std::string code = R"(
        namespace inventory {
        namespace models {
        
        class Inventory {
        public:
            Inventory() = default;
            
            std::string getId() const { return id_; }
            void setId(const std::string& id) { id_ = id; }
            
            int getQuantity() const { return quantity_; }
            void setQuantity(int quantity) { quantity_ = quantity; }
            
            json toJson() const {
                return {
                    {"id", id_},
                    {"productId", productId_},
                    {"quantity", quantity_},
                    {"reservedQuantity", reservedQuantity_}
                };
            }
            
            static Inventory fromJson(const json& j) {
                Inventory inv;
                inv.id_ = j["id"];
                inv.productId_ = j["productId"];
                inv.quantity_ = j["quantity"];
                inv.reservedQuantity_ = j["reservedQuantity"];
                return inv;
            }
            
        private:
            std::string id_;
            std::string productId_;
            int quantity_ = 0;
            int reservedQuantity_ = 0;
        };
        
        } // namespace models
        } // namespace inventory
    )";
    
    auto classes = CppCodeParser::parseCode(code);
    REQUIRE(classes.size() == 1);
    
    const auto& cls = classes[0];
    REQUIRE(cls.name == "Inventory");
    REQUIRE(cls.members.size() >= 3);
    REQUIRE(cls.toJson.has_value());
    REQUIRE(cls.fromJson.has_value());
    
    // Verify toJson fields
    const auto& toJson = cls.toJson.value();
    REQUIRE(toJson.fields.size() == 4);
    
    // Verify we found all member variables
    bool hasId = false, hasProductId = false, hasQuantity = false;
    for (const auto& member : cls.members) {
        if (member.name == "id_") hasId = true;
        if (member.name == "productId_") hasProductId = true;
        if (member.name == "quantity_") hasQuantity = true;
    }
    REQUIRE(hasId);
    REQUIRE(hasProductId);
    REQUIRE(hasQuantity);
}

TEST_CASE("CppCodeParser parses actual Inventory model", "[parser][integration]") {
    // This would be a real file path test
    // For now, test with inline code matching our actual model
    
    std::string inventoryCode = R"(
        class Inventory {
        public:
            json toJson() const {
                json j = {
                    {"id", id_},
                    {"productId", productId_},
                    {"warehouseId", warehouseId_},
                    {"locationId", locationId_},
                    {"quantity", quantity_},
                    {"reservedQuantity", reservedQuantity_},
                    {"allocatedQuantity", allocatedQuantity_},
                    {"availableQuantity", getAvailableQuantity()},
                    {"status", toStatusString(status_)},
                    {"createdAt", createdAt_},
                    {"updatedAt", updatedAt_}
                };
                
                if (batchNumber_) j["batchNumber"] = *batchNumber_;
                if (expiryDate_) j["expiryDate"] = *expiryDate_;
                if (metadata_) j["metadata"] = *metadata_;
                
                return j;
            }
            
        private:
            std::string id_;
            std::string productId_;
            std::string warehouseId_;
            std::string locationId_;
            int quantity_;
            int reservedQuantity_;
            int allocatedQuantity_;
            InventoryStatus status_;
            std::string createdAt_;
            std::string updatedAt_;
            std::optional<std::string> batchNumber_;
            std::optional<std::string> expiryDate_;
            std::optional<json> metadata_;
        };
    )";
    
    auto classes = CppCodeParser::parseCode(inventoryCode);
    REQUIRE(classes.size() == 1);
    
    const auto& cls = classes[0];
    REQUIRE(cls.name == "Inventory");
    
    // Should have extracted toJson
    REQUIRE(cls.toJson.has_value());
    const auto& toJson = cls.toJson.value();
    
    // Should have found most fields (at least 10)
    REQUIRE(toJson.fields.size() >= 10);
    
    // Verify required fields are present
    std::vector<std::string> requiredKeys = {
        "id", "productId", "warehouseId", "locationId",
        "quantity", "reservedQuantity", "allocatedQuantity"
    };
    
    for (const auto& key : requiredKeys) {
        bool found = false;
        for (const auto& field : toJson.fields) {
            if (field.jsonKey == key) {
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }
    
    // Verify optional fields
    std::vector<std::string> optionalKeys = {"batchNumber", "expiryDate", "metadata"};
    for (const auto& key : optionalKeys) {
        bool found = false;
        for (const auto& field : toJson.fields) {
            if (field.jsonKey == key) {
                found = true;
                REQUIRE(field.isOptional);
                break;
            }
        }
        REQUIRE(found);
    }
}

TEST_CASE("CppCodeParser utility functions", "[parser][utils]") {
    SECTION("isOptionalType") {
        REQUIRE(CppCodeParser::isOptionalType("std::optional<std::string>"));
        REQUIRE(CppCodeParser::isOptionalType("optional<int>"));
        REQUIRE_FALSE(CppCodeParser::isOptionalType("std::string"));
        REQUIRE_FALSE(CppCodeParser::isOptionalType("int"));
    }
    
    SECTION("unwrapOptionalType") {
        REQUIRE(CppCodeParser::unwrapOptionalType("std::optional<std::string>") == "std::string");
        REQUIRE(CppCodeParser::unwrapOptionalType("optional<int>") == "int");
        REQUIRE(CppCodeParser::unwrapOptionalType("std::string") == "std::string");
    }
    
    SECTION("normalizeType") {
        REQUIRE(CppCodeParser::normalizeType("const std::string&") == "std::string");
        REQUIRE(CppCodeParser::normalizeType("const int*") == "int");
        REQUIRE(CppCodeParser::normalizeType("std::string") == "std::string");
    }
    
    SECTION("trim") {
        REQUIRE(CppCodeParser::trim("  hello  ") == "hello");
        REQUIRE(CppCodeParser::trim("\t\nworld\r\n") == "world");
        REQUIRE(CppCodeParser::trim("test") == "test");
    }
}
