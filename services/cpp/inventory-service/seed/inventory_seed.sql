-- Seed data for inventory-service (development and test)
-- This script is idempotent and safe to run multiple times.

BEGIN;

INSERT INTO inventory (
    id,
    product_id,
    warehouse_id,
    location_id,
    quantity,
    available_quantity,
    reserved_quantity,
    allocated_quantity,
    status,
    quality_status,
    batch_number
) VALUES
    (
        'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa',
        'bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb',
        'cccccccc-cccc-cccc-cccc-cccccccccccc',
        'dddddddd-dddd-dddd-dddd-dddddddddddd',
        100,
        80,
        10,
        10,
        'available',
        'not_tested',
        'SEED-BATCH-001'
    ),
    (
        'eeeeeeee-eeee-eeee-eeee-eeeeeeeeeeee',
        'ffffffff-ffff-ffff-ffff-ffffffffffff',
        'cccccccc-cccc-cccc-cccc-cccccccccccc',
        'dddddddd-dddd-dddd-dddd-dddddddddddd',
        50,
        20,
        20,
        10,
        'reserved',
        'pending',
        'SEED-BATCH-002'
    )
ON CONFLICT (id) DO NOTHING;

COMMIT;
