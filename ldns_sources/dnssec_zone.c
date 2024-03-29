/*
 * special zone file structures and functions for better dnssec handling
 */

#include "ldns/config.h"

#include "ldns.h"

#ifdef HAVE_SSL
/* this entire file is rather useless when you don't have
 * crypto...
 */
ldns_dnssec_rrs *
ldns_dnssec_rrs_new()
{
	ldns_dnssec_rrs *new_rrs;
	new_rrs = LDNS_MALLOC(ldns_dnssec_rrs);
	new_rrs->rr = NULL;
	new_rrs->next = NULL;
	return new_rrs;
}

void
ldns_dnssec_rrs_free(ldns_dnssec_rrs *rrs)
{
	if (rrs) {
		if (rrs->next) {
			ldns_dnssec_rrs_free(rrs->next);
		}
		LDNS_FREE(rrs);
	}
}

ldns_status
ldns_dnssec_rrs_add_rr(ldns_dnssec_rrs *rrs, ldns_rr *rr)
{
	int cmp;
	ldns_dnssec_rrs *new_rrs;
	if (!rrs || !rr) {
		return LDNS_STATUS_ERR;
	}

	/* this could be done more efficiently; name and type should already
	   be equal */
	cmp = ldns_rr_compare(rrs->rr,
					  rr);
	/* should we error on equal? */
	if (cmp <= 0) {
		if (rrs->next) {
			ldns_dnssec_rrs_add_rr(rrs->next, rr);
		} else {
			new_rrs = ldns_dnssec_rrs_new();
			new_rrs->rr = rr;
			rrs->next = new_rrs;
		}
	} else if (cmp > 0) {
		/* put the current old rr in the new next, put the new
		   rr in the current container */
		new_rrs = ldns_dnssec_rrs_new();
		new_rrs->rr = rrs->rr;
		new_rrs->next = rrs->next;
		rrs->rr = rr;
		rrs->next = new_rrs;
	}
	return LDNS_STATUS_OK;
}

void
ldns_dnssec_rrs_print(FILE *out, ldns_dnssec_rrs *rrs)
{
	if (!rrs) {
		fprintf(out, "<void>");
	} else {
		if (rrs->rr) {
			ldns_rr_print(out, rrs->rr);
		}
		if (rrs->next) {
			ldns_dnssec_rrs_print(out, rrs->next);
		}
	}
}

ldns_dnssec_rrsets *
ldns_dnssec_rrsets_new()
{
	ldns_dnssec_rrsets *new_rrsets;
	new_rrsets = LDNS_MALLOC(ldns_dnssec_rrsets);
	new_rrsets->rrs = NULL;
	new_rrsets->type = 0;
	new_rrsets->signatures = NULL;
	new_rrsets->next = NULL;
	return new_rrsets;
}

void
ldns_dnssec_rrsets_free(ldns_dnssec_rrsets *rrsets)
{
	if (rrsets) {
		if (rrsets->rrs) {
			ldns_dnssec_rrs_free(rrsets->rrs);
		}
		if (rrsets->next) {
			ldns_dnssec_rrsets_free(rrsets->next);
		}
		if (rrsets->signatures) {
			ldns_dnssec_rrs_free(rrsets->signatures);
		}
		LDNS_FREE(rrsets);
	}
}

ldns_rr_type
ldns_dnssec_rrsets_type(ldns_dnssec_rrsets *rrsets)
{
	if (rrsets) {
		return rrsets->type;
	} else {
		return 0;
	}
}

ldns_status
ldns_dnssec_rrsets_set_type(ldns_dnssec_rrsets *rrsets,
					   ldns_rr_type type)
{
	if (rrsets) {
		rrsets->type = type;
		return LDNS_STATUS_OK;
	}
	return LDNS_STATUS_ERR;
}

