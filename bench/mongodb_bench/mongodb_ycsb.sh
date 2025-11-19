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

mkdir -p /eval/real-workloads/mongodb/data

Dravx_path="/eval/bin/dravx/bin64/dravx"
sde_path="/eval/bin/sde-9.58/sde64"

MongoDB_cmd="/eval/real-workloads/mongodb/bin/mongod --dbpath /eval/real-workloads/mongodb/data --logpath /eval/real-workloads/mongodb/mongodb.log"

raw_data_path="/eval/results/mongodb/mongodb_raw_data"
# Allow environment variables to override defaults
MongoDB_THREADS=${MongoDB_THREADS:-16}
MongoDB_COUNT=${MongoDB_COUNT:-7200000}
MongoDB_SCENARIO=("workloadb" "workloadc" "workloadd")

echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║${WHITE}${BOLD}       MongoDB Benchmark Configuration                ${CYAN}║${NC}"
echo -e "${CYAN}╠════════════════════════════════════════════════════════════╣${NC}"
echo -e "${CYAN}║${NC} ${YELLOW}Threads:${NC}           ${GREEN}${MongoDB_THREADS}${NC}"
echo -e "${CYAN}║${NC} ${YELLOW}Operation Count:${NC}   ${GREEN}${MongoDB_COUNT}${NC}"
echo -e "${CYAN}║${NC} ${YELLOW}Workloads:${NC}         ${GREEN}${MongoDB_SCENARIO[@]}${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"

# Create directories if they don't exist
mkdir -p "$raw_data_path"


declare -A mongodb_result
mongodb_result["origin_mongodb_workloada"]=0
mongodb_result["origin_mongodb_workloadb"]=0
mongodb_result["origin_mongodb_workloadc"]=0
mongodb_result["dravx_mongodb_workloada"]=0
mongodb_result["dravx_mongodb_workloadb"]=0
mongodb_result["dravx_mongodb_workloadc"]=0
mongodb_result["sde_mongodb_workloada"]=0
mongodb_result["sde_mongodb_workloadb"]=0
mongodb_result["sde_mongodb_workloadc"]=0

run_origin_MongoDB() {
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${WHITE}${BOLD}    🚀 Running ORIGIN MongoDB Benchmark (Baseline)   ${GREEN}║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Starting MongoDB server...${NC}"
    $MongoDB_cmd &
    pid=$!
    echo -e "${YELLOW}⏳ Waiting for MongoDB to start (PID: ${pid})${NC}"
    sleep 10
    for i in "${MongoDB_SCENARIO[@]}"; do
        echo -e "\n${CYAN}▶ Running workload: ${BOLD}${i}${NC}"
        echo -e "${WHITE}  Loading data...${NC}"
        python2 /eval/tools/ycsb-mongodb-binding-0.17.0/bin/ycsb load mongodb -s -P /eval/tools/ycsb-mongodb-binding-0.17.0/workloads/${i} -p mongodb.url="mongodb://localhost:27017/ycsb?w=0" -p operationcount=${MongoDB_COUNT} -p recordcount=100000  -p threadcount=${MongoDB_THREADS}
        echo -e "${WHITE}  Running benchmark...${NC}"
        python2 /eval/tools/ycsb-mongodb-binding-0.17.0/bin/ycsb run mongodb -s -P /eval/tools/ycsb-mongodb-binding-0.17.0/workloads/${i} -p mongodb.url="mongodb://localhost:27017/ycsb?w=0" -p operationcount=${MongoDB_COUNT} -p recordcount=100000  -p threadcount=${MongoDB_THREADS} | tee ${raw_data_path}/origin_mongodb_${i}.txt
        sleep 10
    done
    kill $pid
    echo -e "${GREEN}✓ Origin MongoDB benchmark completed${NC}\n"
}

