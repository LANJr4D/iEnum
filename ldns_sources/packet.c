/*
 * packet.c
 *
 * dns packet implementation
 *
 * a Net::DNS like library for C
 *
 * (c) NLnet Labs, 2004-2006
 *
 * See the file LICENSE for the license
 */

#include "ldns/config.h"

#include "ldns.h"

#include <strings.h>
#include <limits.h>

#ifdef HAVE_SSL
#include <openssl/rand.h>
#endif

/* Access functions 
 * do this as functions to get type checking
 */

#define LDNS_EDNS_MASK_DO_BIT 0x8000

/* TODO defines for 3600 */
/* convert to and from numerical flag values */
ldns_lookup_table ldns_edns_flags[] = {
	{ 3600, "do"},
	{ 0, NULL}
};

/* read */
uint16_t
ldns_pkt_id(const ldns_pkt *packet)
{
	return packet->_header->_id;
}

bool
ldns_pkt_qr(const ldns_pkt *packet)
{
	return packet->_header->_qr;
}

bool
ldns_pkt_aa(const ldns_pkt *packet)
{
	return packet->_header->_aa;
}

bool
ldns_pkt_tc(const ldns_pkt *packet)
{
	return packet->_header->_tc;
}

bool
ldns_pkt_rd(const ldns_pkt *packet)
{
	return packet->_header->_rd;
}

bool
ldns_pkt_cd(const ldns_pkt *packet)
{
	return packet->_header->_cd;
}

bool
ldns_pkt_ra(const ldns_pkt *packet)
{
	return packet->_header->_ra;
}

bool
ldns_pkt_ad(const ldns_pkt *packet)
{
	return packet->_header->_ad;
}

ldns_pkt_opcode
ldns_pkt_get_opcode(const ldns_pkt *packet)
{
	return packet->_header->_opcode;
}

ldns_pkt_rcode
ldns_pkt_get_rcode(const ldns_pkt *packet)
{
	return packet->_header->_rcode;
}

uint16_t
ldns_pkt_qdcount(const ldns_pkt *packet)
{
	return packet->_header->_qdcount;
}

uint16_t
ldns_pkt_ancount(const ldns_pkt *packet)
{
	return packet->_header->_ancount;
}

uint16_t
ldns_pkt_nscount(const ldns_pkt *packet)
{
	return packet->_header->_nscount;
}

uint16_t
ldns_pkt_arcount(const ldns_pkt *packet)
{
	return packet->_header->_arcount;
}

ldns_rr_list *
ldns_pkt_question(const ldns_pkt *packet)
{
	return packet->_question;
}

ldns_rr_list *
ldns_pkt_answer(const ldns_pkt *packet)
{
	return packet->_answer;
}

ldns_rr_list *
ldns_pkt_authority(const ldns_pkt *packet)
{
	return packet->_authority;
}

ldns_rr_list *
ldns_pkt_additional(const ldns_pkt *packet)
{
	return packet->_additional;
}

/* return ALL section concatenated */
ldns_rr_list *
ldns_pkt_all(const ldns_pkt *packet)
{
	ldns_rr_list *all, *prev_all;

	all = ldns_rr_list_cat_clone(
			ldns_pkt_question(packet),
			ldns_pkt_answer(packet));
	prev_all = all;
	all = ldns_rr_list_cat_clone(all,
			ldns_pkt_authority(packet));
	ldns_rr_list_deep_free(prev_all);
	prev_all = all;
	all = ldns_rr_list_cat_clone(all,
			ldns_pkt_additional(packet));
	ldns_rr_list_deep_free(prev_all);
	return all;
}

ldns_rr_list *
ldns_pkt_all_noquestion(const ldns_pkt *packet)
{
	ldns_rr_list *all, *all2;

	all = ldns_rr_list_cat_clone(
			ldns_pkt_answer(packet),
			ldns_pkt_authority(packet));
	all2 = ldns_rr_list_cat_clone(all,
			ldns_pkt_additional(packet));
	
	ldns_rr_list_deep_free(all);
	return all2;
}

size_t
ldns_pkt_size(const ldns_pkt *packet)
{
	return packet->_size;
}

uint32_t 
ldns_pkt_querytime(const ldns_pkt *packet)
{
	return packet->_querytime;
}

ldns_rdf *
ldns_pkt_answerfrom(const ldns_pkt *packet)
{
	return packet->_answerfrom;
}

