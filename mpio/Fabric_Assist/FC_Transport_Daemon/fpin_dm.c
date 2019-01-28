/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * Functions to find DM to LUN mapping.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Authors:
 *      Ganesh Murugesan <ganesh.murugesan@broadcom.com>
 *      Ganesh Pai <ganesh.pai@broadcom.com>
 */

#include "fpin.h"
extern struct fpin_global global;
/*
 * Function:
 * 	fpin_insert_dm(struct list_head *dm_list_head, char *dm_name,
 * 			char *dm_node, char *uid_name)
 *
 * Inputs:
 * 	1. Pointer to the Linked list head containing dm name and node.
 * 	2. The dm name which id of form mpath* (mpatha/mpathb etc)
 * 	3. The DM node name in /dev
 *
 * Description:
 * 	This function inserts the dm name (which is in form of sd*) and
 * 	device Mapper Name (dm-*) into the Linked list which is later used to
 * 	fail the path using multipath daemon.
 */
int
fpin_insert_dm(struct list_head *dm_list_head, char *dm_name,
				char *dm_node, char *uid_name) {
	struct dm_dev_list *new_node = NULL;
	char *uid_ptr = NULL;

	/* Create a node */
	new_node = (struct dm_dev_list*) malloc(sizeof(struct dm_dev_list));
	if (new_node != NULL) {
		memset(new_node->dm_dev_node, '\0', sizeof(new_node->dm_dev_node));
		memset(new_node->dm_name, '\0', sizeof(new_node->dm_name));
		memset(new_node->dm_uuid, '\0', sizeof(new_node->dm_uuid));

		/* Set values in new node */
		strncpy(new_node->dm_name, dm_name, sizeof(new_node->dm_name));
		strncpy(new_node->dm_dev_node, dm_node, sizeof(new_node->dm_dev_node));
		uid_ptr = (strchr(uid_name, '-') + 1);
		if (uid_ptr != NULL) {
			strncpy(new_node->dm_uuid, uid_ptr,
					 sizeof(new_node->dm_uuid));
		}

		FPIN_ILOG("Inserted %s : %s : %s into dm list\n",
			new_node->dm_name, new_node->dm_dev_node, new_node->dm_uuid);
		list_add_tail(&(new_node->dm_list_head), dm_list_head);
	} else {
		FPIN_ELOG("Failed to add %s : %s, OOM\n",
				new_node->dm_name, new_node->dm_dev_node);
		return -ENOMEM;
	}

	return (0);
}

/*
 * Function:
 * 	fpin_insert_sd(struct list_head *list_head, char *dev_name,
 * 					char *sd_node, char *serial_id)
 *
 * Inputs:
 * 	1. Pointer to the Linked list head containing dev name and node.
 * 	2. The dev name which is of form sd* (mpatha/mpathb etc)
 * 	3. The DM node name which is of the form MAJOR:MINOR
 *
 * Description:
 * 	This function inserts the dev name (which is in form of sd*) and
 * 	serial ID into the Linked list which is later used to
 * 	fail the path using multipath daemon.
 */
int
fpin_insert_sd(struct list_head *impacted_dev_list_head, char *dev_name,
				char *sd_node, char *serial_id) {
	struct impacted_dev_list *new_node = NULL;

	/* Create a node */
	new_node = (struct impacted_dev_list*)
				malloc(sizeof(struct impacted_dev_list));
	if (new_node != NULL) {
		memset(new_node->dev_node, '\0', sizeof(new_node->dev_node));
		memset(new_node->dev_name, '\0', sizeof(new_node->dev_name));
		memset(new_node->dev_serial_id, '\0',
					sizeof(new_node->dev_serial_id));

		/* Set values in new node */
		strncpy(new_node->dev_name, dev_name, sizeof(new_node->dev_name));
		strncpy(new_node->dev_node, sd_node, sizeof(new_node->dev_node));
		strncpy(new_node->dev_serial_id, serial_id,
				sizeof(new_node->dev_serial_id));
		FPIN_ILOG("Inserted %s : %s : %s into sd list\n",
			new_node->dev_name, new_node->dev_node, new_node->dev_serial_id);
		list_add_tail(&(new_node->dev_list_head), impacted_dev_list_head);
	} else {
		FPIN_ELOG("Failed to add %s : %s, OOM\n",
				new_node->dev_name, new_node->dev_node);
		return -ENOMEM;
	}

	return (0);
}

