#!/usr/bin/env bash

# always use a dot as decimal separator
LC_NUMERIC=C

SRV_OUTPUT=srv_output.txt
USR_OUTPUT=usr_output.txt

ADMIN_PASSWD="SOPE_admin"
USER_PASSWD="SOPE_user"

# stress tests run from account ID 1 up to this value
STRESS_MAX_ACCOUNT_ID=4096

srv_pid=-1
srv_args=""

usr_pids=()
usr_args=()
usr_times=()

show_usage() {
  echo "Usage: $0 <test_no [1-19]>"

  exit 1
}

print_server_info() { printf "  $srv_pid: $srv_args"; }

print_user_info() {
  local index=$(($1))

  printf "  [%08.3f s] ${usr_pids[$index]}: ${usr_args[$index]}" ${usr_times[$index]}
}

print_exit_status() { printf "exit status: ${1:-0}"; }

run_server() { 
  ./server $@ > $SRV_OUTPUT 2>&1 &
  srv_pid=$!
  srv_args="./server $@"
  printf "$(print_server_info)\n"
}

run_user() {
  local delay="${1:-0}"
  
  # delay
  [ ! "$delay" = "0" ] && sleep $delay

  ./user "${@:2:$#-1}" >> $USR_OUTPUT 2>&1 &

  usr_pids[${#usr_pids[*]}]=$!
  if [ $# -lt 6 ]; then
    usr_args[${#usr_args[*]}]="./user ${@:2:$#-1}"
  else
    usr_args[${#usr_args[*]}]="./user ${@:2:$#-2} \"${!#}\""
  fi

  if [ ${#usr_times[*]} -gt 0 ]; then
    usr_times[${#usr_times[*]}]="$(echo "${usr_times[-1]} + $delay" | bc)"
  else
    usr_times[${#usr_times[*]}]="$delay"
  fi

  printf "$(print_user_info -1)\n"
}

# kill all server and user processes if there are any running
killall_procs() {
  for proc in server user; do
    if pgrep -x $proc > /dev/null 2>&1; then
      $(killall -KILL $proc > /dev/null 2>&1)
    fi
  done
}

# clean any FIFOs and output files
clean_fifos_and_files() {
  rm -f $SRV_OUTPUT $USR_OUTPUT
  rm -f slog.txt ulog.txt
  rm -f /tmp/secure_*
}

start_server() {
  case "$1" in
    # 1) too few arguments
    1) run_server "3";;
    # 2) too many arguments
    2) run_server "3 too many";;
    # 3) no offices
    3) run_server "0 $ADMIN_PASSWD";;
    # 4) too many threads
    4) run_server "100 $ADMIN_PASSWD";;
    # 5) not a number
    5) run_server "A $ADMIN_PASSWD";;
    # 6) number with letters
    6) run_server "9Z $ADMIN_PASSWD";;
    # 7) huge number (segfault test)
    7) run_server "12345678901234567890 password";;
    # 8) long password (segfault test)
    8) run_server "1 extremely_long_password_just_too_see_if_it_causes_a_segfault";;
    # 9) single bank office
    9) run_server "1 $ADMIN_PASSWD";;
    # 10) 3 bank offices
    10) run_server "3 $ADMIN_PASSWD";;
    # 11+) 5 bank offices
    11|*) run_server "5 $ADMIN_PASSWD";;
  esac
}

start_users() {
  local i

  case "$1" in
    # 1) # missing argument list / too many arguments
    1)
      run_user 0 0 password 50 0
      run_user 0 0 password 50 0 3 "";;
    # 2) password too short
    2) run_user 0 0 pass 50 0 "";;
    # 3) password too long
    3) run_user 0 0 password_size_exceeds_limit 50 0 "";;
    # 4) invalid operation
    4) run_user 0 0 password 50 4 "";;
    # 5) invalid delay (too high)
    5) run_user 0 0 password 100000 3 "";;
    # 6) invalid account_id (too high) 
    6) run_user 0 4097 password 50 1 "";;
    # 7) huge number (segfault test)
    7) run_user 0 12345678901234567890 password 50 1 "";;
    # 8) long password (segfault test)
    8) run_user 0 1 extremely_long_password_just_too_see_if_it_causes_a_segfault 50 1 "";;
    # 9) ADMIN OP_NALLOW (BALANCE and TRANSFER) 
    9)
      run_user 0.1 0 $ADMIN_PASSWD 50 1 ""
      run_user 0.1 0 $ADMIN_PASSWD 50 2 "1 200"
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 10) plain shutdown 
    10) run_user 0 0 $ADMIN_PASSWD 500 3 "";;
    # 11) CREATE (accounts 1 and 19)
    11)
      run_user 0.1 0 $ADMIN_PASSWD 100 0 "1 200 $USER_PASSWD"
      run_user 0.1 0 $ADMIN_PASSWD 100 0 "19 123 $USER_PASSWD"
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 12) USER OP_NALLOW (CREATE and SHUTDOWN)
    12)
      run_user 0.5 0 $ADMIN_PASSWD 100 0 "1 234 $USER_PASSWD"
      run_user 0.1 1 $USER_PASSWD 35 0 "2 50 $ADMIN_PASSWD"
      run_user 0.1 1 $USER_PASSWD 78 3 ""
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 13) CREATE + BALANCE 
    13)
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "1 321 $USER_PASSWD"
      run_user 0.4 1 $USER_PASSWD 35 1 ""
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 14) CREATE + BALANCE (LOGIN_FAIL)
    14)
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "4095 321 $USER_PASSWD"
      run_user 0.4 1 $USER_PASSWD 35 1 ""
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 15) CREATE x 2 + TRANSFER + BALANCE
    15)
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "1 321 $USER_PASSWD"
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "20 123 $USER_PASSWD"
      run_user 0.6 20 $USER_PASSWD 400 2 "1 23"
      run_user 0.1 1 $USER_PASSWD 90 1 ""
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 16) SIMPLE DEADLOCK (SIMULTANEOUS TRANSFERS: 1 -> 2; 2 -> 1)
    16)
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "1 100 $USER_PASSWD"
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "2 250 $USER_PASSWD"
      run_user 0.1 1 $USER_PASSWD 500 2 "2 50"
      run_user 0.05 2 $USER_PASSWD 100 2 "1 100"
      run_user 1.2 1 $USER_PASSWD 50 1 ""
      run_user 0.05 2 $USER_PASSWD 50 1 ""
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 17) PROPER CLOSING
    17)
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "1 123 $USER_PASSWD"
      run_user 0.1 0 $ADMIN_PASSWD 85 0 "2 234 $USER_PASSWD"
      run_user 0.1 0 $ADMIN_PASSWD 500 3 ""
      run_user 0.05 1 $USER_PASSWD 50 1 ""
      run_user 0.05 2 $USER_PASSWD 50 1 ""
      run_user 0.6 2 $USER_PASSWD 50 3 "";;
    # 18) MULTIPLE SHUTDOWN
    18)
      run_user 0.05 0 $ADMIN_PASSWD 500 3 ""
      run_user 0.05 0 $ADMIN_PASSWD 400 3 ""
      run_user 0.05 0 $ADMIN_PASSWD 300 3 ""
      run_user 0.05 0 $ADMIN_PASSWD 200 3 "";;
    # 19) STRESS TEST (CREATE)
    19)
      for i in $(seq 1 $STRESS_MAX_ACCOUNT_ID); do
        run_user 0.001 0 $ADMIN_PASSWD 1 0 "$i $((i*2)) $USER_PASSWD"
      done
      run_user 0.1 0 $ADMIN_PASSWD 500 3 "";;
    # 20) STRESS TEST (SHUTDOWN)
    20)
      for i in $(seq 1 4 $STRESS_MAX_ACCOUNT_ID); do
        run_user 0.004 0 $ADMIN_PASSWD $(($STRESS_MAX_ACCOUNT_ID - $i)) 3 ""
      done;;
   *) echo "Invalid Test No!"; show_usage;;
  esac
}

