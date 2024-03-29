/*
 * special zone file structures and functions for better dnssec handling
 *
 * A zone contains a SOA dnssec_zone_rrset, and an AVL tree of 'normal'
 * dnssec_zone_rrsets, indexed by name and type
 */

#ifndef LDNS_DNSSEC_ZONE_H
#define LDNS_DNSSEC_ZONE_H
 
#include "ldns.h"
#include "rbtree.h"

/**
 * Singly linked list of rrs
 */
typedef struct ldns_struct_dnssec_rrs ldns_dnssec_rrs;
struct ldns_struct_dnssec_rrs
{
	ldns_rr *rr;
	ldns_dnssec_rrs *next;
};

/**
 * Singly linked list of RRsets
 */
typedef struct ldns_struct_dnssec_rrsets ldns_dnssec_rrsets;
struct ldns_struct_dnssec_rrsets
{
	ldns_dnssec_rrs *rrs;
	ldns_rr_type type;
	ldns_dnssec_rrs *signatures;
	ldns_dnssec_rrsets *next;
};

/**
 * Structure containing all resource records for a domain name
 * Including the derived NSEC3, if present
 */
typedef struct ldns_struct_dnssec_name ldns_dnssec_name;
struct ldns_struct_dnssec_name
{
	/**
	 * pointer to a dname containing the name.
	 * Usually points to the owner name of the first RR of the first RRset
	 */
	ldns_rdf *name;
	/** 
	 * Usually, the name is a pointer to the owner name of the first rr for
	 * this name, but sometimes there is no actual data to point to, 
	 * for instance in
	 * names representing empty nonterminals. If so, set alloced to true to
	 * indicate that this data must also be freed when the name is freed
	 */
	bool name_alloced;
	/**
	 * The rrsets for this name
	 */
	ldns_dnssec_rrsets *rrsets;
	/**
	 * NSEC pointing to the next name (or NSEC3 pointing to the next NSEC3)
	 */
	ldns_rr *nsec;
	/**
	 * signatures for the NSEC record
	 */
	ldns_dnssec_rrs *nsec_signatures;
	/**
	 * Set to true if this name is glue
	 * (as marked by ldns_dnssec_zone_mark_glue())
	 */
	bool is_glue;
	/**
	 * pointer to store the hashed name (only used when in an NSEC3 zone
	 */
	ldns_rdf *hashed_name;
};

/**
 * Structure containing a dnssec zone
 */
struct ldns_struct_dnssec_zone {
	/** points to the name containing the SOA RR */
	ldns_dnssec_name *soa;
	/** tree of ldns_dnssec_names */
	ldns_rbtree_t *names;
};
typedef struct ldns_struct_dnssec_zone ldns_dnssec_zone;

/**
 * Creates a new entry for 1 pointer to an rr and 1 pointer to the next rrs
 * \return the allocated data
 */
ldns_dnssec_rrs *ldns_dnssec_rrs_new();

/**
 * Frees the list of rrs, but *not* its data
 *
 * \param[in] rrs the data structure to free
 */
void ldns_dnssec_rrs_free(ldns_dnssec_rrs *rrs);

/**
 * Adds an RR to the list of RRs. The list will remain ordered
 *
 * \param[in] rrs the list to add to
 * \param[in] rr the RR to add
 * \return LDNS_STATUS_OK on success
 */
ldns_status ldns_dnssec_rrs_add_rr(ldns_dnssec_rrs *rrs, ldns_rr *rr);

/**
 * Prints the given rrs to the file descriptor
 *
 * \param[in] out the file descriptor to print to
 * \param[in] rrs the list of RRs to print
 */
void ldns_dnssec_rrs_print(FILE *out, ldns_dnssec_rrs *rrs);

/**
 * Creates a new list (entry) of RRsets
 * \return the newly allocated structure
 */
ldns_dnssec_rrsets *ldns_dnssec_rrsets_new();

/**
 * Frees the list of rrsets and their rrs, but *not* their data
 *
 * \param[in] rrsets the data structure to free
 */
void ldns_dnssec_rrsets_free(ldns_dnssec_rrsets *rrsets);

/**
 * Returns the rr type of the rrset (that is head of the given list)
 *
 * \param[in] rrsets the rrset to get the type of
 * \return the rr type
 */
ldns_rr_type ldns_dnssec_rrsets_type(ldns_dnssec_rrsets *rrsets);

/**
 * Sets the RR type of the rrset (that is head of the given list)
 *
 * \param[in] rrsets the rrset to set the type of
 * \param[in] type the type to set
 * \return LDNS_STATUS_OK on success
 */
ldns_status ldns_dnssec_rrsets_set_type(ldns_dnssec_rrsets *rrsets,
					   ldns_rr_type type);

/**
 * Add an ldns_rr to the corresponding RRset in the given list of RRsets.
 * If it is not present, add it as a new RRset with 1 record.
 *
 * \param[in] rrsets the list of rrsets to add the RR to
 * \param[in] rr the rr to add to the list of rrsets
 * \return LDNS_STATUS_OK on success
 */
ldns_status ldns_dnssec_rrsets_add_rr(ldns_dnssec_rrsets *rrsets, ldns_rr *rr);

/**
 * Print the given list of rrsets to the fiven file descriptor
 * 
 * \param[in] out the file descriptor to print to
 * \param[in] rrsets the list of RRsets to print
 * \param[in] follow if set to false, only print the first RRset
 */ 
