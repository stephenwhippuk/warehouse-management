-- Revert order-service:001_initial_schema from pg

BEGIN;

DROP TRIGGER IF EXISTS update_line_items_updated_at ON order_line_items;
DROP TRIGGER IF EXISTS update_orders_updated_at ON orders;
DROP TABLE IF EXISTS order_line_items;
DROP TABLE IF EXISTS orders;
DROP FUNCTION IF EXISTS update_updated_at_column();

COMMIT;
