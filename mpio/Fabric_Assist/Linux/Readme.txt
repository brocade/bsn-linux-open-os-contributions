Downloaded the linux base version 4.20 from https://github.com/torvalds/linux.git
All our further changes will be on top of this tree

Steps to compile the kernel.
1. Download the Git code from https://github.com/brocade/bsn-linux-open-os-contributions.git
2. From the clone directory goto fc transport linux path
        cd  mpio/Fabric_Assist/Linux/linux
3.copy the default config file from boot directory to the  linux like below
	cp /boot/config-4.16.3-301.fc28.x86_64 .config
4.run make menuconfig and enable FC Transport adapter module

enable the fc_transport_adapter module under 
	--->Device Drivers
	  -->SCSI device support
           --->SCSI low-level drivers
		[M] FC Transport adapter module
and save the config 
4.make -j $(nproc)
5.make -j $(nproc) modules
6.make -j $(nproc) modules_install
7.make -j $(nproc) install

Chnage the  grub configuration accordingly 
8.Reboot the sytem

Once the system is up we will see a device fctxpd under /dev direcotry