struct timeval
ldns_pkt_timestamp(const ldns_pkt *packet)
{
	return packet->timestamp;
}

uint16_t
ldns_pkt_edns_udp_size(const ldns_pkt *packet)
{
	return packet->_edns_udp_size;
}

uint8_t
ldns_pkt_edns_extended_rcode(const ldns_pkt *packet)
{
	return packet->_edns_extended_rcode;
}

uint8_t
ldns_pkt_edns_version(const ldns_pkt *packet)
{
	return packet->_edns_version;
}

uint16_t
ldns_pkt_edns_z(const ldns_pkt *packet)
{
	return packet->_edns_z;
}

bool
ldns_pkt_edns_do(const ldns_pkt *packet)
{
	return (packet->_edns_z & LDNS_EDNS_MASK_DO_BIT);
}

void
ldns_pkt_set_edns_do(ldns_pkt *packet, bool value)
{
	if (value) {
		packet->_edns_z = packet->_edns_z | LDNS_EDNS_MASK_DO_BIT;
	} else {
		packet->_edns_z = packet->_edns_z & !LDNS_EDNS_MASK_DO_BIT;
	}
}

ldns_rdf *
ldns_pkt_edns_data(const ldns_pkt *packet)
{
	return packet->_edns_data;
}

/* return only those rr that share the ownername */
ldns_rr_list *
ldns_pkt_rr_list_by_name(ldns_pkt *packet, ldns_rdf *ownername, ldns_pkt_section sec)
{
	ldns_rr_list *rrs;
	ldns_rr_list *new;
	ldns_rr_list *ret;
	uint16_t i;

	if (!packet) {
		return NULL;
	}

	rrs = ldns_pkt_get_section_clone(packet, sec);
	new = ldns_rr_list_new();
	ret = NULL;

	for(i = 0; i < ldns_rr_list_rr_count(rrs); i++) {
		if (ldns_rdf_compare(ldns_rr_owner(
						ldns_rr_list_rr(rrs, i)), 
					ownername) == 0) {
			/* owner names match */
			ldns_rr_list_push_rr(new, ldns_rr_list_rr(rrs, i));
			ret = new;
		}
	}
	return ret;
}

/* return only those rr that share a type */
ldns_rr_list *
ldns_pkt_rr_list_by_type(const ldns_pkt *packet, ldns_rr_type type, ldns_pkt_section sec)
{
	ldns_rr_list *rrs;
	ldns_rr_list *new;
	uint16_t i;

	if(!packet) {
		return NULL;
	}
	
	rrs = ldns_pkt_get_section_clone(packet, sec);
	new = ldns_rr_list_new();
	
	for(i = 0; i < ldns_rr_list_rr_count(rrs); i++) {
		if (type == ldns_rr_get_type(ldns_rr_list_rr(rrs, i))) {
			/* types match */
			ldns_rr_list_push_rr(new, 
			                     ldns_rr_clone(
			                     	ldns_rr_list_rr(rrs, i))
					     );
		}
	}
	ldns_rr_list_deep_free(rrs);

	if (ldns_rr_list_rr_count(new) == 0) {
		ldns_rr_list_free(new);
		return NULL;
	} else {
		return new;
	}
}

/* return only those rrs that share name and type */
ldns_rr_list *
ldns_pkt_rr_list_by_name_and_type(const ldns_pkt *packet, const ldns_rdf *ownername, ldns_rr_type type, ldns_pkt_section sec)
{
	ldns_rr_list *rrs;
	ldns_rr_list *new;
	ldns_rr_list *ret;
	uint16_t i;

	if(!packet) {
		return NULL;
	}
	
	rrs = ldns_pkt_get_section_clone(packet, sec);
	new = ldns_rr_list_new();
	ret = NULL;

	for(i = 0; i < ldns_rr_list_rr_count(rrs); i++) {
		if (type == ldns_rr_get_type(ldns_rr_list_rr(rrs, i)) &&
		    ldns_rdf_compare(ldns_rr_owner(ldns_rr_list_rr(rrs, i)),
		                     ownername
		                    ) == 0
		   ) {
			/* types match */
			ldns_rr_list_push_rr(new, ldns_rr_clone(ldns_rr_list_rr(rrs, i)));
			ret = new;
		}
	}
	ldns_rr_list_deep_free(rrs);
	if (!ret) {
		ldns_rr_list_free(new);
	}
	return ret;
}

