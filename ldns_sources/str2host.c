/*
 * str2host.c
 *
 * conversion routines from the presentation format
 * to the host format
 *
 * a Net::DNS like library for C
 *
 * (c) NLnet Labs, 2004-2006
 *
 * See the file LICENSE for the license
 */
#include "ldns/config.h"

#include "ldns.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <time.h>

#include <errno.h>
#ifdef HAVE_NETDB_H
#include <netdb.h> 
#endif

#include <limits.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

ldns_status
ldns_str2rdf_int16(ldns_rdf **rd, const char *shortstr)
{
	char *end = NULL;    
	uint16_t *r;
	r = LDNS_MALLOC(uint16_t);
	
	*r = htons((uint16_t)strtol((char *)shortstr, &end, 0));
	
	if(*end != 0) {
		LDNS_FREE(r);
		return LDNS_STATUS_INVALID_INT;
	} else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT16, sizeof(uint16_t), r);
		LDNS_FREE(r);
		return LDNS_STATUS_OK;
	}
}

ldns_status
ldns_str2rdf_time(ldns_rdf **rd, const char *time)
{
	/* convert a time YYYYDDMMHHMMSS to wireformat */
	uint16_t *r = NULL;
	struct tm tm;
	uint32_t l;
	char *end;

	/* Try to scan the time... */
	r = (uint16_t*)LDNS_MALLOC(uint32_t);

	memset(&tm, 0, sizeof(tm));

	if (strlen(time) == 14 &&
	    sscanf(time, "%4d%2d%2d%2d%2d%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6
	   ) {
	   	tm.tm_year -= 1900;
	   	tm.tm_mon--;
	   	/* Check values */
		if (tm.tm_year < 70) {
			goto bad_format;
		}
		if (tm.tm_mon < 0 || tm.tm_mon > 11) {
			goto bad_format;
		}
		if (tm.tm_mday < 1 || tm.tm_mday > 31) {
			goto bad_format;
		}
		
		if (tm.tm_hour < 0 || tm.tm_hour > 23) {
			goto bad_format;
		}

		if (tm.tm_min < 0 || tm.tm_min > 59) {
			goto bad_format;
		}

		if (tm.tm_sec < 0 || tm.tm_sec > 59) {
			goto bad_format;
		}

		l = htonl(mktime_from_utc(&tm));
		memcpy(r, &l, sizeof(uint32_t));
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_TIME, sizeof(uint32_t), r);
		LDNS_FREE(r);
		return LDNS_STATUS_OK;
	} else {
		/* handle it as 32 bits timestamp */
		l = htonl((uint32_t)strtol((char*)time, &end, 0));
		if(*end != 0) {
			LDNS_FREE(r);
			return LDNS_STATUS_ERR;
		} else {
			memcpy(r, &l, sizeof(uint32_t));
			*rd = ldns_rdf_new_frm_data(
				LDNS_RDF_TYPE_INT32, sizeof(uint32_t), r);
			LDNS_FREE(r);
			return LDNS_STATUS_OK;
		}
	}
	
	bad_format:
	LDNS_FREE(r);
	return LDNS_STATUS_INVALID_TIME;
}

ldns_status
ldns_str2rdf_nsec3_salt(ldns_rdf **rd, const char *salt_str)
{
	uint8_t salt_length;
	uint8_t c;

	uint8_t *salt;
	uint8_t *data;

	salt_length = (uint8_t) strlen(salt_str);
	if (salt_length == 1 && salt_str[0] == '-') {
		salt_length = 0;
	} else if (salt_length % 2 != 0) {
		return LDNS_STATUS_INVALID_HEX;
	}
	
	salt = LDNS_XMALLOC(uint8_t, salt_length / 2);
	for (c = 0; c < salt_length; c += 2) {
		if (isxdigit(salt_str[c]) && isxdigit(salt_str[c+1])) {
			salt[c/2] = (uint8_t) ldns_hexdigit_to_int(salt_str[c]) * 16 +
					  ldns_hexdigit_to_int(salt_str[c+1]);
		} else {
			LDNS_FREE(salt);
			return LDNS_STATUS_INVALID_HEX;
		}
	}
	salt_length = salt_length / 2;
	
	data = LDNS_XMALLOC(uint8_t, 1 + salt_length);
	data[0] = salt_length;
	memcpy(&data[1], salt, salt_length);
	*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_NSEC3_SALT, 1 + salt_length, data);
	LDNS_FREE(data);
	LDNS_FREE(salt);

	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_period(ldns_rdf **rd,const char *period)
{
        uint32_t p;
        const char *end;

        /* Allocate required space... */
        p = ldns_str2period(period, &end);

        if (*end != 0) {
		return LDNS_STATUS_ERR;
        } else {
                p = (uint32_t) htonl(p);
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_PERIOD, sizeof(uint32_t), &p);
        }
	return LDNS_STATUS_OK;
}

