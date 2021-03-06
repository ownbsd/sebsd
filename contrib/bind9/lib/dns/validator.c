/*
 * Copyright (C) 2004, 2005  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id$ */

#include <config.h>

#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/ds.h>
#include <dns/dnssec.h>
#include <dns/events.h>
#include <dns/keytable.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/ncache.h>
#include <dns/nsec.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/rdataset.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/validator.h>
#include <dns/view.h>

#define VALIDATOR_MAGIC			ISC_MAGIC('V', 'a', 'l', '?')
#define VALID_VALIDATOR(v)		ISC_MAGIC_VALID(v, VALIDATOR_MAGIC)

#define VALATTR_SHUTDOWN		0x0001
#define VALATTR_FOUNDNONEXISTENCE	0x0002
#define VALATTR_TRIEDVERIFY		0x0004
#define VALATTR_NEGATIVE		0x0008
#define VALATTR_INSECURITY		0x0010
#define VALATTR_DLVTRIED		0x0020

#define VALATTR_NEEDNOQNAME		0x0100
#define VALATTR_NEEDNOWILDCARD		0x0200
#define VALATTR_NEEDNODATA		0x0400

#define VALATTR_FOUNDNOQNAME		0x1000
#define VALATTR_FOUNDNOWILDCARD		0x2000
#define VALATTR_FOUNDNODATA		0x4000

#define NEEDNODATA(val) ((val->attributes & VALATTR_NEEDNODATA) != 0)
#define NEEDNOQNAME(val) ((val->attributes & VALATTR_NEEDNOQNAME) != 0)
#define NEEDNOWILDCARD(val) ((val->attributes & VALATTR_NEEDNOWILDCARD) != 0)
#define DLVTRIED(val) ((val->attributes & VALATTR_DLVTRIED) != 0)

#define SHUTDOWN(v)		(((v)->attributes & VALATTR_SHUTDOWN) != 0)

static void
destroy(dns_validator_t *val);

static isc_result_t
get_dst_key(dns_validator_t *val, dns_rdata_rrsig_t *siginfo,
	    dns_rdataset_t *rdataset);

static isc_result_t
validate(dns_validator_t *val, isc_boolean_t resume);

static isc_result_t
validatezonekey(dns_validator_t *val);

static isc_result_t
nsecvalidate(dns_validator_t *val, isc_boolean_t resume);

static isc_result_t
proveunsecure(dns_validator_t *val, isc_boolean_t resume);

static void
validator_logv(dns_validator_t *val, isc_logcategory_t *category,
	       isc_logmodule_t *module, int level, const char *fmt, va_list ap)
     ISC_FORMAT_PRINTF(5, 0);

static void
validator_log(dns_validator_t *val, int level, const char *fmt, ...)
     ISC_FORMAT_PRINTF(3, 4);

static void
validator_logcreate(dns_validator_t *val,
		    dns_name_t *name, dns_rdatatype_t type,
		    const char *caller, const char *operation);

static isc_result_t
dlv_validatezonekey(dns_validator_t *val);

static isc_result_t
dlv_validator_start(dns_validator_t *val);

static isc_result_t
finddlvsep(dns_validator_t *val, isc_boolean_t resume);

static inline void
markanswer(dns_validator_t *val) {
	validator_log(val, ISC_LOG_DEBUG(3), "marking as answer");
	if (val->event->rdataset)
		val->event->rdataset->trust = dns_trust_answer;
	if (val->event->sigrdataset)
		val->event->sigrdataset->trust = dns_trust_answer;
}

static void
validator_done(dns_validator_t *val, isc_result_t result) {
	isc_task_t *task;

	if (val->event == NULL)
		return;

	/*
	 * Caller must be holding the lock.
	 */

	val->event->result = result;
	task = val->event->ev_sender;
	val->event->ev_sender = val;
	val->event->ev_type = DNS_EVENT_VALIDATORDONE;
	val->event->ev_action = val->action;
	val->event->ev_arg = val->arg;
	isc_task_sendanddetach(&task, (isc_event_t **)&val->event);
}

static inline isc_boolean_t
exit_check(dns_validator_t *val) {
	/*
	 * Caller must be holding the lock.
	 */
	if (!SHUTDOWN(val))
		return (ISC_FALSE);

	INSIST(val->event == NULL);

	if (val->fetch != NULL || val->subvalidator != NULL)
		return (ISC_FALSE);

	return (ISC_TRUE);
}

static void
auth_nonpending(dns_message_t *message) {
	isc_result_t result;
	dns_name_t *name;
	dns_rdataset_t *rdataset;

	for (result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, DNS_SECTION_AUTHORITY))
	{
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			if (rdataset->trust == dns_trust_pending)
				rdataset->trust = dns_trust_authauthority;
		}
	}
}

static isc_boolean_t
isdelegation(dns_name_t *name, dns_rdataset_t *rdataset,
	     isc_result_t dbresult)
{
	dns_rdataset_t set;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_boolean_t found;
	isc_result_t result;

	REQUIRE(dbresult == DNS_R_NXRRSET || dbresult == DNS_R_NCACHENXRRSET);

	dns_rdataset_init(&set);
	if (dbresult == DNS_R_NXRRSET)
		dns_rdataset_clone(rdataset, &set);
	else {
		result = dns_ncache_getrdataset(rdataset, name,
						dns_rdatatype_nsec, &set);
		if (result != ISC_R_SUCCESS)
			return (ISC_FALSE);
	}

	INSIST(set.type == dns_rdatatype_nsec);

	found = ISC_FALSE;
	result = dns_rdataset_first(&set);
	if (result == ISC_R_SUCCESS) {
		dns_rdataset_current(&set, &rdata);
		found = dns_nsec_typepresent(&rdata, dns_rdatatype_ns);
	}
	dns_rdataset_disassociate(&set);
	return (found);
}

static void
fetch_callback_validator(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent;
	dns_validator_t *val;
	dns_rdataset_t *rdataset;
	isc_boolean_t want_destroy;
	isc_result_t result;
	isc_result_t eresult;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_FETCHDONE);
	devent = (dns_fetchevent_t *)event;
	val = devent->ev_arg;
	rdataset = &val->frdataset;
	eresult = devent->result;

	/* Free resources which are not of interest. */
	if (devent->node != NULL)
		dns_db_detachnode(devent->db, &devent->node);
	if (devent->db != NULL)
		dns_db_detach(&devent->db);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);
	isc_event_free(&event);
	dns_resolver_destroyfetch(&val->fetch);

	INSIST(val->event != NULL);

	validator_log(val, ISC_LOG_DEBUG(3), "in fetch_callback_validator");
	LOCK(&val->lock);
	if (eresult == ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "keyset with trust %d", rdataset->trust);
		/*
		 * Only extract the dst key if the keyset is secure.
		 */
		if (rdataset->trust >= dns_trust_secure) {
			result = get_dst_key(val, val->siginfo, rdataset);
			if (result == ISC_R_SUCCESS)
				val->keyset = &val->frdataset;
		}
		result = validate(val, ISC_TRUE);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	} else {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "fetch_callback_validator: got %s",
			      isc_result_totext(eresult));
		if (eresult == ISC_R_CANCELED)
			validator_done(val, eresult);
		else
			validator_done(val, DNS_R_NOVALIDKEY);
	}
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

static void
dsfetched(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent;
	dns_validator_t *val;
	dns_rdataset_t *rdataset;
	isc_boolean_t want_destroy;
	isc_result_t result;
	isc_result_t eresult;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_FETCHDONE);
	devent = (dns_fetchevent_t *)event;
	val = devent->ev_arg;
	rdataset = &val->frdataset;
	eresult = devent->result;

	/* Free resources which are not of interest. */
	if (devent->node != NULL)
		dns_db_detachnode(devent->db, &devent->node);
	if (devent->db != NULL)
		dns_db_detach(&devent->db);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);
	isc_event_free(&event);
	dns_resolver_destroyfetch(&val->fetch);

	INSIST(val->event != NULL);

	validator_log(val, ISC_LOG_DEBUG(3), "in dsfetched");
	LOCK(&val->lock);
	if (eresult == ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "dsset with trust %d", rdataset->trust);
		val->dsset = &val->frdataset;
		result = validatezonekey(val);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	} else if (eresult == DNS_R_NXRRSET ||
		   eresult == DNS_R_NCACHENXRRSET)
	{
		validator_log(val, ISC_LOG_DEBUG(3),
			      "falling back to insecurity proof");
		val->attributes |= VALATTR_INSECURITY;
		result = proveunsecure(val, ISC_FALSE);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	} else {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "dsfetched: got %s",
			      isc_result_totext(eresult));
		if (eresult == ISC_R_CANCELED)
			validator_done(val, eresult);
		else
			validator_done(val, DNS_R_NOVALIDDS);
	}
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

/*
 * XXX there's too much duplicated code here.
 */
static void
dsfetched2(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent;
	dns_validator_t *val;
	dns_name_t *tname;
	isc_boolean_t want_destroy;
	isc_result_t result;
	isc_result_t eresult;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_FETCHDONE);
	devent = (dns_fetchevent_t *)event;
	val = devent->ev_arg;
	eresult = devent->result;

	/* Free resources which are not of interest. */
	if (devent->node != NULL)
		dns_db_detachnode(devent->db, &devent->node);
	if (devent->db != NULL)
		dns_db_detach(&devent->db);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);
	dns_resolver_destroyfetch(&val->fetch);

	INSIST(val->event != NULL);

	validator_log(val, ISC_LOG_DEBUG(3), "in dsfetched2");
	LOCK(&val->lock);
	if (eresult == DNS_R_NXRRSET || eresult == DNS_R_NCACHENXRRSET) {
		/*
		 * There is no DS.  If this is a delegation, we're done.
		 */
		tname = dns_fixedname_name(&devent->foundname);
		if (isdelegation(tname, &val->frdataset, eresult)) {
			if (val->mustbesecure) {
				validator_log(val, ISC_LOG_WARNING,
					      "must be secure failure");
				validator_done(val, DNS_R_MUSTBESECURE);
			} else {
				markanswer(val);
				validator_done(val, ISC_R_SUCCESS);
			}
		} else {
			result = proveunsecure(val, ISC_TRUE);
			if (result != DNS_R_WAIT)
				validator_done(val, result);
		}
	} else if (eresult == ISC_R_SUCCESS ||
		   eresult == DNS_R_NXDOMAIN ||
		   eresult == DNS_R_NCACHENXDOMAIN)
	{
		/*
		 * Either there is a DS or this is not a zone cut.  Continue.
		 */
		result = proveunsecure(val, ISC_TRUE);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	} else {
		if (eresult == ISC_R_CANCELED)
			validator_done(val, eresult);
		else
			validator_done(val, DNS_R_NOVALIDDS);
	}
	isc_event_free(&event);
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