bool
ldns_pkt_rr(ldns_pkt *pkt, ldns_pkt_section sec, ldns_rr *rr)
{
	ldns_rr_list *rrs;
	uint16_t rr_count;
	uint16_t i;

	rrs = ldns_pkt_get_section_clone(pkt, sec);
	if (!rrs) {
		return false;
	}
	rr_count = ldns_rr_list_rr_count(rrs);
	
	/* walk the rrs and compare them with rr */	
	for(i = 0; i < rr_count; i++) {
		if (ldns_rr_compare(ldns_rr_list_rr(rrs, i), rr) == 0) {
			/* a match */
			return true;
		}
	}
	return false;
}

uint16_t
ldns_pkt_section_count(const ldns_pkt *packet, ldns_pkt_section s)
{
	switch(s) {
	case LDNS_SECTION_QUESTION:
		return ldns_pkt_qdcount(packet);
	case LDNS_SECTION_ANSWER:
		return ldns_pkt_ancount(packet);
	case LDNS_SECTION_AUTHORITY:
		return ldns_pkt_nscount(packet);
	case LDNS_SECTION_ADDITIONAL:
		return ldns_pkt_arcount(packet);
	case LDNS_SECTION_ANY:
		return ldns_pkt_qdcount(packet) +
			ldns_pkt_ancount(packet) +
			ldns_pkt_nscount(packet) +
			ldns_pkt_arcount(packet);
	case LDNS_SECTION_ANY_NOQUESTION:
		return ldns_pkt_ancount(packet) +
			ldns_pkt_nscount(packet) +
			ldns_pkt_arcount(packet);
	default:
		return 0;
	}
}

bool
ldns_pkt_empty(ldns_pkt *p)
{
	if (!p) {
		return true; /* NULL is empty? */
	}
	if (ldns_pkt_section_count(p, LDNS_SECTION_ANY) > 0) {
		return true;
	} else
		return false;
}


ldns_rr_list *
ldns_pkt_get_section_clone(const ldns_pkt *packet, ldns_pkt_section s)
{
	switch(s) {
	case LDNS_SECTION_QUESTION:
		return ldns_rr_list_clone(ldns_pkt_question(packet));
	case LDNS_SECTION_ANSWER:
		return ldns_rr_list_clone(ldns_pkt_answer(packet));
	case LDNS_SECTION_AUTHORITY:
		return ldns_rr_list_clone(ldns_pkt_authority(packet));
	case LDNS_SECTION_ADDITIONAL:
		return ldns_rr_list_clone(ldns_pkt_additional(packet));
	case LDNS_SECTION_ANY:
		/* these are already clones */
		return ldns_pkt_all(packet);
	case LDNS_SECTION_ANY_NOQUESTION:
		return ldns_pkt_all_noquestion(packet);
	default:
		return NULL;
	}
}

ldns_rr *ldns_pkt_tsig(const ldns_pkt *pkt) {
	return pkt->_tsig_rr;
}

/* write */
void
ldns_pkt_set_id(ldns_pkt *packet, uint16_t id)
{
	packet->_header->_id = id;
}

void
ldns_pkt_set_random_id(ldns_pkt *packet)
{
	uint16_t rid = 0;
#ifdef HAVE_SSL
	unsigned char *rb;
	rb = LDNS_XMALLOC(unsigned char, 2);
	if (RAND_bytes(rb, 2) == 1) {
		rid = ldns_read_uint16(rb);
	}
	LDNS_FREE(rb);
#endif
	if (rid == 0) {
		rid = (uint16_t) random();
	}

	ldns_pkt_set_id(packet, rid);
}


void
ldns_pkt_set_qr(ldns_pkt *packet, bool qr)
{
	packet->_header->_qr = qr;
}

void
ldns_pkt_set_aa(ldns_pkt *packet, bool aa)
{
	packet->_header->_aa = aa;
}

void
ldns_pkt_set_tc(ldns_pkt *packet, bool tc)
{
	packet->_header->_tc = tc;
}

void
ldns_pkt_set_rd(ldns_pkt *packet, bool rd)
{
	packet->_header->_rd = rd;
}

void
ldns_pkt_set_additional(ldns_pkt *p, ldns_rr_list *rr)
{
	p->_additional = rr;
}

void
ldns_pkt_set_question(ldns_pkt *p, ldns_rr_list *rr)
{
	p->_question = rr;
}

