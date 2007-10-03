mkdir backup

move opald_2005.dtf backup\
copy ..\..\lib\debug\opald.def opald_2005.dtf
move opal_2005.dtf backup\
copy ..\..\lib\release\opal.def opal_2005.dtf
move opaln_2005.dtf backup\
copy "..\..\lib\no trace\opaln.def" opaln_2005.dtf

move opald_2005_wm.dtf backup\
copy ..\..\lib\wm5\debug\opald.def opald_2005_wm.dtf
move opal_2005_wm.dtf backup\
copy ..\..\lib\wm5\release\opal.def opal_2005_wm.dtf
move opaln_2005_wm.dtf backup\
copy "..\..\lib\wm5\no trace\opaln.def" opaln_2005_wm.dtf

move opald_2005_wm6.dtf backup\
copy ..\..\lib\wm6\debug\opald.def opald_2005_wm6.dtf
move opal_2005_wm6.dtf backup\
copy ..\..\lib\wm6\release\opal.def opal_2005_wm6.dtf
move opaln_2005_wm6.dtf backup\
copy "..\..\lib\wm6\no trace\opaln.def" opaln_2005_wm6.dtf

pause