static void
keyvalidated(isc_task_t *task, isc_event_t *event) {
	dns_validatorevent_t *devent;
	dns_validator_t *val;
	isc_boolean_t want_destroy;
	isc_result_t result;
	isc_result_t eresult;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_VALIDATORDONE);

	devent = (dns_validatorevent_t *)event;
	val = devent->ev_arg;
	eresult = devent->result;

	isc_event_free(&event);
	dns_validator_destroy(&val->subvalidator);

	INSIST(val->event != NULL);

	validator_log(val, ISC_LOG_DEBUG(3), "in keyvalidated");
	LOCK(&val->lock);
	if (eresult == ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "keyset with trust %d", val->frdataset.trust);
		/*
		 * Only extract the dst key if the keyset is secure.
		 */
		if (val->frdataset.trust >= dns_trust_secure)
			(void) get_dst_key(val, val->siginfo, &val->frdataset);
		result = validate(val, ISC_TRUE);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	} else {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "keyvalidated: got %s",
			      isc_result_totext(eresult));
		validator_done(val, eresult);
	}
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

static void
dsvalidated(isc_task_t *task, isc_event_t *event) {
	dns_validatorevent_t *devent;
	dns_validator_t *val;
	isc_boolean_t want_destroy;
	isc_result_t result;
	isc_result_t eresult;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_VALIDATORDONE);

	devent = (dns_validatorevent_t *)event;
	val = devent->ev_arg;
	eresult = devent->result;

	isc_event_free(&event);
	dns_validator_destroy(&val->subvalidator);

	INSIST(val->event != NULL);

	validator_log(val, ISC_LOG_DEBUG(3), "in dsvalidated");
	LOCK(&val->lock);
	if (eresult == ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "dsset with trust %d", val->frdataset.trust);
		if ((val->attributes & VALATTR_INSECURITY) != 0)
			result = proveunsecure(val, ISC_TRUE);
		else
			result = validatezonekey(val);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	} else {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "dsvalidated: got %s",
			      isc_result_totext(eresult));
		validator_done(val, eresult);
	}
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

/*
 * Return ISC_R_SUCCESS if we can determine that the name doesn't exist
 * or we can determine whether there is data or not at the name.
 * If the name does not exist return the wildcard name.
 */
static isc_result_t
nsecnoexistnodata(dns_validator_t *val, dns_name_t* name, dns_name_t *nsecname,
		  dns_rdataset_t *nsecset, isc_boolean_t *exists,
		  isc_boolean_t *data, dns_name_t *wild)
{
	int order;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;
	dns_namereln_t relation;
	unsigned int olabels, nlabels, labels;
	dns_rdata_nsec_t nsec;
	isc_boolean_t atparent;

	REQUIRE(exists != NULL);
	REQUIRE(data != NULL);
	REQUIRE(nsecset != NULL &&
		nsecset->type == dns_rdatatype_nsec);

	result = dns_rdataset_first(nsecset);
	if (result != ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(3),
			"failure processing NSEC set");
		return (result);
	}
	dns_rdataset_current(nsecset, &rdata);

	validator_log(val, ISC_LOG_DEBUG(3), "looking for relevant nsec");
	relation = dns_name_fullcompare(name, nsecname, &order, &olabels);

	if (order < 0) {
		/*
		 * The name is not within the NSEC range.
		 */
		validator_log(val, ISC_LOG_DEBUG(3),
			      "NSEC does not cover name, before NSEC");
		return (ISC_R_IGNORE);
	}

	if (order == 0) {
		/*
		 * The names are the same.
		 */
		atparent = dns_rdatatype_atparent(val->event->type);
		if (dns_nsec_typepresent(&rdata, dns_rdatatype_ns) &&
		    !dns_nsec_typepresent(&rdata, dns_rdatatype_soa))
		{
			if (!atparent) {
				/*
				 * This NSEC record is from somewhere higher in
				 * the DNS, and at the parent of a delegation.
				 * It can not be legitimately used here.
				 */
				validator_log(val, ISC_LOG_DEBUG(3),
					      "ignoring parent nsec");
				return (ISC_R_IGNORE);
			}
		} else if (atparent) {
			/*
			 * This NSEC record is from the child.
			 * It can not be legitimately used here.
			 */
			validator_log(val, ISC_LOG_DEBUG(3),
				      "ignoring child nsec");
			return (ISC_R_IGNORE);
		}
		*exists = ISC_TRUE;
		*data = dns_nsec_typepresent(&rdata, val->event->type);
		validator_log(val, ISC_LOG_DEBUG(3),
			      "nsec proves name exists (owner) data=%d",
			      *data);
		return (ISC_R_SUCCESS);
	}

	if (relation == dns_namereln_subdomain &&
	    dns_nsec_typepresent(&rdata, dns_rdatatype_ns) &&
	    !dns_nsec_typepresent(&rdata, dns_rdatatype_soa))
	{
		/*
		 * This NSEC record is from somewhere higher in
		 * the DNS, and at the parent of a delegation.
		 * It can not be legitimately used here.
		 */
		validator_log(val, ISC_LOG_DEBUG(3), "ignoring parent nsec");
		return (ISC_R_IGNORE);
	}

	result = dns_rdata_tostruct(&rdata, &nsec, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);
	relation = dns_name_fullcompare(&nsec.next, name, &order, &nlabels);
	if (order == 0) {
		dns_rdata_freestruct(&nsec);
		validator_log(val, ISC_LOG_DEBUG(3),
			      "ignoring nsec matches next name");
		return (ISC_R_IGNORE);
	}

	if (order < 0 && !dns_name_issubdomain(nsecname, &nsec.next)) {
		/*
		 * The name is not within the NSEC range.
		 */
		dns_rdata_freestruct(&nsec);
		validator_log(val, ISC_LOG_DEBUG(3),
			    "ignoring nsec because name is past end of range");
		return (ISC_R_IGNORE);
	}

	if (order > 0 && relation == dns_namereln_subdomain) {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "nsec proves name exist (empty)");
		dns_rdata_freestruct(&nsec);
		*exists = ISC_TRUE;
		*data = ISC_FALSE;
		return (ISC_R_SUCCESS);
	}
	if (wild != NULL) {
		dns_name_t common;
		dns_name_init(&common, NULL);
		if (olabels > nlabels) {
			labels = dns_name_countlabels(nsecname);
			dns_name_getlabelsequence(nsecname, labels - olabels,
						  olabels, &common);
		} else {
			labels = dns_name_countlabels(&nsec.next);
			dns_name_getlabelsequence(&nsec.next, labels - nlabels,
						  nlabels, &common);
		}
		result = dns_name_concatenate(dns_wildcardname, &common,
					       wild, NULL);
		if (result != ISC_R_SUCCESS) {
			validator_log(val, ISC_LOG_DEBUG(3),
				    "failure generating wilcard name");
			return (result);
		}
	}
	dns_rdata_freestruct(&nsec);
	validator_log(val, ISC_LOG_DEBUG(3), "nsec range ok");
	*exists = ISC_FALSE;
	return (ISC_R_SUCCESS);
}

static void
authvalidated(isc_task_t *task, isc_event_t *event) {
	dns_validatorevent_t *devent;
	dns_validator_t *val;
	dns_rdataset_t *rdataset;
	isc_boolean_t want_destroy;
	isc_result_t result;
	isc_boolean_t exists, data;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_VALIDATORDONE);

	devent = (dns_validatorevent_t *)event;
	rdataset = devent->rdataset;
	val = devent->ev_arg;
	result = devent->result;
	dns_validator_destroy(&val->subvalidator);

	INSIST(val->event != NULL);

	validator_log(val, ISC_LOG_DEBUG(3), "in authvalidated");
	LOCK(&val->lock);
	if (result != ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "authvalidated: got %s",
			      isc_result_totext(result));
		if (result == ISC_R_CANCELED)
			validator_done(val, result);
		else {
			result = nsecvalidate(val, ISC_TRUE);
			if (result != DNS_R_WAIT)
				validator_done(val, result);
		}
	} else {
		dns_name_t **proofs = val->event->proofs;
		
		if (rdataset->trust == dns_trust_secure)
			val->seensig = ISC_TRUE;

		if (rdataset->type == dns_rdatatype_nsec &&
		    rdataset->trust == dns_trust_secure &&
		    ((val->attributes & VALATTR_NEEDNODATA) != 0 ||
		     (val->attributes & VALATTR_NEEDNOQNAME) != 0) &&
	            (val->attributes & VALATTR_FOUNDNODATA) == 0 &&
		    (val->attributes & VALATTR_FOUNDNOQNAME) == 0 &&
		    nsecnoexistnodata(val, val->event->name, devent->name,
				      rdataset, &exists, &data,
				      dns_fixedname_name(&val->wild))
				      == ISC_R_SUCCESS)
		 {
			if (exists && !data) {
				val->attributes |= VALATTR_FOUNDNODATA;
				if (NEEDNODATA(val))
					proofs[DNS_VALIDATOR_NODATAPROOF] =
						devent->name;
			}
			if (!exists) {
				val->attributes |= VALATTR_FOUNDNOQNAME;
				if (NEEDNOQNAME(val))
					proofs[DNS_VALIDATOR_NOQNAMEPROOF] =
						devent->name;
			}
		}
		result = nsecvalidate(val, ISC_TRUE);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	}
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);

	/*
	 * Free stuff from the event.
	 */
	isc_event_free(&event);
}

static void
negauthvalidated(isc_task_t *task, isc_event_t *event) {
	dns_validatorevent_t *devent;
	dns_validator_t *val;
	isc_boolean_t want_destroy;
	isc_result_t eresult;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_VALIDATORDONE);

	devent = (dns_validatorevent_t *)event;
	val = devent->ev_arg;
	eresult = devent->result;
	isc_event_free(&event);
	dns_validator_destroy(&val->subvalidator);

	INSIST(val->event != NULL);

	validator_log(val, ISC_LOG_DEBUG(3), "in negauthvalidated");
	LOCK(&val->lock);
	if (eresult == ISC_R_SUCCESS) {
		val->attributes |= VALATTR_FOUNDNONEXISTENCE;
		validator_log(val, ISC_LOG_DEBUG(3),
			      "nonexistence proof found");
		auth_nonpending(val->event->message);
		validator_done(val, ISC_R_SUCCESS);
	} else {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "negauthvalidated: got %s",
			      isc_result_totext(eresult));
		validator_done(val, eresult);
	}
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