void
ldns_pkt_set_answer(ldns_pkt *p, ldns_rr_list *rr)
{
	p->_answer = rr;
}

void
ldns_pkt_set_authority(ldns_pkt *p, ldns_rr_list *rr)
{
	p->_authority = rr;
}

void
ldns_pkt_set_cd(ldns_pkt *packet, bool cd)
{
	packet->_header->_cd = cd;
}

void
ldns_pkt_set_ra(ldns_pkt *packet, bool ra)
{
	packet->_header->_ra = ra;
}

void
ldns_pkt_set_ad(ldns_pkt *packet, bool ad)
{
	packet->_header->_ad = ad;
}

void
ldns_pkt_set_opcode(ldns_pkt *packet, ldns_pkt_opcode opcode)
{
	packet->_header->_opcode = opcode;
}

void
ldns_pkt_set_rcode(ldns_pkt *packet, uint8_t rcode)
{
	packet->_header->_rcode = rcode;
}

void
ldns_pkt_set_qdcount(ldns_pkt *packet, uint16_t qdcount)
{
	packet->_header->_qdcount = qdcount;
}

void
ldns_pkt_set_ancount(ldns_pkt *packet, uint16_t ancount)
{
	packet->_header->_ancount = ancount;
}

void
ldns_pkt_set_nscount(ldns_pkt *packet, uint16_t nscount)
{
	packet->_header->_nscount = nscount;
}

void
ldns_pkt_set_arcount(ldns_pkt *packet, uint16_t arcount)
{
	packet->_header->_arcount = arcount;
}

void
ldns_pkt_set_querytime(ldns_pkt *packet, uint32_t time) 
{
	packet->_querytime = time;
}

void
ldns_pkt_set_answerfrom(ldns_pkt *packet, ldns_rdf *answerfrom)
{
	packet->_answerfrom = answerfrom;
}

void
ldns_pkt_set_timestamp(ldns_pkt *packet, struct timeval timeval)
{
	packet->timestamp.tv_sec = timeval.tv_sec;
	packet->timestamp.tv_usec = timeval.tv_usec;
}

void
ldns_pkt_set_size(ldns_pkt *packet, size_t s)
{
	packet->_size = s;
}

void
ldns_pkt_set_edns_udp_size(ldns_pkt *packet, uint16_t s)
{
	packet->_edns_udp_size = s;
}

void
ldns_pkt_set_edns_extended_rcode(ldns_pkt *packet, uint8_t c)
{
	packet->_edns_extended_rcode = c;
}

void
ldns_pkt_set_edns_version(ldns_pkt *packet, uint8_t v)
{
	packet->_edns_version = v;
}

void
ldns_pkt_set_edns_z(ldns_pkt *packet, uint16_t z)
{
	packet->_edns_z = z;
}

void
ldns_pkt_set_edns_data(ldns_pkt *packet, ldns_rdf *data)
{
	packet->_edns_data = data;
}

void
ldns_pkt_set_section_count(ldns_pkt *packet, ldns_pkt_section s, uint16_t count)
{
	switch(s) {
		case LDNS_SECTION_QUESTION:
			ldns_pkt_set_qdcount(packet, count);
			break;
		case LDNS_SECTION_ANSWER:
			ldns_pkt_set_ancount(packet, count);
			break;
		case LDNS_SECTION_AUTHORITY:
			ldns_pkt_set_nscount(packet, count);
			break;
		case LDNS_SECTION_ADDITIONAL:
			ldns_pkt_set_arcount(packet, count);
			break;
		case LDNS_SECTION_ANY:
		case LDNS_SECTION_ANY_NOQUESTION:
			break;
	}
}

void ldns_pkt_set_tsig(ldns_pkt *pkt, ldns_rr *rr)
{
	pkt->_tsig_rr = rr;
}

bool
ldns_pkt_push_rr(ldns_pkt *packet, ldns_pkt_section section, ldns_rr *rr)
{
	switch(section) {
		case LDNS_SECTION_QUESTION:
			ldns_rr_list_push_rr(ldns_pkt_question(packet), rr);
			ldns_pkt_set_qdcount(packet, ldns_pkt_qdcount(packet) + 1);
			break;
		case LDNS_SECTION_ANSWER:
			ldns_rr_list_push_rr(ldns_pkt_answer(packet), rr);
			ldns_pkt_set_ancount(packet, ldns_pkt_ancount(packet) + 1);
			break;
		case LDNS_SECTION_AUTHORITY:
			ldns_rr_list_push_rr(ldns_pkt_authority(packet), rr);
			ldns_pkt_set_nscount(packet, ldns_pkt_nscount(packet) + 1);
			break;
		case LDNS_SECTION_ADDITIONAL:
			ldns_rr_list_push_rr(ldns_pkt_additional(packet), rr);
			ldns_pkt_set_arcount(packet, ldns_pkt_arcount(packet) + 1);
			break;
		case LDNS_SECTION_ANY:
		case LDNS_SECTION_ANY_NOQUESTION:
			/* shouldn't this error? */
			break;
	}
	return true;
}