run_dravx_MongoDB() {
    echo ""
    echo -e "${MAGENTA}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║${WHITE}${BOLD}      🔥 Running DR-AVX MongoDB Benchmark 🔥          ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${MAGENTA}║${NC} ${YELLOW}Mode:${NC} ${WHITE}Quick R/W with DR-AVX instrumentation${NC}"
    echo -e "${MAGENTA}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Starting MongoDB server with DR-AVX...${NC}"
    $Dravx_path -quick_rw 1 -- $MongoDB_cmd &
    pid=$!
    echo -e "${YELLOW}⏳ Waiting for MongoDB to start under DR-AVX (PID: ${pid})${NC}"
    sleep 10
    for i in "${MongoDB_SCENARIO[@]}"; do
        echo -e "\n${CYAN}▶ Running workload: ${BOLD}${i}${NC} ${MAGENTA}[with DR-AVX]${NC}"
        echo -e "${WHITE}  Loading data...${NC}"
        python2 /eval/tools/ycsb-mongodb-binding-0.17.0/bin/ycsb load mongodb -s -P /eval/tools/ycsb-mongodb-binding-0.17.0/workloads/${i} -p mongodb.url="mongodb://localhost:27017/ycsb?w=0" -p operationcount=${MongoDB_COUNT} -p recordcount=100000  -p threadcount=${MongoDB_THREADS}
        echo -e "${WHITE}  Running benchmark...${NC}"
        python2 /eval/tools/ycsb-mongodb-binding-0.17.0/bin/ycsb run mongodb -s -P /eval/tools/ycsb-mongodb-binding-0.17.0/workloads/${i} -p mongodb.url="mongodb://localhost:27017/ycsb?w=0" -p operationcount=${MongoDB_COUNT} -p recordcount=100000  -p threadcount=${MongoDB_THREADS} | tee ${raw_data_path}/dravx_mongodb_${i}.txt
        sleep 10
    done
    kill $pid
    echo -e "${MAGENTA}✓ DR-AVX MongoDB benchmark completed${NC}\n"
}

run_sde_MongoDB() {
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${WHITE}${BOLD}       ⚡ Running SDE MongoDB Benchmark ⚡            ${BLUE}║${NC}"
    echo -e "${BLUE}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${BLUE}║${NC} ${YELLOW}Mode:${NC} ${WHITE}Intel SDE with AVX-512 simulation${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Starting MongoDB server with Intel SDE...${NC}"
    $sde_path -avx512 1 -- $MongoDB_cmd &
    pid=$!
    echo -e "${YELLOW}⏳ Waiting for MongoDB to start under Intel SDE (PID: ${pid})${NC}"
    echo -e "${YELLOW}   (SDE emulation requires significant initialization time - 480s)${NC}"
    echo -e "${RED}${BOLD}   ⚠️  This may take up to 8 minutes, please be patient...${NC}"
    sleep 480
    for i in "${MongoDB_SCENARIO[@]}"; do
        echo -e "\n${CYAN}▶ Running workload: ${BOLD}${i}${NC} ${BLUE}[with Intel SDE]${NC}"
        echo -e "${WHITE}  Loading data...${NC}"
        python2 /eval/tools/ycsb-mongodb-binding-0.17.0/bin/ycsb load mongodb -s -P /eval/tools/ycsb-mongodb-binding-0.17.0/workloads/${i} -p mongodb.url="mongodb://localhost:27017/ycsb?w=0" -p operationcount=${MongoDB_COUNT} -p recordcount=100000  -p threadcount=${MongoDB_THREADS}
        echo -e "${WHITE}  Running benchmark...${NC}"
        python2 /eval/tools/ycsb-mongodb-binding-0.17.0/bin/ycsb run mongodb -s -P /eval/tools/ycsb-mongodb-binding-0.17.0/workloads/${i} -p mongodb.url="mongodb://localhost:27017/ycsb?w=0" -p operationcount=${MongoDB_COUNT} -p recordcount=100000  -p threadcount=${MongoDB_THREADS} | tee ${raw_data_path}/sde_mongodb_${i}.txt
        sleep 10
    done
    kill $pid
    echo -e "${BLUE}✓ SDE MongoDB benchmark completed${NC}\n"
}

