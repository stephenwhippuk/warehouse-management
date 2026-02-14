-- Revert warehouse-service:002_rename_location_fields from pg

BEGIN;

-- Revert column renames
ALTER TABLE locations RENAME COLUMN bay TO rack;
ALTER TABLE locations RENAME COLUMN level TO shelf;

COMMIT;
