mkdir backup

move opald_2012_x86.dtf backup\
copy ..\..\lib\Win32\debug\opald.def opald_2012_x86.dtf
move opal_2012_x86.dtf backup\
copy ..\..\lib\Win32\release\opal.def opal_2012_x86.dtf
move opaln_2012_x86.dtf backup\
copy "..\..\lib\Win32\No Trace\opaln.def" opaln_2012_x86.dtf

move opald_2012_x64.dtf backup\
copy ..\..\lib\x64\debug\opal64d.def opald_2012_x64.dtf
move opal_2012_x64.dtf backup\
copy ..\..\lib\x64\release\opal64.def opal_2012_x64.dtf
move opaln_2012_x64.dtf backup\
copy "..\..\lib\x64\no trace\opal64n.def" opaln_2012_x64.dtf

pause