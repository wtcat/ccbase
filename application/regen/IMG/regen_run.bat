
..\regen.exe --dir=..\IMG --format=rgb565 --compress=lz4 --groups=ani,Date,Hr,Hour,Week,Min,readiness --filter=preview_4*:rgb565:none --out=res.bin --verbose
::..\regen.exe --dir=..\IMG --format=rgb565 --compress=none --groups=health --out=res.bin --verbose
pause