static inline isc_result_t
view_find(dns_validator_t *val, dns_name_t *name, dns_rdatatype_t type) {
	dns_fixedname_t fixedname;
	dns_name_t *foundname;
	dns_rdata_nsec_t nsec;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;
	unsigned int options;
	char buf1[DNS_NAME_FORMATSIZE];
	char buf2[DNS_NAME_FORMATSIZE];
	char buf3[DNS_NAME_FORMATSIZE];

	if (dns_rdataset_isassociated(&val->frdataset))
		dns_rdataset_disassociate(&val->frdataset);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);

	if (val->view->zonetable == NULL)
		return (ISC_R_CANCELED);

	options = DNS_DBFIND_PENDINGOK;
	if (type == dns_rdatatype_dlv)
		options |= DNS_DBFIND_COVERINGNSEC;
	dns_fixedname_init(&fixedname);
	foundname = dns_fixedname_name(&fixedname);
	result = dns_view_find(val->view, name, type, 0, options,
			       ISC_FALSE, NULL, NULL, foundname,
			       &val->frdataset, &val->fsigrdataset);
	if (result == DNS_R_NXDOMAIN) {
		if (dns_rdataset_isassociated(&val->frdataset))
			dns_rdataset_disassociate(&val->frdataset);
		if (dns_rdataset_isassociated(&val->fsigrdataset))
			dns_rdataset_disassociate(&val->fsigrdataset);
	} else if (result == DNS_R_COVERINGNSEC) {
		validator_log(val, ISC_LOG_DEBUG(3), "DNS_R_COVERINGNSEC");
		/*
		 * Check if the returned NSEC covers the name.
		 */
		INSIST(type == dns_rdatatype_dlv);
		if (val->frdataset.trust != dns_trust_secure) {
			validator_log(val, ISC_LOG_DEBUG(3),
				      "covering nsec: trust %u",
				      val->frdataset.trust);
			goto notfound;
		}
		result = dns_rdataset_first(&val->frdataset);
		if (result != ISC_R_SUCCESS)
			goto notfound;
		dns_rdataset_current(&val->frdataset, &rdata);
		if (dns_nsec_typepresent(&rdata, dns_rdatatype_ns) &&
		    !dns_nsec_typepresent(&rdata, dns_rdatatype_soa)) {
			/* Parent NSEC record. */
			if (dns_name_issubdomain(name, foundname)) {
				validator_log(val, ISC_LOG_DEBUG(3),
					      "covering nsec: for parent");
				goto notfound;
			}
		}
		result = dns_rdata_tostruct(&rdata, &nsec, NULL);
		if (result != ISC_R_SUCCESS)
			goto notfound;
		if (dns_name_compare(foundname, &nsec.next) >= 0) {
			/* End of zone chain. */
			if (!dns_name_issubdomain(name, &nsec.next)) {
				/*
 				 * XXXMPA We could look for a parent NSEC
				 * at nsec.next and if found retest with
				 * this NSEC.
				 */
				dns_rdata_freestruct(&nsec);
				validator_log(val, ISC_LOG_DEBUG(3),
					      "covering nsec: not in zone");
				goto notfound;
			}
		} else if (dns_name_compare(name, &nsec.next) >= 0) {
			/*
			 * XXXMPA We could check if this NSEC is at a zone
			 * apex and if the qname is not below it and look for
			 * a parent NSEC with the same name.  This requires
			 * that we can cache both NSEC records which we
			 * currently don't support.
			 */
			dns_rdata_freestruct(&nsec);
			validator_log(val, ISC_LOG_DEBUG(3),
				      "covering nsec: not in range");
			goto notfound;
		}
		if (isc_log_wouldlog(dns_lctx,ISC_LOG_DEBUG(3))) {
			dns_name_format(name, buf1, sizeof buf1);
			dns_name_format(foundname, buf2, sizeof buf2);
			dns_name_format(&nsec.next, buf3, sizeof buf3);
			validator_log(val, ISC_LOG_DEBUG(3),
				      "covering nsec found: '%s' '%s' '%s'",
				      buf1, buf2, buf3);
		}
		if (dns_rdataset_isassociated(&val->frdataset))
			dns_rdataset_disassociate(&val->frdataset);
		if (dns_rdataset_isassociated(&val->fsigrdataset))
			dns_rdataset_disassociate(&val->fsigrdataset);
		dns_rdata_freestruct(&nsec);
		result = DNS_R_NCACHENXDOMAIN;
	} else if (result != ISC_R_SUCCESS &&
                   result != DNS_R_GLUE &&
                   result != DNS_R_HINT &&
                   result != DNS_R_NCACHENXDOMAIN &&
                   result != DNS_R_NCACHENXRRSET &&
                   result != DNS_R_NXRRSET &&
                   result != DNS_R_HINTNXRRSET &&
                   result != ISC_R_NOTFOUND) {
		goto  notfound;
	}
	return (result);

 notfound:
	if (dns_rdataset_isassociated(&val->frdataset))
		dns_rdataset_disassociate(&val->frdataset);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);
	return (ISC_R_NOTFOUND);
}

static inline isc_boolean_t
check_deadlock(dns_validator_t *val, dns_name_t *name, dns_rdatatype_t type) {
	dns_validator_t *parent;

	for (parent = val->parent; parent != NULL; parent = parent->parent) {
		if (parent->event != NULL &&
		    parent->event->type == type &&
		    dns_name_equal(parent->event->name, name))
		{
			validator_log(val, ISC_LOG_DEBUG(3),
				      "continuing validation would lead to "
				      "deadlock: aborting validation");
			return (ISC_TRUE);
		}
	}
	return (ISC_FALSE);
}

static inline isc_result_t
create_fetch(dns_validator_t *val, dns_name_t *name, dns_rdatatype_t type,
	     isc_taskaction_t callback, const char *caller)
{
	if (dns_rdataset_isassociated(&val->frdataset))
		dns_rdataset_disassociate(&val->frdataset);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);

	if (check_deadlock(val, name, type))
		return (DNS_R_NOVALIDSIG);

	validator_logcreate(val, name, type, caller, "fetch");
	return (dns_resolver_createfetch(val->view->resolver, name, type,
					 NULL, NULL, NULL, 0,
					 val->event->ev_sender,
					 callback, val,
					 &val->frdataset,
					 &val->fsigrdataset,
					 &val->fetch));
}

static inline isc_result_t
create_validator(dns_validator_t *val, dns_name_t *name, dns_rdatatype_t type,
		 dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		 isc_taskaction_t action, const char *caller)
{
	isc_result_t result;

	if (check_deadlock(val, name, type))
		return (DNS_R_NOVALIDSIG);

	validator_logcreate(val, name, type, caller, "validator");
	result = dns_validator_create(val->view, name, type,
				      rdataset, sigrdataset, NULL, 0,
				      val->task, action, val,
				      &val->subvalidator);
	if (result == ISC_R_SUCCESS) {
		val->subvalidator->parent = val;
		val->subvalidator->depth = val->depth + 1;
	}
	return (result);
}

/*
 * Try to find a key that could have signed 'siginfo' among those
 * in 'rdataset'.  If found, build a dst_key_t for it and point
 * val->key at it.
 *
 * If val->key is non-NULL, this returns the next matching key.
 */
static isc_result_t
get_dst_key(dns_validator_t *val, dns_rdata_rrsig_t *siginfo,
	    dns_rdataset_t *rdataset)
{
	isc_result_t result;
	isc_buffer_t b;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dst_key_t *oldkey = val->key;
	isc_boolean_t foundold;

	if (oldkey == NULL)
		foundold = ISC_TRUE;
	else {
		foundold = ISC_FALSE;
		val->key = NULL;
	}

	result = dns_rdataset_first(rdataset);
	if (result != ISC_R_SUCCESS)
		goto failure;
	do {
		dns_rdataset_current(rdataset, &rdata);

		isc_buffer_init(&b, rdata.data, rdata.length);
		isc_buffer_add(&b, rdata.length);
		INSIST(val->key == NULL);
		result = dst_key_fromdns(&siginfo->signer, rdata.rdclass, &b,
					 val->view->mctx, &val->key);
		if (result != ISC_R_SUCCESS)
			goto failure;
		if (siginfo->algorithm ==
		    (dns_secalg_t)dst_key_alg(val->key) &&
		    siginfo->keyid ==
		    (dns_keytag_t)dst_key_id(val->key) &&
		    dst_key_iszonekey(val->key))
		{
			if (foundold)
				/*
				 * This is the key we're looking for.
				 */
				return (ISC_R_SUCCESS);
			else if (dst_key_compare(oldkey, val->key) == ISC_TRUE)
			{
				foundold = ISC_TRUE;
				dst_key_free(&oldkey);
			}
		}
		dst_key_free(&val->key);
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(rdataset);
	} while (result == ISC_R_SUCCESS);
	if (result == ISC_R_NOMORE)
		result = ISC_R_NOTFOUND;

 failure:
	if (oldkey != NULL)
		dst_key_free(&oldkey);

	return (result);
}

static isc_result_t
get_key(dns_validator_t *val, dns_rdata_rrsig_t *siginfo) {
	isc_result_t result;
	unsigned int nlabels;
	int order;
	dns_namereln_t namereln;

	/*
	 * Is the signer name appropriate for this signature?
	 *
	 * The signer name must be at the same level as the owner name
	 * or closer to the the DNS root.
	 */
	namereln = dns_name_fullcompare(val->event->name, &siginfo->signer,
					&order, &nlabels);
	if (namereln != dns_namereln_subdomain &&
	    namereln != dns_namereln_equal)
		return (DNS_R_CONTINUE);

	if (namereln == dns_namereln_equal) {
		/*
		 * If this is a self-signed keyset, it must not be a zone key
		 * (since get_key is not called from validatezonekey).
		 */
		if (val->event->rdataset->type == dns_rdatatype_dnskey)
			return (DNS_R_CONTINUE);

		/*
		 * Records appearing in the parent zone at delegation
		 * points cannot be self-signed.
		 */
		if (dns_rdatatype_atparent(val->event->rdataset->type))
			return (DNS_R_CONTINUE);
	}

	/*
	 * Do we know about this key?
	 */
	result = view_find(val, &siginfo->signer, dns_rdatatype_dnskey);
	if (result == ISC_R_SUCCESS) {
		/*
		 * We have an rrset for the given keyname.
		 */
		val->keyset = &val->frdataset;
		if (val->frdataset.trust == dns_trust_pending &&
		    dns_rdataset_isassociated(&val->fsigrdataset))
		{
			/*
			 * We know the key but haven't validated it yet.
			 */
			result = create_validator(val, &siginfo->signer,
						  dns_rdatatype_dnskey,
						  &val->frdataset,
						  &val->fsigrdataset,
						  keyvalidated,
						  "get_key");
			if (result != ISC_R_SUCCESS)
				return (result);
			return (DNS_R_WAIT);
		} else if (val->frdataset.trust == dns_trust_pending) {
			/*
			 * Having a pending key with no signature means that
			 * something is broken.
			 */
			result = DNS_R_CONTINUE;
		} else if (val->frdataset.trust < dns_trust_secure) {
			/*
			 * The key is legitimately insecure.  There's no
			 * point in even attempting verification.
			 */
			val->key = NULL;
			result = ISC_R_SUCCESS;
		} else {
			/*
			 * See if we've got the key used in the signature.
			 */
			validator_log(val, ISC_LOG_DEBUG(3),
				      "keyset with trust %d",
				      val->frdataset.trust);
			result = get_dst_key(val, siginfo, val->keyset);
			if (result != ISC_R_SUCCESS) {
				/*
				 * Either the key we're looking for is not
				 * in the rrset, or something bad happened.
				 * Give up.
				 */
				result = DNS_R_CONTINUE;
			}
		}
	} else if (result == ISC_R_NOTFOUND) {
		/*
		 * We don't know anything about this key.
		 */
		result = create_fetch(val, &siginfo->signer, dns_rdatatype_dnskey,
				      fetch_callback_validator, "get_key");
		if (result != ISC_R_SUCCESS)
			return (result);
		return (DNS_R_WAIT);
	} else if (result ==  DNS_R_NCACHENXDOMAIN ||
		   result == DNS_R_NCACHENXRRSET ||
		   result == DNS_R_NXDOMAIN ||
		   result == DNS_R_NXRRSET)
	{
		/*
		 * This key doesn't exist.
		 */
		result = DNS_R_CONTINUE;
	}

	if (dns_rdataset_isassociated(&val->frdataset) &&
	    val->keyset != &val->frdataset)
		dns_rdataset_disassociate(&val->frdataset);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);

	return (result);
}

