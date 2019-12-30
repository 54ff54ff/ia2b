HDR  = adt
SRC  = cmd aig util sat msat114 msat220 sfc glucose
MAIN = main
EXEC = ia2b

SRCIN = $(addsuffix \, $(SRC))
HDRIN = $(addsuffix \, $(HDR) $(SRC))

.PHONY: debug libs main cleanall cleandebug

all  : TARGET = target
debug: TARGET = debug
all debug: libs main

clean     : CLEAN = clean
cleanall  : CLEAN = cleanall
cleandebug: CLEAN = cleandebug
clean cleanall cleandebug: cleanSub

libs:
	@echo "*** Compiling ***"
	@for dir in $(SRC); \
	do \
		echo "Checking $$dir..."; \
		make -C src/$$dir -f ../make.sub $(TARGET) --no-print-directory FLAG=BUILD PKG=$$dir HDR=$(HDRIN) ; \
	done

main:
	@echo "Checking $(MAIN)...";
	@make -C src/$(MAIN) -f ../make.sub $(TARGET) --no-print-directory FLAG=BUILD EXEC=$(EXEC) HDR=$(HDRIN)  SRC=$(SRCIN) ; \

cleanSub:
	@echo "*** Cleaning  ***"
	@for dir in $(SRC); \
	do \
		echo "Cleaning $$dir..."; \
		make -C src/$$dir -f ../make.sub $(CLEAN) --no-print-directory FLAG=CLEAN PKG=$$dir; \
	done
	@echo "Cleaning $(MAIN)...";
	@make -C src/$(MAIN) -f ../make.sub $(CLEAN) --no-print-directory FLAG=CLEAN EXEC=$(EXEC); \