ldns_status 
ldns_str2rdf_int32(ldns_rdf **rd, const char *longstr)
{
	char *end;  
	uint16_t *r = NULL;
	uint32_t l;

	r = (uint16_t*)LDNS_MALLOC(uint32_t);
	l = htonl((uint32_t)strtol((char*)longstr, &end, 0));

	if(*end != 0) {
		LDNS_FREE(r);
		return LDNS_STATUS_ERR;
        } else {
		memcpy(r, &l, sizeof(uint32_t));
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT32, sizeof(uint32_t), r);
		LDNS_FREE(r);
		return LDNS_STATUS_OK;
	}
}

ldns_status
ldns_str2rdf_int8(ldns_rdf **rd, const char *bytestr)
{
	char *end;     
	uint8_t *r = NULL;

	r = LDNS_MALLOC(uint8_t);
 
	*r = (uint8_t)strtol((char*)bytestr, &end, 0);

        if(*end != 0) {
		LDNS_FREE(r);
		return LDNS_STATUS_ERR;
        } else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT8, sizeof(uint8_t), r);
		LDNS_FREE(r);
		return LDNS_STATUS_OK;
        }
}

/*
 * No special care is taken, all dots are translated into
 * label seperators.
 * Could be made more efficient....we do 3 memcpy's in total...
 */