static dns_keytag_t
compute_keytag(dns_rdata_t *rdata, dns_rdata_dnskey_t *key) {
	isc_region_t r;

	dns_rdata_toregion(rdata, &r);
	return (dst_region_computeid(&r, key->algorithm));
}

/*
 * Is this keyset self-signed?
 */
static isc_boolean_t
isselfsigned(dns_validator_t *val) {
	dns_rdataset_t *rdataset, *sigrdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	dns_rdata_dnskey_t key;
	dns_rdata_rrsig_t sig;
	dns_keytag_t keytag;
	isc_result_t result;

	rdataset = val->event->rdataset;
	sigrdataset = val->event->sigrdataset;

	INSIST(rdataset->type == dns_rdatatype_dnskey);

	for (result = dns_rdataset_first(rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset))
	{
		dns_rdata_reset(&rdata);
		dns_rdataset_current(rdataset, &rdata);
		(void)dns_rdata_tostruct(&rdata, &key, NULL);
		keytag = compute_keytag(&rdata, &key);
		for (result = dns_rdataset_first(sigrdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(sigrdataset))
		{
			dns_rdata_reset(&sigrdata);
			dns_rdataset_current(sigrdataset, &sigrdata);
			(void)dns_rdata_tostruct(&sigrdata, &sig, NULL);

			if (sig.algorithm == key.algorithm &&
			    sig.keyid == keytag)
				return (ISC_TRUE);
		}
	}
	return (ISC_FALSE);
}

static isc_result_t
verify(dns_validator_t *val, dst_key_t *key, dns_rdata_t *rdata) {
	isc_result_t result;
	dns_fixedname_t fixed;

	val->attributes |= VALATTR_TRIEDVERIFY;
	dns_fixedname_init(&fixed);
	result = dns_dnssec_verify2(val->event->name, val->event->rdataset,
				    key, ISC_FALSE, val->view->mctx, rdata,
				    dns_fixedname_name(&fixed));
	validator_log(val, ISC_LOG_DEBUG(3),
		      "verify rdataset: %s",
		      isc_result_totext(result));
	if (result == DNS_R_FROMWILDCARD) {
		if (!dns_name_equal(val->event->name,
				    dns_fixedname_name(&fixed)))
			val->attributes |= VALATTR_NEEDNOQNAME;
		result = ISC_R_SUCCESS;
	}
	return (result);
}

/*
 * Attempts positive response validation of a normal RRset.
 *
 * Returns:
 *	ISC_R_SUCCESS	Validation completed successfully
 *	DNS_R_WAIT	Validation has started but is waiting
 *			for an event.
 *	Other return codes are possible and all indicate failure.
 */
static isc_result_t
validate(dns_validator_t *val, isc_boolean_t resume) {
	isc_result_t result;
	dns_validatorevent_t *event;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	/*
	 * Caller must be holding the validator lock.
	 */

	event = val->event;

	if (resume) {
		/*
		 * We already have a sigrdataset.
		 */
		result = ISC_R_SUCCESS;
		validator_log(val, ISC_LOG_DEBUG(3), "resuming validate");
	} else {
		result = dns_rdataset_first(event->sigrdataset);
	}

	for (;
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(event->sigrdataset))
	{
		dns_rdata_reset(&rdata);
		dns_rdataset_current(event->sigrdataset, &rdata);
		if (val->siginfo == NULL) {
			val->siginfo = isc_mem_get(val->view->mctx,
						   sizeof(*val->siginfo));
			if (val->siginfo == NULL)
				return (ISC_R_NOMEMORY);
		}
		result = dns_rdata_tostruct(&rdata, val->siginfo, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);

		/*
		 * At this point we could check that the signature algorithm
		 * was known and "sufficiently good".
		 */
		if (!dns_resolver_algorithm_supported(val->view->resolver,
						      event->name,
						      val->siginfo->algorithm))
			continue;

		if (!resume) {
			result = get_key(val, val->siginfo);
			if (result == DNS_R_CONTINUE)
				continue; /* Try the next SIG RR. */
			if (result != ISC_R_SUCCESS)
				return (result);
		}

		/*
		 * The key is insecure, so mark the data as insecure also.
		 */
		if (val->key == NULL) {
			if (val->mustbesecure) {
				validator_log(val, ISC_LOG_WARNING,
					      "must be secure failure");
				return (DNS_R_MUSTBESECURE);
			}
			markanswer(val);
			return (ISC_R_SUCCESS);
		}

		do {
			result = verify(val, val->key, &rdata);
			if (result == ISC_R_SUCCESS)
				break;
			if (val->keynode != NULL) {
				dns_keynode_t *nextnode = NULL;
				result = dns_keytable_findnextkeynode(
							val->keytable,
							val->keynode,
							&nextnode);
				dns_keytable_detachkeynode(val->keytable,
							   &val->keynode);
				val->keynode = nextnode;
				if (result != ISC_R_SUCCESS) {
					val->key = NULL;
					break;
				}
				val->key = dns_keynode_key(val->keynode);
			} else {
				if (get_dst_key(val, val->siginfo, val->keyset)
				    != ISC_R_SUCCESS)
					break;
			}
		} while (1);
		if (result != ISC_R_SUCCESS)
			validator_log(val, ISC_LOG_DEBUG(3),
				      "failed to verify rdataset");
		else {
			isc_uint32_t ttl;
			isc_stdtime_t now;

			isc_stdtime_get(&now);
			ttl = ISC_MIN(event->rdataset->ttl,
				      val->siginfo->timeexpire - now);
			if (val->keyset != NULL)
				ttl = ISC_MIN(ttl, val->keyset->ttl);
			event->rdataset->ttl = ttl;
			event->sigrdataset->ttl = ttl;
		}

		if (val->keynode != NULL)
			dns_keytable_detachkeynode(val->keytable,
						   &val->keynode);
		else {
			if (val->key != NULL)
				dst_key_free(&val->key);
			if (val->keyset != NULL) {
				dns_rdataset_disassociate(val->keyset);
				val->keyset = NULL;
			}
		}
		val->key = NULL;
		if ((val->attributes & VALATTR_NEEDNOQNAME) != 0) {
			if (val->event->message == NULL) {
				validator_log(val, ISC_LOG_DEBUG(3),
				      "no message available for noqname proof");
				return (DNS_R_NOVALIDSIG);
			}
			validator_log(val, ISC_LOG_DEBUG(3),
				      "looking for noqname proof");
			return (nsecvalidate(val, ISC_FALSE));
		} else if (result == ISC_R_SUCCESS) {
			event->rdataset->trust = dns_trust_secure;
			event->sigrdataset->trust = dns_trust_secure;
			validator_log(val, ISC_LOG_DEBUG(3),
				      "marking as secure");
			return (result);
		} else {
			validator_log(val, ISC_LOG_DEBUG(3),
				      "verify failure: %s",
				      isc_result_totext(result));
			resume = ISC_FALSE;
		}
	}
	if (result != ISC_R_NOMORE) {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "failed to iterate signatures: %s",
			      isc_result_totext(result));
		return (result);
	}

	validator_log(val, ISC_LOG_INFO, "no valid signature found");
	return (DNS_R_NOVALIDSIG);
}

static isc_result_t
dlv_validatezonekey(dns_validator_t *val) {
	dns_keytag_t keytag;
	dns_rdata_dlv_t dlv;
	dns_rdata_dnskey_t key;
	dns_rdata_rrsig_t sig;
	dns_rdata_t dlvrdata = DNS_RDATA_INIT;
	dns_rdata_t keyrdata = DNS_RDATA_INIT;
	dns_rdata_t newdsrdata = DNS_RDATA_INIT;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	dns_rdataset_t trdataset;
	dst_key_t *dstkey;
	isc_boolean_t supported_algorithm;
	isc_result_t result;
	unsigned char dsbuf[DNS_DS_BUFFERSIZE];

	validator_log(val, ISC_LOG_DEBUG(3), "dlv_validatezonekey");
	/*
	 * Look through the DLV record and find the keys that can sign the
	 * key set and the matching signature.  For each such key, attempt
	 * verification.
	 */

	supported_algorithm = ISC_FALSE;

	for (result = dns_rdataset_first(&val->dlv);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&val->dlv))
	{
		dns_rdata_reset(&dlvrdata);
		dns_rdataset_current(&val->dlv, &dlvrdata);
		(void)dns_rdata_tostruct(&dlvrdata, &dlv, NULL);

		if (dlv.digest_type != DNS_DSDIGEST_SHA1 ||
		    !dns_resolver_algorithm_supported(val->view->resolver,
						      val->event->name,
						      dlv.algorithm))
			continue;

		supported_algorithm = ISC_TRUE;

		dns_rdataset_init(&trdataset);
		dns_rdataset_clone(val->event->rdataset, &trdataset);

		for (result = dns_rdataset_first(&trdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&trdataset))
		{
			dns_rdata_reset(&keyrdata);
			dns_rdataset_current(&trdataset, &keyrdata);
			(void)dns_rdata_tostruct(&keyrdata, &key, NULL);
			keytag = compute_keytag(&keyrdata, &key);
			if (dlv.key_tag != keytag ||
			    dlv.algorithm != key.algorithm)
				continue;
			dns_rdata_reset(&newdsrdata);
			result = dns_ds_buildrdata(val->event->name,
						   &keyrdata, dlv.digest_type,
						   dsbuf, &newdsrdata);
			if (result != ISC_R_SUCCESS) {
				validator_log(val, ISC_LOG_DEBUG(3),
					      "dns_ds_buildrdata() -> %s",
					      dns_result_totext(result));
				continue;
			}
			/* Covert to DLV */
			newdsrdata.type = dns_rdatatype_dlv;
			if (dns_rdata_compare(&dlvrdata, &newdsrdata) == 0)
				break;
		}
		if (result != ISC_R_SUCCESS) {
			validator_log(val, ISC_LOG_DEBUG(3),
				      "no DNSKEY matching DLV");
			continue;
		}
		validator_log(val, ISC_LOG_DEBUG(3),
		      "Found matching DLV record: checking for signature");

		for (result = dns_rdataset_first(val->event->sigrdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(val->event->sigrdataset))
		{
			dns_rdata_reset(&sigrdata);
			dns_rdataset_current(val->event->sigrdataset,
					     &sigrdata);
			(void)dns_rdata_tostruct(&sigrdata, &sig, NULL);
			if (dlv.key_tag != sig.keyid &&
			    dlv.algorithm != sig.algorithm)
				continue;
			dstkey = NULL;
			result = dns_dnssec_keyfromrdata(val->event->name,
							 &keyrdata,
							 val->view->mctx,
							 &dstkey);
			if (result != ISC_R_SUCCESS)
				/*
				 * This really shouldn't happen, but...
				 */
				continue;

			result = verify(val, dstkey, &sigrdata);
			dst_key_free(&dstkey);
			if (result == ISC_R_SUCCESS)
				break;
		}
		dns_rdataset_disassociate(&trdataset);
		if (result == ISC_R_SUCCESS)
			break;
		validator_log(val, ISC_LOG_DEBUG(3),
			      "no RRSIG matching DLV key");
	}
	if (result == ISC_R_SUCCESS) {
		val->event->rdataset->trust = dns_trust_secure;
		val->event->sigrdataset->trust = dns_trust_secure;
		validator_log(val, ISC_LOG_DEBUG(3), "marking as secure");
		return (result);
	} else if (result == ISC_R_NOMORE && !supported_algorithm) {
		if (val->mustbesecure) {
			validator_log(val, ISC_LOG_WARNING,
				      "must be secure failure");
			return (DNS_R_MUSTBESECURE);
		}
		validator_log(val, ISC_LOG_DEBUG(3),
			      "no supported algorithm/digest (dlv)");
		markanswer(val);
		return (ISC_R_SUCCESS);
	} else
		return (DNS_R_NOVALIDSIG);
}

