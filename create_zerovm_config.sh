if [ ! -f zvm_netw.db ]
then
/usr/local/bin/sqlite3 zvm_netw.db < zerovm_config.sql
fi
mkdir -p /tmp/zmq