# check number of arguments
[ $# -ne 1 ] && show_usage

# check test number
regex='^([1-9]|1[0-9]|20)$'
[[ ! "$1" =~ $regex ]] && echo "Invalid test no!" && show_usage

# integer value
testno=$(($1))

# 
# ensure that there are no FIFOs, files or processes running (server and user)
#
clean_fifos_and_files
killall_procs

#
# server
#

echo "SERVER:"
start_server $testno

#
# user(s)
#

echo ""
echo "USERS:"
start_users $testno

# invalid argument test cases
if [ $testno -le 8 ]; then
  sleep 2
# valid test cases
elif [ $testno -le 18 ]; then
  sleep 2
# stress test cases
else
  sleep 35
fi

# kill any server or user procs that are still running
killall_procs

echo ""
printf '%*s' 140 | tr ' ' '-';
echo ""

# get server exit status
echo "SERVER:"
wait $srv_pid
status=$?
printf "$(print_server_info) => $(print_exit_status $status)\n"
if [ $testno -gt 18 ] && [ -f slog.txt ]; then
  echo "(...)"
  cat slog.txt | tail -n 15
fi

echo ""
echo "USERS:"
if [ $testno -le 18 ]; then
  for i in ${!usr_pids[@]}; do
    wait ${usr_pids[$i]}
    status=$?
    printf "$(print_user_info $i) => $(print_exit_status $status)\n"
  done
elif [ -f ulog.txt ]; then
  echo "(...)"
  cat ulog.txt | tail -n 15
fi