/*
 * Attempts positive response validation of an RRset containing zone keys.
 *
 * Returns:
 *	ISC_R_SUCCESS	Validation completed successfully
 *	DNS_R_WAIT	Validation has started but is waiting
 *			for an event.
 *	Other return codes are possible and all indicate failure.
 */
static isc_result_t
validatezonekey(dns_validator_t *val) {
	isc_result_t result;
	dns_validatorevent_t *event;
	dns_rdataset_t trdataset;
	dns_rdata_t dsrdata = DNS_RDATA_INIT;
	dns_rdata_t newdsrdata = DNS_RDATA_INIT;
	dns_rdata_t keyrdata = DNS_RDATA_INIT;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	unsigned char dsbuf[DNS_DS_BUFFERSIZE];
	dns_keytag_t keytag;
	dns_rdata_ds_t ds;
	dns_rdata_dnskey_t key;
	dns_rdata_rrsig_t sig;
	dst_key_t *dstkey;
	isc_boolean_t supported_algorithm;

	/*
	 * Caller must be holding the validator lock.
	 */

	event = val->event;

	if (val->havedlvsep && val->dlv.trust >= dns_trust_secure &&
	    dns_name_equal(event->name, dns_fixedname_name(&val->dlvsep)))
		return (dlv_validatezonekey(val));

	if (val->dsset == NULL) {
		/*
		 * First, see if this key was signed by a trusted key.
		 */
		for (result = dns_rdataset_first(val->event->sigrdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(val->event->sigrdataset))
		{
			dns_keynode_t *keynode = NULL, *nextnode = NULL;

			dns_rdata_reset(&sigrdata);
			dns_rdataset_current(val->event->sigrdataset,
					     &sigrdata);
			(void)dns_rdata_tostruct(&sigrdata, &sig, NULL);
			result = dns_keytable_findkeynode(val->keytable,
							  val->event->name,
							  sig.algorithm,
							  sig.keyid,
							  &keynode);
			while (result == ISC_R_SUCCESS) {
				dstkey = dns_keynode_key(keynode);
				result = verify(val, dstkey, &sigrdata);
				if (result == ISC_R_SUCCESS) {
					dns_keytable_detachkeynode(val->keytable,
								   &keynode);
					break;
				}
				result = dns_keytable_findnextkeynode(
								val->keytable,
								keynode,
								&nextnode);
				dns_keytable_detachkeynode(val->keytable,
							   &keynode);
				keynode = nextnode;
			}
			if (result == ISC_R_SUCCESS) {
				event->rdataset->trust = dns_trust_secure;
				event->sigrdataset->trust = dns_trust_secure;
				validator_log(val, ISC_LOG_DEBUG(3),
					      "signed by trusted key; "
					      "marking as secure");
				return (result);
			}
		}

		/*
		 * If this is the root name and there was no trusted key,
		 * give up, since there's no DS at the root.
		 */
		if (dns_name_equal(event->name, dns_rootname)) {
			if ((val->attributes & VALATTR_TRIEDVERIFY) != 0)
				return (DNS_R_NOVALIDSIG);
			else
				return (DNS_R_NOVALIDDS);
		}

		/*
		 * Otherwise, try to find the DS record.
		 */
		result = view_find(val, val->event->name, dns_rdatatype_ds);
		if (result == ISC_R_SUCCESS) {
			/*
			 * We have DS records.
			 */
			val->dsset = &val->frdataset;
			if (val->frdataset.trust == dns_trust_pending &&
			    dns_rdataset_isassociated(&val->fsigrdataset))
			{
				result = create_validator(val,
							  val->event->name,
							  dns_rdatatype_ds,
							  &val->frdataset,
							  &val->fsigrdataset,
							  dsvalidated,
							  "validatezonekey");
				if (result != ISC_R_SUCCESS)
					return (result);
				return (DNS_R_WAIT);
			} else if (val->frdataset.trust == dns_trust_pending) {
				/*
				 * There should never be an unsigned DS.
				 */
				dns_rdataset_disassociate(&val->frdataset);
				validator_log(val, ISC_LOG_DEBUG(2),
					      "unsigned DS record");
				return (DNS_R_NOVALIDSIG);
			} else
				result = ISC_R_SUCCESS;
		} else if (result == ISC_R_NOTFOUND) {
			/*
			 * We don't have the DS.  Find it.
			 */
			result = create_fetch(val, val->event->name,
					      dns_rdatatype_ds, dsfetched,
					      "validatezonekey");
			if (result != ISC_R_SUCCESS)
				return (result);
			return (DNS_R_WAIT);
		 } else if (result ==  DNS_R_NCACHENXDOMAIN ||
			   result == DNS_R_NCACHENXRRSET ||
			   result == DNS_R_NXDOMAIN ||
			   result == DNS_R_NXRRSET)
		{
			/*
			 * The DS does not exist.
			 */
			if (dns_rdataset_isassociated(&val->frdataset))
				dns_rdataset_disassociate(&val->frdataset);
			if (dns_rdataset_isassociated(&val->fsigrdataset))
				dns_rdataset_disassociate(&val->fsigrdataset);
			validator_log(val, ISC_LOG_DEBUG(2), "no DS record");
			return (DNS_R_NOVALIDSIG);
		}
	}

	/*
	 * We have a DS set.
	 */
	INSIST(val->dsset != NULL);

	if (val->dsset->trust < dns_trust_secure) {
		if (val->mustbesecure) {
			validator_log(val, ISC_LOG_WARNING,
				      "must be secure failure");
			return (DNS_R_MUSTBESECURE);
		}
		markanswer(val);
		return (ISC_R_SUCCESS);
	}

	/*
	 * Look through the DS record and find the keys that can sign the
	 * key set and the matching signature.  For each such key, attempt
	 * verification.
	 */

	supported_algorithm = ISC_FALSE;

	for (result = dns_rdataset_first(val->dsset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(val->dsset))
	{
		dns_rdata_reset(&dsrdata);
		dns_rdataset_current(val->dsset, &dsrdata);
		(void)dns_rdata_tostruct(&dsrdata, &ds, NULL);

		if (ds.digest_type != DNS_DSDIGEST_SHA1)
			continue;
		if (!dns_resolver_algorithm_supported(val->view->resolver,
						      val->event->name,
						      ds.algorithm))
			continue;

		supported_algorithm = ISC_TRUE;

		dns_rdataset_init(&trdataset);
		dns_rdataset_clone(val->event->rdataset, &trdataset);

		for (result = dns_rdataset_first(&trdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&trdataset))
		{
			dns_rdata_reset(&keyrdata);
			dns_rdataset_current(&trdataset, &keyrdata);
			(void)dns_rdata_tostruct(&keyrdata, &key, NULL);
			keytag = compute_keytag(&keyrdata, &key);
			if (ds.key_tag != keytag ||
			    ds.algorithm != key.algorithm)
				continue;
			dns_rdata_reset(&newdsrdata);
			result = dns_ds_buildrdata(val->event->name,
						   &keyrdata, ds.digest_type,
						   dsbuf, &newdsrdata);
			if (result != ISC_R_SUCCESS)
				continue;
			if (dns_rdata_compare(&dsrdata, &newdsrdata) == 0)
				break;
		}
		if (result != ISC_R_SUCCESS) {
			validator_log(val, ISC_LOG_DEBUG(3),
				      "no DNSKEY matching DS");
			continue;
		}
		
		for (result = dns_rdataset_first(val->event->sigrdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(val->event->sigrdataset))
		{
			dns_rdata_reset(&sigrdata);
			dns_rdataset_current(val->event->sigrdataset,
					     &sigrdata);
			(void)dns_rdata_tostruct(&sigrdata, &sig, NULL);
			if (ds.key_tag != sig.keyid &&
			    ds.algorithm != sig.algorithm)
				continue;

			dstkey = NULL;
			result = dns_dnssec_keyfromrdata(val->event->name,
							 &keyrdata,
							 val->view->mctx,
							 &dstkey);
			if (result != ISC_R_SUCCESS)
				/*
				 * This really shouldn't happen, but...
				 */
				continue;

			result = verify(val, dstkey, &sigrdata);
			dst_key_free(&dstkey);
			if (result == ISC_R_SUCCESS)
				break;
		}
		dns_rdataset_disassociate(&trdataset);
		if (result == ISC_R_SUCCESS)
			break;
		validator_log(val, ISC_LOG_DEBUG(3),
			      "no RRSIG matching DS key");
	}
	if (result == ISC_R_SUCCESS) {
		event->rdataset->trust = dns_trust_secure;
		event->sigrdataset->trust = dns_trust_secure;
		validator_log(val, ISC_LOG_DEBUG(3), "marking as secure");
		return (result);
	} else if (result == ISC_R_NOMORE && !supported_algorithm) {
		if (val->mustbesecure) {
			validator_log(val, ISC_LOG_WARNING,
				      "must be secure failure");
			return (DNS_R_MUSTBESECURE);
		}
		validator_log(val, ISC_LOG_DEBUG(3),
			      "no supported algorithm/digest (DS)");
		markanswer(val);
		return (ISC_R_SUCCESS);
	} else
		return (DNS_R_NOVALIDSIG);
}

/*
 * Starts a positive response validation.
 *
 * Returns:
 *	ISC_R_SUCCESS	Validation completed successfully
 *	DNS_R_WAIT	Validation has started but is waiting
 *			for an event.
 *	Other return codes are possible and all indicate failure.
 */
static isc_result_t
start_positive_validation(dns_validator_t *val) {
	/*
	 * If this is not a key, go straight into validate().
	 */
	if (val->event->type != dns_rdatatype_dnskey || !isselfsigned(val))
		return (validate(val, ISC_FALSE));

	return (validatezonekey(val));
}

static isc_result_t
checkwildcard(dns_validator_t *val) {
	dns_name_t *name, *wild;
	dns_message_t *message = val->event->message;
	isc_result_t result;
	isc_boolean_t exists, data;
	char namebuf[DNS_NAME_FORMATSIZE];

	wild = dns_fixedname_name(&val->wild);
	dns_name_format(wild, namebuf, sizeof(namebuf));
	validator_log(val, ISC_LOG_DEBUG(3), "in checkwildcard: %s", namebuf);

	for (result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, DNS_SECTION_AUTHORITY))
	{
		dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;

		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);

		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			if (rdataset->type != dns_rdatatype_nsec)
				continue;
			val->nsecset = rdataset;

			for (sigrdataset = ISC_LIST_HEAD(name->list);
			     sigrdataset != NULL;
			     sigrdataset = ISC_LIST_NEXT(sigrdataset, link))
			{
				if (sigrdataset->type == dns_rdatatype_rrsig &&
				    sigrdataset->covers == rdataset->type)
					break;
			}
			if (sigrdataset == NULL)
				continue;

			if (rdataset->trust != dns_trust_secure)
				continue;

			if (((val->attributes & VALATTR_NEEDNODATA) != 0 ||
			     (val->attributes & VALATTR_NEEDNOWILDCARD) != 0) &&
			    (val->attributes & VALATTR_FOUNDNODATA) == 0 &&
			    (val->attributes & VALATTR_FOUNDNOWILDCARD) == 0 &&
			    nsecnoexistnodata(val, wild, name, rdataset,
					      &exists, &data, NULL)
					       == ISC_R_SUCCESS)
			{
				dns_name_t **proofs = val->event->proofs;
				if (exists && !data)
					val->attributes |= VALATTR_FOUNDNODATA;
				if (exists && !data && NEEDNODATA(val))
					proofs[DNS_VALIDATOR_NODATAPROOF] =
							 name;
				if (!exists)
					val->attributes |=
						 VALATTR_FOUNDNOWILDCARD;
				if (!exists && NEEDNOQNAME(val))
					proofs[DNS_VALIDATOR_NOWILDCARDPROOF] =
							 name;
				return (ISC_R_SUCCESS);
			}
		}
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	return (result);
}

static isc_result_t
nsecvalidate(dns_validator_t *val, isc_boolean_t resume) {
	dns_name_t *name;
	dns_message_t *message = val->event->message;
	isc_result_t result;

	if (!resume)
		result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	else {
		result = ISC_R_SUCCESS;
		validator_log(val, ISC_LOG_DEBUG(3), "resuming nsecvalidate");
	}

	for (;
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, DNS_SECTION_AUTHORITY))
	{
		dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;

		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		if (resume) {
			rdataset = ISC_LIST_NEXT(val->currentset, link);
			val->currentset = NULL;
			resume = ISC_FALSE;
		} else
			rdataset = ISC_LIST_HEAD(name->list);

		for (;
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			if (rdataset->type == dns_rdatatype_rrsig)
				continue;

			if (rdataset->type == dns_rdatatype_soa) {
				val->soaset = rdataset;
				val->soaname = name;
			} else if (rdataset->type == dns_rdatatype_nsec)
				val->nsecset = rdataset;

			for (sigrdataset = ISC_LIST_HEAD(name->list);
			     sigrdataset != NULL;
			     sigrdataset = ISC_LIST_NEXT(sigrdataset,
							 link))
			{
				if (sigrdataset->type == dns_rdatatype_rrsig &&
				    sigrdataset->covers == rdataset->type)
					break;
			}
			if (sigrdataset == NULL)
				continue;
			/*
			 * If a signed zone is missing the zone key, bad
			 * things could happen.  A query for data in the zone
			 * would lead to a query for the zone key, which
			 * would return a negative answer, which would contain
			 * an SOA and an NSEC signed by the missing key, which
			 * would trigger another query for the DNSKEY (since
			 * the first one is still in progress), and go into an
			 * infinite loop.  Avoid that.
			 */
			if (val->event->type == dns_rdatatype_dnskey &&
			    dns_name_equal(name, val->event->name))
			{
				dns_rdata_t nsec = DNS_RDATA_INIT;

				if (rdataset->type != dns_rdatatype_nsec)
					continue;

				result = dns_rdataset_first(rdataset);
				if (result != ISC_R_SUCCESS)
					return (result);
				dns_rdataset_current(rdataset, &nsec);
				if (dns_nsec_typepresent(&nsec,
							dns_rdatatype_soa))
					continue;
			}
			val->currentset = rdataset;
			result = create_validator(val, name, rdataset->type,
						  rdataset, sigrdataset,
						  authvalidated,
						  "nsecvalidate");
			if (result != ISC_R_SUCCESS)
				return (result);
			return (DNS_R_WAIT);

		}
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * Do we only need to check for NOQNAME?
	 */
	if ((val->attributes & VALATTR_NEEDNODATA) == 0 &&
	    (val->attributes & VALATTR_NEEDNOWILDCARD) == 0 &&
	    (val->attributes & VALATTR_NEEDNOQNAME) != 0) {
		if ((val->attributes & VALATTR_FOUNDNOQNAME) != 0) {
			validator_log(val, ISC_LOG_DEBUG(3),
				      "noqname proof found");
			validator_log(val, ISC_LOG_DEBUG(3),
				      "marking as secure");
			val->event->rdataset->trust = dns_trust_secure;
			val->event->sigrdataset->trust = dns_trust_secure;
			return (ISC_R_SUCCESS);
		}
		validator_log(val, ISC_LOG_DEBUG(3),
			      "noqname proof not found");
		return (DNS_R_NOVALIDNSEC);
	}

	/*
	 * Do we need to check for the wildcard?
	 */
	if ((val->attributes & VALATTR_FOUNDNOQNAME) != 0 &&
	    (((val->attributes & VALATTR_NEEDNODATA) != 0 &&
	      (val->attributes & VALATTR_FOUNDNODATA) == 0) ||
	     (val->attributes & VALATTR_NEEDNOWILDCARD) != 0)) {
		result = checkwildcard(val);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	if (((val->attributes & VALATTR_NEEDNODATA) != 0 &&
	     (val->attributes & VALATTR_FOUNDNODATA) != 0) ||
	    ((val->attributes & VALATTR_NEEDNOQNAME) != 0 &&
	     (val->attributes & VALATTR_FOUNDNOQNAME) != 0 &&
	     (val->attributes & VALATTR_NEEDNOWILDCARD) != 0 &&
	     (val->attributes & VALATTR_FOUNDNOWILDCARD) != 0))
		val->attributes |= VALATTR_FOUNDNONEXISTENCE;

	if ((val->attributes & VALATTR_FOUNDNONEXISTENCE) == 0) {
		if (!val->seensig && val->soaset != NULL) {
			result = create_validator(val, val->soaname,
						  dns_rdatatype_soa,
						  val->soaset, NULL,
						  negauthvalidated,
						  "nsecvalidate");
			if (result != ISC_R_SUCCESS)
				return (result);
			return (DNS_R_WAIT);
		}
		validator_log(val, ISC_LOG_DEBUG(3),
			      "nonexistence proof not found");
		return (DNS_R_NOVALIDNSEC);
	} else {
		validator_log(val, ISC_LOG_DEBUG(3),
			      "nonexistence proof found");
		return (ISC_R_SUCCESS);
	}
}

static isc_boolean_t
check_ds(dns_validator_t *val, dns_name_t *name, dns_rdataset_t *rdataset) {
	dns_rdata_t dsrdata = DNS_RDATA_INIT;
	dns_rdata_ds_t ds;
	isc_result_t result;

	for (result = dns_rdataset_first(rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset)) {
		dns_rdataset_current(rdataset, &dsrdata);
		(void)dns_rdata_tostruct(&dsrdata, &ds, NULL);

		if (ds.digest_type == DNS_DSDIGEST_SHA1 &&
		    dns_resolver_algorithm_supported(val->view->resolver,
						     name, ds.algorithm)) {
			dns_rdata_reset(&dsrdata);
			return (ISC_TRUE);
		}
		dns_rdata_reset(&dsrdata);
	}
	return (ISC_FALSE);
}

static void
dlvfetched(isc_task_t *task, isc_event_t *event) {
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_fetchevent_t *devent;
	dns_validator_t *val;
	isc_boolean_t want_destroy;
	isc_result_t eresult;
	isc_result_t result;

	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_FETCHDONE);
	devent = (dns_fetchevent_t *)event;
	val = devent->ev_arg;
	eresult = devent->result;

	/* Free resources which are not of interest. */
	if (devent->node != NULL)
		dns_db_detachnode(devent->db, &devent->node);
	if (devent->db != NULL)
		dns_db_detach(&devent->db);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);
	isc_event_free(&event);
	dns_resolver_destroyfetch(&val->fetch);

	INSIST(val->event != NULL);
	validator_log(val, ISC_LOG_DEBUG(3), "in dlvfetched: %s",
		      dns_result_totext(eresult));

	LOCK(&val->lock);
	if (eresult == ISC_R_SUCCESS) {
		dns_name_format(dns_fixedname_name(&val->dlvsep), namebuf,
				sizeof(namebuf));
		dns_rdataset_clone(&val->frdataset, &val->dlv);
		val->havedlvsep = ISC_TRUE;
		validator_log(val, ISC_LOG_DEBUG(3), "DLV %s found", namebuf);
		result = dlv_validator_start(val);
		if (result != DNS_R_WAIT)
			validator_done(val, result);
	} else if (eresult == DNS_R_NXRRSET ||
		   eresult == DNS_R_NXDOMAIN ||
		   eresult == DNS_R_NCACHENXRRSET ||
		   eresult == DNS_R_NCACHENXDOMAIN) {
		   result = finddlvsep(val, ISC_TRUE);
		if (result == ISC_R_SUCCESS) {
			dns_name_format(dns_fixedname_name(&val->dlvsep),
					namebuf, sizeof(namebuf));
			validator_log(val, ISC_LOG_DEBUG(3), "DLV %s found",
				      namebuf);
			result = dlv_validator_start(val);
			if (result != DNS_R_WAIT)
				validator_done(val, result);
		} else if (result == ISC_R_NOTFOUND) {
			validator_log(val, ISC_LOG_DEBUG(3), "DLV not found");
			markanswer(val);
			validator_done(val, ISC_R_SUCCESS);
		} else {
			validator_log(val, ISC_LOG_DEBUG(3), "DLV lookup: %s",
				      dns_result_totext(result));
			if (result != DNS_R_WAIT)
				validator_done(val, result);
		}
	} else {
		validator_log(val, ISC_LOG_DEBUG(3), "DLV lookup: %s",
			      dns_result_totext(eresult));
	}
	want_destroy = exit_check(val);
	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

static isc_result_t
startfinddlvsep(dns_validator_t *val, dns_name_t *unsecure) {
	char namebuf[DNS_NAME_FORMATSIZE];
	isc_result_t result;

	INSIST(!DLVTRIED(val));

	val->attributes |= VALATTR_DLVTRIED;

	dns_name_format(unsecure, namebuf, sizeof(namebuf));
	validator_log(val, ISC_LOG_DEBUG(3),
		      "plain DNSSEC returns unsecure (%s): looking for DLV",
		      namebuf);

	if (dns_name_issubdomain(val->event->name, val->view->dlv)) {
		validator_log(val, ISC_LOG_WARNING, "must be secure failure");
		return (DNS_R_MUSTBESECURE);
	}

	val->dlvlabels = dns_name_countlabels(unsecure) - 1;
	result = finddlvsep(val, ISC_FALSE);
	if (result == ISC_R_NOTFOUND) {
		validator_log(val, ISC_LOG_DEBUG(3), "DLV not found");
		markanswer(val);
		return (ISC_R_SUCCESS);
	}
	if (result != ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(3), "DLV lookup: %s",
			      dns_result_totext(result));
		return (result);
	}
	dns_name_format(dns_fixedname_name(&val->dlvsep), namebuf,
			sizeof(namebuf));
	validator_log(val, ISC_LOG_DEBUG(3), "DLV %s found", namebuf);
	return (dlv_validator_start(val));
}

