comments about blob library i am going to use for zrt
-----------------------------------------------------

facts:
1. if blob library is specified in manifest file it will be loaded due
   initialization. if blob is loaded it will replace user entry point
   (means: library will take control before nexe)
2. blob library must be dynamic. only "dynamic text" will be loaded by zerovm
3. blob library loading can be disabled by environment variable 
   NACL_DISABLE_DYNAMIC_LOADING. this (and all other) variable should be
   replaced by manifest keys
   
notes:
- zerovm source codes for dynamic text loading use some imc functionality
- some dynamic text loader routinies has needless defines (for windows e.g.)

plans:
1. create proper blob library and load it with zerovm
2. substitute i/o syscalls with simple procedures uses input/output/user_log
3. write tests to ensure zrt is working as meant
4. replace "simple" i/o syscalls with "sqllite/fs" combo. add the rest
   of the usable syscalls.
5. complete zrt with basic implementation of "threads" syscalls

note: zerovm does not contain "multimedia" syscalls. firstly it is
inherited from sel_ldr, secondly it was made intensionally: zerovm is not
interactive application and will not support interactive nexes in the nearest
future.

