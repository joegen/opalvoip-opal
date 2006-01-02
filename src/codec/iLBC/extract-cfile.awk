BEGIN { srcname = "nothing"; }
{ 
  if (/^A\.[0-9][0-9]*\.  [a-zA-Z][a-zA-Z_0-9]*\.[ch]/) {
    if (srcname != "nothing")
      close(srcname);
    srcname = $2;
    printf("creating source file %s\n", srcname);
  }else if (srcname != "nothing") {
    if (/^Andersen, et al\./ || /^RFC 3951/)
      printf("skipping %s\n", $0);
    else
      print $0 >> srcname;
  }
}
END {
  printf("ending file %s\n", srcname);
  close(srcname);
}