static isc_result_t
finddlvsep(dns_validator_t *val, isc_boolean_t resume) {
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_fixedname_t dlvfixed;
	dns_name_t *dlvname;
	dns_name_t *dlvsep;
	dns_name_t noroot;
	isc_result_t result;
	unsigned int labels;
		
	INSIST(val->view->dlv != NULL);

	if (!resume) {

		if (dns_name_issubdomain(val->event->name, val->view->dlv)) {
			validator_log(val, ISC_LOG_WARNING,
				      "must be secure failure");
			return (DNS_R_MUSTBESECURE);
		}

		dns_fixedname_init(&val->dlvsep);
		dlvsep = dns_fixedname_name(&val->dlvsep);
		dns_name_copy(val->event->name, dlvsep, NULL);
		if (val->event->type == dns_rdatatype_ds) {
			labels = dns_name_countlabels(dlvsep);
			if (labels == 0)
				return (ISC_R_NOTFOUND);
			dns_name_getlabelsequence(dlvsep, 1, labels - 1,
						  dlvsep);
		}
	} else {
		dlvsep = dns_fixedname_name(&val->dlvsep);
		labels = dns_name_countlabels(dlvsep);
		dns_name_getlabelsequence(dlvsep, 1, labels - 1, dlvsep);
	}
	dns_name_init(&noroot, NULL);
	dns_fixedname_init(&dlvfixed);
	dlvname = dns_fixedname_name(&dlvfixed);
	labels = dns_name_countlabels(dlvsep);
	if (labels == 0)
		return (ISC_R_NOTFOUND);
	dns_name_getlabelsequence(dlvsep, 0, labels - 1, &noroot);
	result = dns_name_concatenate(&noroot, val->view->dlv, dlvname, NULL);
	while (result == ISC_R_NOSPACE) {
		labels = dns_name_countlabels(dlvsep);
		dns_name_getlabelsequence(dlvsep, 1, labels - 1, dlvsep);
		dns_name_getlabelsequence(dlvsep, 0, labels - 2, &noroot);
		result = dns_name_concatenate(&noroot, val->view->dlv,
					      dlvname, NULL);
	}
	if (result != ISC_R_SUCCESS) {
		validator_log(val, ISC_LOG_DEBUG(2), "DLV concatenate failed");
		return (DNS_R_NOVALIDSIG);
	}

	while (dns_name_countlabels(dlvname) >=
	       dns_name_countlabels(val->view->dlv) + val->dlvlabels) {
		dns_name_format(dlvname, namebuf, sizeof(namebuf));
		validator_log(val, ISC_LOG_DEBUG(3), "looking for DLV %s",
			      namebuf);
		result = view_find(val, dlvname, dns_rdatatype_dlv);
		if (result == ISC_R_SUCCESS) {
			if (val->frdataset.trust < dns_trust_secure)
				return (DNS_R_NOVALIDSIG);
			val->havedlvsep = ISC_TRUE;
			dns_rdataset_clone(&val->frdataset, &val->dlv);
			return (ISC_R_SUCCESS);
		}
		if (result == ISC_R_NOTFOUND) {
			result = create_fetch(val, dlvname, dns_rdatatype_dlv,
					      dlvfetched, "finddlvsep");
			if (result != ISC_R_SUCCESS)
				return (result);
			return (DNS_R_WAIT);
		}
		if (result != DNS_R_NXRRSET &&
		    result != DNS_R_NXDOMAIN &&
		    result != DNS_R_NCACHENXRRSET &&
		    result != DNS_R_NCACHENXDOMAIN)
			return (result);
		/*
		 * Strip first labels from both dlvsep and dlvname.
		 */
		labels = dns_name_countlabels(dlvsep);
		if (labels == 0)
			break;
		dns_name_getlabelsequence(dlvsep, 1, labels - 1, dlvsep);
		labels = dns_name_countlabels(dlvname);
		dns_name_getlabelsequence(dlvname, 1, labels - 1, dlvname);
	}
	return (ISC_R_NOTFOUND);
}