ldns_dnssec_rrsets *
ldns_dnssec_rrsets_new_frm_rr(ldns_rr *rr)
{
	ldns_dnssec_rrsets *new_rrsets;
	ldns_rr_type rr_type;
	bool rrsig;

	new_rrsets = ldns_dnssec_rrsets_new();
	rr_type = ldns_rr_get_type(rr);
	if (rr_type == LDNS_RR_TYPE_RRSIG) {
		rrsig = true;
		rr_type = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));
	} else {
		rrsig = false;
	}
	if (!rrsig) {
		new_rrsets->rrs = ldns_dnssec_rrs_new();
		new_rrsets->rrs->rr = rr;
	} else {
		new_rrsets->signatures = ldns_dnssec_rrs_new();
		new_rrsets->signatures->rr = rr;
	}
	new_rrsets->type = rr_type;
	return new_rrsets;
}

ldns_status
ldns_dnssec_rrsets_add_rr(ldns_dnssec_rrsets *rrsets, ldns_rr *rr)
{
	ldns_dnssec_rrsets *new_rrsets;
	ldns_rr_type rr_type;
	bool rrsig = false;
	ldns_status result = LDNS_STATUS_OK;

	if (!rrsets || !rr) {
		return LDNS_STATUS_ERR;
	}

	rr_type = ldns_rr_get_type(rr);

	if (rr_type == LDNS_RR_TYPE_RRSIG) {
		rrsig = true;
		rr_type = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));
	}

	if (!rrsets->rrs && rrsets->type == 0 && !rrsets->signatures) {
		if (!rrsig) {
			rrsets->rrs = ldns_dnssec_rrs_new();
			rrsets->rrs->rr = rr;
			rrsets->type = rr_type;
		} else {
			rrsets->signatures = ldns_dnssec_rrs_new();
			rrsets->signatures->rr = rr;
			rrsets->type = rr_type;
		}
		return LDNS_STATUS_OK;
	}

	if (rr_type > ldns_dnssec_rrsets_type(rrsets)) {
		if (rrsets->next) {
			result = ldns_dnssec_rrsets_add_rr(rrsets->next, rr);
		} else {
			new_rrsets = ldns_dnssec_rrsets_new_frm_rr(rr);
			rrsets->next = new_rrsets;
		}
	} else if (rr_type < ldns_dnssec_rrsets_type(rrsets)) {
		/* move the current one into the new next, 
		   replace field of current with data from new rr */
		new_rrsets = ldns_dnssec_rrsets_new();
		new_rrsets->rrs = rrsets->rrs;
		new_rrsets->type = rrsets->type;
		new_rrsets->signatures = rrsets->signatures;
		new_rrsets->next = rrsets->next;
		if (!rrsig) {
			rrsets->rrs = ldns_dnssec_rrs_new();
			rrsets->rrs->rr = rr;
			rrsets->signatures = NULL;
		} else {
			rrsets->rrs = NULL;
			rrsets->signatures = ldns_dnssec_rrs_new();
			rrsets->signatures->rr = rr;
		}
		rrsets->type = rr_type;
		rrsets->next = new_rrsets;
	} else {
		/* equal, add to current rrsets */
		if (rrsig) {
			if (rrsets->signatures) {
				result = ldns_dnssec_rrs_add_rr(rrsets->signatures, rr);
			} else {
				rrsets->signatures = ldns_dnssec_rrs_new();
				rrsets->signatures->rr = rr;
			}
		} else {
			if (rrsets->rrs) {
				result = ldns_dnssec_rrs_add_rr(rrsets->rrs, rr);
			} else {
				rrsets->rrs = ldns_dnssec_rrs_new();
				rrsets->rrs->rr = rr;
			}
		}
	}

	return result;
}

void
ldns_dnssec_rrsets_print_soa(FILE *out,
					    ldns_dnssec_rrsets *rrsets,
					    bool follow,
					    bool show_soa)
{
	if (!rrsets) {
		fprintf(out, "<void>\n");
	} else {
		if (rrsets->rrs &&
		    (show_soa ||
			ldns_rr_get_type(rrsets->rrs->rr) != LDNS_RR_TYPE_SOA
		    )
		   ) {
			ldns_dnssec_rrs_print(out, rrsets->rrs);
			if (rrsets->signatures) {
				ldns_dnssec_rrs_print(out, rrsets->signatures);
			}
		}
		if (follow && rrsets->next) {
			ldns_dnssec_rrsets_print_soa(out, rrsets->next, follow, show_soa);
		}
	}
}

