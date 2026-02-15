-- Verify order-service:002_add_reference_caches on pg

BEGIN;

-- Verify tables exist with correct structure
SELECT product_id, sku, name, cached_at, updated_at
FROM product_cache
WHERE FALSE;

SELECT warehouse_id, code, name, cached_at, updated_at
FROM warehouse_cache
WHERE FALSE;

-- Verify indexes exist
SELECT 1 FROM pg_indexes 
WHERE tablename = 'product_cache' 
AND indexname = 'idx_product_cache_sku';

SELECT 1 FROM pg_indexes 
WHERE tablename = 'warehouse_cache' 
AND indexname = 'idx_warehouse_cache_code';

ROLLBACK;