/*
 * proveunsecure walks down from the SEP looking for a break in the
 * chain of trust.   That occurs when we can prove the DS record does
 * not exist at a delegation point or the DS exists at a delegation
 * but we don't support the algorithm/digest.
 */
static isc_result_t
proveunsecure(dns_validator_t *val, isc_boolean_t resume) {
	isc_result_t result;
	dns_fixedname_t fixedsecroot;
	dns_name_t *secroot;
	dns_name_t *tname;
	char namebuf[DNS_NAME_FORMATSIZE];

	dns_fixedname_init(&fixedsecroot);
	secroot = dns_fixedname_name(&fixedsecroot);
	if (val->havedlvsep)
		dns_name_copy(dns_fixedname_name(&val->dlvsep), secroot, NULL);
	else {
		result = dns_keytable_finddeepestmatch(val->keytable,
						       val->event->name,
						       secroot);

		if (result == ISC_R_NOTFOUND) {
			validator_log(val, ISC_LOG_DEBUG(3),
				      "not beneath secure root");
			if (val->mustbesecure) {
				validator_log(val, ISC_LOG_WARNING,
					      "must be secure failure");
				result = DNS_R_MUSTBESECURE;
				goto out;
			}
			if (val->view->dlv == NULL || DLVTRIED(val)) {
				markanswer(val);
				return (ISC_R_SUCCESS);
			}
			return (startfinddlvsep(val, dns_rootname));
		} else if (result != ISC_R_SUCCESS)
			return (result);
	}

	if (!resume) {
		/*
		 * We are looking for breaks below the SEP so add a label.
		 */
		val->labels = dns_name_countlabels(secroot) + 1;
	} else {
		validator_log(val, ISC_LOG_DEBUG(3), "resuming proveunsecure");
		if (val->frdataset.trust >= dns_trust_secure &&
		    !check_ds(val, dns_fixedname_name(&val->fname),
			      &val->frdataset)) {
			dns_name_format(dns_fixedname_name(&val->fname),
					namebuf, sizeof(namebuf));
			if (val->mustbesecure) {
				validator_log(val, ISC_LOG_WARNING,
					      "must be secure failure at '%s'",
					      namebuf);
				result = DNS_R_MUSTBESECURE;
				goto out;
			}
			validator_log(val, ISC_LOG_DEBUG(3),
				      "no supported algorithm/digest (%s/DS)",
				      namebuf);
			if (val->view->dlv == NULL || DLVTRIED(val)) {
				markanswer(val);
				result = ISC_R_SUCCESS;
				goto out;
			}
			result = startfinddlvsep(val,
					      dns_fixedname_name(&val->fname));
			goto out;
		}
		val->labels++;
	}

	for (;
	     val->labels <= dns_name_countlabels(val->event->name);
	     val->labels++)
	{

		dns_fixedname_init(&val->fname);
		tname = dns_fixedname_name(&val->fname);
		if (val->labels == dns_name_countlabels(val->event->name))
			dns_name_copy(val->event->name, tname, NULL);
		else
			dns_name_split(val->event->name, val->labels,
				       NULL, tname);

		dns_name_format(tname, namebuf, sizeof(namebuf));
		validator_log(val, ISC_LOG_DEBUG(3),
			      "checking existence of DS at '%s'",
			      namebuf);

		result = view_find(val, tname, dns_rdatatype_ds);
		if (result == DNS_R_NXRRSET || result == DNS_R_NCACHENXRRSET) {
			/*
			 * There is no DS.  If this is a delegation,
			 * we maybe done.
			 */
			if (val->frdataset.trust < dns_trust_secure) {
				/*
				 * This shouldn't happen, since the negative
				 * response should have been validated.  Since
				 * there's no way of validating existing
				 * negative response blobs, give up.
				 */
				result = DNS_R_NOVALIDSIG;
				goto out;
			}
			if (isdelegation(tname, &val->frdataset, result)) {
				if (val->mustbesecure) {
					validator_log(val, ISC_LOG_WARNING,
						      "must be secure failure");
					return (DNS_R_MUSTBESECURE);
				}
				if (val->view->dlv == NULL || DLVTRIED(val)) {
					markanswer(val);
					return (ISC_R_SUCCESS);
				}
				return (startfinddlvsep(val, tname));
			}
			continue;
		} else if (result == ISC_R_SUCCESS) {
			/*
			 * There is a DS here.  Verify that it's secure and
			 * continue.
			 */
			if (val->frdataset.trust >= dns_trust_secure) {
				if (!check_ds(val, tname, &val->frdataset)) {
					validator_log(val, ISC_LOG_DEBUG(3),
						     "no supported algorithm/"
						     "digest (%s/DS)", namebuf);
					if (val->mustbesecure) {
						validator_log(val,
							      ISC_LOG_WARNING,
						      "must be secure failure");
						result = DNS_R_MUSTBESECURE;
						goto out;
					}
					if (val->view->dlv == NULL ||
					    DLVTRIED(val)) {
						markanswer(val);
						result = ISC_R_SUCCESS;
						goto out;
					}
					result = startfinddlvsep(val, tname);
					goto out;
				}
				continue;
			}
			else if (!dns_rdataset_isassociated(&val->fsigrdataset))
			{
				result = DNS_R_NOVALIDSIG;
				goto out;
			}
			result = create_validator(val, tname, dns_rdatatype_ds,
						  &val->frdataset,
						  &val->fsigrdataset,
						  dsvalidated,
						  "proveunsecure");
			if (result != ISC_R_SUCCESS)
				goto out;
			return (DNS_R_WAIT);
		} else if (result == DNS_R_NXDOMAIN ||
			   result == DNS_R_NCACHENXDOMAIN)
		{
			/*
			 * This is not a zone cut.  Assuming things are
			 * as expected, continue.
			 */
			if (!dns_rdataset_isassociated(&val->frdataset)) {
				/*
				 * There should be an NSEC here, since we
				 * are still in a secure zone.
				 */
				result = DNS_R_NOVALIDNSEC;
				goto out;
			} else if (val->frdataset.trust < dns_trust_secure) {
				/*
				 * This shouldn't happen, since the negative
				 * response should have been validated.  Since
				 * there's no way of validating existing
				 * negative response blobs, give up.
				 */
				result = DNS_R_NOVALIDSIG;
				goto out;
			}
			continue;
		} else if (result == ISC_R_NOTFOUND) {
			/*
			 * We don't know anything about the DS.  Find it.
			 */
			result = create_fetch(val, tname, dns_rdatatype_ds,
					      dsfetched2, "proveunsecure");
			if (result != ISC_R_SUCCESS)
				goto out;
			return (DNS_R_WAIT);
		}
	}
	validator_log(val, ISC_LOG_DEBUG(3), "insecurity proof failed");
	return (DNS_R_NOTINSECURE); /* Couldn't complete insecurity proof */

 out:
	if (dns_rdataset_isassociated(&val->frdataset))
		dns_rdataset_disassociate(&val->frdataset);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);
	return (result);
}