void
ldns_dnssec_rrsets_print(FILE *out, ldns_dnssec_rrsets *rrsets, bool follow)
{
	ldns_dnssec_rrsets_print_soa(out, rrsets, follow, true);
}

ldns_dnssec_name *
ldns_dnssec_name_new()
{
	ldns_dnssec_name *new_name;

	new_name = LDNS_MALLOC(ldns_dnssec_name);
	if (!new_name) {
		return NULL;
	}

	new_name->name = NULL;
	new_name->rrsets = NULL;
	new_name->name_alloced = false;
	new_name->nsec = NULL;
	new_name->nsec_signatures = NULL;

	new_name->is_glue = false;
	new_name->hashed_name = NULL;

	return new_name;
}

ldns_dnssec_name *
ldns_dnssec_name_new_frm_rr(ldns_rr *rr)
{
	ldns_dnssec_name *new_name = ldns_dnssec_name_new();

	new_name->name = ldns_rr_owner(rr);
	ldns_dnssec_name_add_rr(new_name, rr);

	return new_name;
}

void
ldns_dnssec_name_free(ldns_dnssec_name *name)
{
	if (name) {
		if (name->name_alloced) {
			ldns_rdf_deep_free(name->name);
		}
		if (name->rrsets) {
			ldns_dnssec_rrsets_free(name->rrsets);
		}
		if (name->nsec_signatures) {
			ldns_dnssec_rrs_free(name->nsec_signatures);
		}
		LDNS_FREE(name);
	}
}

ldns_rdf *
ldns_dnssec_name_name(ldns_dnssec_name *name)
{
	if (name) {
		return name->name;
	}
	return NULL;
}

void
ldns_dnssec_name_set_name(ldns_dnssec_name *rrset,
					 ldns_rdf *dname)
{
	if (rrset && dname) {
		rrset->name = dname;
	}
}

ldns_rr *
ldns_dnssec_name_nsec(ldns_dnssec_name *rrset)
{
	if (rrset) {
		return rrset->nsec;
	}
	return NULL;
}

void
ldns_dnssec_name_set_nsec(ldns_dnssec_name *rrset, ldns_rr *nsec)
{
	if (rrset && nsec) {
		rrset->nsec = nsec;
	}
}

int
ldns_dnssec_name_cmp(const void *a, const void *b)
{
	ldns_dnssec_name *na = (ldns_dnssec_name *) a;
	ldns_dnssec_name *nb = (ldns_dnssec_name *) b;

	if (na && nb) {
		return ldns_dname_compare(ldns_dnssec_name_name(na),
							 ldns_dnssec_name_name(nb));
	} else if (na) {
		return 1;
	} else if (nb) {
		return -1;
	} else {
		return 0;
	}
}