/*
 * Function:
 *	fpin_dm_insert_target
 *
 * Inputs:
 * 	1. Pointer to the Linked list which contains list of targets impacted.
 * 	2. The target id to be inserted to the above list.
 *
 * Description:
 * 	This function inserts the target name to a list of impacted targets.
 */
int
fpin_dm_insert_target(struct list_head *tgt_list_head, char *target) {
	struct target_list *new_node = NULL;

	/* Create a node */
	new_node = (struct target_list*) malloc(sizeof(struct target_list));
	if (new_node != NULL) {
		memset(new_node->target, '\0', sizeof(new_node->target));
		/* Set values in new node */
		strncpy(new_node->target, target, sizeof(new_node->target));
		FPIN_ILOG("Inserted %s into target list\n", new_node->target);
		list_add_tail(&(new_node->target_head), tgt_list_head);
	} else {
		FPIN_CLOG("Failed to insert target %s\n", target);
		return -ENOMEM;
	}

	return (0);
}

void
fpin_display_dm_list(struct list_head *list_head) {
	struct dm_dev_list *temp = NULL;
	if (list_empty(list_head)) {
		FPIN_DLOG("DM list is empty, not failing any sd\n");
	} else {
		list_for_each_entry(temp, list_head, dm_list_head) {
			FPIN_DLOG("Contains: dm_name: %s, dm_node: %s\n",
				temp->dm_name, temp->dm_dev_node);
		}
	}
}

void
fpin_display_impacted_dev_list(struct list_head *list_head) {
	struct impacted_dev_list *temp = NULL;
	if (list_empty(list_head)) {
		FPIN_DLOG("DM list is empty, not failing any sd\n");
	} else {
		list_for_each_entry(temp, list_head, dev_list_head) {
			FPIN_DLOG("Contains: dev_name: %s, dev_node: %s\n",
				temp->dev_name, temp->dev_node);
		}
	}
}

/*
 * Function:
 * 	fpin_fetch_dm_for_sd
 *
 * Inputs:
 * 	1. List of all multipath DMs in the host.
 * 	2. The serial ID of the device to be mapped with UUID of DM.
 * 	3. Pointer to the memory where the impacted DM name (mpath*) will be stored.
 *
 * Returns:
 * 	1: If the corresponding holder of device is found., -1 Otherwise.
 *
 * Description:
 * 	This function gets the DM name in (mpath*) format, for the device whose
 * serial ID is passed. The DM name is stored in impacted_dm parameter, which
 * will be passed to dm_get_status of device mapper to get the number of active
 * paths for the DM.
 */
int
fpin_fetch_dm_for_sd(struct list_head *dm_head,
				char *dev_serial_id, char *impacted_dm) {
	struct dm_dev_list *dm = NULL;
	int found = -1;

	if (list_empty(dm_head)) {
		FPIN_ELOG("DM list is empty, returning -1\n");
		return (found);
	} else {
		list_for_each_entry(dm, dm_head, dm_list_head) {
			if (strncmp(dev_serial_id, dm->dm_uuid, UUID_LEN) == 0) {
				strncpy(impacted_dm, dm->dm_dev_node, DEV_NAME_LEN);
				FPIN_DLOG("Found impacted dm %s\n", impacted_dm);
				found  = 1;
				break;
			}
		}
	}

	return (found);
}

/*
 * Function:
 * 	fpin_dm_get_active_path_count
 *
 * Inputs:
 * 	dm_status: The status string retrieved from device mapper for a particular
 * 		dm (mapth*).
 *
 * Returns:
 * 	No. of active paths in the DM.
 *
 * Description:
 * 	This function counts and returns the number of active paths in a particular
 * 	DM by parsing the status string recieved from DM. The string is of the below
 * 	format:
 * 		2 0 1 0 1 1 A 0 2 2 8:32 A 0 0 1 8:48 A 0 0 1
 */