bool
ldns_pkt_safe_push_rr(ldns_pkt *pkt, ldns_pkt_section sec, ldns_rr *rr)
{

	/* check to see if its there */
	if (ldns_pkt_rr(pkt, sec, rr)) {
		/* already there */
		return false;
	}
	return ldns_pkt_push_rr(pkt, sec, rr);
}

bool
ldns_pkt_push_rr_list(ldns_pkt *p, ldns_pkt_section s, ldns_rr_list *list)
{
	size_t i;
	for(i = 0; i < ldns_rr_list_rr_count(list); i++) {
		if (!ldns_pkt_push_rr(p, s, ldns_rr_list_rr(list, i))) {
			return false;
		}
	}
	return true;
}

bool
ldns_pkt_safe_push_rr_list(ldns_pkt *p, ldns_pkt_section s, ldns_rr_list *list)
{
	size_t i;
	for(i = 0; i < ldns_rr_list_rr_count(list); i++) {
		if (!ldns_pkt_safe_push_rr(p, s, ldns_rr_list_rr(list, i))) {
			return false;
		}
	}
	return true;
}

bool
ldns_pkt_edns(const ldns_pkt *pkt) {
	return (ldns_pkt_edns_udp_size(pkt) > 0 ||
		ldns_pkt_edns_extended_rcode(pkt) > 0 ||
		ldns_pkt_edns_data(pkt) ||
		ldns_pkt_edns_do(pkt)
	       );
}


/* Create/destroy/convert functions
 */
ldns_pkt *
ldns_pkt_new()
{
	ldns_pkt *packet;
	packet = LDNS_MALLOC(ldns_pkt);
	if (!packet) {
		return NULL;
	}

	packet->_header = LDNS_MALLOC(ldns_hdr);
	if (!packet->_header) {
		LDNS_FREE(packet);
		return NULL;
	}

	packet->_question = ldns_rr_list_new();
	packet->_answer = ldns_rr_list_new();
	packet->_authority = ldns_rr_list_new();
	packet->_additional = ldns_rr_list_new();

	/* default everything to false */
	ldns_pkt_set_qr(packet, false);
	ldns_pkt_set_aa(packet, false);
	ldns_pkt_set_tc(packet, false);
	ldns_pkt_set_rd(packet, false);
	ldns_pkt_set_ra(packet, false);
	ldns_pkt_set_ad(packet, false);
	ldns_pkt_set_cd(packet, false);

	ldns_pkt_set_opcode(packet, LDNS_PACKET_QUERY);
	ldns_pkt_set_rcode(packet, 0);
	ldns_pkt_set_id(packet, 0); 
	ldns_pkt_set_size(packet, 0);
	ldns_pkt_set_querytime(packet, 0);
	memset(&packet->timestamp, 0, sizeof(packet->timestamp));
	ldns_pkt_set_answerfrom(packet, NULL);
	ldns_pkt_set_section_count(packet, LDNS_SECTION_QUESTION, 0);
	ldns_pkt_set_section_count(packet, LDNS_SECTION_ANSWER, 0);
	ldns_pkt_set_section_count(packet, LDNS_SECTION_AUTHORITY, 0);
	ldns_pkt_set_section_count(packet, LDNS_SECTION_ADDITIONAL, 0);
	
	ldns_pkt_set_edns_udp_size(packet, 0);
	ldns_pkt_set_edns_extended_rcode(packet, 0);
	ldns_pkt_set_edns_version(packet, 0);
	ldns_pkt_set_edns_z(packet, 0);
	ldns_pkt_set_edns_data(packet, NULL);
	
	ldns_pkt_set_tsig(packet, NULL);
	
	return packet;
}

