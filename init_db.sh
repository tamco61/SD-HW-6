#!/bin/bash
# =============================================================================
# BlablaCar Service - Database Initialization Script
# Домашнее задание 03: Проектирование и оптимизация реляционной базы данных
# =============================================================================

set -e

# Default configuration
DB_HOST="${DB_HOST:-localhost}"
DB_PORT="${DB_PORT:-5432}"
DB_NAME="${DB_NAME:-blablacar_db}"
DB_USER="${DB_USER:-postgres}"
DB_PASSWORD="${DB_PASSWORD:-postgres_password}"

export PGPASSWORD="$DB_PASSWORD"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}BlablaCar Service - DB Initialization${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""

# Function to check if PostgreSQL is running
check_postgres() {
    if pg_isready -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" > /dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Function to create database
create_database() {
    echo -e "${YELLOW}Creating database: ${DB_NAME}...${NC}"
    
    # Create database if not exists
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -tc "SELECT 1 FROM pg_database WHERE datname = '$DB_NAME'" | grep -q 1 || \
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -c "CREATE DATABASE $DB_NAME"
    
    echo -e "${GREEN}✓ Database created successfully${NC}"
}

# Function to run schema.sql
run_schema() {
    echo -e "${YELLOW}Running schema.sql...${NC}"
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f schema.sql
    echo -e "${GREEN}✓ Schema created successfully${NC}"
}

# Function to run data.sql
run_data() {
    echo -e "${YELLOW}Running data.sql...${NC}"
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f data.sql
    echo -e "${GREEN}✓ Test data inserted successfully${NC}"
}

# Function to verify installation
verify_installation() {
    echo ""
    echo -e "${YELLOW}Verifying installation...${NC}"
    
    echo "Tables count:"
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "
        SELECT COUNT(*) as table_count 
        FROM information_schema.tables 
        WHERE table_schema = 'public' AND table_type = 'BASE TABLE';
    "
    
    echo ""
    echo "Records per table:"
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "
        SELECT 
            schemaname,
            relname AS table_name,
            n_live_tup AS row_count
        FROM pg_stat_user_tables
        ORDER BY n_live_tup DESC;
    "
    
    echo ""
    echo "Indexes count:"
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "
        SELECT COUNT(*) as index_count
        FROM pg_indexes
        WHERE schemaname = 'public';
    "
    
    echo -e "${GREEN}✓ Verification complete${NC}"
}

# Function to drop database (for cleanup)
drop_database() {
    echo -e "${YELLOW}Dropping database: ${DB_NAME}...${NC}"
    psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -c "DROP DATABASE IF EXISTS $DB_NAME"
    echo -e "${GREEN}✓ Database dropped successfully${NC}"
}

# Main execution
case "${1:-setup}" in
    setup)
        if ! check_postgres; then
            echo -e "${RED}✗ PostgreSQL is not running at ${DB_HOST}:${DB_PORT}${NC}"
            echo -e "${YELLOW}Please start PostgreSQL first:${NC}"
            echo -e "  docker-compose up -d postgres"
            exit 1
        fi
        
        create_database
        run_schema
        run_data
        verify_installation
        
        echo ""
        echo -e "${GREEN}=========================================${NC}"
        echo -e "${GREEN}Database setup completed successfully!${NC}"
        echo -e "${GREEN}=========================================${NC}"
        ;;
    
    reset)
        echo -e "${YELLOW}Resetting database...${NC}"
        drop_database
        create_database
        run_schema
        run_data
        verify_installation
        
        echo ""
        echo -e "${GREEN}=========================================${NC}"
        echo -e "${GREEN}Database reset completed!${NC}"
        echo -e "${GREEN}=========================================${NC}"
        ;;
    
    drop)
        drop_database
        ;;
    
    verify)
        verify_installation
        ;;
    
    test)
        echo -e "${YELLOW}Running test queries...${NC}"
        echo ""
        echo "Test 1: Count users"
        psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "SELECT COUNT(*) as users_count FROM users;"
        
        echo "Test 2: Count routes"
        psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "SELECT COUNT(*) as routes_count FROM routes;"
        
        echo "Test 3: Count trips"
        psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "SELECT COUNT(*) as trips_count FROM trips;"
        
        echo "Test 4: Count participants"
        psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "SELECT COUNT(*) as participants_count FROM trip_participants;"
        
        echo ""
        echo -e "${GREEN}✓ All test queries completed${NC}"
        ;;
    
    explain)
        echo -e "${YELLOW}Running EXPLAIN ANALYZE on sample query...${NC}"
        echo ""
        psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "
            EXPLAIN ANALYZE
            SELECT login, first_name, last_name
            FROM users
            WHERE login = 'ivan_petrov';
        "
        ;;
    
    *)
        echo "Usage: $0 {setup|reset|drop|verify|test|explain}"
        echo ""
        echo "Commands:"
        echo "  setup   - Create database, schema, and test data (default)"
        echo "  reset   - Drop and recreate database with schema and data"
        echo "  drop    - Drop the database"
        echo "  verify  - Verify database installation"
        echo "  test    - Run test queries"
        echo "  explain - Run EXPLAIN ANALYZE on sample query"
        exit 1
        ;;
esac
