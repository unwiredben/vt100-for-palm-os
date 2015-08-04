# $Id: Makefile,v 1.6 1998/02/24 20:06:41 swetland Exp $
thisdir := $(shell basename $$PWD)
HTML=html

#### Make macros to substitute the associated macros in .html.in files ###
# factor to enlarge images for presentation on the Web
ZOOMFACTOR=4

# Uniform page bodies coloring (the images background is transparent,
# so their conversion flags need not generally be changed with this
# unless too dark a background is selected).
#BODY_COLORS=BGCOLOR="\#FFFFFF" # Brian prefers this one
BODY_COLORS=# Vassilii relies on user's dflt


# The Significant Us
BRIAN=<A HREF="$(BRIAN_URL)">$(BRIAN_NAME)</A>
VASSILII=<A HREF="$(VASSILII_URL)">$(VASSILII_NAME)</A>
# See also the parent makefile.
##########################################################################

CONVERT = convert
CONVERT_GIFFLAGS = -interlace Line
.PHONY: all prep

all: $(PRC) web

$(PRC): ../$(PRC)
	cp $< $@

clean: webclean
	-rm -rf *~

### Web stuff
.PHONY: web webclean

web: $(HTML) $(HTML)/index.html $(HTML)/ToDo.html $(HTML)/ChangeLog.html

$(HTML):
	mkdir $@

$(HTML)/%: ./%
	cp $< $@

# For consistency, the zip/gzip files should be wrapped into
# macros -- both in the makefile and the HTML
$(HTML)/%: %.in
	ICON_GIF_GEOM=$(shell echo $(word 2,$(shell \
				$(CONVERT) -verbose $(HTML)\/icon.gif NULL: 2>&1)) |\
				sed -e 's/+.*//')\
	FONT_GIF_GEOM=$(shell echo $(word 2,$(shell \
				$(CONVERT) -verbose $(HTML)\/font.gif NULL: 2>&1)) |\
				sed -e 's/+.*//')\
	ZOOM_FONT_GIF_GEOM=$(shell echo $(word 2,$(shell \
				$(CONVERT) -verbose $(HTML)\/zoom.font.gif NULL: 2>&1)) |\
				sed -e 's/+.*//')\
	;(\
	ICON_GIF_WIDTH=`echo $$ICON_GIF_GEOM | sed -e 's/x.*//'`\
	ICON_GIF_HEIGHT=`echo $$ICON_GIF_GEOM | sed -e 's/[^x]*x//'`\
	FONT_GIF_WIDTH=`echo $$FONT_GIF_GEOM | sed -e 's/x.*//'`\
	FONT_GIF_HEIGHT=`echo $$FONT_GIF_GEOM | sed -e 's/[^x]*x//'`\
	ZOOM_FONT_GIF_WIDTH=`echo $$ZOOM_FONT_GIF_GEOM | sed -e 's/x.*//'`\
	ZOOM_FONT_GIF_HEIGHT=`echo $$ZOOM_FONT_GIF_GEOM | sed -e 's/[^x]*x//'`\
		;(sed \
		-e 's/@ZOOMFACTOR@/$(ZOOMFACTOR)/g' \
		-e 's/@PRC@/$(PRC)/g' \
		-e 's/@ICONTEXT@/$(ICONTEXT)/g' \
		-e 's/@RELEASE@/$(RELEASE)/g' \
		-e 's/@ZIPSIZE@/$(word 5,$(shell ls -l $(HTML)/$(APP).zip ))/g' \
		-e 's/@GZIPSIZE@/$(word 5,$(shell ls -l $(HTML)/$(PRC).gz ))/g' \
		-e 's/@PRCSIZE@/$(word 5,$(shell ls -l $(PRC) ))/g' \
		-e 's/@BRIAN_MAIL@/$(BRIAN_MAIL)/g' \
		-e 's/@BRIAN_URL@/$(subst /,\/,$(BRIAN_URL))/g' \
		-e 's/@BRIAN@/$(subst /,\/,$(BRIAN))/g' \
		-e 's/@VASSILII_MAIL@/$(VASSILII_MAIL)/g' \
		-e 's/@VASSILII_URL@/$(subst /,\/,$(VASSILII_URL))/g' \
		-e 's/@VASSILII@/$(subst /,\/,$(VASSILII))/g' \
		-e "s/@ICON_GIF_WIDTH@/$$ICON_GIF_WIDTH/g"\
		-e "s/@ICON_GIF_HEIGHT@/$$ICON_GIF_HEIGHT/g"\
		-e "s/@FONT_GIF_WIDTH@/$$FONT_GIF_WIDTH/g"\
		-e "s/@FONT_GIF_HEIGHT@/$$FONT_GIF_HEIGHT/g"\
		-e "s/@ZOOM_FONT_GIF_WIDTH@/$$ZOOM_FONT_GIF_WIDTH/g"\
		-e "s/@ZOOM_FONT_GIF_HEIGHT@/$$ZOOM_FONT_GIF_HEIGHT/g"\
		-e 's/@BODY_COLORS@/$(BODY_COLORS)/g'\
		$< > $@ ))

$(HTML)/index.html: $(HTML)/icon.gif $(HTML)/font.gif $(HTML)/zoom.font.gif
$(HTML)/index.html: $(HTML)/about.gif $(HTML)/login.gif
$(HTML)/index.html: $(HTML)/emacs.gif $(HTML)/vi.gif
$(HTML)/index.html: $(HTML)/$(APP).zip $(HTML)/$(PRC).gz
$(HTML)/index.html: $(HTML)/LICENSE

$(HTML)/$(APP).zip: $(PRC)
	-rm -f $@
	zip $@ $<

$(HTML)/$(PRC).gz: $(PRC)
	gzip -9 < $< > $@

$(HTML)/$(PRC).bz2: $(PRC)
	bzip2 -9 < $< > $@

$(HTML)/%.gif: %.ppm
	$(CONVERT) $(CONVERT_GIFFLAGS) $< gif:$@

$(HTML)/%.gif: %.pbm
	$(CONVERT) $(CONVERT_GIFFLAGS) -transparency white $< gif:$@

# convert is cute, but it smooths the enlarged image; can that be switched off?
# We enlarge them to emphasize the pixels!!!!
#	$(CONVERT) -geometry $(ZOOMFACTOR)00%x$(ZOOMFACTOR)00% $< $@
# XXX the above is the command that produces the smoothing effect
# So we use the giftopnm and pnmenlarge from the Jef Poskanzer's
# image convertors suite instead:
zoom.%.gif: %.gif
	giftopnm $< | pnmenlarge $(ZOOMFACTOR) | \
		$(CONVERT) $(CONVERT_GIFFLAGS) -transparency white - gif:$@

webclean:
	-rm -rf html *.prc

### end
