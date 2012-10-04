mkdir backup

move opald_2010.dtf backup\
copy ..\..\lib\Win32\debug\opald.def opald_2010.dtf
move opal_2010.dtf backup\
copy ..\..\lib\Win32\release\opal.def opal_2010.dtf
move opaln_2010.dtf backup\
copy "..\..\lib\Win32\No Trace\opaln.def" opaln_2010.dtf

move opald_2010_x64.dtf backup\
copy ..\..\lib\x64\debug\opal64d.def opald_2010_x64.dtf
move opal_2010_x64.dtf backup\
copy ..\..\lib\x64\release\opal64.def opal_2010_x64.dtf
move opaln_2010_x64.dtf backup\
copy "..\..\lib\x64\no trace\opal64n.def" opaln_2010_x64.dtf

pause