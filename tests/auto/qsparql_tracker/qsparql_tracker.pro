include(../sparqltest.pri)
CONFIG += qt warn_on console depend_includepath
QT += testlib
SOURCES  += tst_qsparql_tracker.cpp

check.depends = $$TARGET
check.commands = ./tst_qsparql_tracker

memcheck.depends = $$TARGET
memcheck.commands = $$VALGRIND $$VALGRIND_OPT ./tst_qsparql_tracker

QMAKE_EXTRA_TARGETS += check memcheck

#QT = sparql # enable this later
