# What we have below is some fun Makefile magic to run a script that modifies generated source
# code before the default 'build' and 'program' targets, which are also in untracked files
prebuild:
ifdef CAR
	@echo -e '\n\033[0;33mInjecting malware into generated source code\033[0m\n'
	@python3 rename_car.py $(CAR)
endif

build: prebuild
	@$(MAKE) -f common.mk $@

program: prebuild
	@$(MAKE) -f common.mk $@
