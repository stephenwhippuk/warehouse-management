-- Revert warehouse-service:001_initial_schema from pg

BEGIN;

-- Drop tables in reverse dependency order
DROP TABLE IF EXISTS locations;
DROP TABLE IF EXISTS warehouses;

-- Drop extension (only if no other databases are using it)
-- Note: We don't drop uuid-ossp as other schemas may depend on it
-- DROP EXTENSION IF EXISTS "uuid-ossp";

COMMIT;
