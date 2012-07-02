LUA_PACKAGE_PATH=lua-5.2.1

all: lua_package
	
lua_package:
	make ansi -C${LUA_PACKAGE_PATH}

clean:
	make clean -C${LUA_PACKAGE_PATH}
	rm -f *.nexe *.log *.result
