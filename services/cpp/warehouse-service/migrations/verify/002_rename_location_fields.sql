-- Verify warehouse-service:002_rename_location_fields on pg

BEGIN;

-- Verify new columns exist
SELECT id, warehouse_id, code, aisle, bay, level, bin
FROM locations
WHERE FALSE;

ROLLBACK;
