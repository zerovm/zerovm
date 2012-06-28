if [ ! -f gtest/data/zerovm_test.db ]
then
/usr/local/bin/sqlite3 gtest/data/zerovm_test.db < gtest/data/sqluse_test_config.sql
fi
