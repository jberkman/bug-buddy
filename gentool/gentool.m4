##
## gentool - some funky makefile rules for some generated files in gnome
##
## used in the style of intltool
##
## Copyright 2001 Ximian, Inc.
## Author: jacob berkman  <jacob@ximian.com>
##
## don't expect much yet, this is just a toy.
##

AC_DEFUN(AC_PROG_GENTOOL,
[

GENTOOL_MARSHAL_C_RULE='%.c: %.list $(GLIB_GENMARSHAL) ; $(GLIB_GENMARSHAL) $($(subst -,_,[$]*)_list_genmarshal_flags) $< --body > xgen-[$]@ && (cmp -s xgen-[$]@ [$]@ || cp xgen-[$]@ [$]@) && rm -f xgen-[$]@'
GENTOOL_MARSHAL_H_RULE='%.h: %.list $(GLIB_GENMARSHAL) ; $(GLIB_GENMARSHAL) $($(subst -,_,[$]*)_list_genmarshal_flags) $< --header > xgen-[$]@ && (cmp -s xgen-[$]@ [$]@ || cp xgen-[$]@ [$]@) && rm -f xgen-[$]@'
GENTOOL_BUILTINS_H_RULE='%.h: %.builtins $($(subst -,_,[$]*)_builtins_headers) $(GLIB_MKENUMS) Makefile ; files="" && type_prefix="$($(subst -,_,[$]*)_builtins_TYPE_PREFIX)_" && if test $$type_prefix = "_" ; then type_prefix="" ; fi && for file in $($(subst -,_,[$]*)_builtins_headers) ; do if test -f $(builddir)/$$file  ; then files="$$files $(builddir)/$$file" ; elif test -f $(srcdir)/$$file ; then files="$$files $(srcdir)/$$file" ; fi done && $(GLIB_MKENUMS) --fprod "\n/* enumerations from \"@filename@\" */" --eprod "#define "$$type_prefix"TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" --eprod "GType @enum_name@_get_type (void);\n" $$files > xgen-[$]@ && (cmp -s xgen-[$]@ [$]@ || cp xgen-[$]@ [$]@) && rm -f xgen-[$]@'

AC_SUBST(GENTOOL_MARSHAL_C_RULE)
AC_SUBST(GENTOOL_MARSHAL_H_RULE)
AC_SUBST(GENTOOL_BUILTINS_H_RULE)

])