ldns_status
ldns_dnssec_name_add_rr(ldns_dnssec_name *name,
				    ldns_rr *rr)
{
	ldns_status result = LDNS_STATUS_OK;
	ldns_rdf *name_name;
	bool hashed_name = false;
	ldns_rr_type rr_type = ldns_rr_get_type(rr);
	ldns_rr_type typecovered = 0;

	/* special handling for NSEC3 and NSECX covering RRSIGS */

	if (!name || !rr) {
		return LDNS_STATUS_ERR;
	}

	rr_type = ldns_rr_get_type(rr);

	if (rr_type == LDNS_RR_TYPE_RRSIG) {
		typecovered = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));
	}

	if (rr_type == LDNS_RR_TYPE_NSEC3 ||
	    typecovered == LDNS_RR_TYPE_NSEC3) {
		name_name = ldns_nsec3_hash_name_frm_nsec3(rr,
										   ldns_dnssec_name_name(name));
		hashed_name = true;
	} else {
		name_name = ldns_dnssec_name_name(name);
	}

	if (rr_type == LDNS_RR_TYPE_NSEC ||
	    rr_type == LDNS_RR_TYPE_NSEC3) {
		/* XX check if is already set (and error?) */
		name->nsec = rr;
	} else if (typecovered == LDNS_RR_TYPE_NSEC ||
			 typecovered == LDNS_RR_TYPE_NSEC3) {
		if (name->nsec_signatures) {
			ldns_dnssec_rrs_add_rr(name->nsec_signatures, rr);
		} else {
			name->nsec_signatures = ldns_dnssec_rrs_new();
			name->nsec_signatures->rr = rr;
		}
	} else {
		/* it's a 'normal' RR, add it to the right rrset */
		if (name->rrsets) {
			result = ldns_dnssec_rrsets_add_rr(name->rrsets, rr);
		} else {
			name->rrsets = ldns_dnssec_rrsets_new();
			result = ldns_dnssec_rrsets_add_rr(name->rrsets, rr);
		}
	}

	if (hashed_name) {
		ldns_rdf_deep_free(name_name);
	}

	return result;
}

ldns_dnssec_rrsets *
ldns_dnssec_name_find_rrset(ldns_dnssec_name *name,
					   ldns_rr_type type) {
	ldns_dnssec_rrsets *result;

	result = name->rrsets;
	while (result) {
		if (result->type == type) {
			return result;
		} else {
			result = result->next;
		}
	}
	return NULL;
}

ldns_dnssec_rrsets *
ldns_dnssec_zone_find_rrset(ldns_dnssec_zone *zone,
					   ldns_rdf *dname,
					   ldns_rr_type type)
{
	ldns_rbnode_t *node;

	if (!zone || !dname) {
		return NULL;
	}

	node = ldns_rbtree_search(zone->names, dname);
	if (node) {
		return ldns_dnssec_name_find_rrset((ldns_dnssec_name *)node->data,
									type);
	} else {
		return NULL;
	}
}

static inline void
print_indent(FILE *out, int c)
{
	int i;
	for (i=0; i<c; i++) {
		fprintf(out, "    ");
	}
}

void
ldns_dnssec_name_print_soa(FILE *out, ldns_dnssec_name *name, bool show_soa)
{
	if (name) {
		if(name->rrsets) {
			ldns_dnssec_rrsets_print_soa(out, name->rrsets, true, show_soa);
		} else {
			fprintf(out, ";; Empty nonterminal: ");
			ldns_rdf_print(out, name->name);
			fprintf(out, "\n");
		}
		if(name->nsec) {
			ldns_rr_print(out, name->nsec);
		}
		if (name->nsec_signatures) {
			ldns_dnssec_rrs_print(out, name->nsec_signatures);
		}
	} else {
		fprintf(out, "<void>\n");
	}
}

void
ldns_dnssec_name_print(FILE *out, ldns_dnssec_name *name)
{
	ldns_dnssec_name_print_soa(out, name, true);
}

ldns_dnssec_zone *
ldns_dnssec_zone_new()
{
	ldns_dnssec_zone *zone = LDNS_MALLOC(ldns_dnssec_zone);
	zone->soa = NULL;
	zone->names = NULL;

	return zone;
}

void
ldns_dnssec_name_node_free(ldns_rbnode_t *node, void *arg) {
	arg = arg;
	ldns_dnssec_name_free((ldns_dnssec_name *)node->data);
	free(node);
}

void
ldns_dnssec_zone_free(ldns_dnssec_zone *zone)
{
	if (zone) {
		if (zone->names) {
			/* destroy all name structures within the tree */
			ldns_traverse_postorder(zone->names,
						    ldns_dnssec_name_node_free,
						    NULL);
			free(zone->names);
		}
		LDNS_FREE(zone);
	}
}

