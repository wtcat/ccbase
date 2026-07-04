
..\regen.exe --dir=..\IMG --format=rgb565 --compress=lz4 --groups=ani,Date,Hr,Min,readiness --out=res.bin --verbose
::..\regen.exe --dir=..\IMG --format=rgb565 --compress=none --groups=health --out=res.bin --verbose
pause