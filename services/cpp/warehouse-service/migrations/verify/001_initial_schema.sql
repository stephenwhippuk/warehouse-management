-- Verify warehouse-service:001_initial_schema on pg

BEGIN;

-- Verify warehouses table exists with expected columns
SELECT id, code, name, description, address, coordinates, type, 
       total_area, storage_capacity, contact_person, operating_hours, 
       capabilities, status, metadata, created_at, created_by, 
       updated_at, updated_by
FROM warehouses
WHERE FALSE;

-- Verify locations table exists with expected columns
SELECT id, warehouse_id, code, name, type, zone, aisle, rack, shelf, bin,
       parent_location_id, dimensions, max_weight, max_volume, 
       is_pickable, is_receivable, requires_equipment, 
       temperature_controlled, temperature_range, barcode, status, metadata,
       created_at, created_by, updated_at, updated_by
FROM locations
WHERE FALSE;

-- Verify indexes exist
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'warehouses' AND indexname = 'idx_warehouses_code';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'warehouses' AND indexname = 'idx_warehouses_status';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'warehouses' AND indexname = 'idx_warehouses_type';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'locations' AND indexname = 'idx_locations_warehouse';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'locations' AND indexname = 'idx_locations_code';
SELECT 1/COUNT(*) FROM pg_indexes WHERE tablename = 'locations' AND indexname = 'idx_locations_status';

-- Verify foreign key constraint
SELECT 1/COUNT(*) FROM pg_constraint 
WHERE conname = 'locations_warehouse_id_fkey' 
AND conrelid = 'locations'::regclass;

-- Verify UUID extension
SELECT 1/COUNT(*) FROM pg_extension WHERE extname = 'uuid-ossp';

ROLLBACK;