/* use for dname comparison in tree */
int
ldns_dname_compare_v(const void *a, const void *b) {
	return ldns_dname_compare((ldns_rdf *)a, (ldns_rdf *)b);
}


ldns_rbnode_t *
ldns_dnssec_zone_find_nsec3_original(ldns_dnssec_zone *zone,
							  ldns_rr *rr) {
	ldns_rbnode_t *current_node = ldns_rbtree_first(zone->names);
	ldns_dnssec_name *current_name;
	ldns_rdf *hashed_name;

	hashed_name = ldns_dname_label(ldns_rr_owner(rr), 0);

	while (current_node != LDNS_RBTREE_NULL) {
		current_name = (ldns_dnssec_name *) current_node->data;

		if (!current_name->hashed_name) {
			current_name->hashed_name =
				ldns_nsec3_hash_name_frm_nsec3(rr, current_name->name);
		}
		if (ldns_dname_compare(hashed_name,
						   current_name->hashed_name)
		    == 0) {
			return current_node;
		}
		current_node = ldns_rbtree_next(current_node);
	}
	return NULL;
}

ldns_status
ldns_dnssec_zone_add_rr(ldns_dnssec_zone *zone, ldns_rr *rr)
{
	ldns_status result = LDNS_STATUS_OK;
	ldns_dnssec_name *cur_name;
	ldns_rbnode_t *cur_node;
	ldns_rr_type type_covered = 0;

	if (!zone || !rr) {
		return LDNS_STATUS_ERR;
	}

	if (!zone->names) {
		zone->names = ldns_rbtree_create(ldns_dname_compare_v);
	}
	
	/* we need the original of the hashed name if this is
	   an NSEC3, or an RRSIG that covers an NSEC3 */
	if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_RRSIG) {
		type_covered = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));;
	}
	if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_NSEC3 ||
	    type_covered == LDNS_RR_TYPE_NSEC3) {
		cur_node = ldns_dnssec_zone_find_nsec3_original(zone,
					 						   rr);
		if (!cur_node) {
			return LDNS_STATUS_DNSSEC_NSEC3_ORIGINAL_NOT_FOUND;
		}
	} else {
		cur_node = ldns_rbtree_search(zone->names, ldns_rr_owner(rr));
	}

	if (!cur_node) {
		/* add */
		cur_name = ldns_dnssec_name_new_frm_rr(rr);
		cur_node = LDNS_MALLOC(ldns_rbnode_t);
		cur_node->key = ldns_rr_owner(rr);
		cur_node->data = cur_name;
		ldns_rbtree_insert(zone->names, cur_node);
	} else {
		cur_name = (ldns_dnssec_name *) cur_node->data;
		ldns_dnssec_name_add_rr(cur_name, rr);
	}

	if (result != LDNS_STATUS_OK) {
		fprintf(stderr, "error adding rr: ");
		ldns_rr_print(stderr, rr);
	}

	/*TODO ldns_dnssec_name_print_names(stdout, zone->names, 0);*/
	if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_SOA) {
		zone->soa = cur_name;
	}

	return result;
}

void
ldns_dnssec_zone_names_print(FILE *out, ldns_rbtree_t *tree, bool print_soa)
{
	ldns_rbnode_t *node;
	ldns_dnssec_name *name;

	node = ldns_rbtree_first(tree);
	while (node != LDNS_RBTREE_NULL) {
		name = (ldns_dnssec_name *) node->data;
		ldns_dnssec_name_print_soa(out, name, print_soa);
		fprintf(out, ";\n");
		node = ldns_rbtree_next(node);
	}
}

void
ldns_dnssec_zone_print(FILE *out, ldns_dnssec_zone *zone)
{
	if (zone) {
		if (zone->soa) {
			fprintf(out, ";; Zone: ");
			ldns_rdf_print(out, ldns_dnssec_name_name(zone->soa));
			fprintf(out, "\n;\n");
			ldns_dnssec_rrsets_print(
			    out,
			    ldns_dnssec_name_find_rrset(zone->soa,
									  LDNS_RR_TYPE_SOA),
			    false);
			fprintf(out, ";\n");
		}

		if (zone->names) {
			ldns_dnssec_zone_names_print(out, zone->names, false);
		}
	}
}

