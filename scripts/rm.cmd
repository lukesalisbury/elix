@echo off
set string=%*
set string=%string:/=\%
del /S %string%