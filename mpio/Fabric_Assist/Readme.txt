Added FC_Transport_Daemon,  Linux directories under Fabric Assist.
Inorder to run this feature we need to compile and install both the linux
and FC_Transport_Daemon in this particular directory.
And the steps to compile and install the same are available in the coressponding
Readme.txt files 
Fibre Channel Transport Services on Linux
###############################################
	The purpose of this daemon is to add FC network intelligence in host and
host intelligence in FC network. This daemon would interoperate with
Brocade FC fabric in order to improve the response time of the MPIO failovers.
In future, it can also collect the congestion related details and perform
workload analysis, and provide QOS at application level by interoperating with
application performance profiling software.
 
Prerequisites
	For this daemon to be fully functional, following devices are required.
	1. A Linux host with FC HBA attached.
	2. A Brocade FC network switch which sends the FPIN-LI ELS frame.
	3. The FC HBA driver which passes through the FPIN-LI ELS sent from switch
		to host.
Note:This daemon cannot be launched without HBA driver support.

Usage:
	Currently, the daemon suports only the marginal path failover. The FPIN-LI
ELS structure is yet to be finalized and hence a temporary beacon ELS frame is
used as a POC to test the feature.

Steps performed during daemon execution:
1.	The FC networking switch sends an FPIN-LI ELS frame,
	to the HBA port. This frame currently contains the port ID of the HBA port.

2.	The HBA driver passes the ELS frame up to the FCTXPTD daemon.

3.	The daemon, on receiveing the ELS frame, extracts the HBA port ID in the
	frame and translates it to Port WWN. This is done by parsing the 
	/sys/class/fc_host/host* directory, where one directory is created per HBA
	port to the host. This host directory contains the Port ID, Port WWN,
	symbolic link to target LUNs etc.

4.	Port WWNs of the targets to be failed is populated in a list. These WWNs are
	sent by FC networking switch through FPIN-LI ELS frame.

5.	Once the WWNs are populated above, the daemon populates all the target IDs
	which are visible from the HBA port on which the ELS frame was received.
	They will be in the form of targetx:x:x.

6.	With the list of targets populated, the daemon parses through the 
	/sys/class/fc_transport directory, where all the targets which are visible
	through HBA ports are stored. Only the targets in the list populated in
	point 4, is parsed to get the sd* and dm-* information. A list of sd* which
	are to be failed is populated here. 

FC Transport Adpater:
##################################################################################
FC Transport Adpater is the module which interacts with both daemon and underlying HBA
devices for receiving FPIN-LI ELS frames from underlying HBA device and sending the
notifcations to the daemon regarding the same.
All the hba modules needs to register with this FC Transport Adapter module for sending
the ELS notifications to the daemon residing in user space .


FC HBA changes:
#######################################################################################
Added FPIN ELS support and changes to register with the FC Transport Adpater for sending
ELS notifications .

