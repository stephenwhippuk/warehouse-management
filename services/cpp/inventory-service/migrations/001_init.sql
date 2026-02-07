-- Inventory Service Database Schema
-- Version: 1.0.0

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Create inventory table
CREATE TABLE IF NOT EXISTS inventory (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    product_id UUID NOT NULL,
    warehouse_id UUID NOT NULL,
    location_id UUID NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    available_quantity INTEGER NOT NULL DEFAULT 0 CHECK (available_quantity >= 0),
    reserved_quantity INTEGER NOT NULL DEFAULT 0 CHECK (reserved_quantity >= 0),
    allocated_quantity INTEGER NOT NULL DEFAULT 0 CHECK (allocated_quantity >= 0),
    serial_number VARCHAR(100),
    batch_number VARCHAR(100),
    expiration_date DATE,
    manufacture_date DATE,
    received_date TIMESTAMP,
    last_counted_date TIMESTAMP,
    last_counted_by VARCHAR(255),
    cost_per_unit DECIMAL(10, 2),
    status VARCHAR(50) DEFAULT 'available' CHECK (status IN ('available', 'reserved', 'allocated', 'quarantine', 'damaged', 'expired', 'recalled')),
    quality_status VARCHAR(50) DEFAULT 'not_tested' CHECK (quality_status IN ('passed', 'failed', 'pending', 'not_tested')),
    notes TEXT,
    metadata JSONB,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_by VARCHAR(255),
    updated_by VARCHAR(255)
);

-- Create indexes
CREATE INDEX idx_inventory_product ON inventory(product_id);
CREATE INDEX idx_inventory_warehouse ON inventory(warehouse_id);
CREATE INDEX idx_inventory_location ON inventory(location_id);
CREATE INDEX idx_inventory_status ON inventory(status);
CREATE INDEX idx_inventory_expiration ON inventory(expiration_date);
CREATE INDEX idx_inventory_batch ON inventory(batch_number);
CREATE INDEX idx_inventory_serial ON inventory(serial_number);
CREATE INDEX idx_inventory_available ON inventory(available_quantity);

-- Composite indexes for common queries
CREATE INDEX idx_inventory_product_location ON inventory(product_id, location_id);
CREATE INDEX idx_inventory_product_warehouse ON inventory(product_id, warehouse_id);

-- Create inventory_movements table for tracking changes
CREATE TABLE IF NOT EXISTS inventory_movements (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    inventory_id UUID NOT NULL REFERENCES inventory(id) ON DELETE CASCADE,
    movement_type VARCHAR(50) NOT NULL CHECK (movement_type IN ('receive', 'issue', 'transfer', 'adjust', 'reserve', 'release', 'allocate', 'deallocate', 'count')),
    quantity_change INTEGER NOT NULL,
    quantity_before INTEGER NOT NULL,
    quantity_after INTEGER NOT NULL,
    reference_type VARCHAR(50),
    reference_id UUID,
    reason TEXT,
    notes TEXT,
    metadata JSONB,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_by VARCHAR(255)
);

-- Create indexes for movements
CREATE INDEX idx_movements_inventory ON inventory_movements(inventory_id);
CREATE INDEX idx_movements_type ON inventory_movements(movement_type);
CREATE INDEX idx_movements_created ON inventory_movements(created_at);
CREATE INDEX idx_movements_reference ON inventory_movements(reference_type, reference_id);

-- Create function to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Create trigger for inventory table
CREATE TRIGGER update_inventory_updated_at
    BEFORE UPDATE ON inventory
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Create function to validate inventory quantities
CREATE OR REPLACE FUNCTION validate_inventory_quantities()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.quantity != (NEW.available_quantity + NEW.reserved_quantity + NEW.allocated_quantity) THEN
        RAISE EXCEPTION 'Inventory quantity must equal available + reserved + allocated';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Create trigger for quantity validation
CREATE TRIGGER validate_inventory_quantities_trigger
    BEFORE INSERT OR UPDATE ON inventory
    FOR EACH ROW
    EXECUTE FUNCTION validate_inventory_quantities();

-- Create function to log inventory movements
CREATE OR REPLACE FUNCTION log_inventory_movement()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'UPDATE' AND OLD.quantity != NEW.quantity THEN
        INSERT INTO inventory_movements (
            inventory_id,
            movement_type,
            quantity_change,
            quantity_before,
            quantity_after,
            reason,
            created_by
        ) VALUES (
            NEW.id,
            'adjust',
            NEW.quantity - OLD.quantity,
            OLD.quantity,
            NEW.quantity,
            'Automatic adjustment',
            NEW.updated_by
        );
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Create trigger for movement logging
CREATE TRIGGER log_inventory_movement_trigger
    AFTER UPDATE ON inventory
    FOR EACH ROW
    EXECUTE FUNCTION log_inventory_movement();

-- Insert sample data (optional, for testing)
INSERT INTO inventory (product_id, warehouse_id, location_id, quantity, available_quantity, batch_number, status) VALUES
    (uuid_generate_v4(), uuid_generate_v4(), uuid_generate_v4(), 100, 100, 'BATCH001', 'available'),
    (uuid_generate_v4(), uuid_generate_v4(), uuid_generate_v4(), 50, 40, 'BATCH002', 'available');

-- Grant permissions
GRANT ALL PRIVILEGES ON TABLE inventory TO inventory_user;
GRANT ALL PRIVILEGES ON TABLE inventory_movements TO inventory_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO inventory_user;