ldns_status
ldns_str2rdf_dname(ldns_rdf **d, const char *str)
{
	size_t len;

	uint8_t *s,*p,*q, *pq, val, label_len;
	uint8_t buf[LDNS_MAX_DOMAINLEN + 1];
	*d = NULL;
	
	len = strlen((char*)str);
	/* octet representation can make strings a lot longer than actual length */
	if (len > LDNS_MAX_DOMAINLEN * 3) {
		return LDNS_STATUS_DOMAINNAME_OVERFLOW;
	}
	if (0 == len) {
		return LDNS_STATUS_DOMAINNAME_UNDERFLOW;
	} 
	
	/* root label */
	if (1 == len && *str == '.') {
		*d = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_DNAME, 1, "\0"); 
		return LDNS_STATUS_OK;
	}

	/* get on with the rest */

	/* s is on the current dot
	 * p on the previous one
	 * q builds the dname
	 */
	len = 0;
	q = buf+1;
	pq = buf;
	label_len = 0;
	for (s = p = (uint8_t *) str; *s; s++, q++) {
		*q = 0;
		switch (*s) {
		case '.':
			if (label_len > LDNS_MAX_LABELLEN) {
				return LDNS_STATUS_LABEL_OVERFLOW;
			}
			if (label_len == 0) {
				return LDNS_STATUS_EMPTY_LABEL;
			}
			len += label_len + 1;
			*pq = label_len;
			label_len = 0;
			pq = q;
			p = s+1;
			break;
		case '\\':
			/* octet value or literal char */
			if (strlen((char *)s) > 3 &&
			    isdigit((int) s[1]) &&
			    isdigit((int) s[2]) &&
			    isdigit((int) s[3])) {
				/* cast this so it fits */
				val = (uint8_t) ldns_hexdigit_to_int((char) s[1]) * 100 +
				                ldns_hexdigit_to_int((char) s[2]) * 10 +
				                ldns_hexdigit_to_int((char) s[3]);
				*q = val;
				s += 3;
			} else {
				s++;
				*q = *s;
			}
			label_len++;
			break;
		default:
			*q = *s;
			label_len++;
		}
	}

	/* add root label if last char was not '.' */
	if (!ldns_dname_str_absolute(str)) {
		len += label_len + 1;
		*pq = label_len;
		*q = 0;
	}
	len++;

	*d = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_DNAME, len, buf); 
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_a(ldns_rdf **rd, const char *str)
{
	in_addr_t address;
        if (inet_pton(AF_INET, (char*)str, &address) != 1) {
                return LDNS_STATUS_INVALID_IP4;
        } else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_A, sizeof(address), &address);
        }
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_aaaa(ldns_rdf **rd, const char *str)
{
	uint8_t address[LDNS_IP6ADDRLEN + 1];

	if (inet_pton(AF_INET6, (char*)str, address) != 1) {
		return LDNS_STATUS_INVALID_IP6;
	} else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_AAAA, sizeof(address) - 1, &address);
	}
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_str(ldns_rdf **rd, const char *str)
{
	uint8_t *data;
	uint8_t val;
	size_t i, str_i;
	
	if (strlen(str) > 255) {
		return LDNS_STATUS_INVALID_STR;
	}

	data = LDNS_XMALLOC(uint8_t, strlen(str) + 1);
	i = 1;
	for (str_i = 0; str_i < strlen(str); str_i++) {
		if (str[str_i] == '\\') {
			if(str_i + 3 < strlen(str) && 
			   isdigit(str[str_i + 1]) &&
			   isdigit(str[str_i + 2]) &&
			   isdigit(str[str_i + 3])) {
				val = (uint8_t) ldns_hexdigit_to_int((char) str[str_i + 1]) * 100 +
				                ldns_hexdigit_to_int((char) str[str_i + 2]) * 10 +
				                ldns_hexdigit_to_int((char) str[str_i + 3]);
				data[i] = val;
				i++;
				str_i += 3;
			} else {
				str_i++;
				data[i] = (uint8_t) str[str_i];
				i++;
			}
		} else {
			data[i] = (uint8_t) str[str_i];
			i++;
		}
	}
	data[0] = i - 1;
	*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_STR, i, data);
	LDNS_FREE(data);
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_apl(ldns_rdf **rd, const char *str)
{
	const char *my_str = str;

	char *my_ip_str;
	size_t ip_str_len;

	uint16_t family;
	bool negation;
	uint8_t afdlength = 0;
	uint8_t *afdpart;
	uint8_t prefix;

	uint8_t *data;
	
	size_t i = 0;

	/* [!]afi:address/prefix */
	if (strlen(my_str) < 2) {
		return LDNS_STATUS_INVALID_STR;
	}

	if (my_str[0] == '!') {
		negation = true;
		my_str += 1;
	} else {
		negation = false;
	}

	family = (uint16_t) atoi(my_str);
	
	my_str = strchr(my_str, ':') + 1;

	/* need ip addr and only ip addr for inet_pton */
	ip_str_len = (size_t) (strchr(my_str, '/') - my_str);
	my_ip_str = LDNS_XMALLOC(char, ip_str_len + 1);
	strncpy(my_ip_str, my_str, ip_str_len + 1);
	my_ip_str[ip_str_len] = '\0';

	if (family == 1) {
		/* ipv4 */
		afdpart = LDNS_XMALLOC(uint8_t, 4);
		if (inet_pton(AF_INET, my_ip_str, afdpart) == 0) {
			return LDNS_STATUS_INVALID_STR;
		}
		for (i = 0; i < 4; i++) {
			if (afdpart[i] != 0) {
				afdlength = i + 1;
			}
		}
	} else if (family == 2) {
		/* ipv6 */
		afdpart = LDNS_XMALLOC(uint8_t, 16);
		if (inet_pton(AF_INET6, my_ip_str, afdpart) == 0) {
			return LDNS_STATUS_INVALID_STR;
		}
		for (i = 0; i < 16; i++) {
			if (afdpart[i] != 0) {
				afdlength = i + 1;
			}
		}
	} else {
		/* unknown family */
		LDNS_FREE(my_ip_str);
		return LDNS_STATUS_INVALID_STR;
	}

	my_str = strchr(my_str, '/') + 1;
	prefix = (uint8_t) atoi(my_str);

	data = LDNS_XMALLOC(uint8_t, 4 + afdlength);
	ldns_write_uint16(data, family);
	data[2] = prefix;
	data[3] = afdlength;
	if (negation) {
		/* set bit 1 of byte 3 */
		data[3] = data[3] | 0x80;
	}
	
	memcpy(data + 4, afdpart, afdlength);

	*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_APL, afdlength + 4, data);
	LDNS_FREE(afdpart);
	LDNS_FREE(data);
	LDNS_FREE(my_ip_str);

	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_b64(ldns_rdf **rd, const char *str)
{
	uint8_t *buffer;
	int16_t i;
	
	buffer = LDNS_XMALLOC(uint8_t, b64_ntop_calculate_size(strlen(str)));
	
	i = (uint16_t)b64_pton((const char*)str, buffer, 
	                        b64_ntop_calculate_size(strlen(str)));
	if (-1 == i) {
		LDNS_FREE(buffer);
		return LDNS_STATUS_INVALID_B64;
	} else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_B64, (uint16_t) i, buffer);
	}
	LDNS_FREE(buffer);

	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_b32_ext(ldns_rdf **rd, const char *str)
{
	uint8_t *buffer;
	int i;
	/* first byte contains length of actual b32 data */
	uint8_t len = b32_pton_calculate_size(strlen(str));
	buffer = LDNS_XMALLOC(uint8_t, len + 1);
	buffer[0] = len;
	
	i = b32_pton_extended_hex((const char*)str, strlen(str), buffer + 1, 
                                  b32_ntop_calculate_size(strlen(str)));
	if (i < 0) {
		return LDNS_STATUS_INVALID_B32_EXT;
	} else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_B32_EXT, (uint16_t) i + 1, buffer);
	}
	LDNS_FREE(buffer);

	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_hex(ldns_rdf **rd, const char *str)
{
        uint8_t *t, *t_orig;
        int i;
	size_t len;

	len = strlen(str);

        if (len % 2 != 0) {
                return LDNS_STATUS_INVALID_HEX;
        } else if (len > LDNS_MAX_RDFLEN * 2) {
		return LDNS_STATUS_LABEL_OVERFLOW;
        } else {
		t = LDNS_XMALLOC(uint8_t, (len / 2));
		t_orig = t;
                /* Now process octet by octet... */
                while (*str) {
                        *t = 0;
                        for (i = 16; i >= 1; i -= 15) {
                                if (isxdigit(*str)) {
                                        *t += ldns_hexdigit_to_int(*str) * i;
                                } else {
					/* error or be lenient and skip? */
                                        /*return LDNS_STATUS_ERR;*/
                                }
                                ++str;
                        }
                        ++t;
                }
                t = t_orig;
		*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_HEX, len / 2, t);
		LDNS_FREE(t);
        }
        return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_nsec(ldns_rdf **rd, const char *str)
{
	const char *delimiters = "\n\t ";
	char *token = LDNS_XMALLOC(char, LDNS_MAX_RDFLEN);
	uint8_t *bitmap = LDNS_XMALLOC(uint8_t, 1);
	uint16_t bm_len = 0;
	ldns_buffer *str_buf;
	ssize_t c;
	uint16_t cur_type;
	uint8_t cur_data[32];
	uint8_t cur_window = 0;
	uint8_t cur_window_max = 0;
	uint16_t cur_data_size = 0;
	uint16_t i;
	uint8_t *data = NULL;

	str_buf = LDNS_MALLOC(ldns_buffer);
	ldns_buffer_new_frm_data(str_buf, (char *)str, strlen(str));

	bitmap[0] = 0;
	while ((c = ldns_bget_token(str_buf, token, delimiters, LDNS_MAX_RDFLEN)) != -1) {
		cur_type = ldns_get_rr_type_by_name(token);
		if ((cur_type / 8) + 1 > bm_len) {
			bitmap = LDNS_XREALLOC(bitmap, uint8_t, (cur_type / 8) + 1);
			/* set to 0 */
			for (; bm_len <= cur_type / 8; bm_len++) {
				bitmap[bm_len] = 0;
			}
		}
		ldns_set_bit(bitmap + (int) cur_type / 8, (int) (7 - (cur_type % 8)), true);
	}

	memset(cur_data, 0, 32);
	for (i = 0; i < bm_len; i++) {
		if (i / 32 > cur_window) {
			/* check, copy, new */
			if (cur_window_max > 0) {
				/* this window has stuff, add it */
				data = LDNS_XREALLOC(data, uint8_t, cur_data_size + cur_window_max + 3);
				data[cur_data_size] = cur_window;
				data[cur_data_size + 1] = cur_window_max + 1;
				memcpy(data + cur_data_size + 2, cur_data, cur_window_max+1);
				cur_data_size += cur_window_max + 3;
			}
			cur_window++;
			cur_window_max = 0;
			memset(cur_data, 0, 32);
		} else {
			cur_data[i%32] = bitmap[i];
			if (bitmap[i] > 0) {
				cur_window_max = i%32;
			}
		}
	}
	if (cur_window_max > 0) {
		/* this window has stuff, add it */
		data = LDNS_XREALLOC(data, uint8_t, cur_data_size + cur_window_max + 3);
		data[cur_data_size] = cur_window;
		data[cur_data_size + 1] = cur_window_max + 1;
		memcpy(data + cur_data_size + 2, cur_data, cur_window_max+1);
		cur_data_size += cur_window_max + 3;
	}

	*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_NSEC, cur_data_size, data);
	if(data)
		LDNS_FREE(data);
	if (token)
		LDNS_FREE(token);
	if(bitmap)
		LDNS_FREE(bitmap);
	ldns_buffer_free(str_buf);
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_type(ldns_rdf **rd, const char *str)
{
	uint16_t type;
	type = htons(ldns_get_rr_type_by_name(str));
	/* ldns_rr_type is a 16 bit value */
	*rd = ldns_rdf_new_frm_data(
		LDNS_RDF_TYPE_TYPE, sizeof(uint16_t), &type);
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_class(ldns_rdf **rd, const char *str)
{
	uint16_t klass;
	klass = htons(ldns_get_rr_class_by_name(str));
	/* class is 16 bit */
	*rd = ldns_rdf_new_frm_data(
		LDNS_RDF_TYPE_CLASS, sizeof(uint16_t), &klass);
	return LDNS_STATUS_OK;
}

/* An certificate alg field can either be specified as a 8 bits number
 * or by its symbolic name. Handle both
 */
ldns_status
ldns_str2rdf_cert_alg(ldns_rdf **rd, const char *str)
{
	ldns_lookup_table *lt;
	ldns_status st;
	uint8_t idd[2];
	lt = ldns_lookup_by_name(ldns_cert_algorithms, str);
	st = LDNS_STATUS_OK;

	if (lt) {
		ldns_write_uint16(idd, (uint16_t) lt->id);
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT16, sizeof(uint16_t), idd);
		if (!*rd) {
			st = LDNS_STATUS_ERR;
		}
	} else {
		/* try as-is (a number) */
		st = ldns_str2rdf_int16(rd, str);
		if (st == LDNS_STATUS_OK &&
		    ldns_rdf2native_int16(*rd) == 0) {
			st = LDNS_STATUS_CERT_BAD_ALGORITHM;
		}
	}

	return st;
}
		
