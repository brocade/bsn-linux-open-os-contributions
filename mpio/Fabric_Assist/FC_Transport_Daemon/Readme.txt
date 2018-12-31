Steps to compile:

Before we start downloading we need to install udev,pthread ,devmapper libraries
Once the above libraries are downloaded please use the below steps for further

1. Download the Git code from https://github.com/brocade/bsn-linux-open-os-contributions.git
2. From the clone directory goto fc transport daemon path
	cd  mpio/Fabric_Assist/FC_Transport_Daemon
3.make clean;
4.make
5.copy the fctxptd.service to /usr/lib/systemd/system
5.run systemctl start fxtxptd.service to start the fc-transport daemon to start the service
6.Fctxptd daemon will start on every reboot
