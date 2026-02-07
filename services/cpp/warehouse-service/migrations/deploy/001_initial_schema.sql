-- Deploy warehouse-service:001_initial_schema to pg
-- requires: 

BEGIN;

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Warehouses table
CREATE TABLE warehouses (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    code VARCHAR(20) UNIQUE NOT NULL,
    name VARCHAR(200) NOT NULL,
    description TEXT,
    address JSONB NOT NULL,
    coordinates JSONB,
    type VARCHAR(50) NOT NULL,
    total_area DECIMAL(10,2),
    storage_capacity DECIMAL(10,2),
    contact_person JSONB,
    operating_hours JSONB,
    capabilities JSONB,
    status VARCHAR(20) NOT NULL DEFAULT 'active',
    metadata JSONB,
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    created_by VARCHAR(100) NOT NULL,
    updated_at TIMESTAMP,
    updated_by VARCHAR(100),
    
    CONSTRAINT chk_warehouse_type CHECK (type IN ('distribution', 'fulfillment', 'storage', 'cold_storage', 'cross_dock')),
    CONSTRAINT chk_warehouse_status CHECK (status IN ('active', 'inactive', 'archived'))
);

-- Locations table
CREATE TABLE locations (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    warehouse_id UUID NOT NULL REFERENCES warehouses(id) ON DELETE CASCADE,
    code VARCHAR(50) NOT NULL,
    name VARCHAR(200),
    type VARCHAR(50) NOT NULL,
    zone VARCHAR(50),
    aisle VARCHAR(10),
    rack VARCHAR(10),
    shelf VARCHAR(10),
    bin VARCHAR(10),
    parent_location_id UUID REFERENCES locations(id) ON DELETE SET NULL,
    dimensions JSONB,
    max_weight JSONB,
    max_volume DECIMAL(10,2),
    is_pickable BOOLEAN NOT NULL DEFAULT true,
    is_receivable BOOLEAN NOT NULL DEFAULT true,
    requires_equipment VARCHAR(50) NOT NULL DEFAULT 'none',
    temperature_controlled BOOLEAN NOT NULL DEFAULT false,
    temperature_range JSONB,
    barcode VARCHAR(100),
    status VARCHAR(20) NOT NULL DEFAULT 'active',
    metadata JSONB,
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    created_by VARCHAR(100) NOT NULL,
    updated_at TIMESTAMP,
    updated_by VARCHAR(100),
    
    CONSTRAINT chk_location_type CHECK (type IN ('bin', 'shelf', 'rack', 'pallet', 'floor', 'staging', 'receiving', 'shipping', 'picking', 'returns')),
    CONSTRAINT chk_location_status CHECK (status IN ('active', 'inactive', 'full', 'reserved', 'damaged', 'maintenance')),
    CONSTRAINT chk_equipment CHECK (requires_equipment IN ('none', 'forklift', 'ladder', 'cherry_picker', 'pallet_jack')),
    CONSTRAINT uk_warehouse_location_code UNIQUE (warehouse_id, code)
);

-- Indexes for warehouses
CREATE INDEX idx_warehouses_code ON warehouses(code);
CREATE INDEX idx_warehouses_status ON warehouses(status);
CREATE INDEX idx_warehouses_type ON warehouses(type);

-- Indexes for locations
CREATE INDEX idx_locations_warehouse ON locations(warehouse_id);
CREATE INDEX idx_locations_code ON locations(code);
CREATE INDEX idx_locations_status ON locations(status);
CREATE INDEX idx_locations_type ON locations(type);
CREATE INDEX idx_locations_zone ON locations(zone);
CREATE INDEX idx_locations_aisle ON locations(aisle);
CREATE INDEX idx_locations_barcode ON locations(barcode);
CREATE INDEX idx_locations_parent ON locations(parent_location_id);

-- Comments
COMMENT ON TABLE warehouses IS 'Warehouse facilities - managed by warehouse-service';
COMMENT ON TABLE locations IS 'Storage locations within warehouses - managed by warehouse-service';

-- Sample data for testing (only in development)
INSERT INTO warehouses (id, code, name, address, type, status, created_by)
VALUES (
    uuid_generate_v4(),
    'WH-001',
    'Main Distribution Center',
    '{"street": "123 Industrial Pkwy", "city": "Chicago", "state": "IL", "postalCode": "60601", "country": "US"}'::jsonb,
    'distribution',
    'active',
    'system'
) ON CONFLICT (code) DO NOTHING;

COMMIT;