void ldns_dnssec_rrsets_print(FILE *out,
						ldns_dnssec_rrsets *rrsets,
						bool follow);

/**
 * Create a new data structure for a dnssec name
 * \return the allocated structure
 */
ldns_dnssec_name *ldns_dnssec_name_new();

/**
 * Create a new data structure for a dnssec name for the given RR
 *
 * \param[in] rr the RR to derive properties from, and to add to the name
 */
ldns_dnssec_name *ldns_dnssec_name_new_frm_rr(ldns_rr *rr);

/**
 * Frees the name structure and its rrs and rrsets.
 * Individual ldns_rr records therein are not freed
 *
 * \param[in] name the structure to free
 */
void ldns_dnssec_name_free(ldns_dnssec_name *name);

/**
 * Returns the domain name of the given dnssec_name structure
 *
 * \param[in] name the dnssec name to get the domain name from
 * \return the domain name
 */
ldns_rdf *ldns_dnssec_name_name(ldns_dnssec_name *name);


/**
 * Sets the domain name of the given dnssec_name structure
 *
 * \param[in] name the dnssec name to set the domain name of
 * \param[in] dname the domain name to set it to. This data is *not* copied.
 */
void ldns_dnssec_name_set_name(ldns_dnssec_name *name,
						 ldns_rdf *dname);

/**
 * Sets the NSEC(3) RR of the given dnssec_name structure
 *
 * \param[in] name the dnssec name to set the domain name of
 * \param[in] nsec the nsec rr to set it to. This data is *not* copied.
 */
void ldns_dnssec_name_set_nsec(ldns_dnssec_name *name, ldns_rr *nsec);

/**
 * Compares the domain names of the two arguments in their
 * canonical ordening.
 * 
 * \param[in] a The first dnssec_name to compare
 * \param[in] b The second dnssec_name to compare
 * \return -1 if the domain name of a comes before that of b in canonical
 *            ordening, 1 if it is the other way around, and 0 if they are
 *            equal
 */
int ldns_dnssec_name_cmp(const void *a, const void *b);

/**
 * Inserts the given rr at the right place in the current dnssec_name
 * No checking is done whether the name matches
 *
 * \param[in] name The ldns_dnssec_name to add the RR to
 * \param[in] rr The RR to add
 * \return LDNS_STATUS_OK on success, error code otherwise
 */
ldns_status ldns_dnssec_name_add_rr(ldns_dnssec_name *name,
							 ldns_rr *rr);

/**
 * Find the RRset with the given type in within this name structure
 *
 * \param[in] name the name to find the RRset in
 * \param[in] type the type of the RRset to find
 * \return the RRset, or NULL if not present
 */
ldns_dnssec_rrsets *ldns_dnssec_name_find_rrset(ldns_dnssec_name *name,
									   ldns_rr_type type);

/**
 * Find the RRset with the given name and type in the zone
 *
 * \param[in] zone the zone structure to find the RRset in
 * \param[in] dname the domain name of the RRset to find
 * \param[in] type the type of the RRset to find
 * \return the RRset, or NULL if not present
 */
ldns_dnssec_rrsets *ldns_dnssec_zone_find_rrset(ldns_dnssec_zone *zone,
									   ldns_rdf *dname,
									   ldns_rr_type type);

/**
 * Prints the RRs in the  dnssec name structure to the given
 * file descriptor
 *
 * \param[in] out the file descriptor to print to
 * \param[in] name the name structure to print the contents of
 */
void ldns_dnssec_name_print(FILE *out, ldns_dnssec_name *name);

/**
 * Creates a new dnssec_zone structure
 * \return the allocated structure
 */
ldns_dnssec_zone *ldns_dnssec_zone_new();

/**
 * Frees the given zone structure, and its rbtree of dnssec_names
 * Individual ldns_rr RRs within those names are *not* freed
 * \param[in] *zone the zone to free
 */ 
void ldns_dnssec_zone_free(ldns_dnssec_zone *zone);

/**
 * Adds the given RR to the zone.
 * It find whether there is a dnssec_name with that name present.
 * If so, add it to that, if not create a new one. 
 * Special handling of NSEC and RRSIG provided
 *
 * \param[in] zone the zone to add the RR to
 * \param[in] rr The RR to add
 * \return LDNS_STATUS_OK on success, an error code otherwise
 */
ldns_status ldns_dnssec_zone_add_rr(ldns_dnssec_zone *zone,
							 ldns_rr *rr);

/**
 * Prints the rbtree of ldns_dnssec_name structures to the file descriptor
 *
 * \param[in] out the file descriptor to print the names to
 * \param[in] tree the tree of ldns_dnssec_name structures to print
 * \param[in] print_soa if true, print SOA records, if false, skip them
 */
void ldns_dnssec_zone_names_print(FILE *out, ldns_rbtree_t *tree, bool print_soa);

/**
 * Prints the complete zone to the given file descriptor
 *
 * \param[in] out the file descriptor to print to
 * \param[in] zone the dnssec_zone to print
 */
void ldns_dnssec_zone_print(FILE *out, ldns_dnssec_zone *zone);

/**
 * Adds explicit dnssec_name structures for the empty nonterminals
 * in this zone. (this is needed for NSEC3 generation)
 *
 * \param[in] zone the zone to check for empty nonterminals
 * return LDNS_STATUS_OK on success.
 */
ldns_status ldns_dnssec_zone_add_empty_nonterminals(ldns_dnssec_zone *zone);

#endif
