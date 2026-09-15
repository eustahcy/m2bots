#!/bin/sh

SFROOT=$PWD

DeleteFiles()
{
	rm -rf log/*
	rm -rf start.log
	rm -rf packet_info.txt
	rm -rf syserr
	rm -rf syslog
	rm -rf stdout
	rm -rf PTS
	rm -rf p2p_packet_info.txt
	rm -rf mob_count
	rm -rf DEV_LOG.log
	rm -rf VERSION.txt
	rm -rf udp_packet_info.txt
	rm -rf usage.txt
	rm -rf mob_data.txt
	rm -rf atak.txt
	rm -rf protection.txt

	rm -rf game.core
	rm -rf auth.core
	rm -rf db.core
	rm -rf ch11.core
	rm -rf ch12.core
	rm -rf ch21.core
	rm -rf ch22.core
	rm -rf ch31.core
	rm -rf ch32.core
	rm -rf ch41.core
	rm -rf ch42.core
	rm -rf ch99.core
}

cd $SFROOT/db/
DeleteFiles

for i1 in 1 2 3 4
do
	for i2 in 1 2 3
	do
		cd $SFROOT/chs/ch$i1$i2$i3/
		DeleteFiles
	done
done

# echo -e "Czyszczenie ch99"
cd $SFROOT/chs/ch99/
DeleteFiles

cd $SFROOT/auth/
DeleteFiles

cd $SFROOT/chs/ch00/
DeleteFiles

cd $SFROOT/chs/ch01/
DeleteFiles