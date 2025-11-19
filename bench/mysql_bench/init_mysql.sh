#!/bin/bash

echo "=== Starting MySQL Database Setup ==="

mkdir -p /eval/real-workloads/mysql/data
mkdir -p /eval/real-workloads/mysql/tmp

# Create data directory and initialize
echo "Initializing MySQL..."
/eval/real-workloads/mysql/bin/mysqld --defaults-file=/eval/real-workloads/mysql/my.cnf --initialize-insecure --user=root --datadir=/eval/real-workloads/mysql/data

# Start MySQL server in background
echo "Starting MySQL server..."
/eval/real-workloads/mysql/bin/mysqld --defaults-file=/eval/real-workloads/mysql/my.cnf --user=root --port=3306 --datadir=/eval/real-workloads/mysql/data &
MYSQL_PID=$!

# Wait for MySQL to be ready
echo "Waiting for MySQL to be ready..."
sleep 40

# Create database
echo "Creating test_db database..."
/eval/real-workloads/mysql/bin/mysql --defaults-file=/eval/real-workloads/mysql/my.cnf -u root -e "CREATE DATABASE IF NOT EXISTS test_db;"

# Shutdown MySQL
echo "Shutting down MySQL..."
/eval/real-workloads/mysql/bin/mysqladmin --defaults-file=/eval/real-workloads/mysql/my.cnf -u root shutdown

# Wait for shutdown to complete
wait $MYSQL_PID 2>/dev/null || true

echo "=== MySQL Setup Complete ==="

echo "Installation directory: /eval/real-workloads/mysql"
echo "Initialized user: root"
echo "Initialized password: none"
echo "Initialized database: test_db"