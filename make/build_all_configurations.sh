#~/bin/sh

outfile="$1"
if [ -z "$outfile" ]; then
  echo usage: $0 outfile
  exit 1
fi

if [ -z "$PTLIBDIR" ]; then
  echo "No PTLIBDIR set."
  exit 1
fi

if [ -z "$OPALDIR" ]; then
  echo "No OPALDIR set."
  exit 1
fi

PTLIB_OPTS=`$PTLIBDIR/configure --help | \
          sed --silent --regexp-extended \
              --expression=s/--disable-FEATURE// \
              --expression="s/^  (--disable-[^ ]+).*$/\\1/p"`

OPAL_OPTS=`$OPALDIR/configure --help | \
          sed --silent --regexp-extended \
              --expression=s/--disable-FEATURE// \
              --expression="s/^  (--disable-[^ ]+).*$/\\1/p"`

echo $0                                                             > $outfile
for opt in $PTLIB_OPTS ; do
  echo =========================================================   >> $outfile
  echo Trying $opt                                           | tee -a $outfile
  echo ---------------------------------------------------------   >> $outfile
  make -C $PTLIBDIR clean                                          >> $outfile 2>&1
  make -C $OPALDIR clean                                           >> $outfile 2>&1
  echo ---------------------------------------------------------   >> $outfile
  make -C $PTLIBDIR CONFIG_PARMS="$opt --enable-samples" config    >> $outfile 2>&1
  make -C $OPALDIR config                                          >> $outfile 2>&1
  if [ $? -ne 0 ]; then
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                    >> $outfile
    echo "!!!!!!!!!! CONFIG FAILED !!!!!!!!!!!"              | tee -a $outfile
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                    >> $outfile
  else
    echo --------------------------------------------------------- >> $outfile
    make -C $OPALDIR all                                           >> $outfile 2>&1
    if [ $? -ne 0 ]; then
      echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                   >> $outfile
      echo "!!!!!!!!!! BUILD FAILED !!!!!!!!!!!"             | tee -a $outfile
      echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                   >> $outfile
    fi
  fi
done

echo =========================================================     >> $outfile
echo Restoring full options                                  | tee -a $outfile
echo ---------------------------------------------------------     >> $outfile
make -C $PTLIBDIR clean                                            >> $outfile 2>&1
echo ---------------------------------------------------------     >> $outfile
make -C $PTLIBDIR  CONFIG_PARMS="--enable-samples" config          >> $outfile 2>&1
echo ---------------------------------------------------------     >> $outfile
make -C $PTLIBDIR all                                              >> $outfile 2>&1

for opt in $OPAL_OPTS ; do
  echo =========================================================   >> $outfile
  echo Trying $opt                                           | tee -a $outfile
  echo ---------------------------------------------------------   >> $outfile
  make -C $OPALDIR clean                                           >> $outfile 2>&1
  echo ---------------------------------------------------------   >> $outfile
  make -C $OPALDIR CONFIG_PARMS="$opt --enable-samples" config     >> $outfile 2>&1
  echo ---------------------------------------------------------   >> $outfile
  make -C $OPALDIR all                                             >> $outfile 2>&1
  if [ $? -ne 0 ]; then
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                     >> $outfile
    echo "!!!!!!!!!! BUILD FAILED !!!!!!!!!!!"               | tee -a $outfile
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                     >> $outfile
  fi
done

echo =========================================================     >> $outfile
echo "Completed building all possible configurations."       | tee -a $outfile

