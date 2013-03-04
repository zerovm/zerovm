todo:
this is example of a bad behavior: zvm must not crash with "segmentation fault",
but show a message like "data execution is prohibited".

for now zerovm shows:
This should fault... // message from the user module 
validator state = 0
user return code = 0
etag = 87ca93f96d6b5e7b16d409fd7ea8130cb69f812f
accounting = 0.00 0.00 33554432 0 0 0 1 21 0 0 0 0
exit state = Signal 11 from untrusted code: Halting at 0x440A00040000
