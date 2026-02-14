#include "product/utils/SwaggerGenerator.hpp"

namespace product::utils {

json SwaggerGenerator::generateSpec(const std::string& version) {
    json spec = {
        {"openapi", "3.0.0"},
        {"info", {
            {"title", "Product Service API"},
            {"version", version},
            {"description", "Product master data management service"}
        }},
        {"servers", json::array({
            {{"url", "http://localhost:8082"}, {"description", "Development server"}}
        })},
        {"paths", {
            {"/api/v1/products", {
                {"get", {
                    {"summary", "List all products"},
                    {"operationId", "listProducts"},
                    {"parameters", json::array({
                        {
                            {"name", "page"},
                            {"in", "query"},
                            {"schema", {{"type", "integer"}, {"default", 1}}}
                        },
                        {
                            {"name", "pageSize"},
                            {"in", "query"},
                            {"schema", {{"type", "integer"}, {"default", 50}}}
                        }
                    })},
                    {"responses", {
                        {"200", {
                            {"description", "List of products"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ProductListDto"}}}
                            }}}}
                        }},
                        {"500", {
                            {"description", "Internal server error"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }}
                    }}
                }},
                {"post", {
                    {"summary", "Create a new product"},
                    {"operationId", "createProduct"},
                    {"requestBody", {
                        {"required", true},
                        {"content", {{"application/json", {
                            {"schema", {{"$ref", "#/components/schemas/CreateProductRequest"}}}
                        }}}}
                    }},
                    {"responses", {
                        {"201", {
                            {"description", "Product created"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ProductItemDto"}}}
                            }}}}
                        }},
                        {"400", {
                            {"description", "Invalid request"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }},
                        {"500", {
                            {"description", "Internal server error"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }}
                    }}
                }}
            }},
            {"/api/v1/products/{id}", {
                {"get", {
                    {"summary", "Get product by ID"},
                    {"operationId", "getProductById"},
                    {"parameters", json::array({
                        {
                            {"name", "id"},
                            {"in", "path"},
                            {"required", true},
                            {"schema", {{"type", "string"}, {"format", "uuid"}}}
                        }
                    })},
                    {"responses", {
                        {"200", {
                            {"description", "Product found"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ProductItemDto"}}}
                            }}}}
                        }},
                        {"404", {
                            {"description", "Product not found"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }},
                        {"500", {
                            {"description", "Internal server error"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }}
                    }}
                }},
                {"put", {
                    {"summary", "Update product"},
                    {"operationId", "updateProduct"},
                    {"parameters", json::array({
                        {
                            {"name", "id"},
                            {"in", "path"},
                            {"required", true},
                            {"schema", {{"type", "string"}, {"format", "uuid"}}}
                        }
                    })},
                    {"requestBody", {
                        {"required", true},
                        {"content", {{"application/json", {
                            {"schema", {{"$ref", "#/components/schemas/UpdateProductRequest"}}}
                        }}}}
                    }},
                    {"responses", {
                        {"200", {
                            {"description", "Product updated"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ProductItemDto"}}}
                            }}}}
                        }},
                        {"400", {
                            {"description", "Invalid request"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }},
                        {"404", {
                            {"description", "Product not found"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }},
                        {"500", {
                            {"description", "Internal server error"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }}
                    }}
                }},
                {"delete", {
                    {"summary", "Delete product"},
                    {"operationId", "deleteProduct"},
                    {"parameters", json::array({
                        {
                            {"name", "id"},
                            {"in", "path"},
                            {"required", true},
                            {"schema", {{"type", "string"}, {"format", "uuid"}}}
                        }
                    })},
                    {"responses", {
                        {"204", {
                            {"description", "Product deleted"}
                        }},
                        {"404", {
                            {"description", "Product not found"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }},
                        {"500", {
                            {"description", "Internal server error"},
                            {"content", {{"application/json", {
                                {"schema", {{"$ref", "#/components/schemas/ErrorDto"}}}
                            }}}}
                        }}
                    }}
                }}
            }},
            {"/health", {
                {"get", {
                    {"summary", "Health check"},
                    {"operationId", "health"},
                    {"responses", {
                        {"200", {
                            {"description", "Service is healthy"},
                            {"content", {{"application/json", {
                                {"schema", {{
                                    {"type", "object"},
                                    {"properties", {
                                        {"status", {{"type", "string"}}},
                                        {"timestamp", {{"type", "string"}}}
                                    }}
                                }}}
                            }}}}
                        }}
                    }}
                }}
            }}
        }},
        {"components", {
            {"schemas", {
                {"ProductItemDto", {
                    {"type", "object"},
                    {"required", {"id", "sku", "name", "status"}},
                    {"properties", {
                        {"id", {{"type", "string"}, {"format", "uuid"}}},
                        {"sku", {{"type", "string"}}},
                        {"name", {{"type", "string"}}},
                        {"description", {{"type", "string"}, {"nullable", true}}},
                        {"category", {{"type", "string"}, {"nullable", true}}},
                        {"status", {{"type", "string"}, {"enum", {"active", "inactive", "discontinued"}}}}
                    }}
                }},
                {"ProductListDto", {
                    {"type", "object"},
                    {"required", {"items", "totalCount", "page", "pageSize", "totalPages"}},
                    {"properties", {
                        {"items", {
                            {"type", "array"},
                            {"items", {{"$ref", "#/components/schemas/ProductItemDto"}}}
                        }},
                        {"totalCount", {{"type", "integer"}}},
                        {"page", {{"type", "integer"}}},
                        {"pageSize", {{"type", "integer"}}},
                        {"totalPages", {{"type", "integer"}}}
                    }}
                }},
                {"CreateProductRequest", {
                    {"type", "object"},
                    {"required", {"sku", "name"}},
                    {"properties", {
                        {"sku", {{"type", "string"}, {"pattern", "^[A-Z0-9-]+$"}}},
                        {"name", {{"type", "string"}}},
                        {"description", {{"type", "string"}, {"nullable", true}}},
                        {"category", {{"type", "string"}, {"nullable", true}}}
                    }}
                }},
                {"UpdateProductRequest", {
                    {"type", "object"},
                    {"required", {"name", "status"}},
                    {"properties", {
                        {"name", {{"type", "string"}}},
                        {"description", {{"type", "string"}, {"nullable", true}}},
                        {"category", {{"type", "string"}, {"nullable", true}}},
                        {"status", {{"type", "string"}, {"enum", {"active", "inactive", "discontinued"}}}}
                    }}
                }},
                {"ErrorDto", {
                    {"type", "object"},
                    {"required", {"error", "message"}},
                    {"properties", {
                        {"error", {{"type", "string"}}},
                        {"message", {{"type", "string"}}},
                        {"details", {{"type", "string"}, {"nullable", true}}}
                    }}
                }}
            }}
        }}
    };
    
    return spec;
}

}  // namespace product::utils
