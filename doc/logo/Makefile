# SPDX-License-Identifier: GPL-2.0-or-later

$(obj)/%.bmp: $(obj)/%.svg
	$(Q)$(BASH) $(src)/logogen.sh

$(obj)/logo.c: $(obj)/logo.bmp
	$(Q)$(MAKE) $(build)=tools/logo
	$(Q)tools/logo/bmp2c -o $@ $^

obj-y += logo.o terminal.o
clean-y += logo.c *.bmp
