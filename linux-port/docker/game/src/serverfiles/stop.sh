#!/bin/sh
SFROOT=$PWD

echo -e "\033[32m"

echo -e "# ---------------------------------------------------------------------------- #"
echo -e "# Zatrzymywanie auth"
cd $SFROOT/auth/
if [ -r ./pid ]; then
	kill -1 `cat ./pid`
fi

for i1 in 1 2 3 4
do
	for i2 in 1 2 3
	do
		echo -e "# Zatrzymywanie ch$i1$i2"
		cd $SFROOT/chs/ch$i1$i2$i3/
		if [ -r ./pid ]; then
			kill -1 `cat ./pid`
		fi
	done
done

echo -e "# Zatrzymywanie ch99"
cd $SFROOT/chs/ch99/
if [ -r ./pid ]; then
	kill -1 `cat ./pid`
fi

echo -e "# Zatrzymywanie ch00"
cd $SFROOT/chs/ch00/
if [ -r ./pid ]; then
	kill -1 `cat ./pid`
fi

echo -e "# Zatrzymywanie ch01"
cd $SFROOT/chs/ch01/
if [ -r ./pid ]; then
	kill -1 `cat ./pid`
fi

echo -e "# Zatrzymywanie db"
cd $SFROOT/db/
if [ -r ./pid ]; then
	kill -1 `cat ./pid`
fi

echo -e "# ---------------------------------------------------------------------------- #"
echo -e "\033[0m"