#!/bin/sh

rm -rf iso
mkdir iso
cd iso
mkdir boot
mkdir boot/grub
cd boot/grub

echo "set timeout=15" > grub.cfg
echo "set default=0" >> grub.cfg
echo "menuentry \"RTLK\" {" >> grub.cfg
echo "multiboot /boot/kernel.bin" >> grub.cfg
echo "   boot" >> grub.cfg                
echo "}" >> grub.cfg

cd ../../
cp ../build/x86-qemu/pok.elf ./boot/kernel.bin
grub-mkrescue -o bootable.iso .

mv bootable.iso ../
cd ..
rm -rf iso
