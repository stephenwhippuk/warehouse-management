-- Verify inventory-service:002_add_product_cache on pg

BEGIN;

-- Verify the table exists and has the correct structure
SELECT product_id, sku, name, cached_at, updated_at
FROM product_cache
WHERE FALSE;

-- Verify the indexes exist
SELECT 1 FROM pg_indexes 
WHERE tablename = 'product_cache' 
AND indexname = 'idx_product_cache_sku';

ROLLBACK;