int fpin_dm_get_active_path_count(char *dm_status) {
	char *ptr = NULL;
	int count = 0;

	while ((ptr = strchr(dm_status, ':')) != NULL) {
		while((*ptr != '\0') && (!isspace(*ptr))) {
			ptr++;
		}

		/* Skip white space */
		ptr++;
		FPIN_DLOG("Got status as %c\n", *ptr);
		if (*ptr != 'F') {
			count++;
		}

		dm_status = ptr;
	}

	return(count);
}

/*
 * Function:
 * 	fpin_dm_fail_path
 *
 * Inputs:
 * 	dm_list_head:			List of all DMs in the host.
 * 	impacted_dev_list_head: List of all impacted devices, whose WWN was sent
 * 							as part of FPIN ELS frame.
 *
 * Description:
 * 	Uses Multipath daemon help to fail a path permanently unless manually
 * 	reinstated. Maps the impacted Devices to their corresponding holders/dms',
 * 	and fails the path only if there is at least one other active path present.
 */
void
fpin_dm_fail_path(struct list_head *dm_list_head,
			struct list_head *impacted_dev_list_head,
			struct hba_port_wwn_info *port_info) {
	struct impacted_dev_list *temp = NULL;
	char impacted_dm[DEV_NAME_LEN];
	char cmd[100], dm_status[DM_PARAMS_SIZE];
	int ret = -1;

	if (list_empty(dm_list_head)) {
		FPIN_ELOG("DM list is empty, not failing any sd\n");
	} else if (list_empty(impacted_dev_list_head)) {
		FPIN_ELOG("SD List is empty, not failing any sd\n");
	} else {
		list_for_each_entry(temp, impacted_dev_list_head, dev_list_head) {
			memset(impacted_dm, '\0', DEV_NAME_LEN);
			ret = fpin_fetch_dm_for_sd(dm_list_head, temp->dev_serial_id, impacted_dm);
			if (ret <= 0) {
				FPIN_ELOG("Failed to fetch DM for sd %s\n", temp->dev_name);
				continue;
			} else {
				memset(dm_status, '\0', DM_PARAMS_SIZE);
				ret = dm_get_status(impacted_dm, dm_status);
				if (!ret) {
					ret = fpin_dm_get_active_path_count(dm_status);
					if (ret > 1) {
						FPIN_ILOG("Failing %s:%s\n", temp->dev_node,
												temp->dev_name);
						snprintf(cmd, sizeof(cmd), "multipathd fail path %s",
									temp->dev_name);
						system(cmd);
						FPIN_ILOG("Sending Initiator PWWN as 0x%llx\n", port_info->initiator_hba_wwn);
						ret = ioctl(global.fctxp_device_rfd,
							FCTXPD_FAILBACK_IO, port_info);
						FPIN_ELOG("Got Ret as %d from IOCTL\n", ret);
					} else {
						FPIN_ELOG("Not Failing %s, not enough Active paths\n",
							temp->dev_name);
						continue;
					}
				}
			}
		}
	}
}

void
fpin_dm_display_target(struct list_head *tgt_head) {
	struct target_list *temp = NULL;

	if (list_empty(tgt_head)) {
		FPIN_DLOG("Target List is empty\n");
	} else {
		list_for_each_entry(temp, tgt_head, target_head)
			FPIN_DLOG("Target is %s\n", temp->target);
	}
}

int
fpin_dm_find_target(struct list_head *tgt_head, char *target) {
	struct target_list *temp = NULL;
	int found  = 0;

	if (list_empty(tgt_head)) {
		FPIN_DLOG("Target List is empty, %s not found\n", target);
	} else {
		list_for_each_entry(temp, tgt_head, target_head) {
			if (strncmp(temp->target, target, strlen(temp->target)) == 0) {
				FPIN_DLOG("Found Target %s\n", target);
				found = 1;
				return (found);
			}
		}
	}

	return (found);
}

