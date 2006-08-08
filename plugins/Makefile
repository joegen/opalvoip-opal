SUBDIRS = audio/GSM0610 \
          audio/Speex \
          audio/iLBC \
	  audio/LPC_10 \
	  audio/G726 \
	  audio/IMA_ADPCM \
	  audio/gsm-amr \
#	  video/H.261-vic \
#	  video/H.263-ffmpeg

all debug opt both \
optshared debugshared bothshared \
optnoshared debugnoshared bothnoshared:
	@set -e; $(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir); )

clean optclean debugclean:
	@set -e; $(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) clean; )

install uninstall:
	@set -e; $(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) $@; )

optdepend debugdepend bothdepend release tagbuild:
	@echo
