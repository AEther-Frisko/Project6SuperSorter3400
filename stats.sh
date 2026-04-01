echo "Time,Name,CPU,Memory,NetIO,BlockIO" > stats.csv
while true; do
  timestamp=$(date +"%H:%M:%S")
  docker stats --no-stream --format "$timestamp,{{.Name}},{{.CPUPerc}},{{.MemUsage}},{{.NetIO}},{{.BlockIO}}" >> stats.csv
  sleep 1
done