void
ldns_pkt_free(ldns_pkt *packet)
{
	if (packet) {
		LDNS_FREE(packet->_header);
		ldns_rr_list_deep_free(packet->_question);
		ldns_rr_list_deep_free(packet->_answer);
		ldns_rr_list_deep_free(packet->_authority);
		ldns_rr_list_deep_free(packet->_additional);
		ldns_rr_free(packet->_tsig_rr);
		LDNS_FREE(packet);
	}
}

bool
ldns_pkt_set_flags(ldns_pkt *packet, uint16_t flags)
{
	if (!packet) {
		return false;
	}
	if ((flags & LDNS_QR) == LDNS_QR) {
		ldns_pkt_set_qr(packet, true);
	}
	if ((flags & LDNS_AA) == LDNS_AA) {
		ldns_pkt_set_aa(packet, true);
	}
	if ((flags & LDNS_RD) == LDNS_RD) {
		ldns_pkt_set_rd(packet, true);
	}
	if ((flags & LDNS_TC) == LDNS_TC) {
		ldns_pkt_set_tc(packet, true);
	}
	if ((flags & LDNS_CD) == LDNS_CD) {
		ldns_pkt_set_cd(packet, true);
	}
	if ((flags & LDNS_RA) == LDNS_RA) {
		ldns_pkt_set_ra(packet, true);
	}
	if ((flags & LDNS_AD) == LDNS_AD) {
		ldns_pkt_set_ad(packet, true);
	}
	return true;
}

ldns_status
ldns_pkt_query_new_frm_str(ldns_pkt **p, const char *name, ldns_rr_type rr_type, 
		ldns_rr_class rr_class, uint16_t flags)
{
	ldns_pkt *packet;
	ldns_rr *question_rr;
	ldns_rdf *name_rdf;

	packet = ldns_pkt_new();
	if (!packet) {
		return LDNS_STATUS_MEM_ERR;
	}
	
	if (!ldns_pkt_set_flags(packet, flags)) {
		return LDNS_STATUS_ERR;
	}
	
	question_rr = ldns_rr_new();
	if (!question_rr) {
		return LDNS_STATUS_MEM_ERR;
	}

	if (rr_type == 0) {
		rr_type = LDNS_RR_TYPE_A;
	}
	if (rr_class == 0) {
		rr_class = LDNS_RR_CLASS_IN;
	}

	if (ldns_str2rdf_dname(&name_rdf, name) == LDNS_STATUS_OK) {
		ldns_rr_set_owner(question_rr, name_rdf);
		ldns_rr_set_type(question_rr, rr_type);
		ldns_rr_set_class(question_rr, rr_class);
		
		ldns_pkt_push_rr(packet, LDNS_SECTION_QUESTION, question_rr);
	} else {
		ldns_rr_free(question_rr);
		ldns_pkt_free(packet);
		return LDNS_STATUS_ERR;
	}
	
	packet->_tsig_rr = NULL;
	
	ldns_pkt_set_answerfrom(packet, NULL);
	if (p) {
		*p = packet;
		return LDNS_STATUS_OK;
	} else {
		return LDNS_STATUS_NULL;
	}
}

ldns_pkt *
ldns_pkt_query_new(ldns_rdf *rr_name, ldns_rr_type rr_type, ldns_rr_class rr_class,
		uint16_t flags)
{
	ldns_pkt *packet;
	ldns_rr *question_rr;

	packet = ldns_pkt_new();
	if (!packet) {
		return NULL;
	}

	if (!ldns_pkt_set_flags(packet, flags)) {
		return NULL;
	}
	
	question_rr = ldns_rr_new();
	if (!question_rr) {
		return NULL;
	}

	if (rr_type == 0) {
		rr_type = LDNS_RR_TYPE_A;
	}
	if (rr_class == 0) {
		rr_class = LDNS_RR_CLASS_IN;
	}

	ldns_rr_set_owner(question_rr, rr_name);
	ldns_rr_set_type(question_rr, rr_type);
	ldns_rr_set_class(question_rr, rr_class);
	
	packet->_tsig_rr = NULL;
	
	ldns_pkt_push_rr(packet, LDNS_SECTION_QUESTION, question_rr);

	return packet;
}

