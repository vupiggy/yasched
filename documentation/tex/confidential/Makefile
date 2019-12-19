### Makefile --- 

## Author: <lukehuang.ca@gmail.com>

PDF=yasched.pdf

default: $(PDF)

yasched.pdf:
	$(call cmd,latex)
	-$(call cmd,bibtex)
	$(call cmd,latex)
	$(call cmd,latex)

include ../../Makefile.in

### Makefile ends here
