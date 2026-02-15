-- Revert order-service:002_add_reference_caches from pg

BEGIN;

DROP TRIGGER IF EXISTS update_warehouse_cache_updated_at ON warehouse_cache;
DROP TRIGGER IF EXISTS update_product_cache_updated_at ON product_cache;
DROP INDEX IF EXISTS idx_warehouse_cache_code;
DROP INDEX IF EXISTS idx_product_cache_sku;
DROP TABLE IF EXISTS warehouse_cache;
DROP TABLE IF EXISTS product_cache;

COMMIT;