void
fpin_free_dm(struct list_head *dm_head) {
	struct list_head *current_node = NULL;
	struct list_head *temp = NULL;
	struct dm_dev_list *tmp_dm = NULL;
	if (list_empty(dm_head)) {
		FPIN_DLOG("List is empty, nothing to delete..\n");
		return;
	} else {
		list_for_each_safe(current_node, temp, dm_head) {
			tmp_dm = list_entry(current_node, struct dm_dev_list, dm_list_head);
			FPIN_DLOG("Free dm %s\n", tmp_dm->dm_dev_node);
			list_del(current_node);
			free(tmp_dm);
		}
	}
}

void
fpin_dm_free_target(struct list_head *tgt_head) {
	struct list_head *current_node = NULL;
	struct list_head *temp = NULL;
	struct target_list *tmp_tgt = NULL;
	if (list_empty(tgt_head)) {
		FPIN_DLOG("List is empty, nothing to delete..\n");
		return;
	} else {
		list_for_each_safe(current_node, temp, tgt_head) {
			tmp_tgt = list_entry(current_node, struct target_list, target_head);
			FPIN_DLOG("Free target %s\n", tmp_tgt->target);
			list_del(current_node);
			free(tmp_tgt);
		}
	}
}

void
fpin_dm_free_dev(struct  list_head *sd_head) {
	struct list_head *current_node = NULL;
	struct list_head *temp = NULL;
	struct impacted_dev_list *tmp_sd = NULL;
	if (list_empty(sd_head)) {
		FPIN_DLOG("List is empty, nothing to delete..\n");
		return;
	} else {
		list_for_each_safe(current_node, temp, sd_head) {
			tmp_sd = list_entry(current_node, struct impacted_dev_list, dev_list_head);
			FPIN_DLOG("Free sd %s\n", tmp_sd->dev_name);
			list_del(current_node);
			free(tmp_sd);
		}
	}
}

int dm_get_status(const char *name, char *outstatus)
{
        int r = 1;
        struct dm_task *dmt;
        uint64_t start, length;
        char *target_type = NULL;
        char *status = NULL;

        if (!(dmt = dm_task_create(DM_DEVICE_STATUS)))
                return 1;

        if (!dm_task_set_name(dmt, name))
                goto out;

        dm_task_no_open_count(dmt);

        if (!dm_task_run(dmt))
                goto out;

        /* Fetch 1st target */
        dm_get_next_target(dmt, NULL, &start, &length,
                           &target_type, &status);
        if (!status) {
                FPIN_ELOG("get null status.\n");
                goto out;
        }

        if (snprintf(outstatus, DM_PARAMS_SIZE, "%s", status) <= DM_PARAMS_SIZE)
                r = 0;
out:
        if (r)
                FPIN_ELOG("%s: error getting map status string", name);

        dm_task_destroy(dmt);
        return r;
}
/*
 * Function:
 *	fpin_fetch_dm_lun_data
 *
 * Inputs:
 * 	1. Pointer to the Linked list which contains list of port WWNs impacted.
 * 	2. Pointer to the Linked list which will be populated with impacted sd
 * 	   and dms. These sd* will be failed using multipathd.
 *
 * Description:
 * 	This function takes in the list of impacted WWNs as input and translates
 * 	them into sd* and dm-* which will be failed by multipathd.
 */
int
fpin_fetch_dm_lun_data(struct wwn_list *list, struct list_head *dm_list_head,
					struct list_head *impacted_dev_list_head) {
	struct list_head impacted_tgt_list_head;
	int ret = -1;

	INIT_LIST_HEAD(&impacted_tgt_list_head);
	/* Get Targets linked to the port on whichthe ELS frame was recieved */
	ret = fpin_dm_populate_target(list, &impacted_tgt_list_head);
	if (ret <= 0) {
		FPIN_ELOG("No targets found, returning ret %d\n", ret);
		return (ret);
	}

	fpin_dm_display_target(&impacted_tgt_list_head);

	/* Get sd to dm mapping for populated targets */
	ret = fpin_populate_dm_lun(dm_list_head,
			impacted_dev_list_head, &impacted_tgt_list_head);
	if (ret <= 0) {
		FPIN_ELOG("No sd found to fail, returning ret %d\n", ret);
		fpin_dm_free_target(&impacted_tgt_list_head);
		return (ret);
	}

	fpin_display_dm_list(dm_list_head);
	fpin_display_impacted_dev_list(impacted_dev_list_head);

	fpin_dm_free_target(&impacted_tgt_list_head);
	return (ret);
}

