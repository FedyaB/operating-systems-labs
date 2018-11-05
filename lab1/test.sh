#!/bin/bash
daemon_name="lab_daemon"
config_name="config.txt"
press_enter_test="Press ENTER to start the test"
press_enter_test_end="Press ENTER to end the test"
press_enter="Press ENTER to continue"
#Make if daemon doesn't exist
if [ ! -f "$daemon_name" ]; then
  echo "Please run make before test"
  exit 1
fi
echo "DUMMY" >$config_name
echo "2000000000" >>$config_name
echo "0" >>$config_name
#Start daemon
echo "Starting daemon..."
./$daemon_name "start"
sleep 2
echo "Showing that daemon is in a process list. It's PID is $(cat ~/run/daemon.pid)"
ps ajx | egrep "PID|$daemon_name"
read -p "$press_enter"
echo
#First test
echo "Running the first test..."
echo "Command \"./$daemon_name start\". New instance should replace the outdated one"
./$daemon_name start
sleep 2
echo "Current process list state"
ps ajx | egrep "PID|$daemon_name"
read -p "$press_enter"
echo
echo "Current ~/run folder state"
ls -a ~/run/
echo
echo "Command \"./$daemon_name stop\". The daemon should clean ~/run/daemon.pid"
./$daemon_name stop
sleep 2
echo "Current process list state"
ps ajx | egrep "PID|$daemon_name"
echo
echo "Current ~/run folder state"
ls -a ~/run/
echo
read -p "$press_enter"
echo
#Second test
rm -f $config_name
echo "Running the second test..."
echo "The test represents reminder with a fixed date"
echo "Message appears in a new terminal instance after 5 + [0..10] seconds"
echo "Wait for 5 + [0..10] seconds after ENTER press to view the result. The result will automatically close after 10 seconds"
read -p "$press_enter_test"
v_date=$(date '+%Y-%m-%d %H:%M:%S' -d '+5 seconds')
echo "Date to be placed into config: $v_date"
./add_event $v_date "Second test message in a new terminal"
./$daemon_name start
read -p "$press_enter_test_end"
echo
#Third test
echo "Running the third test..."
echo "The test represents a periodic reminder"
echo "Note that we set event EPOCH time to 0, but the daemon should handle this case since it's a periodic event"
echo "Message appears in a new terminal instance every 10 seconds"
read -p "$press_enter_test"
rm -f $config_name
echo "Third test message in a new terminal" >> $config_name
echo 0 >> $config_name
echo 10 >> $config_name
echo "Sending SIGHUP to the running daemon..."
kill -SIGHUP $(cat ~/run/daemon.pid)
echo "Current process list state. The daemon PID from 2 test is $(cat ~/run/daemon.pid) (should be the same since SIGHUP)"
ps ajx | egrep "PID|$daemon_name"
echo
echo "Wait for 5 seconds to view the result"
read -p "$press_enter_test_end"
echo "End of the test. Building dummy config and sending SIGHUP to the running daemon..."
echo "DUMMY" >$config_name
echo "2000000000" >>$config_name
echo "0" >>$config_name
kill -SIGHUP $(cat ~/run/daemon.pid)
echo
#Fourth test
echo "Running the fourth test..."
echo "The test represents multiple (three) events at the same time"
echo "Message appears in a new terminal instance each after 5 + [0..10] seconds"
echo "Wait for 5 + [0..10] seconds to view the result"
read -p "$press_enter_test"
v_date=$(date '+%Y-%m-%d %H:%M:%S' -d '+5 seconds')
echo "Date to be placed into config: $v_date"
rm -f $config_name
echo "Sending SIGHUP to the running daemon..."
./add_event $v_date "Multiple"
./add_event $v_date "Events"
./add_event $v_date "Test"
kill -SIGHUP $(cat ~/run/daemon.pid)
read -p "$press_enter_test_end"
echo
#Cleaning up
echo "Cleaning up..."
echo "Deleting generated config..."
rm -f $config_name
echo "Sending SIGTERM to the running daemon..."
kill -SIGTERM $(cat ~/run/daemon.pid)
sleep 2
echo "Current process list state"
ps ajx | egrep "PID|$daemon_name"
echo
echo "Current ~/run folder state"
ls -a ~/run/
echo
