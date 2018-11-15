#!/bin/bash
declare -a executables=("host_mmap" "host_shm" "host_mq" "host_pipe" "host_fifo" "host_sock")
ex_num=${#executables[@]}
declare -a ex_msg=("Running mmap executable" "Running shared memory executable" "Running message queue executable" "Running pipe executable" "Running fifo executable" "Running sockets executable")
declare -a ex_out=("tmp1" "tmp2" "tmp3" "tmp4" "tmp5" "tmp6")
press_enter="Press ENTER to continue"
press_enter_start="Press ENTER to start"

#Check for make run
for executable in "${executables[@]}"
do
  if [[ ! -f "$executable" ]]; then
    echo "Please run make before test"
    exit 1
  fi 
done

#Run every executable
for i in $(seq 1 $ex_num)
do
  executable=${executables[$i-1]}
  message=${ex_msg[$i-1]}
  out_file=${ex_out[$i-1]}
  echo $message
  read -p "$press_enter_start"
  echo
  ./$executable > $out_file
  cat $out_file
  read -p "$press_enter"
  echo
done

#Check that every output is equal
for i in $(seq 2 $ex_num)
do 
  DIFF=$(diff ${ex_out[$i-1]} ${ex_out[$i-2]})
  if [ "$DIFF" != "" ]; then
    echo "Output files difference detected! ${executables[$i-1]} and ${executables[$i-2]} "
    exit 1
  fi
done
echo "Output files have no differences"
read -p "$press_enter"
#Clean everything
echo "Cleaning execution output files..."
rm -f tmp*