/*
 * Function:
 *	fpin_dm_populate_target
 *
 * Inputs:
 * 	1. Pointer to the Linked list which contains list of port WWNs impacted.
 * 	2. Pointer to the Linked list which will be populated with impacted targets.
 *
 * Description:
 * 	This function takes in the list of impacted WWNs as input and translates
 * 	them into target IDs. These target IDs are used to fetch sd* and dm-*
 * 	details, which will be failed by multipath daemon.
 */
int
fpin_dm_populate_target(struct wwn_list *list,
					struct list_head *tgt_list) {

	char dir_path_buf[SYS_PATH_LEN], target_buf[DEV_NODE_LEN];
	char port_wwn_buf[WWN_LEN], host_buf[DEV_NODE_LEN];
	int target_count = 0, host_found = 0, wwn_exists = 0;
	struct udev *udev = NULL;
	struct udev_enumerate *enumerate = NULL;
	struct udev_list_entry *devices = NULL, *dev_list_entry = NULL;
	struct udev_device *dev = NULL, *parent_dev = NULL;
	int i = 0, ret = 0;
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return(-1);
	}

	/* Create a list of the devices in the 'fc_host' subsystem. */
	enumerate = udev_enumerate_new(udev);
	if (enumerate == NULL) {
		FPIN_ELOG("Could not enumerate udev\n");
		return (-EBADF);
	}

	ret = udev_enumerate_add_match_subsystem(enumerate, "fc_host");
	if (ret < 0) {
		FPIN_ELOG("Could not enumerate fc_host subsystem with ret %d\n", ret);
		return (ret);
	}
	ret = udev_enumerate_scan_devices(enumerate);
	if (ret < 0) {
		FPIN_ELOG("Could not scan fc_host subsystem with ret %d\n", ret);
		return (ret);
	}
	devices = udev_enumerate_get_list_entry(enumerate);
	if (devices == NULL) {
		FPIN_ELOG("NO devices found under fc_host subsystem\n");
		return(-ENODEV);
	}
	/* For each item enumerated, find if the target matches.
	 * fetch port_name and compare it with the HBA PWWN in ELS.
	 * Whichever matches, store the sys_name
	 */
	FPIN_DLOG("HBA PWWN = %s\n", list->hba_wwn);
	udev_list_entry_foreach(dev_list_entry, devices) {
		snprintf(dir_path_buf, sizeof(dir_path_buf), "%s",
			udev_list_entry_get_name(dev_list_entry));
		dev = udev_device_new_from_syspath(udev, dir_path_buf);
		if (dev == NULL) {
			FPIN_ELOG("Failed to get host device struct from path %s, err %d\n",
						dir_path_buf, errno);
			continue;
		}

		FPIN_DLOG("Got host as %s\n", udev_device_get_sysname(dev));
		snprintf(port_wwn_buf, sizeof(port_wwn_buf), "%s", 
			udev_device_get_sysattr_value(dev, "port_name"));
		FPIN_DLOG("Got Port WWN as %s\n", port_wwn_buf);
		if (strncmp(port_wwn_buf, list->hba_wwn, strlen(port_wwn_buf)) == 0) {
			snprintf(host_buf, sizeof(host_buf), "%s",
				udev_device_get_sysname(dev));
			host_found = 1;
			udev_device_unref(dev);
			break;
		}

		udev_device_unref(dev);
	}

	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);

	udev_unref(udev);

	if (host_found == 1) {
		FPIN_DLOG("Find targets visible to host %s\n", host_buf);
		/* Create the udev object */
		udev = udev_new();
		if (!udev) {
				printf("Can't create udev\n");
				return(-1);
		}

		/* Create a list of the devices in the 'fc_transport' subsystem. */
		enumerate = udev_enumerate_new(udev);
		if (enumerate == NULL) {
			FPIN_ELOG("Could not enumerate udev for fc_trans\n");
			return (-EBADF);
		}
		ret = udev_enumerate_add_match_subsystem(enumerate, "fc_transport");
		if (ret < 0) {
			FPIN_ELOG("Could not enumerate fc_tx sub with ret %d\n", ret);
			return (ret);
		}

		ret = udev_enumerate_scan_devices(enumerate);
		if (ret < 0) {
			FPIN_ELOG("Could not scan block subsystem with ret %d\n", ret);
			return (ret);
		}

		devices = udev_enumerate_get_list_entry(enumerate);
		if (devices == NULL) {
			FPIN_ELOG("NO devices found under block subsystem\n");
			return(-ENODEV);
		}

		/* For each item enumerated, find if the target matches.
		 * fetch port_name and compare it with the HBA PWWN in ELS.
		 * Whichever matches, store the sys_name
		 */
		udev_list_entry_foreach(dev_list_entry, devices) {
				snprintf(dir_path_buf, sizeof(dir_path_buf), "%s",
								udev_list_entry_get_name(dev_list_entry));
				dev = udev_device_new_from_syspath(udev, dir_path_buf);
				if (dev == NULL) {
					FPIN_ELOG("Failed to get device struct from path %s, err %d\n",
								dir_path_buf, errno);
					continue;
				}

				parent_dev = udev_device_get_parent_with_subsystem_devtype(
								dev, "scsi", "scsi_host");
				if (!parent_dev) {
					FPIN_ELOG("Unable to find parent usb device for %s\n",
									udev_device_get_sysname(dev));
					continue;
				}

				snprintf(target_buf, sizeof(target_buf), "%s",
								udev_device_get_sysname(dev));
				FPIN_DLOG("Got Port Target_buf as %s\n", target_buf);
				if (strncmp(host_buf, udev_device_get_sysname(parent_dev),
										strlen(host_buf)) == 0) {
					snprintf(port_wwn_buf, sizeof(port_wwn_buf), "%s", 
								udev_device_get_sysattr_value(dev, "port_name"));
					FPIN_DLOG("Got tgt WWN as %s\n", port_wwn_buf);
					wwn_exists = fpin_els_wwn_exists(list, port_wwn_buf);
					if (wwn_exists) {
						FPIN_DLOG("Found a target %s\n", target_buf);
						if ((fpin_dm_insert_target(tgt_list, target_buf)) == 0) {
							target_count ++;
						}
					}
				}

				//udev_device_unref(dev);
				udev_device_unref(parent_dev);
			}
			/* Free the enumerator object */
			udev_enumerate_unref(enumerate);

			udev_unref(udev);
	}

	return (target_count);
}

