-- Deploy inventory-service:002_add_product_cache to pg
-- requires: 001_initial_schema

BEGIN;

-- Create product_cache table to store denormalized Product reference data
-- This avoids repeated calls to product-service for identity fields
CREATE TABLE product_cache (
    product_id UUID PRIMARY KEY,
    sku VARCHAR(100) NOT NULL UNIQUE,
    name VARCHAR(200),
    cached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create index on sku for lookup by product identifier
CREATE INDEX idx_product_cache_sku ON product_cache(sku);

-- Create trigger to update product_cache timestamp
CREATE TRIGGER update_product_cache_updated_at
    BEFORE UPDATE ON product_cache
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

COMMIT;
