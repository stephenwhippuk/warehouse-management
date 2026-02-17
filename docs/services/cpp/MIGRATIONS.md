# Database Migrations - Sqitch Setup

Both warehouse-service and inventory-service now use **Sqitch** for database migrations.

## Features

✅ **Version Control** - Migrations tracked in Git  
✅ **Rollback Support** - Revert to any previous version  
✅ **Verification** - Test migrations without applying  
✅ **Dependencies** - Track migration dependencies  
✅ **Automatic Deployment** - Runs on Docker startup  

## Structure

```
service/
├── sqitch.conf                    # Configuration
├── sqitch.plan                    # Migration history
├── migrations/
│   ├── deploy/                    # Forward migrations
│   │   └── 001_initial_schema.sql
│   ├── revert/                    # Rollback scripts
│   │   └── 001_initial_schema.sql
│   └── verify/                    # Verification tests
│       └── 001_initial_schema.sql
└── docker-entrypoint.sh           # Auto-applies migrations
```

## Common Commands

```bash
# Check migration status
sqitch status

# Deploy all pending migrations
sqitch deploy

# Deploy with verification
sqitch deploy --verify

# Verify without deploying
sqitch verify

# Rollback last migration
sqitch revert

# Rollback to specific version
sqitch revert 001_initial_schema

# View migration history
sqitch log

# Add new migration
sqitch add 002_feature_name -n "Description of changes"
```

## Targets

Configured targets in `sqitch.conf`:

- **development** - `localhost:5432`
- **docker** - `postgres:5432` (container)
- **production** - Production database

```bash
# Deploy to specific target
sqitch deploy docker
sqitch deploy production
```

## Creating New Migrations

```bash
cd services/cpp/warehouse-service  # or inventory-service

# Add migration
sqitch add 002_add_warehouse_zones -n "Add zone support to warehouses"

# This creates three files:
# - migrations/deploy/002_add_warehouse_zones.sql
# - migrations/revert/002_add_warehouse_zones.sql
# - migrations/verify/002_add_warehouse_zones.sql

# Edit the files:
# deploy/ - Add new columns/tables
# revert/ - Remove what was added
# verify/ - Test the changes exist

# Test locally
sqitch deploy
sqitch verify

# Test rollback
sqitch revert
sqitch deploy

# Commit to Git
git add migrations/ sqitch.plan
git commit -m "Add warehouse zone support"
```

## Docker Integration

Migrations run automatically on container startup via `docker-entrypoint.sh`:

```bash
# Startup sequence:
1. Wait for PostgreSQL to be ready
2. Check Sqitch status
3. Deploy any pending migrations
4. Verify migrations
5. Start the C++ service
```

View migration logs:
```bash
docker logs warehouse-service
docker logs inventory-service
```

## Migration Best Practices

### 1. Always Test Rollback
```bash
sqitch deploy
sqitch verify
sqitch revert    # Make sure this works!
sqitch deploy    # Deploy again
```

### 2. Use Transactions
```sql
-- Deploy
BEGIN;
  ALTER TABLE warehouses ADD COLUMN zone VARCHAR(50);
COMMIT;

-- Revert
BEGIN;
  ALTER TABLE warehouses DROP COLUMN zone;
COMMIT;
```

### 3. Make Idempotent When Possible
```sql
-- Good
ALTER TABLE warehouses ADD COLUMN IF NOT EXISTS zone VARCHAR(50);
CREATE INDEX IF NOT EXISTS idx_warehouses_zone ON warehouses(zone);

-- Bad (fails if already exists)
ALTER TABLE warehouses ADD COLUMN zone VARCHAR(50);
```

### 4. Write Verification Tests
```sql
-- Verify
BEGIN;
  SELECT zone FROM warehouses WHERE FALSE;  -- Fails if column missing
  SELECT 1/COUNT(*) FROM pg_indexes 
    WHERE indexname = 'idx_warehouses_zone';  -- Fails if index missing
ROLLBACK;
```

### 5. Document Dependencies
```sql
-- Deploy warehouse-service:002_add_zones to pg
-- requires: 001_initial_schema

BEGIN;
  -- migration code
COMMIT;
```

## CI/CD Integration

### GitHub Actions Example

```yaml
- name: Install Sqitch
  run: |
    sudo apt-get update
    sudo apt-get install -y sqitch libdbd-pg-perl

- name: Run Migrations
  env:
    DATABASE_URL: ${{ secrets.DATABASE_URL }}
  run: |
    cd services/cpp/warehouse-service
    sqitch deploy $DATABASE_URL
    sqitch verify $DATABASE_URL
```

## Troubleshooting

### Migration Failed
```bash
# Check status
sqitch status

# View error details
sqitch log --event fail

# Fix the deploy script and retry
sqitch deploy
```

### Reset Development Database
```bash
# Drop and recreate
sqitch revert --to @ROOT
sqitch deploy
```

### Migration Conflicts
```bash
# Rebundle from Git
sqitch bundle --dest-dir /tmp/bundle

# View plan
cat sqitch.plan
```

## References

- [Sqitch Documentation](https://sqitch.org/)
- [Sqitch Tutorial](https://sqitch.org/docs/manual/sqitchtutorial/)
- See: `/docs/cpp-database-migrations.md` for detailed guide
