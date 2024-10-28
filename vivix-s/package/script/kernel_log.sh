#!/bin/sh

file_count=$(ls -d /mnt/mmc/p3/kernel/* | wc -l)
while true
do
    pkill -f 'cat /proc/kmsg'
    dir_size=$(du -m /mnt/mmc/p3/kernel/ | awk '{print $1}')
    
    if [ $dir_size -gt "300" ]
    then
            file_name=$(ls -d -t1 /mnt/mmc/p3/kernel/* | tail -n 1)
            rm $file_name
    fi
                    
    log_file_name=$(date -I)
    
    cat /proc/kmsg >> /mnt/mmc/p3/kernel/$log_file_name.log &
    
    while true
    do
        sleep 1
        sync
        
        date_name=$(date -I)
        if [ $log_file_name != $date_name ]
        then
            break
        fi
    done
done
