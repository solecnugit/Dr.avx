#! /bin/bash

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color
BOLD='\033[1m'

Dravx_path="/eval/bin/dravx/bin64/dravx"
sde_path="/eval/bin/sde-9.58/sde64"

MySQL_cmd="/eval/real-workloads/mysql/bin/mysqld --defaults-file=/eval/real-workloads/mysql/my.cnf --user=root"

raw_data_path="/eval/results/mysql/mysql_raw_data"
draw_data_path="/eval/results/mysql/mysql_draw_data"

# Support environment variable overrides with defaults
MySQL_THREADS=${MySQL_THREADS:-8}
MySQL_TIME=${MySQL_TIME:-180}
MySQL_SCENARIO=("read_only" "read_write" "write_only")

echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║${WHITE}${BOLD}        MySQL Benchmark Configuration                ${CYAN}║${NC}"
echo -e "${CYAN}╠════════════════════════════════════════════════════════════╣${NC}"
echo -e "${CYAN}║${NC} ${YELLOW}Threads:${NC}        ${GREEN}${MySQL_THREADS}${NC}"
echo -e "${CYAN}║${NC} ${YELLOW}Time per test:${NC}  ${GREEN}${MySQL_TIME}s${NC}"
echo -e "${CYAN}║${NC} ${YELLOW}Scenarios:${NC}      ${GREEN}${MySQL_SCENARIO[@]}${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"

# Create directories if they don't exist
mkdir -p "$raw_data_path"
mkdir -p "$draw_data_path"

declare -A mysql_result
mysql_result["origin_mysql_read_only"]=0
mysql_result["origin_mysql_read_write"]=0
mysql_result["origin_mysql_write_only"]=0
mysql_result["dravx_mysql_read_only"]=0
mysql_result["dravx_mysql_read_write"]=0
mysql_result["dravx_mysql_write_only"]=0
mysql_result["sde_mysql_read_only"]=0
mysql_result["sde_mysql_read_write"]=0
mysql_result["sde_mysql_write_only"]=0

run_origin_MySQL() {
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${WHITE}${BOLD}     🚀 Running ORIGIN MySQL Benchmark (Baseline)    ${GREEN}║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Starting MySQL server...${NC}"
    $MySQL_cmd &
    pid=$!
    echo -e "${YELLOW}⏳ Waiting for MySQL to start (PID: ${pid})${NC}"
    sleep 10
    for i in "${MySQL_SCENARIO[@]}"; do
        echo -e "\n${CYAN}▶ Running scenario: ${BOLD}${i}${NC}"
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 prepare
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 run | tee ${raw_data_path}/origin_mysql_${i}.txt
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 cleanup
        sleep 10
    done
    kill $pid
    echo -e "${GREEN}✓ Origin MySQL benchmark completed${NC}\n"
}

run_dravx_MySQL() {
    echo ""
    echo -e "${MAGENTA}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║${WHITE}${BOLD}       🔥 Running DR-AVX MySQL Benchmark 🔥           ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${MAGENTA}║${NC} ${YELLOW}Mode:${NC} ${WHITE}Quick R/W with DR-AVX instrumentation${NC}"
    echo -e "${MAGENTA}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Starting MySQL server with DR-AVX...${NC}"
    $Dravx_path -quick_rw 1 -- $MySQL_cmd &
    pid=$!
    echo -e "${YELLOW}⏳ Waiting for MySQL to start under DR-AVX (PID: ${pid})${NC}"
    echo -e "${YELLOW}   (DR-AVX instrumentation takes longer to initialize - 30s)${NC}"
    sleep 30
    for i in "${MySQL_SCENARIO[@]}"; do
        echo -e "\n${CYAN}▶ Running scenario: ${BOLD}${i}${NC} ${MAGENTA}[with DR-AVX]${NC}"
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 prepare
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 run | tee ${raw_data_path}/dravx_mysql_${i}.txt
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 cleanup
        sleep 10
    done
    kill $pid
    echo -e "${MAGENTA}✓ DR-AVX MySQL benchmark completed${NC}\n"
}

run_sde_MySQL() {
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${WHITE}${BOLD}        ⚡ Running SDE MySQL Benchmark ⚡             ${BLUE}║${NC}"
    echo -e "${BLUE}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${BLUE}║${NC} ${YELLOW}Mode:${NC} ${WHITE}Intel SDE with AVX-512 simulation${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Starting MySQL server with Intel SDE...${NC}"
    $sde_path -avx512 1 -- $MySQL_cmd &
    pid=$!
    echo -e "${YELLOW}⏳ Waiting for MySQL to start under Intel SDE (PID: ${pid})${NC}"
    echo -e "${YELLOW}   (SDE emulation requires significant initialization time - 180s)${NC}"
    echo -e "${RED}${BOLD}   ⚠️  This may take up to 3 minutes, please be patient...${NC}"
    sleep 180
    for i in "${MySQL_SCENARIO[@]}"; do
        echo -e "\n${CYAN}▶ Running scenario: ${BOLD}${i}${NC} ${BLUE}[with Intel SDE]${NC}"
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 prepare
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 run | tee ${raw_data_path}/sde_mysql_${i}.txt
        sysbench /usr/share/sysbench/oltp_${i}.lua --threads=${MySQL_THREADS} --time=${MySQL_TIME} --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=root --mysql-db=test_db --tables=50 --table-size=10000 --range_selects=off --db-ps-mode=disable --report-interval=1 cleanup
        sleep 10
    done
    kill $pid
    echo -e "${BLUE}✓ SDE MySQL benchmark completed${NC}\n"
}

