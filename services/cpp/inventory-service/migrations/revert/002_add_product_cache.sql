-- Revert inventory-service:002_add_product_cache from pg

BEGIN;

DROP TRIGGER IF EXISTS update_product_cache_updated_at ON product_cache;
DROP INDEX IF EXISTS idx_product_cache_sku;
DROP TABLE IF EXISTS product_cache;

COMMIT;
