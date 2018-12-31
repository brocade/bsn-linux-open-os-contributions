/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * FPIN ELS frame processing functions.
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
 *      Ganesh Pai <ganesh.pai@broadcom.com>
 */

#include "fpin.h"


pthread_cond_t fpin_li_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t fpin_li_mutex = PTHREAD_MUTEX_INITIALIZER;
extern struct list_head    els_marginal_list_head;

/*
 * Function:
 * 	fpin_els_add_li_frame
 *
 * Input:
 * 	The Link Integrity Frame payload received from HBA driver.
 *
 * Description:
 * 	On Receiving the frame from HBA driver, insert the frame into link
 * 	integrity frame list which will be picked up later by consumer thread for
 * 	processing.
 */
int
fpin_els_add_li_frame(fpin_payload_t *fpin_payload) {
	struct els_marginal_list *els_mrg = NULL;
	els_mrg = malloc(sizeof(struct els_marginal_list));
	if (els_mrg != NULL) {
		els_mrg->hba_pwwn = fpin_payload->hba_wwn;
		memcpy(els_mrg->payload, fpin_payload->payload, FC_PAYLOAD_MAXLEN);
		pthread_mutex_lock(&fpin_li_mutex);
		list_add_tail(&els_mrg->els_frame, &els_marginal_list_head);
		pthread_mutex_unlock(&fpin_li_mutex);
		pthread_cond_signal(&fpin_li_cond);
	} else {
		FPIN_CLOG("NO Memory to add frame payload\n");
		return (-ENOMEM);
	}

	return (0);
}

/*
 * Function:
 * 	fpin_els_insert_port_wwn
 *
 * Input:
 * 	struct wwn_list *list	: List containing impacted WWNs.
 * 	port_wwn_buf			: The WWN to be inserted into above list.
 *
 * Description:
 * 	This function inserts the Port WWN retrieved from FPIN ELS frame, recieved
 * 	from HBA driver. These WWNs are later used to find sd* and dm-* details.
 */

int
fpin_els_insert_port_wwn(struct wwn_list *list, char *port_wwn_buf)
{
	struct impacted_port_wwn_list *new_wwn = NULL;
	FPIN_ILOG("Inserting %s...\n", port_wwn_buf);

	 /* Create a node */
	new_wwn = (struct impacted_port_wwn_list*)malloc(sizeof(struct impacted_port_wwn_list));
	if (new_wwn == NULL) {
		FPIN_CLOG("No memory to assign pwwn %s\n", port_wwn_buf);
		return -ENOMEM;
	}

	memset(new_wwn->impacted_port_wwn, '\0', sizeof(new_wwn->impacted_port_wwn));
	snprintf(new_wwn->impacted_port_wwn, sizeof(new_wwn->impacted_port_wwn),
				"%s", port_wwn_buf);
	FPIN_ILOG(" Assigned  %s to new node\n", new_wwn->impacted_port_wwn);
	list_add_tail(&(new_wwn->impacted_port_wwn_head),
		&(list->impacted_ports.impacted_port_wwn_head));

	return (0);
}

/*
 * Function:
 *	fpin_els_wwn_exists 	
 *
 * Input:
 * 	struct wwn_list *list	: List containing impacted WWNs.
 * 	port_wwn_buf			: The WWN to be searched in the above list.
 *
 * Description:
 * 	This function searches the impacted WWN list for the WWN passed in
 * 	second argument. Returns 1 if found, 0 otherwise.
 */

int
fpin_els_wwn_exists(struct wwn_list *list, char *port_wwn_buf) {
	int found = 0;
	struct impacted_port_wwn_list *temp = NULL;

	if (list_empty(&(list->impacted_ports.impacted_port_wwn_head))) {
        	FPIN_ELOG("List is empty, %s not found\n", port_wwn_buf);
	} else {
		list_for_each_entry(temp, &(list->impacted_ports.impacted_port_wwn_head),
								impacted_port_wwn_head) {
			FPIN_DLOG("Checking for %s and %s\n", temp->impacted_port_wwn,
						port_wwn_buf);
			if (strncmp(temp->impacted_port_wwn, port_wwn_buf,
						strlen(port_wwn_buf)) == 0) {
				FPIN_ILOG("breaking for %s %s\n", temp->impacted_port_wwn,
						port_wwn_buf);
				found = 1;
				return (found);
			}
		}
	}

	return (found);
}

