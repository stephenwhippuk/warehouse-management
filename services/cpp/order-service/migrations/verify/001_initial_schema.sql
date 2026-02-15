-- Verify order-service:001_initial_schema on pg

BEGIN;

-- Verify tables exist
SELECT id, order_number, customer_id, warehouse_id, status, order_date, total
FROM orders
WHERE FALSE;

SELECT id, order_id, product_id, sku, quantity, unit_price, line_total
FROM order_line_items
WHERE FALSE;

ROLLBACK;
