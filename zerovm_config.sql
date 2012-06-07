PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE channels(nodename text,  endpoint text,  fmode character(1), fd int);

INSERT INTO channels VALUES('source', 'ipc:///tmp/histograms-%d', 'w',  3);
INSERT INTO channels VALUES('source', 'ipc:///tmp/detailed-histogram_req-%d', 'w',  4);
INSERT INTO channels VALUES('source', 'ipc:///tmp/detailed-histogram_rep-%d', 'r',  5);
INSERT INTO channels VALUES('source', 'ipc:///tmp/range-request-%d', 'r',  6);

INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-12', 'w', 7);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-13', 'w', 8);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-14', 'w',  9);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-15', 'w',  10);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-16', 'w',  11);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-17', 'w',  12);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-18', 'w',  13);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-19', 'w',  14);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-20', 'w',  15);
INSERT INTO channels VALUES('source', 'ipc:///tmp/ranges-%d-21', 'w',  16);

INSERT INTO channels VALUES('source', 'ipc:///tmp/crc-%d', 'w', 17);

INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-2-%d', 'r', 3);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-3-%d', 'r', 4);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-4-%d', 'r', 5);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-5-%d', 'r', 6);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-6-%d', 'r', 7);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-7-%d', 'r', 8);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-8-%d', 'r', 9);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-9-%d', 'r', 10);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-10-%d', 'r', 11);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/ranges-11-%d', 'r', 12);
INSERT INTO channels VALUES('dest', 'ipc:///tmp/sort-result-%d', 'w', 13);

INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-2', 'r', 3);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-3', 'r', 4);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-4', 'r', 5);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-5', 'r', 6);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-6', 'r', 7);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-7', 'r', 8);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-8', 'r', 9);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-9', 'r', 10);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-10', 'r', 11);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/histograms-11', 'r', 12);

INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-2', 'w', 13);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-3', 'w', 14);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-4', 'w', 15);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-5', 'w', 16);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-6', 'w', 17);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-7', 'w', 18);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-8', 'w', 19);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-9', 'w', 20);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-10', 'w', 21);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_rep-11', 'w', 22);

INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-2', 'r', 23);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-3', 'r', 24);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-4', 'r', 25);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-5', 'r', 26);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-6', 'r', 27);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-7', 'r', 28);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-8', 'r', 29);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-9', 'r', 30);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-10', 'r', 31);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/detailed-histogram_req-11', 'r', 32);

INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-2', 'w', 33);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-3', 'w', 34);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-4', 'w', 35);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-5', 'w', 36);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-6', 'w', 37);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-7', 'w', 38);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-8', 'w', 39);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-9', 'w', 40);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-10', 'w', 41);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/range-request-11', 'w', 42);

INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-12', 'r',  43);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-13', 'r',  44);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-14', 'r',  45);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-15', 'r',  46);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-16', 'r',  47);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-17', 'r',  48);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-18', 'r',  49);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-19', 'r',  50);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-20', 'r',  51);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/sort-result-21', 'r',  52);

INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-2', 'r',  53);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-3', 'r',  54);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-4', 'r',  55);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-5', 'r',  56);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-6', 'r',  57);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-7', 'r',  58);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-8', 'r',  59);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-9', 'r',  60);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-10', 'r',  61);
INSERT INTO channels VALUES('manager', 'ipc:///tmp/crc-11', 'r',  62);

INSERT INTO channels VALUES('test1', 'ipc:///tmp/testa', 'r',  3);
INSERT INTO channels VALUES('test1', 'ipc:///tmp/testb', 'w',  4);
INSERT INTO channels VALUES('test2', 'ipc:///tmp/testb', 'r',  3);
INSERT INTO channels VALUES('test2', 'ipc:///tmp/testa', 'w',  4);
COMMIT;