void
fpin_els_display_wwn(struct wwn_list *list) {
	struct impacted_port_wwn_list *temp = NULL;

	if (list_empty(&(list->impacted_ports.impacted_port_wwn_head))) {
		FPIN_ELOG("WWN List is empty\n");
	} else {
		list_for_each_entry(temp, &(list->impacted_ports.impacted_port_wwn_head),
				impacted_port_wwn_head)
			FPIN_ILOG("WWN Imapcted is %s\n", temp->impacted_port_wwn);
	}

	FPIN_ILOG("HBA WWN is %s\n", list->hba_wwn);
}

void
fpin_els_free_wwn_list(struct wwn_list *list) {
	struct list_head *current_node = NULL;
	struct list_head *temp = NULL;
	struct impacted_port_wwn_list *temp_node = NULL;

	if (list_empty(&(list->impacted_ports.impacted_port_wwn_head))) {
		FPIN_ELOG("WWN List is empty\n");
	} else {
		list_for_each_safe(current_node, temp,
			&(list->impacted_ports.impacted_port_wwn_head)) {
			temp_node = list_entry(current_node, struct impacted_port_wwn_list,
							impacted_port_wwn_head);
			FPIN_DLOG("Free WWN %s\n", temp_node->impacted_port_wwn);
			list_del(current_node);
			free(temp_node);
		}
	}
}


/*
 * Function:
 *	fpin_els_extract_wwn
 *
 * Input:
 *	hba_pwwn				: The Port WWN of HBA on which the ELS was received.
 *	L.I Notification Struct	: The Link Integrity struct with impacted WWN list.
 * 	struct wwn_list *list	: The list to be populated with impacted WWN.
 *
 * Description:
 * 	This function reads though the FPIN ELS recieved from HBA driver, to get and
 * 	populate the impacted WWN list. This list is used to find and fail the
 * 	impacted paths if an alternate path for the same device exists.
 */

int
fpin_els_extract_wwn(wwn_t *hba_pwwn, fpin_link_integrity_notification_t *li,
						struct wwn_list *list) {
	char  port_wwn_buf[WWN_LEN];
	wwn_t *currentPortListOffset_p = NULL;
	uint32_t wwn_count = 0;
	int iter = 0, count = 0;

	/* Update the wwn to list */
	wwn_count = htonl(li->port_list.count);
	snprintf(list->hba_wwn, sizeof(list->hba_wwn), "0x%x%x",
			htonl(hba_pwwn->words[0]), htonl(hba_pwwn->words[1]));

	currentPortListOffset_p = (wwn_t *)&(li->port_list.port_name_list);
	for (iter = 0; iter < wwn_count; iter++) {
		memset(port_wwn_buf, '\0', WWN_LEN);
		snprintf(port_wwn_buf, WWN_LEN, "0x%08x%08x", 
			htonl(currentPortListOffset_p->words[0]),
			htonl(currentPortListOffset_p->words[1]));
		fpin_els_insert_port_wwn(list, port_wwn_buf);
		currentPortListOffset_p++;
		count++;
	}

	fpin_els_display_wwn(list);
	return (count);
}

/*
 * Function:
 *	fpin_process_els_frame
 *
 * Inputs:
 * 	1. The WWN of the HBA on which the ELS frame was received.
 *	2. The ELS frame to be processed. Could be FPIN frame or any other ELS frame
 *	in the future.
 *
 * Description:
 *	This function process the ELS frame recieved from HBA driver,
 *	and fails the impacted paths if an alternate path exists. This function
 *	does the following:
 *		1. Extarct the impacted device WWNs from the FPIN ELS frame.
 *		2. Get the target IDs of the devices from the WWNs extracted.
 *		3. Translate the target IDs into corresponding sd* and dm-*.
 *		4. Fail the sd* using multipath daemon, provided alternate paths exist.
 *		5. Free the resources allocated.
 */
