-- Verify inventory-service:001_initial_schema on pg

BEGIN;

-- Verify inventory table exists with expected columns
SELECT id, product_id, warehouse_id, location_id, quantity, 
       available_quantity, reserved_quantity, allocated_quantity,
       serial_number, batch_number, expiration_date, manufacture_date,
       received_date, last_counted_date, last_counted_by, cost_per_unit,
       status, quality_status, notes, metadata, 
       created_at, updated_at, created_by, updated_by
FROM inventory
WHERE FALSE;

-- Verify inventory_movements table exists with expected columns
SELECT id, inventory_id, movement_type, quantity_change, 
       quantity_before, quantity_after, reference_type, reference_id,
       reason, notes, metadata, created_at, created_by
FROM inventory_movements
WHERE FALSE;

-- Verify indexes exist
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'inventory' AND indexname = 'idx_inventory_product';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'inventory' AND indexname = 'idx_inventory_warehouse';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'inventory' AND indexname = 'idx_inventory_location';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'inventory' AND indexname = 'idx_inventory_status';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'inventory_movements' AND indexname = 'idx_movements_inventory';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'inventory_movements' AND indexname = 'idx_movements_type';

-- Verify foreign key constraint
SELECT 1/COUNT(*) FROM pg_constraint 
WHERE conname = 'inventory_movements_inventory_id_fkey' 
AND conrelid = 'inventory_movements'::regclass;

-- Verify triggers exist
SELECT 1/COUNT(*) FROM pg_trigger WHERE tgname = 'update_inventory_updated_at';
SELECT 1/COUNT(*) FROM pg_trigger WHERE tgname = 'validate_inventory_quantities_trigger';
SELECT 1/COUNT(*) FROM pg_trigger WHERE tgname = 'log_inventory_movement_trigger';

-- Verify functions exist
SELECT 1/COUNT(*) FROM pg_proc WHERE proname = 'update_updated_at_column';
SELECT 1/COUNT(*) FROM pg_proc WHERE proname = 'validate_inventory_quantities';
SELECT 1/COUNT(*) FROM pg_proc WHERE proname = 'log_inventory_movement';

-- Verify UUID extension
SELECT 1/COUNT(*) FROM pg_extension WHERE extname = 'uuid-ossp';

ROLLBACK;
