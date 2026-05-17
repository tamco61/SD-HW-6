@echo off
REM =============================================================================
REM BlablaCar Service - Database Initialization Script (Windows)
REM Домашнее задание 03: Проектирование и оптимизация реляционной базы данных
REM =============================================================================

setlocal enabledelayedexpansion

REM Default configuration
set DB_HOST=%DB_HOST:=localhost%
set DB_PORT=%DB_PORT: =5432%
set DB_NAME=%DB_NAME: =blablacar_db%
set DB_USER=%DB_USER: =postgres%
set DB_PASSWORD=%DB_PASSWORD: =postgres_password%

set PGPASSWORD=%DB_PASSWORD%

echo.
echo =========================================
echo BlablaCar Service - DB Initialization
echo =========================================
echo.

REM Check if psql is available
where psql >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] psql not found! Please install PostgreSQL client or add to PATH.
    echo.
    echo Alternative: Use Docker to run the database:
    echo   docker-compose up -d postgres
    echo.
    pause
    exit /b 1
)

REM Main commands
if "%1"=="" goto setup
if /i "%1"=="setup" goto setup
if /i "%1"=="reset" goto reset
if /i "%1"=="drop" goto drop
if /i "%1"=="verify" goto verify
if /i "%1"=="test" goto test
if /i "%1"=="explain" goto explain

:setup
echo [INFO] Setting up database...
echo.

REM Check if PostgreSQL is running
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -c "SELECT 1" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] PostgreSQL is not running at %DB_HOST%:%DB_PORT%
    echo.
    echo Please start PostgreSQL first:
    echo   docker-compose up -d postgres
    echo.
    pause
    exit /b 1
)

REM Create database
echo [INFO] Creating database: %DB_NAME%...
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -tc "SELECT 1 FROM pg_database WHERE datname = '%DB_NAME%'" | findstr "1" >nul 2>&1
if %errorlevel% neq 0 (
    psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -c "CREATE DATABASE %DB_NAME%"
    echo [OK] Database created
) else (
    echo [OK] Database already exists
)
echo.

REM Run schema
echo [INFO] Running schema.sql...
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -f schema.sql
if %errorlevel% neq 0 (
    echo [ERROR] Failed to run schema.sql
    pause
    exit /b 1
)
echo [OK] Schema created
echo.

REM Run data
echo [INFO] Running data.sql...
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -f data.sql
if %errorlevel% neq 0 (
    echo [ERROR] Failed to run data.sql
    pause
    exit /b 1
)
echo [OK] Test data inserted
echo.

goto verify

:reset
echo [INFO] Resetting database...
echo.
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -c "DROP DATABASE IF EXISTS %DB_NAME%"
echo [OK] Database dropped
goto setup

:drop
echo [INFO] Dropping database: %DB_NAME%...
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -c "DROP DATABASE IF EXISTS %DB_NAME%"
echo [OK] Database dropped
goto :eof

:verify
echo.
echo [INFO] Verifying installation...
echo.
echo Tables count:
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "SELECT COUNT(*) as table_count FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE';"
echo.
echo Records per table:
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "SELECT relname AS table_name, n_live_tup AS row_count FROM pg_stat_user_tables ORDER BY n_live_tup DESC;"
echo.
echo Indexes count:
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "SELECT COUNT(*) as index_count FROM pg_indexes WHERE schemaname = 'public';"
echo.
echo [OK] Verification complete
goto :eof

:test
echo [INFO] Running test queries...
echo.
echo Test 1: Count users
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "SELECT COUNT(*) as users_count FROM users;"
echo.
echo Test 2: Count routes
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "SELECT COUNT(*) as routes_count FROM routes;"
echo.
echo Test 3: Count trips
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "SELECT COUNT(*) as trips_count FROM trips;"
echo.
echo Test 4: Count participants
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "SELECT COUNT(*) as participants_count FROM trip_participants;"
echo.
echo [OK] All test queries completed
goto :eof

:explain
echo [INFO] Running EXPLAIN ANALYZE on sample query...
echo.
psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -c "EXPLAIN ANALYZE SELECT login, first_name, last_name FROM users WHERE login = 'ivan_petrov';"
goto :eof