/* An alg field can either be specified as a 8 bits number
 * or by its symbolic name. Handle both
 */
ldns_status
ldns_str2rdf_alg(ldns_rdf **rd, const char *str)
{
	ldns_lookup_table *lt;
	ldns_status st;

	lt = ldns_lookup_by_name(ldns_algorithms, str);
	st = LDNS_STATUS_OK;

	if (lt) {
		/* it was given as a integer */
		*rd = ldns_native2rdf_int8(LDNS_RDF_TYPE_INT8, (uint8_t) lt->id);
		if (!*rd) {
			st = LDNS_STATUS_ERR;
		}
	} else {
		/* try as-is (a number) */
		st = ldns_str2rdf_int8(rd, str);
	}
	return st;
}
		
ldns_status
ldns_str2rdf_unknown(ldns_rdf **rd, const char *str)
{
	/* this should be caught in an earlier time (general str2host for 
	   rr's */
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_tsig(ldns_rdf **rd, const char *str)
{
	/* there is no strign representation for TSIG rrs */
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_service(ldns_rdf **rd, const char *str)
{
	/* is this used? is this actually WKS? or SRV? */
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

static int
loc_parse_cm(char* my_str, char** endstr, uint8_t* m, uint8_t* e)
{
	/* read <digits>[.<digits>][mM] */
	/* into mantissa exponent format for LOC type */
	uint32_t meters = 0, cm = 0, val;
	while (isblank(*my_str)) {
		my_str++;
	}
	meters = (uint32_t)strtol(my_str, &my_str, 10);
	if (*my_str == '.') {
		my_str++;
		cm = (uint32_t)strtol(my_str, &my_str, 10);
	}
	if (meters >= 1) {
		*e = 2;
		val = meters;
	} else	{
		*e = 0;
		val = cm;
	}
	while(val >= 10) {
		(*e)++;
		val /= 10;
	}
	*m = (uint8_t)val;
	
	if (*e > 9)
		return 0;
	if (*my_str == 'm' || *my_str == 'M') {
		my_str++;
	}
	*endstr = my_str;
	return 1;
}

ldns_status
ldns_str2rdf_loc(ldns_rdf **rd, const char *str)
{
	uint32_t latitude = 0;
	uint32_t longitude = 0;
	uint32_t altitude = 0;

	uint8_t *data;
	uint32_t equator = (uint32_t) ldns_power(2, 31);

	uint32_t h = 0;
	uint32_t m = 0;
	uint8_t size_b = 1, size_e = 2;
	uint8_t horiz_pre_b = 1, horiz_pre_e = 6;
	uint8_t vert_pre_b = 1, vert_pre_e = 3;
	
	double s = 0.0;
	bool northerness;
	bool easterness;

	char *my_str = (char *) str;

	/* only support version 0 */
	if (isdigit(*my_str)) {
		h = (uint32_t) strtol(my_str, &my_str, 10);
	} else {
		return LDNS_STATUS_INVALID_STR;
	}

	while (isblank(*my_str)) {
		my_str++;
	}

	if (isdigit(*my_str)) {
		m = (uint32_t) strtol(my_str, &my_str, 10);
	} else if (*my_str == 'N' || *my_str == 'S') {
		goto north;
	} else {
		return LDNS_STATUS_INVALID_STR;
	}

	while (isblank(*my_str)) {
		my_str++;
	}

	if (isdigit(*my_str)) {
		s = strtod(my_str, &my_str);
	}
north:
	while (isblank(*my_str)) {
		my_str++;
	}

	if (*my_str == 'N') {
		northerness = true;
	} else if (*my_str == 'S') {
		northerness = false;
	} else {
		return LDNS_STATUS_INVALID_STR;
	}

	my_str++;

	/* store number */
	s = 1000.0 * s;
	/* add a little to make floor in conversion a round */
	s += 0.0005;
	latitude = (uint32_t) s;
	latitude += 1000 * 60 * m;
	latitude += 1000 * 60 * 60 * h;
	if (northerness) {
		latitude = equator + latitude;
	} else {
		latitude = equator - latitude;
	}
	while (isblank(*my_str)) {
		my_str++;
	}

	if (isdigit(*my_str)) {
		h = (uint32_t) strtol(my_str, &my_str, 10);
	} else {
		return LDNS_STATUS_INVALID_STR;
	}

	while (isblank(*my_str)) {
		my_str++;
	}

	if (isdigit(*my_str)) {
		m = (uint32_t) strtol(my_str, &my_str, 10);
	} else if (*my_str == 'E' || *my_str == 'W') {
		goto east;
	} else {
		return LDNS_STATUS_INVALID_STR;
	}

	while (isblank(*my_str)) {
		my_str++;
	}

	if (isdigit(*my_str)) {
		s = strtod(my_str, &my_str);
	}

east:
	while (isblank(*my_str)) {
		my_str++;
	}

	if (*my_str == 'E') {
		easterness = true;
	} else if (*my_str == 'W') {
		easterness = false;
	} else {
		return LDNS_STATUS_INVALID_STR;
	}

	my_str++;

	/* store number */
	s *= 1000.0;
	/* add a little to make floor in conversion a round */
	s += 0.0005;
	longitude = (uint32_t) s;
	longitude += 1000 * 60 * m;
	longitude += 1000 * 60 * 60 * h;

	if (easterness) {
		longitude += equator;
	} else {
		longitude = equator - longitude;
	}

	altitude = (uint32_t)(strtod(my_str, &my_str)*100.0 +
		10000000.0 + 0.5);
	if (*my_str == 'm' || *my_str == 'M') {
		my_str++;
	}

	if (strlen(my_str) > 0) {
		if(!loc_parse_cm(my_str, &my_str, &size_b, &size_e))
			return LDNS_STATUS_INVALID_STR;
	}

	if (strlen(my_str) > 0) {
		if(!loc_parse_cm(my_str, &my_str, &horiz_pre_b, &horiz_pre_e))
			return LDNS_STATUS_INVALID_STR;
	}

	if (strlen(my_str) > 0) {
		if(!loc_parse_cm(my_str, &my_str, &vert_pre_b, &vert_pre_e))
			return LDNS_STATUS_INVALID_STR;
	}

	data = LDNS_XMALLOC(uint8_t, 16);
	data[0] = 0;
	data[1] = 0;
	data[1] = ((size_b << 4) & 0xf0) | (size_e & 0x0f);
	data[2] = ((horiz_pre_b << 4) & 0xf0) | (horiz_pre_e & 0x0f);
	data[3] = ((vert_pre_b << 4) & 0xf0) | (vert_pre_e & 0x0f);
	ldns_write_uint32(data + 4, latitude);
	ldns_write_uint32(data + 8, longitude);
	ldns_write_uint32(data + 12, altitude);

	*rd = ldns_rdf_new_frm_data(
		LDNS_RDF_TYPE_LOC, 16, data);

	LDNS_FREE(data);
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_wks(ldns_rdf **rd, const char *str)
{
	uint8_t *bitmap = NULL;
	uint8_t *data;
	int bm_len = 0;
	
	struct protoent *proto = NULL;
	struct servent *serv = NULL;
	int serv_port;
	
	ldns_buffer *str_buf;
	
	char *proto_str = NULL;
	char *token = LDNS_XMALLOC(char, 50);
	
	str_buf = LDNS_MALLOC(ldns_buffer);
	ldns_buffer_new_frm_data(str_buf, (char *)str, strlen(str));

	while(ldns_bget_token(str_buf, token, "\t\n ", strlen(str)) > 0) {
		if (!proto_str) {
			proto_str = strdup(token);
			if (!proto_str) {
				LDNS_FREE(token);
				LDNS_FREE(str_buf);
				return LDNS_STATUS_INVALID_STR;
			}
		} else {
			serv = getservbyname(token, proto_str);
			if (serv) {
				serv_port = (int) ntohs((uint16_t) serv->s_port);
			} else {
				serv_port = atoi(token);
			}
			if (serv_port / 8 > bm_len) {
				bitmap = LDNS_XREALLOC(bitmap, uint8_t, (serv_port / 8) + 1);
				/* set to zero to be sure */
				for (; bm_len <= serv_port / 8; bm_len++) {
					bitmap[bm_len] = 0;
				}
			}
			ldns_set_bit(bitmap + (serv_port / 8), 7 - (serv_port % 8), true);
		}
	}
	
	data = LDNS_XMALLOC(uint8_t, bm_len + 1);
	proto = getprotobyname(proto_str);
	if (proto) {
		data[0] = (uint8_t) proto->p_proto;
	} else {
		data[0] = (uint8_t) atoi(proto_str);
	}
	memcpy(data + 1, bitmap, (size_t) bm_len);
	
	*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_WKS, (uint16_t) (bm_len + 1), data);

	LDNS_FREE(token);
	ldns_buffer_free(str_buf);
	LDNS_FREE(bitmap);
	LDNS_FREE(data);
	free(proto_str);
#ifdef HAVE_ENDSERVENT
	endservent();
#endif
#ifdef HAVE_ENDPROTOENT
	endprotoent();
#endif
	
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_nsap(ldns_rdf **rd, const char *str)
{
	/* just a hex string with optional dots? */
	if (str[0] != '0' || str[1] != 'x') {
		return LDNS_STATUS_INVALID_STR;
	} else {
		return ldns_str2rdf_hex(rd, str+2);
	}
}
