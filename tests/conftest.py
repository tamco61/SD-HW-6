import pathlib
import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins.postgresql', 'pytest_userver.plugins.mongo']
from testsuite.databases.pgsql import discover


@pytest.fixture(scope='session')
def service_source_dir():
    """Path to root directory service."""
    return pathlib.Path(__file__).parent.parent

@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'blablacar_service',
        [service_source_dir.joinpath('postgresql/schemas')],
    )
    return pgsql_local_create(list(databases.values()))

@pytest.fixture(scope='session')
def userver_pg_config(pgsql_local):
    # Фикстура возвращает функцию-хук
    def _hook_pg(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        
        # Находим наш компонент postgres-db и подменяем ему URL
        if 'postgres-db' in components:
            db_name = list(pgsql_local.keys())[0] # Динамически берем имя базы
            components['postgres-db']['dbconnection'] = pgsql_local[db_name].get_uri()
            
    return _hook_pg


@pytest.fixture(scope='session')
def userver_mongo_config(mongo_connection_info):
    def _hook_mongo(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'mongo-blabla' in components:
            components['mongo-blabla']['dbconnection'] = (
                mongo_connection_info.get_uri() + 'blablacar_db'
            )

    return _hook_mongo
