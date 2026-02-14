-- Verify product-service:001_init_schema on pg

BEGIN;

SELECT id, sku, name, description, category, status, created_at, updated_at
FROM products
WHERE FALSE;

ROLLBACK;
