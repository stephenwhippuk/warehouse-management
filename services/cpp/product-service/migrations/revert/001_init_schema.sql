-- Revert product-service:001_init_schema from pg

BEGIN;

DROP TRIGGER IF EXISTS products_update_timestamp ON products;
DROP FUNCTION IF EXISTS update_products_timestamp();
DROP TABLE IF EXISTS products;

COMMIT;
