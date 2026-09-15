#!/bin/sh

# DODAJ TE LINIE NA POCZ¥TKU - WA¯NE!
ulimit -v unlimited
ulimit -m unlimited
ulimit -d unlimited
ulimit -s unlimited

echo -e "\033[32m"
read -p "Ile channeli chcesz uruchomiæ? (1/4) : " rch

if [ -z $rch ] || [ $rch -lt 1 ] || [ $rch -gt 4 ] ; then
	exit
fi

echo -e "\033[0m"
./clear2.sh
echo -e "\033[32m"

SFROOT=$PWD
echo -e "# ---------------------------------------------------------------------------- #"
echo -e "# Uruchamianie db..."
cd $SFROOT/db/
# ZAST¥P TÊ LINIÊ:
# ./db >> start.log 2>&1 & sleep 2
# NA T¥ (z limitami):
(ulimit -v unlimited; ulimit -d unlimited; ./db) >> start.log 2>&1 & sleep 2

i1=1
while [ $i1 -le $rch ]
do
	for i2 in 1 2 3
	do
		echo -e "# Uruchamianie ch$i1$i2..."
		cd $SFROOT/chs/ch$i1$i2/
		# ZAST¥P NA:
		(ulimit -v unlimited; ulimit -d unlimited; ./ch$i1$i2) >> start.log 2>&1 & sleep 1
	done
	i1=$(( $i1 + 1 ))
done

echo -e "# Uruchamianie ch99..."
cd $SFROOT/chs/ch99/
(ulimit -v unlimited; ulimit -d unlimited; ./ch99) >> start.log 2>&1 & sleep 1

echo -e "# Uruchamianie ch00..."
cd $SFROOT/chs/ch00/
(ulimit -v unlimited; ulimit -d unlimited; ./ch00) >> start.log 2>&1 & sleep 1

echo -e "# Uruchamianie ch01..."
cd $SFROOT/chs/ch01/
(ulimit -v unlimited; ulimit -d unlimited; ./ch01) >> start.log 2>&1 & sleep 1

echo -e "# Uruchamianie auth..."
cd $SFROOT/auth/
(ulimit -v unlimited; ulimit -d unlimited; ./auth) >> start.log 2>&1 & sleep 1

echo -e "# ---------------------------------------------------------------------------- #"
echo -e "\033[0m"