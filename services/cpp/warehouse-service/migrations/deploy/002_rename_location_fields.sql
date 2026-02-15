-- Deploy warehouse-service:002_rename_location_fields to pg
-- requires: 001_initial_schema

BEGIN;

-- Rename location columns to match entity contract
-- Contract specifies: aisle, bay, level (not rack, shelf, bin)
ALTER TABLE locations RENAME COLUMN rack TO bay;
ALTER TABLE locations RENAME COLUMN shelf TO level;

-- Note: bin column is kept as it may contain useful data
-- Exposure of this column is governed by the contract system (claims/DTOs)

COMMIT;
