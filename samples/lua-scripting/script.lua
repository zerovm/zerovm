str = "foo"
str = str:gsub("^%l", string.upper)
=str
str = "_foo"
str = str:gsub("%a", string.upper, 1)
=str