static isc_result_t
dlv_validator_start(dns_validator_t *val) {
	isc_event_t *event;

	validator_log(val, ISC_LOG_DEBUG(3), "dlv_validator_start");

	/*
	 * Reset state and try again.
	 */
	val->attributes &= VALATTR_DLVTRIED;
	val->options &= ~DNS_VALIDATOR_DLV;

	event = (isc_event_t *)val->event;
	isc_task_send(val->task, &event);
	return (DNS_R_WAIT);
}

static void
validator_start(isc_task_t *task, isc_event_t *event) {
	dns_validator_t *val;
	dns_validatorevent_t *vevent;
	isc_boolean_t want_destroy = ISC_FALSE;
	isc_result_t result = ISC_R_FAILURE;

	UNUSED(task);
	REQUIRE(event->ev_type == DNS_EVENT_VALIDATORSTART);
	vevent = (dns_validatorevent_t *)event;
	val = vevent->validator;

	/* If the validator has been cancelled, val->event == NULL */
	if (val->event == NULL)
		return;

	if (DLVTRIED(val))
		validator_log(val, ISC_LOG_DEBUG(3), "restarting using DLV");
	else
		validator_log(val, ISC_LOG_DEBUG(3), "starting");

	LOCK(&val->lock);

	if ((val->options & DNS_VALIDATOR_DLV) != 0) {
		validator_log(val, ISC_LOG_DEBUG(3), "looking for DLV");
		result = startfinddlvsep(val, dns_rootname);
	} else if (val->event->rdataset != NULL &&
		   val->event->sigrdataset != NULL) {
		isc_result_t saved_result;

		/*
		 * This looks like a simple validation.  We say "looks like"
		 * because it might end up requiring an insecurity proof.
		 */
		validator_log(val, ISC_LOG_DEBUG(3),
			      "attempting positive response validation");

		INSIST(dns_rdataset_isassociated(val->event->rdataset));
		INSIST(dns_rdataset_isassociated(val->event->sigrdataset));
		result = start_positive_validation(val);
		if (result == DNS_R_NOVALIDSIG &&
		    (val->attributes & VALATTR_TRIEDVERIFY) == 0)
		{
			saved_result = result;
			validator_log(val, ISC_LOG_DEBUG(3),
				      "falling back to insecurity proof");
			val->attributes |= VALATTR_INSECURITY;
			result = proveunsecure(val, ISC_FALSE);
			if (result == DNS_R_NOTINSECURE)
				result = saved_result;
		}
	} else if (val->event->rdataset != NULL) {
		/*
		 * This is either an unsecure subdomain or a response from
		 * a broken server.
		 */
		INSIST(dns_rdataset_isassociated(val->event->rdataset));
		validator_log(val, ISC_LOG_DEBUG(3),
			      "attempting insecurity proof");

		val->attributes |= VALATTR_INSECURITY;
		result = proveunsecure(val, ISC_FALSE);
	} else if (val->event->rdataset == NULL &&
		   val->event->sigrdataset == NULL)
	{
		/*
		 * This is a nonexistence validation.
		 */
		validator_log(val, ISC_LOG_DEBUG(3),
			      "attempting negative response validation");

		val->attributes |= VALATTR_NEGATIVE;
		if (val->event->message->rcode == dns_rcode_nxdomain) {
			val->attributes |= VALATTR_NEEDNOQNAME;
			val->attributes |= VALATTR_NEEDNOWILDCARD;
		} else
			val->attributes |= VALATTR_NEEDNODATA;
		result = nsecvalidate(val, ISC_FALSE);
	} else {
		/*
		 * This shouldn't happen.
		 */
		INSIST(0);
	}

	if (result != DNS_R_WAIT) {
		want_destroy = exit_check(val);
		validator_done(val, result);
	}

	UNLOCK(&val->lock);
	if (want_destroy)
		destroy(val);
}

isc_result_t
dns_validator_create(dns_view_t *view, dns_name_t *name, dns_rdatatype_t type,
		     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		     dns_message_t *message, unsigned int options,
		     isc_task_t *task, isc_taskaction_t action, void *arg,
		     dns_validator_t **validatorp)
{
	isc_result_t result;
	dns_validator_t *val;
	isc_task_t *tclone;
	dns_validatorevent_t *event;

	REQUIRE(name != NULL);
	REQUIRE(type != 0);
	REQUIRE(rdataset != NULL ||
		(rdataset == NULL && sigrdataset == NULL && message != NULL));
	REQUIRE(validatorp != NULL && *validatorp == NULL);

	tclone = NULL;
	result = ISC_R_FAILURE;

	val = isc_mem_get(view->mctx, sizeof(*val));
	if (val == NULL)
		return (ISC_R_NOMEMORY);
	val->view = NULL;
	dns_view_weakattach(view, &val->view);
	event = (dns_validatorevent_t *)
		isc_event_allocate(view->mctx, task,
				   DNS_EVENT_VALIDATORSTART,
				   validator_start, NULL,
				   sizeof(dns_validatorevent_t));
	if (event == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_val;
	}
	isc_task_attach(task, &tclone);
	event->validator = val;
	event->result = ISC_R_FAILURE;
	event->name = name;
	event->type = type;
	event->rdataset = rdataset;
	event->sigrdataset = sigrdataset;
	event->message = message;
	memset(event->proofs, 0, sizeof(event->proofs));
	result = isc_mutex_init(&val->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_event;
	val->event = event;
	val->options = options;
	val->attributes = 0;
	val->fetch = NULL;
	val->subvalidator = NULL;
	val->parent = NULL;
	val->keytable = NULL;
	dns_keytable_attach(val->view->secroots, &val->keytable);
	val->keynode = NULL;
	val->key = NULL;
	val->siginfo = NULL;
	val->task = task;
	val->action = action;
	val->arg = arg;
	val->labels = 0;
	val->currentset = NULL;
	val->keyset = NULL;
	val->dsset = NULL;
	dns_rdataset_init(&val->dlv);
	val->soaset = NULL;
	val->nsecset = NULL;
	val->soaname = NULL;
	val->seensig = ISC_FALSE;
	val->havedlvsep = ISC_FALSE;
	val->depth = 0;
	val->mustbesecure = dns_resolver_getmustbesecure(view->resolver, name);
	dns_rdataset_init(&val->frdataset);
	dns_rdataset_init(&val->fsigrdataset);
	dns_fixedname_init(&val->wild);
	ISC_LINK_INIT(val, link);
	val->magic = VALIDATOR_MAGIC;

	isc_task_send(task, ISC_EVENT_PTR(&event));

	*validatorp = val;

	return (ISC_R_SUCCESS);

 cleanup_event:
	isc_task_detach(&tclone);
	isc_event_free((isc_event_t **)&val->event);

 cleanup_val:
	dns_view_weakdetach(&val->view);
	isc_mem_put(view->mctx, val, sizeof(*val));

	return (result);
}

void
dns_validator_cancel(dns_validator_t *validator) {
	REQUIRE(VALID_VALIDATOR(validator));

	LOCK(&validator->lock);

	validator_log(validator, ISC_LOG_DEBUG(3), "dns_validator_cancel");

	if (validator->event != NULL) {
		if (validator->fetch != NULL)
			dns_resolver_cancelfetch(validator->fetch);

		if (validator->subvalidator != NULL)
			dns_validator_cancel(validator->subvalidator);
	}
	UNLOCK(&validator->lock);
}

static void
destroy(dns_validator_t *val) {
	isc_mem_t *mctx;

	REQUIRE(SHUTDOWN(val));
	REQUIRE(val->event == NULL);
	REQUIRE(val->fetch == NULL);

	if (val->keynode != NULL)
		dns_keytable_detachkeynode(val->keytable, &val->keynode);
	else if (val->key != NULL)
		dst_key_free(&val->key);
	if (val->keytable != NULL)
		dns_keytable_detach(&val->keytable);
	if (val->subvalidator != NULL)
		dns_validator_destroy(&val->subvalidator);
	if (val->havedlvsep)
		dns_rdataset_disassociate(&val->dlv);
	if (dns_rdataset_isassociated(&val->frdataset))
		dns_rdataset_disassociate(&val->frdataset);
	if (dns_rdataset_isassociated(&val->fsigrdataset))
		dns_rdataset_disassociate(&val->fsigrdataset);
	mctx = val->view->mctx;
	if (val->siginfo != NULL)
		isc_mem_put(mctx, val->siginfo, sizeof(*val->siginfo));
	DESTROYLOCK(&val->lock);
	dns_view_weakdetach(&val->view);
	val->magic = 0;
	isc_mem_put(mctx, val, sizeof(*val));
}

void
dns_validator_destroy(dns_validator_t **validatorp) {
	dns_validator_t *val;
	isc_boolean_t want_destroy = ISC_FALSE;

	REQUIRE(validatorp != NULL);
	val = *validatorp;
	REQUIRE(VALID_VALIDATOR(val));

	LOCK(&val->lock);

	val->attributes |= VALATTR_SHUTDOWN;
	validator_log(val, ISC_LOG_DEBUG(3), "dns_validator_destroy");

	want_destroy = exit_check(val);

	UNLOCK(&val->lock);

	if (want_destroy)
		destroy(val);

	*validatorp = NULL;
}

static void
validator_logv(dns_validator_t *val, isc_logcategory_t *category,
	       isc_logmodule_t *module, int level, const char *fmt, va_list ap)
{
	char msgbuf[2048];
	static const char spaces[] = "        *";
	int depth = val->depth * 2;

	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);

	if ((unsigned int) depth >= sizeof spaces)
		depth = sizeof spaces - 1;

	if (val->event != NULL && val->event->name != NULL) {
		char namebuf[DNS_NAME_FORMATSIZE];
		char typebuf[DNS_RDATATYPE_FORMATSIZE];

		dns_name_format(val->event->name, namebuf, sizeof(namebuf));
		dns_rdatatype_format(val->event->type, typebuf,
				     sizeof(typebuf));
		isc_log_write(dns_lctx, category, module, level,
			      "%.*svalidating @%p: %s %s: %s", depth, spaces,
			      val, namebuf, typebuf, msgbuf);
	} else {
		isc_log_write(dns_lctx, category, module, level,
			      "%.*svalidator @%p: %s", depth, spaces,
			       val, msgbuf);
	}
}

static void
validator_log(dns_validator_t *val, int level, const char *fmt, ...) {
	va_list ap;

	if (! isc_log_wouldlog(dns_lctx, level))
		return;

	va_start(ap, fmt);

	validator_logv(val, DNS_LOGCATEGORY_DNSSEC,
		       DNS_LOGMODULE_VALIDATOR, level, fmt, ap);
	va_end(ap);
}

static void
validator_logcreate(dns_validator_t *val,
		    dns_name_t *name, dns_rdatatype_t type,
		    const char *caller, const char *operation)
{
	char namestr[DNS_NAME_FORMATSIZE];
	char typestr[DNS_RDATATYPE_FORMATSIZE];

	dns_name_format(name, namestr, sizeof(namestr));
	dns_rdatatype_format(type, typestr, sizeof(typestr));
	validator_log(val, ISC_LOG_DEBUG(9), "%s: creating %s for %s %s",
		      caller, operation, namestr, typestr);
}
