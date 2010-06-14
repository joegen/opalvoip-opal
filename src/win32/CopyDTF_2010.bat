mkdir backup

move opald_2010.dtf backup\
copy ..\..\lib\debug\opald.def opald_2010.dtf
move opal_2010.dtf backup\
copy ..\..\lib\release\opal.def opal_2010.dtf
move opaln_2010.dtf backup\
copy "..\..\lib\no trace\opaln.def" opaln_2010.dtf

rem move opald_2010_wm.dtf backup\
rem copy ..\..\lib\wm5ppc\debug\opald.def opald_2010_wm.dtf
rem move opal_2010_wm.dtf backup\
rem copy ..\..\lib\wm5ppc\release\opal.def opal_2010_wm.dtf
rem move opaln_2010_wm.dtf backup\
rem copy "..\..\lib\wm5ppc\no trace\opaln.def" opaln_2010_wm.dtf

rem move opald_2010_wm6.dtf backup\
rem copy ..\..\lib\wm6pro\debug\opald.def opald_2010_wm6.dtf
rem move opal_2010_wm6.dtf backup\
rem copy ..\..\lib\wm6pro\release\opal.def opal_2010_wm6.dtf
rem move opaln_2010_wm6.dtf backup\
rem copy "..\..\lib\wm6pro\no trace\opaln.def" opaln_2010_wm6.dtf

pause