/*
 * Function:
 *	fpin_populate_dm_lun
 *
 * Inputs:
 * 	1. Pointer to the Linked list which contains list of port WWNs impacted.
 * 	2. Pointer to the Linked list which will be populated with impacted sd
 * 	   and dms. These sd* will be failed using multipathd.
 *	3. Pointer to the list of impacted target IDs.
 *
 * Description:
 * 	This function takes in the list of impacted WWNs as input and translates
 * 	them into sd* and dm-* which will be failed by multipathd.
 */
int
fpin_populate_dm_lun(struct list_head *dm_list_head,
			struct list_head *impacted_dev_list_head,
			struct list_head *target_head) {
	char dir_path_buf[SYS_PATH_LEN], lun_buf[DEV_NODE_LEN];
	char dm_buf[DEV_NODE_LEN], target_buf[DEV_NODE_LEN];
	char dev_buf[DEV_NODE_LEN] , uid_buf[UUID_LEN];
	int wwn_exists = 0, dm_count = 0;
	int sd_count = 0, ret = 0;

	struct udev *udev = NULL;
	struct udev_enumerate *enumerate = NULL;
	struct udev_list_entry *devices = NULL, *dev_list_entry = NULL;
	struct udev_device *dev = NULL, *parent_dev = NULL;
	int i = 0;
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		FPIN_ELOG("Can't create udev\n");
		return(-ENOMEM);
	}

	/* Create a list of the devices in the 'block' subsystem. */
	enumerate = udev_enumerate_new(udev);
	if (enumerate == NULL) {
		FPIN_ELOG("Could not enumerate udev\n");
		return (-EBADF);
	}

	ret = udev_enumerate_add_match_subsystem(enumerate, "block");
	if (ret < 0) {
		FPIN_ELOG("Could not enumerate block subsystem with ret %d\n", ret);
		return (ret);
	}

	ret = udev_enumerate_scan_devices(enumerate);
	if (ret < 0) {
		FPIN_ELOG("Could not scan block subsystem with ret %d\n", ret);
		return (ret);
	}

	devices = udev_enumerate_get_list_entry(enumerate);
	if (devices == NULL) {
		FPIN_ELOG("NO devices found under block subsystem\n");
		return(-ENODEV);
	}

	/* For each item enumerated, find if the target matches.
	 * fetch port_name and compare it with the HBA PWWN in ELS.
	 * Whichever matches, store the sys_name
	 */
	FPIN_DLOG("Looping over Block...\n");
	udev_list_entry_foreach(dev_list_entry, devices) { //MMK free udev mem
		snprintf(dir_path_buf, sizeof(dir_path_buf), "%s",
			udev_list_entry_get_name(dev_list_entry));
		dev = udev_device_new_from_syspath(udev, dir_path_buf);
		if (dev == NULL) {
			FPIN_ELOG("Failed to get device struct from path %s, err %d\n",
				dir_path_buf, errno);
			continue;
		}
		/* Handle DMs */
		snprintf(dev_buf, sizeof(dev_buf), "%s",
			udev_device_get_sysname(dev));
		FPIN_DLOG("Got dev_name as %s\n", dev_buf);
		if (strncmp("dm-", dev_buf, 3) == 0) {
			snprintf(dm_buf, sizeof(dm_buf), "%s",
				udev_device_get_property_value(dev, "DM_NAME"));
			if (strncmp("mpath", dm_buf, 5) == 0) {
				snprintf(uid_buf, sizeof(uid_buf), "%s",
					udev_device_get_property_value(dev, "DM_UUID"));
				ret = fpin_insert_dm(dm_list_head, dev_buf, dm_buf, uid_buf);
				if (ret < 0) {
					FPIN_ELOG("Failed to Insert %s : %s\n", dev_buf, dm_buf);
				} else {
					dm_count++;
				}
			}

			continue;
		} else if (strncmp("sd", dev_buf, 2) == 0) {
			/* Handle SDs */
			parent_dev = udev_device_get_parent_with_subsystem_devtype(
				dev, "scsi", "scsi_target");
			if (!parent_dev) {
				FPIN_ELOG("Unable to find parent usb device for %s\n",
					udev_device_get_sysname(dev));
				continue;
			}

			snprintf(target_buf, sizeof(target_buf), "%s",
				udev_device_get_sysname(parent_dev));

			if (fpin_dm_find_target(target_head, target_buf) != 0) {
				snprintf(lun_buf, sizeof(lun_buf), "%s:%s",
					udev_device_get_property_value(dev, "MAJOR"),
					udev_device_get_property_value(dev, "MINOR"));
				snprintf(uid_buf, sizeof(uid_buf), "%s",
					udev_device_get_property_value(dev, "ID_SERIAL"));
				ret = fpin_insert_sd(impacted_dev_list_head, dev_buf,
						lun_buf, uid_buf);
				if (ret < 0) {
					FPIN_ELOG("Failed to insert %s %s to sd list\n",
							dev_buf, lun_buf);
				} else {
					sd_count++;
				}
			}
		}

		udev_device_unref(dev);
	}

	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	if (dm_count <= 0) 
		return(dm_count);

	return (sd_count);
}