get_mysql_result() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${WHITE}${BOLD}          📊 Processing Benchmark Results              ${CYAN}║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Parsing output files and calculating metrics...${NC}\n"
    
    for i in "${MySQL_SCENARIO[@]}"; do
        mysql_result["origin_mysql_${i}"]=$(python3 cal_mysql_output.py ${raw_data_path}/origin_mysql_${i}.txt)
        mysql_result["dravx_mysql_${i}"]=$(python3 cal_mysql_output.py ${raw_data_path}/dravx_mysql_${i}.txt)
        mysql_result["sde_mysql_${i}"]=$(python3 cal_mysql_output.py ${raw_data_path}/sde_mysql_${i}.txt)
    done
    
    echo -e "${YELLOW}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${YELLOW}║${WHITE}${BOLD}                   FINAL RESULTS                       ${YELLOW}║${NC}"
    echo -e "${YELLOW}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${YELLOW}║${NC} ${GREEN}${BOLD}ORIGIN (Baseline):${NC}"
    echo -e "${YELLOW}║${NC}   Read Only:  ${WHITE}${mysql_result["origin_mysql_read_only"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read Write: ${WHITE}${mysql_result["origin_mysql_read_write"]}${NC}"
    echo -e "${YELLOW}║${NC}   Write Only: ${WHITE}${mysql_result["origin_mysql_write_only"]}${NC}"
    echo -e "${YELLOW}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${YELLOW}║${NC} ${MAGENTA}${BOLD}DR-AVX:${NC}"
    echo -e "${YELLOW}║${NC}   Read Only:  ${WHITE}${mysql_result["dravx_mysql_read_only"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read Write: ${WHITE}${mysql_result["dravx_mysql_read_write"]}${NC}"
    echo -e "${YELLOW}║${NC}   Write Only: ${WHITE}${mysql_result["dravx_mysql_write_only"]}${NC}"
    echo -e "${YELLOW}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${YELLOW}║${NC} ${BLUE}${BOLD}Intel SDE:${NC}"
    echo -e "${YELLOW}║${NC}   Read Only:  ${WHITE}${mysql_result["sde_mysql_read_only"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read Write: ${WHITE}${mysql_result["sde_mysql_read_write"]}${NC}"
    echo -e "${YELLOW}║${NC}   Write Only: ${WHITE}${mysql_result["sde_mysql_write_only"]}${NC}"
    echo -e "${YELLOW}╚════════════════════════════════════════════════════════════╝${NC}"
}

draw_mysql_chart() {
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${WHITE}${BOLD}             📈 Generating Charts                      ${GREEN}║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Creating visualization charts from benchmark data...${NC}\n"
    
    # Prepare data string for Python script
    mysql_data_string="origin_mysql_read_only: ${mysql_result["origin_mysql_read_only"]}
origin_mysql_read_write: ${mysql_result["origin_mysql_read_write"]}
origin_mysql_write_only: ${mysql_result["origin_mysql_write_only"]}
dravx_mysql_read_only: ${mysql_result["dravx_mysql_read_only"]}
dravx_mysql_read_write: ${mysql_result["dravx_mysql_read_write"]}
dravx_mysql_write_only: ${mysql_result["dravx_mysql_write_only"]}
sde_mysql_read_only: ${mysql_result["sde_mysql_read_only"]}
sde_mysql_read_write: ${mysql_result["sde_mysql_read_write"]}
sde_mysql_write_only: ${mysql_result["sde_mysql_write_only"]}"
    
    # Call Python script to generate chart
    python3 draw_mysql.py "$mysql_data_string" "$draw_data_path"
    
    echo -e "${GREEN}✓ Chart generation completed${NC}"
    echo -e "${WHITE}Charts saved to: ${CYAN}${draw_data_path}${NC}\n"
}

echo ""
echo -e "${WHITE}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo -e "${WHITE}${BOLD}     🚀 MySQL Benchmark Suite - Starting Tests 🚀          ${NC}"
echo -e "${WHITE}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo ""

run_origin_MySQL
echo -e "${YELLOW}⏸️  Cooling down period (60s) before next test...${NC}"
sleep 60

run_dravx_MySQL
echo -e "${YELLOW}⏸️  Cooling down period (60s) before next test...${NC}"
sleep 60

run_sde_MySQL
echo -e "${YELLOW}⏸️  Brief pause (10s) before processing results...${NC}"
sleep 10

get_mysql_result
draw_mysql_chart

echo ""
echo -e "${GREEN}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}${BOLD}     ✅ MySQL Benchmark Suite Completed Successfully ✅     ${NC}"
echo -e "${GREEN}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo -e "${WHITE}Raw data: ${CYAN}${raw_data_path}${NC}"
echo -e "${WHITE}Charts:   ${CYAN}${draw_data_path}${NC}"
echo ""
