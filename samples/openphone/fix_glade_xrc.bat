@PATH=%CYGWIN%\bin;%PATH%

bash -c 'sed --regexp-extended --in-place -e "s/\|wxTHICK_FRAME//" -e "s/(class.*)wxListBox(.*wxCheckListBox)/\1wxCheckListBox\2/" openphone.xrc'
pause
