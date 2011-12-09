ZeroVM (zero virtual machine)

_______________
1. description |

minimalistic sandbox for linux/x86/64. based on Google NaCl project.

_______________
2. details:    |

- nexe validator was taken from the NaCl "as is"
- srpc module - removed
- syscalls disabling switch - added
- manifest (mechanism alternative to command line) - added
- added mechanism to pass information to ward nexe via its stack
- input/output file mapping mechanism added

_______________
3. structure   |

api/ -- reserved for a future
gtest/ -- google testing framwork
include/ -- sandbox header files
obj/ -- folder to hold intermediate files due compilation
samples/ -- examples ro run under sandbox control
src/ -- sandbox source files
test/ -- unit tests

_______________
4. todo        |

- add paging mechanism
- add zero mq channels
- nexe loader speed up
- replace all syscalls with single tonnel syscall
- further code base reducing

_______________
5. API         |

under construction

_______________
6. setup       |

under construction