int
fpin_process_els_frame(wwn_t *hba_pwwn, char *fc_payload) {
	struct list_head dm_list_head, impacted_dev_list_head;
	fpin_link_integrity_request_els_t *fpin_req = NULL;
	fpin_link_integrity_notification_t *li = NULL;
	uint32_t els_cmd = 0;
	struct wwn_list list_of_wwn;
	int count = -1;

	els_cmd = *(uint32_t *)fc_payload;
	FPIN_ILOG("Got CMD while processing as 0x%x\n", els_cmd);
	switch(els_cmd) {
		case ELS_CMD_FPIN:
			fpin_req = (fpin_link_integrity_request_els_t *)fc_payload;
			INIT_LIST_HEAD(&list_of_wwn.impacted_ports.impacted_port_wwn_head);

			/* Get the WWNs recieved from HBA firmware through ELS frame */
			count = fpin_els_extract_wwn(hba_pwwn, &(fpin_req->li_desc),
						&list_of_wwn);
			if (count <= 0) {
				FPIN_ELOG("Could not find any WWNs, ret = %d\n", count);
				return count;
			}

			/* Get the list of paths to be failed from WWNs aquired above */
			INIT_LIST_HEAD(&dm_list_head);
			INIT_LIST_HEAD(&impacted_dev_list_head);
			count = fpin_fetch_dm_lun_data(&list_of_wwn,
					&dm_list_head, &impacted_dev_list_head);
			if (count <= 0) {
				FPIN_ELOG("Could not find any sd to fail, ret = %d\n", count);
				fpin_els_free_wwn_list(&list_of_wwn);
				return count;
			}

			/* Fail the paths using multipath daemon */
			fpin_dm_fail_path(&dm_list_head, &impacted_dev_list_head);

			/* Free the WWNs list extracted from ELS recieved */
			fpin_dm_free_dev(&impacted_dev_list_head);
			fpin_free_dm(&dm_list_head);
			fpin_els_free_wwn_list(&list_of_wwn);
			break;

		default:
			FPIN_ELOG("Invalid command received: 0x%x\n", els_cmd);
			break;
	}

	return (count);
}

/*
 * Function:
 *	fpin_handle_els_frame
 *
 * Inputs:
 *	The ELS frame to be processed.
 *
 * Description:
 *	This function process the FPIN ELS frame received from HBA driver,
 *	and push the frame to approriate frame list. Currently we have only FPIN
 *	LI frame list.
 */
int
fpin_handle_els_frame(fpin_payload_t *fpin_payload) {
	uint32_t els_cmd = 0;
	int ret = -1, retries = 3;


	els_cmd = *(uint32_t *)fpin_payload->payload;
	FPIN_ILOG("Got CMD in add as 0x%x\n", els_cmd);
	switch(els_cmd) {
		case ELS_CMD_MPD:
		case ELS_CMD_FPIN:
			/*Push the Payload to FPIN frame queue. */
retry_add:
			ret = fpin_els_add_li_frame(fpin_payload);
			if (ret != 0) {
				retries --;
				if (retries >= 0) {
					goto retry_add;
				}
			}
			break;

		case ELS_CMD_CJN:
			/*Push the Payload to CJN frame queue. */
			break;
		default:
			FPIN_ELOG("Invalid command received: 0x%x\n", els_cmd);
			break;
	}

	return (ret);
}

/*
 * This is the FPIN ELS consumer thread. The thread sleeps on pthread cond
 * variable unless notified by fpin_fabric_notification_receiver thread.
 * This thread is only to process FPIN-LI ELS frames. A new thread and frame
 * list will be added if any more ELS frames types are to be supported.
 */
void *fpin_els_li_consumer() {
	char payload[FC_PAYLOAD_MAXLEN];
	int retries = 0, ret = 0;
	wwn_t hba_pwwn;
	struct els_marginal_list *els_marg;

	for ( ; ; ) {
		pthread_mutex_lock(&fpin_li_mutex);
		if(list_empty(&els_marginal_list_head)) {
			pthread_cond_wait(&fpin_li_cond, &fpin_li_mutex);
		}
		
		while (!list_empty(&els_marginal_list_head)) {
			els_marg  = list_first_entry(&els_marginal_list_head,
							struct els_marginal_list, els_frame);
			memset(payload, '\0', FC_PAYLOAD_MAXLEN);
			hba_pwwn = els_marg->hba_pwwn;
			memcpy(payload, els_marg->payload, FC_PAYLOAD_MAXLEN);
			list_del(&els_marg->els_frame);
			pthread_mutex_unlock(&fpin_li_mutex);
			free(els_marg);

			/* Now finally process FPIN LI ELS Frame */
			FPIN_ILOG("Got a new Payload buffer, processing it\n");
			retries = 3;
retry:
			ret = fpin_process_els_frame(&hba_pwwn, payload);
			if (ret <= 0 ) {
				FPIN_ELOG("ELS frame processing failed with ret %d\n", ret);
				retries--;
				if (retries) {
					goto retry;
				}
			}
			pthread_mutex_lock(&fpin_li_mutex);			

		}
		pthread_mutex_unlock(&fpin_li_mutex);
	}
}
