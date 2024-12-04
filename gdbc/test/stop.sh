MAX=4096
pid=0

for pid in $(seq 0 $(($MAX - 1)))
do
  curl -X POST "http://localhost:10010/stop?pid=$pid" \
       -H "Content-Type: application/json"
done