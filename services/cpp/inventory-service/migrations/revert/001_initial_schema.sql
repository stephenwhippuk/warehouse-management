-- Revert inventory-service:001_initial_schema from pg

BEGIN;

-- Drop triggers
DROP TRIGGER IF EXISTS log_inventory_movement_trigger ON inventory;
DROP TRIGGER IF EXISTS validate_inventory_quantities_trigger ON inventory;
DROP TRIGGER IF EXISTS update_inventory_updated_at ON inventory;

-- Drop functions
DROP FUNCTION IF EXISTS log_inventory_movement();
DROP FUNCTION IF EXISTS validate_inventory_quantities();
DROP FUNCTION IF EXISTS update_updated_at_column();

-- Drop tables in reverse dependency order
DROP TABLE IF EXISTS inventory_movements;
DROP TABLE IF EXISTS inventory;

-- Drop extension (only if no other databases are using it)
-- Note: We don't drop uuid-ossp as other schemas may depend on it
-- DROP EXTENSION IF EXISTS "uuid-ossp";

COMMIT;
