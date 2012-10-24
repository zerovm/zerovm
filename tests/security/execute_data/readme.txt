todo: FIXED
this is example of a bad behavior: zvm must not crash with "segmentation fault", but 
show a message like "data execution is prohibited".

zerovm must show something like:
validator state = 1
user return code = 0
etag = 2938C63A8E8D60E06E51F52600A35EDB
accounting = 20 0 33554432 0 0 0 0 0 0 0 0 0
exit state = Signal 11 from untrusted code: Halting at 0x440A00110020