get_mongodb_result() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${WHITE}${BOLD}          📊 Processing Benchmark Results              ${CYAN}║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Parsing output files and calculating metrics...${NC}\n"
    
    for i in "${MongoDB_SCENARIO[@]}"; do
        mongodb_result["origin_mongodb_${i}"]=$(python3 cal_mongodb_output.py ${raw_data_path}/origin_mongodb_${i}.txt)
        mongodb_result["dravx_mongodb_${i}"]=$(python3 cal_mongodb_output.py ${raw_data_path}/dravx_mongodb_${i}.txt)
        mongodb_result["sde_mongodb_${i}"]=$(python3 cal_mongodb_output.py ${raw_data_path}/sde_mongodb_${i}.txt)
    done
    
    echo -e "${YELLOW}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${YELLOW}║${WHITE}${BOLD}                   FINAL RESULTS                       ${YELLOW}║${NC}"
    echo -e "${YELLOW}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${YELLOW}║${NC} ${GREEN}${BOLD}ORIGIN (Baseline):${NC}"
    echo -e "${YELLOW}║${NC}   Read + Update: ${WHITE}${mongodb_result["origin_mongodb_workloadb"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read Only:     ${WHITE}${mongodb_result["origin_mongodb_workloadc"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read + Insert: ${WHITE}${mongodb_result["origin_mongodb_workloadd"]}${NC}"
    echo -e "${YELLOW}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${YELLOW}║${NC} ${MAGENTA}${BOLD}DR-AVX:${NC}"
    echo -e "${YELLOW}║${NC}   Read + Update: ${WHITE}${mongodb_result["dravx_mongodb_workloadb"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read Only:     ${WHITE}${mongodb_result["dravx_mongodb_workloadc"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read + Insert: ${WHITE}${mongodb_result["dravx_mongodb_workloadd"]}${NC}"
    echo -e "${YELLOW}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${YELLOW}║${NC} ${BLUE}${BOLD}Intel SDE:${NC}"
    echo -e "${YELLOW}║${NC}   Read + Update: ${WHITE}${mongodb_result["sde_mongodb_workloadb"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read Only:     ${WHITE}${mongodb_result["sde_mongodb_workloadc"]}${NC}"
    echo -e "${YELLOW}║${NC}   Read + Insert: ${WHITE}${mongodb_result["sde_mongodb_workloadd"]}${NC}"
    echo -e "${YELLOW}╚════════════════════════════════════════════════════════════╝${NC}"
}

draw_mongodb_chart() {
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${WHITE}${BOLD}             📈 Generating Charts                      ${GREEN}║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo -e "${WHITE}Creating visualization charts from benchmark data...${NC}\n"
    
    # Prepare data string for Python script
    mongodb_data_string="origin_mongodb_read_update: ${mongodb_result["origin_mongodb_workloadb"]}
origin_mongodb_read: ${mongodb_result["origin_mongodb_workloadc"]}
origin_mongodb_read_insert: ${mongodb_result["origin_mongodb_workloadd"]}
dravx_mongodb_read_update: ${mongodb_result["dravx_mongodb_workloadb"]}
dravx_mongodb_read: ${mongodb_result["dravx_mongodb_workloadc"]}
dravx_mongodb_read_insert: ${mongodb_result["dravx_mongodb_workloadd"]}
sde_mongodb_read_update: ${mongodb_result["sde_mongodb_workloadb"]}
sde_mongodb_read: ${mongodb_result["sde_mongodb_workloadc"]}
sde_mongodb_read_insert: ${mongodb_result["sde_mongodb_workloadd"]}"
    
    # Call Python script to generate chart
    python3 draw_mongodb.py "$mongodb_data_string"
    
    echo -e "${GREEN}✓ Chart generation completed${NC}"
    echo -e "${WHITE}Charts saved to: ${CYAN}${raw_data_path}${NC}\n"
}

echo ""
echo -e "${WHITE}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo -e "${WHITE}${BOLD}    🚀 MongoDB Benchmark Suite - Starting Tests 🚀          ${NC}"
echo -e "${WHITE}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo ""

run_origin_MongoDB
echo -e "${YELLOW}⏸️  Cooling down period (60s) before next test...${NC}"
sleep 60

run_dravx_MongoDB
echo -e "${YELLOW}⏸️  Cooling down period (60s) before next test...${NC}"
sleep 60

run_sde_MongoDB
echo -e "${YELLOW}⏸️  Brief pause (10s) before processing results...${NC}"
sleep 10

get_mongodb_result
draw_mongodb_chart

echo ""
echo -e "${GREEN}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}${BOLD}    ✅ MongoDB Benchmark Suite Completed Successfully ✅    ${NC}"
echo -e "${GREEN}${BOLD}════════════════════════════════════════════════════════════${NC}"
echo -e "${WHITE}Raw data: ${CYAN}${raw_data_path}${NC}"
echo ""