ldns_pkt_type
ldns_pkt_reply_type(ldns_pkt *p)
{
	ldns_rr_list *tmp;

	if (!p) {
		return LDNS_PACKET_UNKNOWN;
	}

	if (ldns_pkt_get_rcode(p) == LDNS_RCODE_NXDOMAIN) {
		return LDNS_PACKET_NXDOMAIN;
	}

	if (ldns_pkt_ancount(p) == 0 && ldns_pkt_arcount(p) == 0
			&& ldns_pkt_nscount(p) == 1) {

		/* check for SOA */
		tmp = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_SOA, 
					LDNS_SECTION_AUTHORITY);
		if (tmp) {
			ldns_rr_list_deep_free(tmp);
			return LDNS_PACKET_NODATA;
		} else {
			/* I have no idea ... */
		}
	}

	if (ldns_pkt_ancount(p) == 0 && ldns_pkt_nscount(p) > 0) {
		tmp = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_NS,
		                               LDNS_SECTION_AUTHORITY);
		if (tmp) {
			/* there are nameservers here */
			ldns_rr_list_deep_free(tmp);
			return LDNS_PACKET_REFERRAL;
		} else {
			/* I have no idea */
		}
		ldns_rr_list_deep_free(tmp);
	}
	
	/* if we cannot determine the packet type, we say it's an 
	 * answer...
	 */
	return LDNS_PACKET_ANSWER;
}

ldns_pkt *
ldns_pkt_clone(ldns_pkt *pkt)
{
	ldns_pkt *new_pkt;
	
	if (!pkt) {
		return NULL;
	}
	new_pkt = ldns_pkt_new();

	ldns_pkt_set_id(new_pkt, ldns_pkt_id(pkt));
	ldns_pkt_set_qr(new_pkt, ldns_pkt_qr(pkt));
	ldns_pkt_set_aa(new_pkt, ldns_pkt_aa(pkt));
	ldns_pkt_set_tc(new_pkt, ldns_pkt_tc(pkt));
	ldns_pkt_set_rd(new_pkt, ldns_pkt_rd(pkt));
	ldns_pkt_set_cd(new_pkt, ldns_pkt_cd(pkt));
	ldns_pkt_set_ra(new_pkt, ldns_pkt_ra(pkt));
	ldns_pkt_set_ad(new_pkt, ldns_pkt_ad(pkt));
	ldns_pkt_set_opcode(new_pkt, ldns_pkt_get_opcode(pkt));
	ldns_pkt_set_rcode(new_pkt, ldns_pkt_get_rcode(pkt));
	ldns_pkt_set_qdcount(new_pkt, ldns_pkt_qdcount(pkt));
	ldns_pkt_set_ancount(new_pkt, ldns_pkt_ancount(pkt));
	ldns_pkt_set_nscount(new_pkt, ldns_pkt_nscount(pkt));
	ldns_pkt_set_arcount(new_pkt, ldns_pkt_arcount(pkt));
	ldns_pkt_set_answerfrom(new_pkt, ldns_pkt_answerfrom(pkt));
	ldns_pkt_set_querytime(new_pkt, ldns_pkt_querytime(pkt));
	ldns_pkt_set_size(new_pkt, ldns_pkt_size(pkt));
	ldns_pkt_set_tsig(new_pkt, ldns_pkt_tsig(pkt));
	
	ldns_pkt_set_edns_udp_size(new_pkt, ldns_pkt_edns_udp_size(pkt));
	ldns_pkt_set_edns_extended_rcode(new_pkt, 
		ldns_pkt_edns_extended_rcode(pkt));
	ldns_pkt_set_edns_version(new_pkt, ldns_pkt_edns_version(pkt));
	ldns_pkt_set_edns_z(new_pkt, ldns_pkt_edns_z(pkt));
	if(ldns_pkt_edns_data(pkt))
		ldns_pkt_set_edns_data(new_pkt, 
			ldns_rdf_clone(ldns_pkt_edns_data(pkt)));
	ldns_pkt_set_edns_do(new_pkt, ldns_pkt_edns_do(pkt));

	ldns_rr_list_deep_free(new_pkt->_question);
	ldns_rr_list_deep_free(new_pkt->_answer);
	ldns_rr_list_deep_free(new_pkt->_authority);
	ldns_rr_list_deep_free(new_pkt->_additional);
	new_pkt->_question = ldns_rr_list_clone(ldns_pkt_question(pkt));
	new_pkt->_answer = ldns_rr_list_clone(ldns_pkt_answer(pkt));
	new_pkt->_authority = ldns_rr_list_clone(ldns_pkt_authority(pkt));
	new_pkt->_additional = ldns_rr_list_clone(ldns_pkt_additional(pkt));
	return new_pkt;
}
