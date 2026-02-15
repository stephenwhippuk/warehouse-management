-- Deploy order-service:002_add_reference_caches to pg
-- requires: 001_initial_schema

BEGIN;

-- Create product_cache table for Product entity references
CREATE TABLE product_cache (
    product_id UUID PRIMARY KEY,
    sku VARCHAR(100) NOT NULL UNIQUE,
    name VARCHAR(200),
    cached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create index on sku for lookup by product identifier
CREATE INDEX idx_product_cache_sku ON product_cache(sku);

-- Create warehouse_cache table for Warehouse entity references
CREATE TABLE warehouse_cache (
    warehouse_id UUID PRIMARY KEY,
    code VARCHAR(20) NOT NULL UNIQUE,
    name VARCHAR(200),
    cached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create index on code for lookup by warehouse identifier
CREATE INDEX idx_warehouse_cache_code ON warehouse_cache(code);

-- Create trigger to update cache timestamps
CREATE TRIGGER update_product_cache_updated_at
    BEFORE UPDATE ON product_cache
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_warehouse_cache_updated_at
    BEFORE UPDATE ON warehouse_cache
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

COMMIT;
