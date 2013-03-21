rm $HOME/sign/kernel/zImage
rm $HOME/sign/system/lib/modules/*.ko
cp arch/arm/boot/zImage ../../sign/kernel/zImage
find . -name "*.ko" -exec cp {} ../../sign/system/lib/modules \;
make clean -j10