ldns_status
ldns_dnssec_zone_add_empty_nonterminals(ldns_dnssec_zone *zone)
{
	ldns_dnssec_name *new_name;
	ldns_dnssec_name *cur_name;
	ldns_dnssec_name *next_name;
	ldns_rbnode_t *cur_node, *next_node, *new_node;

	/* for the detection */
	uint16_t j, cur_label_count, next_label_count;
	ldns_rdf *l1, *l2, *post, *post2;
	bool found_difference = false;

	if (!zone) {
		return LDNS_STATUS_ERR;
	}
	cur_node = ldns_rbtree_first(zone->names);
	while (cur_node != LDNS_RBTREE_NULL) {
		next_node = ldns_rbtree_next(cur_node);
		if (next_node == LDNS_RBTREE_NULL) {
			next_node = ldns_rbtree_first(zone->names);
		}

		cur_name = (ldns_dnssec_name *)cur_node->data;
		next_name = (ldns_dnssec_name *)next_node->data;

		found_difference = false;

		cur_label_count = ldns_dname_label_count(cur_name->name);
		next_label_count = ldns_dname_label_count(next_name->name);

		post = ldns_dname_new_frm_str(".");
		for (j = 1 + ldns_dname_label_count(zone->soa->name); 
			j < cur_label_count && 
				j <= next_label_count && 
				!found_difference;
			j++) {
			l1 = ldns_dname_label(cur_name->name, cur_label_count - j);
			l2 = ldns_dname_label(next_name->name, next_label_count - j);
			post2 = ldns_dname_cat_clone(l2, post);
			ldns_rdf_deep_free(post);
			post = post2;

			if (ldns_dname_compare(l1, l2) != 0 &&
			    j < next_label_count) {
				found_difference = true;
				
				new_name = ldns_dnssec_name_new();
				new_name->name = ldns_rdf_clone(post);
				new_name->name_alloced = true;
				new_node = LDNS_MALLOC(ldns_rbnode_t);
				new_node->key = new_name->name;
				new_node->data = new_name;
				ldns_rbtree_insert(zone->names, new_node);
				//ldns_dnssec_zone_add_name(zone, new_name);
			}
			ldns_rdf_deep_free(l1);
			ldns_rdf_deep_free(l2);
		}
		/* and if next label is longer than cur + 1, these must be empty nons too */
		/* skip current label (total now equal to cur_dname) */
		if (!found_difference && j < cur_label_count && j <= next_label_count) {
			l2 = ldns_dname_label(next_name->name, next_label_count - j);
			post2 = ldns_dname_cat_clone(l2, post);
			ldns_rdf_deep_free(post);
			post = post2;
			j++;
		}
		while (j < next_label_count) {
			l2 = ldns_dname_label(next_name->name, next_label_count - j);
			post2 = ldns_dname_cat_clone(l2, post);
			ldns_rdf_deep_free(post);
			post = post2;
			ldns_dname_cat(post, ldns_dnssec_name_name(zone->soa));
			/*
			  printf("Found empty non-terminal: ");
			  ldns_rdf_print(stdout, post);
			  printf("\n");
			*/
			ldns_rdf_deep_free(l2);
			j++;    
			new_name = ldns_dnssec_name_new();
			new_name->name = ldns_rdf_clone(post);
			new_name->name_alloced = true;
			new_node = LDNS_MALLOC(ldns_rbnode_t);
			new_node->key = new_name->name;
			new_node->data = new_name;
			ldns_rbtree_insert(zone->names, new_node);
		}
		ldns_rdf_deep_free(post);
		/* TODO: other name->name (we need to free this one) */
		

		cur_node = ldns_rbtree_next(cur_node);
	}
	return LDNS_STATUS_OK;
}
#endif