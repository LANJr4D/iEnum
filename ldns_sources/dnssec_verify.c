#include "ldns/config.h"

#include "ldns.h"

#include <strings.h>
#include <time.h>

#ifdef HAVE_SSL
/* this entire file is rather useless when you don't have
 * crypto...
 */
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/md5.h>

ldns_dnssec_data_chain *
ldns_dnssec_data_chain_new()
{
	ldns_dnssec_data_chain *nc = LDNS_XMALLOC(ldns_dnssec_data_chain, 1);
	nc->rrset = NULL;
	nc->parent_type = 0;
	nc->parent = NULL;
	nc->signatures = NULL;
	nc->packet_rcode = 0;
	nc->packet_qtype = 0;
	nc->packet_nodata = false;
	return nc;
}

void
ldns_dnssec_data_chain_free(ldns_dnssec_data_chain *chain)
{
	LDNS_FREE(chain);
}

void
ldns_dnssec_data_chain_deep_free(ldns_dnssec_data_chain *chain)
{
	ldns_rr_list_deep_free(chain->rrset);
	ldns_rr_list_deep_free(chain->signatures);
	if (chain->parent) {
		ldns_dnssec_data_chain_deep_free(chain->parent);
	}
	LDNS_FREE(chain);
}

void
ldns_dnssec_data_chain_print(FILE *out, const ldns_dnssec_data_chain *chain)
{
	ldns_lookup_table *rcode;
	const ldns_rr_descriptor *rr_descriptor;
	if (chain) {
		ldns_dnssec_data_chain_print(out, chain->parent);
		if (ldns_rr_list_rr_count(chain->rrset) > 0) {
			rcode = ldns_lookup_by_id(ldns_rcodes,
								 (int) chain->packet_rcode);
			if (rcode) {
				fprintf(out, ";; rcode: %s\n", rcode->name);
			}

			rr_descriptor = ldns_rr_descript(chain->packet_qtype);
			if (rr_descriptor && rr_descriptor->_name) {
				fprintf(out, ";; qtype: %s\n", rr_descriptor->_name);
			} else if (chain->packet_qtype != 0) {
				fprintf(out, "TYPE%u", 
					   chain->packet_qtype);
			}
			if (chain->packet_nodata) {
				fprintf(out, ";; NODATA response\n");
			}
			fprintf(out, "rrset: %p\n", chain->rrset);
			ldns_rr_list_print(out, chain->rrset);
			fprintf(out, "sigs: %p\n", chain->signatures);
			ldns_rr_list_print(out, chain->signatures);
			fprintf(out, "---\n");
		} else {
			fprintf(out, "<no data>\n");
		}
	}
}

ldns_dnssec_data_chain *
ldns_dnssec_build_data_chain(ldns_resolver *res,
					    uint16_t qflags,
					    const ldns_rr_list *rrset,
					    const ldns_pkt *pkt,
					    ldns_rr *orig_rr)
{
	ldns_rr_list *signatures = NULL, *signatures2 = NULL;
	ldns_rr_list *keys;
	ldns_rr_list *dss;
	
	ldns_rr_list *my_rrset;

	ldns_pkt *my_pkt;

	ldns_rdf *name = NULL, *key_name = NULL;
	ldns_rdf *possible_parent_name;
	ldns_rr_type type = 0;
	ldns_rr_class c = 0;

	bool other_rrset = false;
	
	ldns_dnssec_data_chain *new_chain = ldns_dnssec_data_chain_new();

	if (!ldns_dnssec_pkt_has_rrsigs(pkt)) {
		return new_chain;
	}

	if (orig_rr) {
		new_chain->rrset = ldns_rr_list_new();
		ldns_rr_list_push_rr(new_chain->rrset, orig_rr);
		new_chain->parent = ldns_dnssec_build_data_chain(res,
											    qflags,
											    rrset,
											    pkt,
											    NULL);
		new_chain->packet_rcode = ldns_pkt_get_rcode(pkt);
		new_chain->packet_qtype = ldns_rr_get_type(orig_rr);
		if (ldns_pkt_ancount(pkt) == 0) {
			new_chain->packet_nodata = true;
		}
		return new_chain;
	}
	
	if (!rrset || ldns_rr_list_rr_count(rrset) < 1) {
		/* hmm, no data, do we have denial? only works if pkt was given,
		   otherwise caller has to do the check himself */
		new_chain->packet_nodata = true;
		if (pkt) {

			my_rrset = ldns_pkt_rr_list_by_type(pkt,
										 LDNS_RR_TYPE_NSEC,
										 LDNS_SECTION_ANY_NOQUESTION
										 );
			if (my_rrset) {
				if (ldns_rr_list_rr_count(my_rrset) > 0) {
					type = LDNS_RR_TYPE_NSEC;
					other_rrset = true;
				} else {
					ldns_rr_list_deep_free(my_rrset);
					my_rrset = NULL;
				}
			} else {
				/* nothing, try nsec3 */
				my_rrset = ldns_pkt_rr_list_by_type(pkt,
						     LDNS_RR_TYPE_NSEC3,
							LDNS_SECTION_ANY_NOQUESTION);
				if (my_rrset) {
					if (ldns_rr_list_rr_count(my_rrset) > 0) {
						type = LDNS_RR_TYPE_NSEC3;
						other_rrset = true;
					} else {
						ldns_rr_list_deep_free(my_rrset);
						my_rrset = NULL;
					}
				} else {
					/* nothing, stop */
					return new_chain;
				}
			}
		} else {
			return new_chain;
		}
	} else {
		my_rrset = (ldns_rr_list *) rrset;
	}
	
	if (my_rrset && ldns_rr_list_rr_count(my_rrset) > 0) {
		new_chain->rrset = ldns_rr_list_clone(my_rrset);
		name = ldns_rr_owner(ldns_rr_list_rr(my_rrset, 0));
		type = ldns_rr_get_type(ldns_rr_list_rr(my_rrset, 0));
		c = ldns_rr_get_class(ldns_rr_list_rr(my_rrset, 0));
	}
	
	if (other_rrset) {
		ldns_rr_list_deep_free(my_rrset);
	}
	
	/* normally there will only be 1 signature 'set'
	   but there can be more than 1 denial (wildcards)
	   so check for NSEC
	*/
	if (type == LDNS_RR_TYPE_NSEC || type == LDNS_RR_TYPE_NSEC3) {
		/* just throw in all signatures, the tree builder must sort
		   this out */
		if (pkt) {
			signatures = ldns_dnssec_pkt_get_rrsigs_for_type(pkt, type);
		} else {
			my_pkt = ldns_resolver_query(res, name, type, c, qflags);
			signatures = ldns_dnssec_pkt_get_rrsigs_for_type(pkt, type);
			ldns_pkt_free(my_pkt);
		}
	} else {
		if (pkt) {
			signatures =
				ldns_dnssec_pkt_get_rrsigs_for_name_and_type(pkt,
													name,
													type);
		}
		if (!signatures) {
			my_pkt = ldns_resolver_query(res, name, type, c, qflags);
			signatures =
				ldns_dnssec_pkt_get_rrsigs_for_name_and_type(my_pkt,
													name,
													type);
			ldns_pkt_free(my_pkt);
		}

	}

	if (signatures && ldns_rr_list_rr_count(signatures) > 0) {
		key_name = ldns_rr_rdf(ldns_rr_list_rr(signatures, 0), 7);
	}

	if (!key_name) {
		/* apparently we were not able to find a signing key, so
		   we assume the chain ends here
		*/
		/* try parents for auth denial of DS */
		if (orig_rr) {
			possible_parent_name = ldns_rr_owner(orig_rr);
		} else if (rrset && ldns_rr_list_rr_count(rrset) > 0) {
			possible_parent_name = ldns_rr_owner(ldns_rr_list_rr(rrset, 0));
		} else {
			/* no information to go on, give up */
			printf("[XX] not enough information to go on\n");
			return new_chain;
		}

		my_pkt = ldns_resolver_query(res,
							    possible_parent_name,
							    LDNS_RR_TYPE_DS,
							    LDNS_RR_CLASS_IN,
							    qflags);

		if (ldns_pkt_ancount(my_pkt) > 0) {
			/* add error, no sigs but DS in parent */
			/*ldns_pkt_print(stdout, my_pkt);*/
			ldns_pkt_free(my_pkt);
		} else {
			/* are there signatures? */
			new_chain->parent =  ldns_dnssec_build_data_chain(res, 
													qflags, 
													NULL,
													my_pkt,
													NULL);

			new_chain->parent->packet_qtype = LDNS_RR_TYPE_DS;
			//ldns_pkt_free(my_pkt);
		}
		return new_chain;
	}

	if (type != LDNS_RR_TYPE_DNSKEY) {
		if (signatures && ldns_rr_list_rr_count(signatures) > 0) {
			new_chain->signatures = ldns_rr_list_clone(signatures);
			new_chain->parent_type = 0;

			keys = ldns_pkt_rr_list_by_name_and_type(
				      pkt,
					 key_name,
					 LDNS_RR_TYPE_DNSKEY,
					 LDNS_SECTION_ANY_NOQUESTION
				  );
			if (!keys) {
				my_pkt = ldns_resolver_query(res,
									    key_name,
									    LDNS_RR_TYPE_DNSKEY,
									    c,
									    qflags);
				keys = ldns_pkt_rr_list_by_name_and_type(
					      my_pkt,
						 key_name,
						 LDNS_RR_TYPE_DNSKEY,
						 LDNS_SECTION_ANY_NOQUESTION
					  );
				new_chain->parent = ldns_dnssec_build_data_chain(res,
													    qflags,
													    keys,
													    my_pkt,
													    NULL);
				new_chain->parent->packet_qtype = LDNS_RR_TYPE_DNSKEY;
				ldns_pkt_free(my_pkt);
			} else {
				new_chain->parent = ldns_dnssec_build_data_chain(res,
													    qflags,
													    keys,
													    pkt,
													    NULL);
				new_chain->parent->packet_qtype = LDNS_RR_TYPE_DNSKEY;
			}
			ldns_rr_list_deep_free(keys);
		}
	} else {
		/* 'self-signed', parent is a DS */
		
		/* okay, either we have other keys signing the current one,
		 * or the current
		 * one should have a DS record in the parent zone.
		 * How do we find this out? Try both?
		 *
		 * request DNSKEYS for current zone,
		 * add all signatures to current level
		 */
		new_chain->parent_type = 1;

		my_pkt = ldns_resolver_query(res,
							    key_name,
							    LDNS_RR_TYPE_DS,
							    c,
							    qflags);
		dss = ldns_pkt_rr_list_by_name_and_type(my_pkt,
										key_name,
										LDNS_RR_TYPE_DS,
										LDNS_SECTION_ANY_NOQUESTION
										);
		if (dss) {
			new_chain->parent = ldns_dnssec_build_data_chain(res,
												    qflags,
												    dss,
												    my_pkt,
												    NULL);
			new_chain->parent->packet_qtype = LDNS_RR_TYPE_DS;
			ldns_rr_list_deep_free(dss);
		}
		ldns_pkt_free(my_pkt);

		my_pkt = ldns_resolver_query(res,
							    key_name,
							    LDNS_RR_TYPE_DNSKEY,
							    c,
							    qflags);
		signatures2 = ldns_pkt_rr_list_by_name_and_type(my_pkt,
											   key_name,
											   LDNS_RR_TYPE_RRSIG,
											   LDNS_SECTION_ANSWER);
		if (signatures2) {
			if (new_chain->signatures) {
				printf("There were already sigs!\n");
				ldns_rr_list_deep_free(new_chain->signatures);
				printf("freed\n");
			}
			new_chain->signatures = signatures2;
		}
		ldns_pkt_free(my_pkt);
	}
	if (signatures) {
		ldns_rr_list_deep_free(signatures);
	}

	return new_chain;
}

ldns_dnssec_trust_tree *
ldns_dnssec_trust_tree_new()
{
	ldns_dnssec_trust_tree *new_tree = LDNS_XMALLOC(ldns_dnssec_trust_tree,
										   1);
	new_tree->rr = NULL;
	new_tree->rrset = NULL;
	new_tree->parent_count = 0;

	return new_tree;
}

void
ldns_dnssec_trust_tree_free(ldns_dnssec_trust_tree *tree)
{
	size_t i;
	if (tree) {
		for (i = 0; i < tree->parent_count; i++) {
			ldns_dnssec_trust_tree_free(tree->parents[i]);
		}
	}
	LDNS_FREE(tree);
}

size_t
ldns_dnssec_trust_tree_depth(ldns_dnssec_trust_tree *tree)
{
	size_t result = 0;
	size_t parent = 0;
	size_t i;
	
	for (i = 0; i < tree->parent_count; i++) {
		parent = ldns_dnssec_trust_tree_depth(tree->parents[i]);
		if (parent > result) {
			result = parent;
		}
	}
	return 1 + result;
}

/* TODO ldns_ */
static void
print_tabs(FILE *out, size_t nr, uint8_t *map, size_t treedepth)
{
	size_t i;
	for (i = 0; i < nr; i++) {
		if (i == nr - 1) {
			fprintf(out, "|---");
		} else if (map && i < treedepth && map[i] == 1) {
			fprintf(out, "|   ");
		} else {
			fprintf(out, "    ");
		}
	}
}

void
ldns_dnssec_trust_tree_print_sm(FILE *out,
						  ldns_dnssec_trust_tree *tree,
						  size_t tabs,
						  bool extended,
						  uint8_t *sibmap,
						  size_t treedepth)
{
	size_t i;
	const ldns_rr_descriptor *descriptor;
	bool mapset = false;
	
	if (!sibmap) {
		treedepth = ldns_dnssec_trust_tree_depth(tree);
		sibmap = malloc(treedepth);
		memset(sibmap, 0, treedepth);
		mapset = true;
	}
	
	if (tree) {
		if (tree->rr) {
			print_tabs(out, tabs, sibmap, treedepth);
			ldns_rdf_print(out, ldns_rr_owner(tree->rr));
			descriptor = ldns_rr_descript(ldns_rr_get_type(tree->rr));

			if (descriptor->_name) {
				fprintf(out, " (%s", descriptor->_name);
			} else {
				fprintf(out, " (TYPE%d", 
					   ldns_rr_get_type(tree->rr));
			}
			if (ldns_rr_get_type(tree->rr) == LDNS_RR_TYPE_DNSKEY) {
				fprintf(out, " keytag: %u", ldns_calc_keytag(tree->rr));
			} else if (ldns_rr_get_type(tree->rr) == LDNS_RR_TYPE_DS) {
				fprintf(out, " keytag: ");
				ldns_rdf_print(out, ldns_rr_rdf(tree->rr, 0));
			}
			if (ldns_rr_get_type(tree->rr) == LDNS_RR_TYPE_NSEC) {
				fprintf(out, " ");
				ldns_rdf_print(out, ldns_rr_rdf(tree->rr, 0));
				fprintf(out, " ");
				ldns_rdf_print(out, ldns_rr_rdf(tree->rr, 1));
			}
			
			
			fprintf(out, ")\n");
			for (i = 0; i < tree->parent_count; i++) {
				if (tree->parent_count > 1 && i < tree->parent_count - 1) {
					sibmap[tabs] = 1;
				} else {
					sibmap[tabs] = 0;
				}
				/* only print errors */
				if (ldns_rr_get_type(tree->parents[i]->rr) == 
				    LDNS_RR_TYPE_NSEC ||
				    ldns_rr_get_type(tree->parents[i]->rr) ==
				    LDNS_RR_TYPE_NSEC3) {
					if (tree->parent_status[i] == LDNS_STATUS_OK) {
						print_tabs(out, tabs + 1, sibmap, treedepth);
						fprintf(out, "Existence is denied by:\n");
					} else {
						print_tabs(out, tabs + 1, sibmap, treedepth);
						fprintf(out,
							   "Error in denial of existence: %s\n",
							   ldns_get_errorstr_by_id(
							       tree->parent_status[i]));
					}
				} else
					if (tree->parent_status[i] != LDNS_STATUS_OK) {
						print_tabs(out, tabs + 1, sibmap, treedepth);
						fprintf(out,
							   "%s:\n",
							   ldns_get_errorstr_by_id(
							       tree->parent_status[i]));
						if (tree->parent_status[i]
						    == LDNS_STATUS_SSL_ERR) {
							printf("; SSL Error: ");
							ERR_load_crypto_strings();
							ERR_print_errors_fp(stdout);
							printf("\n");
						}
						ldns_rr_print(out, tree->parent_signature[i]);
						printf("For RRset:\n");
						ldns_rr_list_print(out, tree->rrset);
						printf("With key:\n");
						ldns_rr_print(out, tree->parents[i]->rr);
					}
				ldns_dnssec_trust_tree_print_sm(out,
										  tree->parents[i],
										  tabs+1,
										  extended,
										  sibmap,
										  treedepth);
			}
		} else {
			print_tabs(out, tabs, sibmap, treedepth);
			fprintf(out, "<no data>\n");
		}
	} else {
		fprintf(out, "<null pointer>\n");
	}
	
	if (mapset) {
		free(sibmap);
	}
}

void
ldns_dnssec_trust_tree_print(FILE *out,
					    ldns_dnssec_trust_tree *tree,
					    size_t tabs,
					    bool extended)
{
	ldns_dnssec_trust_tree_print_sm(out, tree, tabs, extended, NULL, 0);
}

ldns_status
ldns_dnssec_trust_tree_add_parent(ldns_dnssec_trust_tree *tree,
                                  const ldns_dnssec_trust_tree *parent,
                                  const ldns_rr *signature,
                                  const ldns_status parent_status)
{
	if (tree
	    && parent
	    && tree->parent_count < LDNS_DNSSEC_TRUST_TREE_MAX_PARENTS) {
		/*
		  printf("Add parent for: ");
		  ldns_rr_print(stdout, tree->rr);
		  printf("parent: ");
		  ldns_rr_print(stdout, parent->rr);
		*/
		tree->parents[tree->parent_count] =
			(ldns_dnssec_trust_tree *) parent;
		tree->parent_status[tree->parent_count] = parent_status;
		tree->parent_signature[tree->parent_count] = (ldns_rr *) signature;
		tree->parent_count++;
		return LDNS_STATUS_OK;
	} else {
		return LDNS_STATUS_ERR;
	}
}

/* if rr is null, take the first from the rrset */
ldns_dnssec_trust_tree *
ldns_dnssec_derive_trust_tree(ldns_dnssec_data_chain *data_chain, ldns_rr *rr)
{
	ldns_rr_list *cur_rrset;
	ldns_rr_list *cur_sigs;
	ldns_rr *cur_rr = NULL;
	ldns_rr *cur_sig_rr;
	uint16_t cur_keytag;
	size_t i, j;

	ldns_dnssec_trust_tree *new_tree = ldns_dnssec_trust_tree_new();
	
	if (data_chain && data_chain->rrset) {
		cur_rrset = data_chain->rrset;
	
		cur_sigs = data_chain->signatures;

		if (rr) {
			cur_rr = rr;
		}

		if (!cur_rr && ldns_rr_list_rr_count(cur_rrset) > 0) {
			cur_rr = ldns_rr_list_rr(cur_rrset, 0);
		}

		if (cur_rr) {
			new_tree->rr = cur_rr;
			new_tree->rrset = cur_rrset;
			/* there are three possibilities:
			   1 - 'normal' rrset, signed by a key
			   2 - dnskey signed by other dnskey
			   3 - dnskey proven by higher level DS
			   (data denied by nsec is a special case that can
			   occur in multiple places)
				   
			*/
			if (cur_sigs) {
				for (i = 0; i < ldns_rr_list_rr_count(cur_sigs); i++) {
					/* find the appropriate key in the parent list */
					cur_sig_rr = ldns_rr_list_rr(cur_sigs, i);
					cur_keytag = 
						ldns_rdf2native_int16(
						    ldns_rr_rrsig_keytag(cur_sig_rr));

					if (ldns_rr_get_type(cur_rr) == LDNS_RR_TYPE_NSEC) {
						if (ldns_dname_compare(ldns_rr_owner(cur_sig_rr),
										   ldns_rr_owner(cur_rr)))
							{
								/* find first that does match */

								for (j = 0;
								     j < ldns_rr_list_rr_count(cur_rrset) && 
										ldns_dname_compare(ldns_rr_owner(cur_sig_rr),ldns_rr_owner(cur_rr)) != 0;
								     j++) {
									cur_rr = ldns_rr_list_rr(cur_rrset, j);
									
								}
								if (ldns_dname_compare(ldns_rr_owner(cur_sig_rr), 
												   ldns_rr_owner(cur_rr)))
									{
										break;
									}
							}
							
					}
					/* option 1 */
					if (data_chain->parent) {
						ldns_dnssec_derive_trust_tree_normal_rrset(
						    new_tree,
						    data_chain,
						    cur_sig_rr);
					}

					/* option 2 */
					ldns_dnssec_derive_trust_tree_dnskey_rrset(
					    new_tree,
					    data_chain,
					    cur_rr,
					    cur_sig_rr);
				}
					
				ldns_dnssec_derive_trust_tree_ds_rrset(new_tree,
											    data_chain,
											    cur_rr);
			} else {
				/* no signatures? maybe it's nsec data */
					
				/* just add every rr from parent as new parent */
				ldns_dnssec_derive_trust_tree_no_sig(new_tree, data_chain);
			}
		}
	}

	return new_tree;
}

void
ldns_dnssec_derive_trust_tree_normal_rrset(ldns_dnssec_trust_tree *new_tree,
                                           ldns_dnssec_data_chain *data_chain,
                                           ldns_rr *cur_sig_rr)
{
	size_t i, j;
	ldns_rr_list *cur_rrset = ldns_rr_list_clone(data_chain->rrset); 
	ldns_dnssec_trust_tree *cur_parent_tree;
	ldns_rr *cur_parent_rr;
	int cur_keytag;
	ldns_rr_list *tmp_rrset = NULL;
	ldns_status cur_status;

	cur_keytag = ldns_rdf2native_int16(ldns_rr_rrsig_keytag(cur_sig_rr));
	
	for (j = 0; j < ldns_rr_list_rr_count(data_chain->parent->rrset); j++) {
		cur_parent_rr = ldns_rr_list_rr(data_chain->parent->rrset, j);
		if (ldns_rr_get_type(cur_parent_rr) == LDNS_RR_TYPE_DNSKEY) {
			if (ldns_calc_keytag(cur_parent_rr) == cur_keytag) {

				/* TODO: check wildcard nsec too */
				if (cur_rrset && ldns_rr_list_rr_count(cur_rrset) > 0) {
					tmp_rrset = cur_rrset;
					if (ldns_rr_get_type(ldns_rr_list_rr(cur_rrset, 0))
					    == LDNS_RR_TYPE_NSEC ||
					    ldns_rr_get_type(ldns_rr_list_rr(cur_rrset, 0))
					    == LDNS_RR_TYPE_NSEC3) {
						/* might contain different names! 
						   sort and split */
						ldns_rr_list_sort(cur_rrset);
						if (tmp_rrset && tmp_rrset != cur_rrset) {
							ldns_rr_list_deep_free(tmp_rrset);
							tmp_rrset = NULL;
						}
						tmp_rrset = ldns_rr_list_pop_rrset(cur_rrset);
						
						/* with nsecs, this might be the wrong one */
						while (tmp_rrset &&
						       ldns_rr_list_rr_count(cur_rrset) > 0 &&
						       ldns_dname_compare(
								ldns_rr_owner(ldns_rr_list_rr(
										        tmp_rrset, 0)),
								ldns_rr_owner(cur_sig_rr)) != 0) {
							ldns_rr_list_deep_free(tmp_rrset);
							tmp_rrset =
								ldns_rr_list_pop_rrset(cur_rrset);
						}
					}
					cur_status = ldns_verify_rrsig(tmp_rrset,
											 cur_sig_rr,
											 cur_parent_rr);
					/* avoid dupes */
					for (i = 0; i < new_tree->parent_count; i++) {
						if (cur_parent_rr == new_tree->parents[i]->rr) {
							goto done;
						}
					}

					cur_parent_tree =
						ldns_dnssec_derive_trust_tree(data_chain->parent,
												cur_parent_rr);
					ldns_dnssec_trust_tree_add_parent(new_tree,
											    cur_parent_tree,
											    cur_sig_rr,
											    cur_status);
				}
			}
		}
	}
 done:
	if (tmp_rrset && tmp_rrset != cur_rrset) {
		ldns_rr_list_deep_free(tmp_rrset);
	}
	ldns_rr_list_deep_free(cur_rrset);
}

void
ldns_dnssec_derive_trust_tree_dnskey_rrset(ldns_dnssec_trust_tree *new_tree,
                                           ldns_dnssec_data_chain *data_chain,
                                           ldns_rr *cur_rr,
                                           ldns_rr *cur_sig_rr)
{
	size_t j;
	ldns_rr_list *cur_rrset = data_chain->rrset;
	ldns_dnssec_trust_tree *cur_parent_tree;
	ldns_rr *cur_parent_rr;
	int cur_keytag;
	ldns_status cur_status;

	cur_keytag = ldns_rdf2native_int16(ldns_rr_rrsig_keytag(cur_sig_rr));

	for (j = 0; j < ldns_rr_list_rr_count(cur_rrset); j++) {
		cur_parent_rr = ldns_rr_list_rr(cur_rrset, j);
		if (cur_parent_rr != cur_rr &&
		    ldns_rr_get_type(cur_parent_rr) == LDNS_RR_TYPE_DNSKEY) {
			if (ldns_calc_keytag(cur_parent_rr) == cur_keytag
			    ) {
				cur_parent_tree = ldns_dnssec_trust_tree_new();
				cur_parent_tree->rr = cur_parent_rr;
				cur_parent_tree->rrset = cur_rrset;
				cur_status = ldns_verify_rrsig(cur_rrset,
										 cur_sig_rr,
										 cur_parent_rr);
				ldns_dnssec_trust_tree_add_parent(new_tree,
										    cur_parent_tree,
										    cur_sig_rr,
										    cur_status);
			}
		}
	}
}

void
ldns_dnssec_derive_trust_tree_ds_rrset(ldns_dnssec_trust_tree *new_tree,
                                       ldns_dnssec_data_chain *data_chain,
                                       ldns_rr *cur_rr)
{
	size_t j, h;
	ldns_rr_list *cur_rrset = data_chain->rrset;
	ldns_dnssec_trust_tree *cur_parent_tree;
	ldns_rr *cur_parent_rr;

	/* try the parent to see whether there are DSs there */
	if (ldns_rr_get_type(cur_rr) == LDNS_RR_TYPE_DNSKEY &&
	    data_chain->parent &&
	    data_chain->parent->rrset
	    ) {
		for (j = 0;
			j < ldns_rr_list_rr_count(data_chain->parent->rrset);
			j++) {
			cur_parent_rr = ldns_rr_list_rr(data_chain->parent->rrset, j);
			if (ldns_rr_get_type(cur_parent_rr) == LDNS_RR_TYPE_DS) {
				for (h = 0; h < ldns_rr_list_rr_count(cur_rrset); h++) {
					cur_rr = ldns_rr_list_rr(cur_rrset, h);
					if (ldns_rr_compare_ds(cur_rr, cur_parent_rr)) {
						cur_parent_tree =
							ldns_dnssec_derive_trust_tree(
							    data_chain->parent, cur_parent_rr);
						ldns_dnssec_trust_tree_add_parent(
						    new_tree,
						    cur_parent_tree,
						    NULL,
						    LDNS_STATUS_OK);
					} else {
						/*ldns_rr_print(stdout, cur_parent_rr);*/
					}
				}
				cur_rr = ldns_rr_list_rr(cur_rrset, 0);
			}
		}
	}
}

void
ldns_dnssec_derive_trust_tree_no_sig(ldns_dnssec_trust_tree *new_tree,
                                     ldns_dnssec_data_chain *data_chain)
{
	size_t i;
	ldns_rr_list *cur_rrset;
	ldns_rr *cur_parent_rr;
	ldns_dnssec_trust_tree *cur_parent_tree;
	ldns_status result;
	
	if (data_chain->parent && data_chain->parent->rrset) {
		cur_rrset = data_chain->parent->rrset;
		/* nsec? */
		if (cur_rrset && ldns_rr_list_rr_count(cur_rrset) > 0) {
			if (ldns_rr_get_type(ldns_rr_list_rr(cur_rrset, 0)) ==
			    LDNS_RR_TYPE_NSEC3) {
				result = ldns_dnssec_verify_denial_nsec3(
					        new_tree->rr,
						   cur_rrset,
						   data_chain->parent->signatures,
						   data_chain->packet_rcode,
						   data_chain->packet_qtype,
						   data_chain->packet_nodata);
			} else if (ldns_rr_get_type(ldns_rr_list_rr(cur_rrset, 0)) ==
					 LDNS_RR_TYPE_NSEC3) {
				result = ldns_dnssec_verify_denial(
					        new_tree->rr,
						   cur_rrset,
						   data_chain->parent->signatures);
			} else {
				/* unsigned zone, unsigned parent */
				result = LDNS_STATUS_OK;
			}
		} else {
			result = LDNS_STATUS_DNSSEC_NSEC_RR_NOT_COVERED;
		}
		for (i = 0; i < ldns_rr_list_rr_count(cur_rrset); i++) {
			cur_parent_rr = ldns_rr_list_rr(cur_rrset, i);
			cur_parent_tree = 
				ldns_dnssec_derive_trust_tree(data_chain->parent,
										cur_parent_rr);
			ldns_dnssec_trust_tree_add_parent(new_tree,
									    cur_parent_tree,
									    NULL,
									    result);
		}
	}
}

/*
 * returns OK if there is a path from tree to key with only OK
 * the (first) error in between otherwise
 * or NOT_FOUND if the key wasn't present at all
 */
ldns_status
ldns_dnssec_trust_tree_contains_keys(ldns_dnssec_trust_tree *tree,
							  ldns_rr_list *trusted_keys)
{
	size_t i;
	ldns_status result = LDNS_STATUS_CRYPTO_NO_DNSKEY;
	bool equal;
	ldns_status parent_result;
	
	if (tree && trusted_keys && ldns_rr_list_rr_count(trusted_keys) > 0)
		{ if (tree->rr) {
				for (i = 0; i < ldns_rr_list_rr_count(trusted_keys); i++) {
					equal = ldns_rr_compare_ds(
							  tree->rr,
							  ldns_rr_list_rr(trusted_keys, i));
					if (equal) {
						result = LDNS_STATUS_OK;
						return result;
					}
				}
			}
			for (i = 0; i < tree->parent_count; i++) {
				parent_result =
					ldns_dnssec_trust_tree_contains_keys(tree->parents[i],
												  trusted_keys);
				if (parent_result != LDNS_STATUS_CRYPTO_NO_DNSKEY) {
					if (tree->parent_status[i] != LDNS_STATUS_OK) {
						result = tree->parent_status[i];
					} else {
						if (ldns_rr_get_type(tree->rr)
						    == LDNS_RR_TYPE_NSEC &&
						    parent_result == LDNS_STATUS_OK
						    ) {
							result =
								LDNS_STATUS_DNSSEC_EXISTENCE_DENIED;
						} else {
							result = parent_result;
						}
					}
				}
			}
		} else {
		result = LDNS_STATUS_ERR;
	}
	
	return result;
}

ldns_status
ldns_verify(ldns_rr_list *rrset, ldns_rr_list *rrsig, const ldns_rr_list *keys, 
		  ldns_rr_list *good_keys)
{
	uint16_t i;
	bool valid;
	ldns_status verify_result = LDNS_STATUS_ERR;

	if (!rrset || !rrsig || !keys) {
		return LDNS_STATUS_ERR;
	}

	valid = false;

	if (ldns_rr_list_rr_count(rrset) < 1) {
		return LDNS_STATUS_ERR;
	}

	if (ldns_rr_list_rr_count(rrsig) < 1) {
		return LDNS_STATUS_CRYPTO_NO_RRSIG;
	}
	
	if (ldns_rr_list_rr_count(keys) < 1) {
		verify_result = LDNS_STATUS_CRYPTO_NO_TRUSTED_DNSKEY;
	} else {
		for (i = 0; i < ldns_rr_list_rr_count(rrsig); i++) {

			if (ldns_verify_rrsig_keylist(rrset,
									ldns_rr_list_rr(rrsig, i),
									keys, good_keys)
			    == LDNS_STATUS_OK) {
				verify_result = LDNS_STATUS_OK;
			}
		}
	}
	return verify_result;
}

ldns_rr_list *
ldns_fetch_valid_domain_keys(const ldns_resolver *res,
					    const ldns_rdf *domain,
					    const ldns_rr_list *keys,
					    ldns_status *status)
{
	ldns_rr_list * trusted_keys = NULL;
	ldns_rr_list * ds_keys = NULL;

	if (res && domain && keys) {

		if ((trusted_keys = ldns_validate_domain_dnskey(res,
											   domain,
											   keys))) {
			*status = LDNS_STATUS_OK;
		} else {
			/* No trusted keys in this domain, we'll have to find some in the parent domain */
			*status = LDNS_STATUS_CRYPTO_NO_TRUSTED_DNSKEY;
      
			if (ldns_rdf_size(domain) > 1) {
				/* Fail if we are at the root */
				ldns_rr_list * parent_keys;
				ldns_rdf * parent_domain = ldns_dname_left_chop(domain);
	
				if ((parent_keys = 
					ldns_fetch_valid_domain_keys(res,
										    parent_domain,
										    keys,
										    status))) {
					/* Check DS records */
					if ((ds_keys =
						ldns_validate_domain_ds(res,
										    domain,
										    parent_keys))) {
						trusted_keys =
							ldns_fetch_valid_domain_keys(res,
												    domain,
												    ds_keys,
												    status);
						ldns_rr_list_deep_free(ds_keys);
					} else {
						/* No valid DS at the parent -- fail */
						*status = LDNS_STATUS_CRYPTO_NO_TRUSTED_DS ;
					}
					ldns_rr_list_deep_free(parent_keys);
				}
				ldns_rdf_free(parent_domain);
			}
		}
	}
	return trusted_keys;
}

ldns_rr_list *
ldns_validate_domain_dnskey(const ldns_resolver * res,
					   const ldns_rdf * domain,
					   const ldns_rr_list * keys)
{
	ldns_status status;
	ldns_pkt * keypkt;
	ldns_rr * cur_key;
	uint16_t key_i; uint16_t key_j; uint16_t key_k;
	uint16_t sig_i; ldns_rr * cur_sig;

	ldns_rr_list * domain_keys = NULL;
	ldns_rr_list * domain_sigs = NULL;
	ldns_rr_list * trusted_keys = NULL;

	/* Fetch keys for the domain */
	if ((keypkt = ldns_resolver_query(res,
							    domain,
							    LDNS_RR_TYPE_DNSKEY,
							    LDNS_RR_CLASS_IN,
							    LDNS_RD))) {

		domain_keys = ldns_pkt_rr_list_by_type(keypkt,
									    LDNS_RR_TYPE_DNSKEY,
									    LDNS_SECTION_ANSWER);
		domain_sigs = ldns_pkt_rr_list_by_type(keypkt,
									    LDNS_RR_TYPE_RRSIG,
									    LDNS_SECTION_ANSWER);

		/* Try to validate the record using our keys */
		for (key_i=0; key_i< ldns_rr_list_rr_count(domain_keys); key_i++) {
      
			cur_key = ldns_rr_list_rr(domain_keys, key_i);
			for (key_j=0; key_j<ldns_rr_list_rr_count(keys); key_j++) {
				if (ldns_rr_compare_ds(ldns_rr_list_rr(keys, key_j),
								   cur_key)) {
          
					/* Current key is trusted -- validate */
					trusted_keys = ldns_rr_list_new();
          
					for (sig_i=0;
						sig_i<ldns_rr_list_rr_count(domain_sigs);
						sig_i++) {
						cur_sig = ldns_rr_list_rr(domain_sigs, sig_i);
						/* Avoid non-matching sigs */
						if (ldns_rdf2native_int16(
							   ldns_rr_rrsig_keytag(cur_sig))
						    == ldns_calc_keytag(cur_key)) {
							if ((status =
								ldns_verify_rrsig(domain_keys,
											   cur_sig,
											   cur_key))
							    == LDNS_STATUS_OK) {
                
								/* Push the whole rrset 
								   -- we can't do much more */
								for (key_k=0;
									key_k<ldns_rr_list_rr_count(
											domain_keys);
									key_k++) {
									ldns_rr_list_push_rr(
									    trusted_keys,
									    ldns_rr_clone(
										   ldns_rr_list_rr(
											  domain_keys,
											  key_k)));
								}
                
								ldns_rr_list_deep_free(domain_keys);
								ldns_rr_list_deep_free(domain_sigs);
								ldns_pkt_free(keypkt);
								return trusted_keys;
							}
						}
					}
	  
					/* Only push our trusted key */
					ldns_rr_list_push_rr(trusted_keys,
									 ldns_rr_clone(cur_key));
				}
			}
		}

		ldns_rr_list_deep_free(domain_keys);
		ldns_rr_list_deep_free(domain_sigs);
		ldns_pkt_free(keypkt);

	} else {
		status = LDNS_STATUS_CRYPTO_NO_DNSKEY;
	}
    
	return trusted_keys;
}

ldns_rr_list *
ldns_validate_domain_ds(const ldns_resolver *res,
				    const ldns_rdf * domain,
				    const ldns_rr_list * keys)
{
	ldns_status status;
	ldns_pkt * dspkt;
	uint16_t key_i;
	ldns_rr_list * rrset = NULL;
	ldns_rr_list * sigs = NULL;
	ldns_rr_list * trusted_keys = NULL;

	/* Fetch DS for the domain */
	if ((dspkt = ldns_resolver_query(res,
							   domain,
							   LDNS_RR_TYPE_DS,
							   LDNS_RR_CLASS_IN,
							   LDNS_RD))) {

		rrset = ldns_pkt_rr_list_by_type(dspkt,
								   LDNS_RR_TYPE_DS,
								   LDNS_SECTION_ANSWER);
		sigs = ldns_pkt_rr_list_by_type(dspkt,
								  LDNS_RR_TYPE_RRSIG,
								  LDNS_SECTION_ANSWER);

		/* Validate sigs */
		if ((status = ldns_verify(rrset, sigs, keys, NULL))
		    == LDNS_STATUS_OK) {
			trusted_keys = ldns_rr_list_new();
			for (key_i=0; key_i<ldns_rr_list_rr_count(rrset); key_i++) {
				ldns_rr_list_push_rr(trusted_keys,
								 ldns_rr_clone(ldns_rr_list_rr(rrset,
														 key_i)
											)
								 );
			}
		}

		ldns_rr_list_deep_free(rrset);
		ldns_rr_list_deep_free(sigs);
		ldns_pkt_free(dspkt);

	} else {
		status = LDNS_STATUS_CRYPTO_NO_DS;
	}

	return trusted_keys;
}

ldns_status
ldns_verify_trusted(ldns_resolver *res,
				ldns_rr_list *rrset,
				ldns_rr_list * rrsigs,
				ldns_rr_list * validating_keys)
{
	uint16_t sig_i; uint16_t key_i;
	ldns_rr * cur_sig; ldns_rr * cur_key;
	ldns_rr_list * trusted_keys = NULL;
	ldns_status result = LDNS_STATUS_ERR;

	if (!res || !rrset || !rrsigs) {
		return LDNS_STATUS_ERR;
	}

	if (ldns_rr_list_rr_count(rrset) < 1) {
		return LDNS_STATUS_ERR;
	}

	if (ldns_rr_list_rr_count(rrsigs) < 1) {
		return LDNS_STATUS_CRYPTO_NO_RRSIG;
	}
  
	/* Look at each sig */
	for (sig_i=0; sig_i < ldns_rr_list_rr_count(rrsigs); sig_i++) {

		cur_sig = ldns_rr_list_rr(rrsigs, sig_i);
		/* Get a valid signer key and validate the sig */
		if ((trusted_keys = ldns_fetch_valid_domain_keys(
						    res,
						    ldns_rr_rrsig_signame(cur_sig),
						    ldns_resolver_dnssec_anchors(res),
						    &result))) {

			for (key_i = 0;
				key_i < ldns_rr_list_rr_count(trusted_keys);
				key_i++) {
				cur_key = ldns_rr_list_rr(trusted_keys, key_i);

				if ((result = ldns_verify_rrsig(rrset,
										  cur_sig,
										  cur_key))
				    == LDNS_STATUS_OK) {
					if (validating_keys) {
						ldns_rr_list_push_rr(validating_keys,
										 ldns_rr_clone(cur_key));
					}
					ldns_rr_list_deep_free(trusted_keys);
					return LDNS_STATUS_OK;
				} else {
					ldns_rr_list_print(stdout, rrset);
					ldns_rr_print(stdout, cur_sig);
					ldns_rr_print(stdout, cur_key);
        	
				}
			}
		}
	}

	ldns_rr_list_deep_free(trusted_keys);
	return result;
}

ldns_status
ldns_dnssec_verify_denial(ldns_rr *rr,
                          ldns_rr_list *nsecs,
                          ldns_rr_list *rrsigs)
{
	ldns_rdf *rr_name;
	ldns_rdf *wildcard_name;
	ldns_rdf *chopped_dname;
	ldns_rr *cur_nsec;
	size_t i;
	ldns_status result;
	/* needed for wildcard check on exact match */
	ldns_rr *rrsig;
	bool name_covered = false;
	bool type_covered = false;
	bool wildcard_covered = false;
	bool wildcard_type_covered = false;

	wildcard_name = ldns_dname_new_frm_str("*");
	rr_name = ldns_rr_owner(rr);
	chopped_dname = ldns_dname_left_chop(rr_name);
	result = ldns_dname_cat(wildcard_name, chopped_dname);
	if (result != LDNS_STATUS_OK) {
		return result;
	}
	
	ldns_rdf_deep_free(chopped_dname);
	
	for  (i = 0; i < ldns_rr_list_rr_count(nsecs); i++) {
		cur_nsec = ldns_rr_list_rr(nsecs, i);
		if (ldns_dname_compare(rr_name, ldns_rr_owner(cur_nsec)) == 0) {
			/* see section 5.4 of RFC4035, if the label count of the NSEC's
			   RRSIG is equal, then it is proven that wildcard expansion 
			   could not have been used to match the request */
			rrsig = ldns_dnssec_get_rrsig_for_name_and_type(
					  ldns_rr_owner(cur_nsec),
					  ldns_rr_get_type(cur_nsec),
					  rrsigs);
			if (rrsig && ldns_rdf2native_int8(ldns_rr_rrsig_labels(rrsig))
			    == ldns_dname_label_count(rr_name)) {
				wildcard_covered = true;
			}
			
			if (ldns_nsec_bitmap_covers_type(ldns_nsec_get_bitmap(cur_nsec),
									   ldns_rr_get_type(rr))) {
				type_covered = true;
			}
		}
		if (ldns_nsec_covers_name(cur_nsec, rr_name)) {
			name_covered = true;
		}
		
		if (ldns_dname_compare(wildcard_name,
						   ldns_rr_owner(cur_nsec)) == 0) {
			if (ldns_nsec_bitmap_covers_type(ldns_nsec_get_bitmap(cur_nsec),
									   ldns_rr_get_type(rr))) {
				wildcard_type_covered = true;
			}
		}
		
		if (ldns_nsec_covers_name(cur_nsec, wildcard_name)) {
			wildcard_covered = true;
		}
		
	}
	
	ldns_rdf_deep_free(wildcard_name);
	
	if (type_covered || !name_covered) {
		return LDNS_STATUS_DNSSEC_NSEC_RR_NOT_COVERED;
	}
	
	if (wildcard_type_covered || !wildcard_covered) {
		return LDNS_STATUS_DNSSEC_NSEC_WILDCARD_NOT_COVERED;
	}

	return LDNS_STATUS_OK;
}

ldns_status
ldns_dnssec_verify_denial_nsec3(ldns_rr *rr,
						  ldns_rr_list *nsecs,
						  ldns_rr_list *rrsigs,
						  ldns_pkt_rcode packet_rcode,
						  ldns_rr_type packet_qtype,
						  bool packet_nodata)
{
	ldns_rdf *closest_encloser;
	ldns_rdf *wildcard;
	ldns_rdf *hashed_wildcard_name;
	bool wildcard_covered = false;
	ldns_rdf *zone_name;
	ldns_rdf *hashed_name;
	size_t i;
	ldns_status result = LDNS_STATUS_DNSSEC_NSEC_RR_NOT_COVERED;

	rrsigs = rrsigs;
	
	zone_name = ldns_dname_left_chop(ldns_rr_owner(ldns_rr_list_rr(nsecs,0)));

	/* section 8.4 */
	if (packet_rcode == LDNS_RCODE_NXDOMAIN) {
		closest_encloser = ldns_dnssec_nsec3_closest_encloser(
						   ldns_rr_owner(rr),
						   ldns_rr_get_type(rr),
						   nsecs);

		wildcard = ldns_dname_new_frm_str("*");
		ldns_dname_cat(wildcard, closest_encloser);

		for (i = 0; i < ldns_rr_list_rr_count(nsecs); i++) {
			hashed_wildcard_name = 
				ldns_nsec3_hash_name_frm_nsec3(ldns_rr_list_rr(nsecs, 0),
										 wildcard
										 );
			ldns_dname_cat(hashed_wildcard_name, zone_name);

			if (ldns_nsec_covers_name(ldns_rr_list_rr(nsecs, i),
								 hashed_wildcard_name)) {
				wildcard_covered = true;
			}
			ldns_rdf_deep_free(hashed_wildcard_name);
		}

		ldns_rdf_deep_free(closest_encloser);
		ldns_rdf_deep_free(wildcard);

		if (!wildcard_covered) {
			result = LDNS_STATUS_DNSSEC_NSEC_WILDCARD_NOT_COVERED;
		} else if (closest_encloser && wildcard_covered) {
			result = LDNS_STATUS_OK;
		} else {
			result = LDNS_STATUS_DNSSEC_NSEC_RR_NOT_COVERED;
		}
	} else if (packet_nodata && packet_qtype != LDNS_RR_TYPE_DS) {
		/* section 8.5 */
		hashed_name = ldns_nsec3_hash_name_frm_nsec3(ldns_rr_list_rr(nsecs,
														 0),
											ldns_rr_owner(rr)
											);
		ldns_dname_cat(hashed_name, zone_name);
		for (i = 0; i < ldns_rr_list_rr_count(nsecs); i++) {
			if (ldns_dname_compare(hashed_name,
							   ldns_rr_owner(ldns_rr_list_rr(nsecs, i)))
			    == 0) {
				if (!ldns_nsec_bitmap_covers_type(
					    ldns_nsec3_bitmap(ldns_rr_list_rr(nsecs, i)),
					    packet_qtype)
				    && 
				    !ldns_nsec_bitmap_covers_type(
					    ldns_nsec3_bitmap(ldns_rr_list_rr(nsecs, i)),
					    LDNS_RR_TYPE_CNAME)) {
					result = LDNS_STATUS_OK;
					goto done;
				}
			}
		}
		result = LDNS_STATUS_DNSSEC_NSEC_RR_NOT_COVERED;
	} else if (packet_nodata && packet_qtype == LDNS_RR_TYPE_DS) {
		/* section 8.6 */
		/* note: up to XXX this is the same as for 8.5 */
		hashed_name = ldns_nsec3_hash_name_frm_nsec3(ldns_rr_list_rr(nsecs,
														 0),
											ldns_rr_owner(rr)
											);
		ldns_dname_cat(hashed_name, zone_name);
		for (i = 0; i < ldns_rr_list_rr_count(nsecs); i++) {
			if (ldns_dname_compare(hashed_name,
							   ldns_rr_owner(ldns_rr_list_rr(nsecs,
													   i)))
			    == 0) {
				if (!ldns_nsec_bitmap_covers_type(
					    ldns_nsec3_bitmap(ldns_rr_list_rr(nsecs, i)),
					    LDNS_RR_TYPE_DS)
				    && 
				    !ldns_nsec_bitmap_covers_type(
					    ldns_nsec3_bitmap(ldns_rr_list_rr(nsecs, i)),
					    LDNS_RR_TYPE_CNAME)) {
					result = LDNS_STATUS_OK;
					goto done;
				}
			}
		}

		/* XXX see note above */
		closest_encloser =
			ldns_dnssec_nsec3_closest_encloser(ldns_rr_owner(rr),
										ldns_rr_get_type(rr),
										nsecs);

		result = LDNS_STATUS_DNSSEC_NSEC_RR_NOT_COVERED;
	}

 done:
	ldns_rdf_deep_free(zone_name);
	return result;
}

ldns_status
ldns_verify_rrsig_buffers(ldns_buffer *rawsig_buf, ldns_buffer *verify_buf, 
					 ldns_buffer *key_buf, uint8_t algo)
{
	return ldns_verify_rrsig_buffers_raw(
			 (unsigned char*)ldns_buffer_begin(rawsig_buf),
			 ldns_buffer_position(rawsig_buf),
			 verify_buf,
			 (unsigned char*)ldns_buffer_begin(key_buf), 
			 ldns_buffer_position(key_buf), algo);
}

ldns_status
ldns_verify_rrsig_buffers_raw(unsigned char* sig, size_t siglen,
						ldns_buffer *verify_buf, unsigned char* key, size_t keylen, 
						uint8_t algo)
{
	/* check for right key */
	switch(algo) {
	case LDNS_DSA:
	case LDNS_DSA_NSEC3:
		return ldns_verify_rrsig_dsa_raw(sig,
								   siglen,
								   verify_buf,
								   key,
								   keylen);
		break;
	case LDNS_RSASHA1:
	case LDNS_RSASHA1_NSEC3:
		return ldns_verify_rrsig_rsasha1_raw(sig,
									  siglen,
									  verify_buf,
									  key,
									  keylen);
		break;
#ifdef USE_SHA2
	case LDNS_RSASHA256:
		return ldns_verify_rrsig_rsasha256_raw(sig,
									    siglen,
									    verify_buf,
									    key,
									    keylen);
		break;
	case LDNS_RSASHA512:
		return ldns_verify_rrsig_rsasha512_raw(sig,
									    siglen,
									    verify_buf,
									    key,
									    keylen);
		break;
#endif
	case LDNS_RSAMD5:
		return ldns_verify_rrsig_rsamd5_raw(sig,
									 siglen,
									 verify_buf,
									 key,
									 keylen);
		break;
	default:
		/* do you know this alg?! */
		return LDNS_STATUS_CRYPTO_UNKNOWN_ALGO;
	}
}

/* Post 1.0 TODO: next 2 functions contain a lot of similar code */
/* 
 * to verify:
 * - create the wire fmt of the b64 key rdata
 * - create the wire fmt of the sorted rrset
 * - create the wire fmt of the b64 sig rdata
 * - create the wire fmt of the sig without the b64 rdata
 * - cat the sig data (without b64 rdata) to the rrset
 * - verify the rrset+sig, with the b64 data and the b64 key data
 */
ldns_status
ldns_verify_rrsig_keylist(ldns_rr_list *rrset,
					 ldns_rr *rrsig,
					 const ldns_rr_list *keys, 
					 ldns_rr_list *good_keys)
{
	ldns_buffer *rawsig_buf;
	ldns_buffer *verify_buf;
	ldns_buffer *key_buf;
	uint32_t orig_ttl;
	uint16_t i;
	uint8_t sig_algo;
	ldns_status result;
	ldns_rr *current_key;
	ldns_rr_list *rrset_clone;
	ldns_rr_list *validkeys;
	time_t now, inception, expiration;
	uint8_t label_count;
	ldns_rdf *wildcard_name;
	ldns_rdf *wildcard_chopped;
	ldns_rdf *wildcard_chopped_tmp;


	if (!rrset) {
		return LDNS_STATUS_ERR;
	}

	validkeys = ldns_rr_list_new();
	if (!validkeys) {
		return LDNS_STATUS_MEM_ERR;
	}
	
	/* canonicalize the sig */
	ldns_dname2canonical(ldns_rr_owner(rrsig));
	
	/* clone the rrset so that we can fiddle with it */
	rrset_clone = ldns_rr_list_clone(rrset);

	/* check if the typecovered is equal to the type checked */
	if (ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rrsig)) !=
	    ldns_rr_get_type(ldns_rr_list_rr(rrset_clone, 0))) {
		ldns_rr_list_deep_free(rrset_clone);
		ldns_rr_list_deep_free(validkeys);
		return LDNS_STATUS_CRYPTO_TYPE_COVERED_ERR;
	}
	
	/* create the buffers which will certainly hold the raw data */
	rawsig_buf = ldns_buffer_new(LDNS_MAX_PACKETLEN);
	verify_buf  = ldns_buffer_new(LDNS_MAX_PACKETLEN);
	
	sig_algo = ldns_rdf2native_int8(ldns_rr_rdf(rrsig, 1));
	result = LDNS_STATUS_ERR;

	/* check the signature time stamps */
	inception = ldns_rdf2native_time_t(ldns_rr_rrsig_inception(rrsig));
	expiration = ldns_rdf2native_time_t(ldns_rr_rrsig_expiration(rrsig));
	now = time(NULL);

	if (expiration - inception < 0) {
		/* bad sig, expiration before inception?? Tsssg */
		ldns_buffer_free(verify_buf);
		ldns_buffer_free(rawsig_buf);
		ldns_rr_list_deep_free(rrset_clone);
		ldns_rr_list_deep_free(validkeys);
		return LDNS_STATUS_CRYPTO_EXPIRATION_BEFORE_INCEPTION;
	}
	if (now - inception < 0) {
		/* bad sig, inception date has not yet come to pass */
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		ldns_rr_list_deep_free(rrset_clone);
		ldns_rr_list_deep_free(validkeys);
		return LDNS_STATUS_CRYPTO_SIG_NOT_INCEPTED;
	}
	if (expiration - now < 0) {
		/* bad sig, expiration date has passed */
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		ldns_rr_list_deep_free(rrset_clone);
		ldns_rr_list_deep_free(validkeys);
		return LDNS_STATUS_CRYPTO_SIG_EXPIRED;
	}
	
	/* create a buffer with b64 signature rdata */
	if (sig_algo == LDNS_DSA) {
		if (ldns_convert_dsa_rrsig_rdf2asn1(rawsig_buf,
									 ldns_rr_rdf(rrsig, 8))
		    != LDNS_STATUS_OK) {
			ldns_buffer_free(rawsig_buf);
			ldns_buffer_free(verify_buf);
			ldns_rr_list_deep_free(rrset_clone);
			ldns_rr_list_deep_free(validkeys);
			return LDNS_STATUS_MEM_ERR;
		}
	} else if (ldns_rdf2buffer_wire(rawsig_buf, 
							ldns_rr_rdf(rrsig, 8))
		    != LDNS_STATUS_OK) {
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		ldns_rr_list_deep_free(rrset_clone);
		ldns_rr_list_deep_free(validkeys);
		return LDNS_STATUS_MEM_ERR;
	}

	orig_ttl = ldns_rdf2native_int32( ldns_rr_rdf(rrsig, 3));
	label_count = ldns_rdf2native_int8(ldns_rr_rdf(rrsig, 2));

	/* reset the ttl in the rrset with the orig_ttl from the sig */
	/* and update owner name if it was wildcard */
	for(i = 0; i < ldns_rr_list_rr_count(rrset_clone); i++) {
		if (label_count < 
		    ldns_dname_label_count(
			   ldns_rr_owner(ldns_rr_list_rr(rrset_clone, i)))) {
			(void) ldns_str2rdf_dname(&wildcard_name, "*");
			wildcard_chopped =
				ldns_rdf_clone(ldns_rr_owner(ldns_rr_list_rr(rrset_clone,
													i)));
			while (label_count < ldns_dname_label_count(wildcard_chopped)) {
				wildcard_chopped_tmp =
					ldns_dname_left_chop(wildcard_chopped);
				ldns_rdf_deep_free(wildcard_chopped);
				wildcard_chopped = wildcard_chopped_tmp;
			}
			(void) ldns_dname_cat(wildcard_name, wildcard_chopped);
			ldns_rdf_deep_free(wildcard_chopped);
			ldns_rdf_deep_free(ldns_rr_owner(ldns_rr_list_rr(rrset_clone,
												    i)));
			ldns_rr_set_owner(ldns_rr_list_rr(rrset_clone, i), 
						   wildcard_name);
				  	
		}
		ldns_rr_set_ttl(ldns_rr_list_rr(rrset_clone, i), orig_ttl);
		/* convert to lowercase */
		ldns_rr2canonical(ldns_rr_list_rr(rrset_clone, i));
	}

	/* sort the rrset in canonical order  */
	ldns_rr_list_sort(rrset_clone);

	/* put the signature rr (without the b64) to the verify_buf */
	if (ldns_rrsig2buffer_wire(verify_buf, rrsig) != LDNS_STATUS_OK) {
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		ldns_rr_list_deep_free(rrset_clone);
		ldns_rr_list_deep_free(validkeys);
		return LDNS_STATUS_MEM_ERR;
	}

	/* add the rrset in verify_buf */
	if (ldns_rr_list2buffer_wire(verify_buf, rrset_clone) != LDNS_STATUS_OK) {
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		ldns_rr_list_deep_free(rrset_clone);
		ldns_rr_list_deep_free(validkeys);
		return LDNS_STATUS_MEM_ERR;
	}

	for(i = 0; i < ldns_rr_list_rr_count(keys); i++) {
		current_key = ldns_rr_list_rr(keys, i);
		/* before anything, check if the keytags match */
		if (ldns_calc_keytag(current_key) ==
		    ldns_rdf2native_int16(ldns_rr_rrsig_keytag(rrsig))) {
			key_buf = ldns_buffer_new(LDNS_MAX_PACKETLEN);
			
			/* put the key-data in a buffer, that's the third rdf, with
			 * the base64 encoded key data */
			if (ldns_rdf2buffer_wire(key_buf,
								ldns_rr_rdf(current_key, 3))
			    != LDNS_STATUS_OK) {
				ldns_buffer_free(rawsig_buf);
				ldns_buffer_free(verify_buf);
				/* returning is bad might screw up good keys later in the list
				   what to do? */
				ldns_rr_list_deep_free(rrset_clone);
				ldns_rr_list_deep_free(validkeys);
				return LDNS_STATUS_MEM_ERR;
			}

			/* check for right key */
			if (sig_algo == ldns_rdf2native_int8(ldns_rr_rdf(current_key, 
												    2))) {
				result = ldns_verify_rrsig_buffers(rawsig_buf, 
											verify_buf,
											key_buf,
											sig_algo);
			} else {
				result = LDNS_STATUS_CRYPTO_UNKNOWN_ALGO;
			}
			
			ldns_buffer_free(key_buf); 

			if (result == LDNS_STATUS_OK) {
				/* one of the keys has matched, don't break
				 * here, instead put the 'winning' key in
				 * the validkey list and return the list 
				 * later */
				if (!ldns_rr_list_push_rr(validkeys, current_key)) {
					/* couldn't push the key?? */
					ldns_buffer_free(rawsig_buf);
					ldns_buffer_free(verify_buf);
					ldns_rr_list_deep_free(rrset_clone);
					ldns_rr_list_deep_free(validkeys);
					return LDNS_STATUS_MEM_ERR;
				}
			} 
		} else {
			if (result == LDNS_STATUS_ERR) {
				result = LDNS_STATUS_CRYPTO_NO_MATCHING_KEYTAG_DNSKEY;
			}
		}
	}

	/* no longer needed */
	ldns_rr_list_deep_free(rrset_clone);
	ldns_buffer_free(rawsig_buf);
	ldns_buffer_free(verify_buf);
	if (ldns_rr_list_rr_count(validkeys) == 0) {
		/* no keys were added, return last error */
		ldns_rr_list_deep_free(validkeys); 
		return result;
	} else {
		ldns_rr_list_cat(good_keys, validkeys);
		ldns_rr_list_free(validkeys);
		return LDNS_STATUS_OK;
	}
}

ldns_status
ldns_verify_rrsig(ldns_rr_list *rrset, ldns_rr *rrsig, ldns_rr *key)
{
	ldns_buffer *rawsig_buf;
	ldns_buffer *verify_buf;
	ldns_buffer *key_buf;
	uint32_t orig_ttl;
	uint16_t i;
	uint8_t sig_algo;
	uint16_t label_count;
	ldns_status result;
	ldns_rr_list *rrset_clone;
	time_t now, inception, expiration;
	ldns_rdf *wildcard_name;
	ldns_rdf *wildcard_chopped;
	ldns_rdf *wildcard_chopped_tmp;

	if (!rrset) {
		return LDNS_STATUS_NO_DATA;
	}

	/* lowercase the rrsig owner name */
	ldns_dname2canonical(ldns_rr_owner(rrsig));

	/* check the signature time stamps */
	inception = ldns_rdf2native_time_t(ldns_rr_rrsig_inception(rrsig));
	expiration = ldns_rdf2native_time_t(ldns_rr_rrsig_expiration(rrsig));
	now = time(NULL);

	if (expiration - inception < 0) {
		/* bad sig, expiration before inception?? Tsssg */
		return LDNS_STATUS_CRYPTO_EXPIRATION_BEFORE_INCEPTION;
	}
	if (now - inception < 0) {
		/* bad sig, inception date has not passed */
		return LDNS_STATUS_CRYPTO_SIG_NOT_INCEPTED;
	}

	if (expiration - now < 0) {
		/* bad sig, expiration date has passed */
		return LDNS_STATUS_CRYPTO_SIG_EXPIRED;
	}
	/* clone the rrset so that we can fiddle with it */
	rrset_clone = ldns_rr_list_clone(rrset);
	
	/* create the buffers which will certainly hold the raw data */
	rawsig_buf = ldns_buffer_new(LDNS_MAX_PACKETLEN);
	verify_buf  = ldns_buffer_new(LDNS_MAX_PACKETLEN);
	
	sig_algo = ldns_rdf2native_int8(ldns_rr_rdf(rrsig, 1));
	
	/* check for known and implemented algo's now (otherwise 
	 * the function could return a wrong error
	 */
	/* create a buffer with signature rdata */
	/* for some algorithms we need other data than for others... */
	/* (the DSA API wants DER encoding for instance) */

	switch(sig_algo) {
	case LDNS_RSAMD5:
	case LDNS_RSASHA1:
	case LDNS_RSASHA1_NSEC3:
#ifdef USE_SHA2
	case LDNS_RSASHA256:
	case LDNS_RSASHA512:
#endif
		if (ldns_rdf2buffer_wire(rawsig_buf,
							ldns_rr_rdf(rrsig, 8)) != LDNS_STATUS_OK) {
			ldns_buffer_free(rawsig_buf);
			ldns_buffer_free(verify_buf);
			return LDNS_STATUS_MEM_ERR;
		}
		break;
	case LDNS_DSA:
	case LDNS_DSA_NSEC3:
		/* EVP takes rfc2459 format, which is a tad longer than dns format */
		exit(0);
		if (ldns_convert_dsa_rrsig_rdf2asn1(rawsig_buf,
									 ldns_rr_rdf(rrsig, 8))
		    != LDNS_STATUS_OK) {
			/*
			  if (ldns_rdf2buffer_wire(rawsig_buf,
			  ldns_rr_rdf(rrsig, 8)) != LDNS_STATUS_OK) {
			*/
			ldns_buffer_free(rawsig_buf);
			ldns_buffer_free(verify_buf);
			return LDNS_STATUS_MEM_ERR;
		}
		break;
		break;
	case LDNS_DH:
	case LDNS_ECC:
	case LDNS_INDIRECT:
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		return LDNS_STATUS_CRYPTO_ALGO_NOT_IMPL;
	default:
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		ldns_rr_list_deep_free(rrset_clone);
		return LDNS_STATUS_CRYPTO_UNKNOWN_ALGO;
	}
	
	result = LDNS_STATUS_ERR;

	/* remove labels if the label count is higher than the label count
	   from the rrsig */
	label_count = ldns_rdf2native_int8(ldns_rr_rdf(rrsig, 2));

	orig_ttl = ldns_rdf2native_int32(
							   ldns_rr_rdf(rrsig, 3));

	/* reset the ttl in the rrset with the orig_ttl from the sig */
	for(i = 0; i < ldns_rr_list_rr_count(rrset_clone); i++) {
		if (label_count < 
		    ldns_dname_label_count(
			   ldns_rr_owner(ldns_rr_list_rr(rrset_clone, i)))) {
			(void) ldns_str2rdf_dname(&wildcard_name, "*");
			wildcard_chopped =
				ldns_rdf_clone(ldns_rr_owner(ldns_rr_list_rr(rrset_clone,
													i)));
			while (label_count < ldns_dname_label_count(wildcard_chopped)) {
				wildcard_chopped_tmp =
					ldns_dname_left_chop(wildcard_chopped);
				ldns_rdf_deep_free(wildcard_chopped);
				wildcard_chopped = wildcard_chopped_tmp;
			}
			(void) ldns_dname_cat(wildcard_name, wildcard_chopped);
			ldns_rdf_deep_free(wildcard_chopped);
			ldns_rdf_deep_free(ldns_rr_owner(ldns_rr_list_rr(rrset_clone,
												    i)));
			ldns_rr_set_owner(ldns_rr_list_rr(rrset_clone, i), 
						   wildcard_name);
				  	
		}
		ldns_rr_set_ttl(
					 ldns_rr_list_rr(rrset_clone, i),
					 orig_ttl);
		/* convert to lowercase */
		ldns_rr2canonical(ldns_rr_list_rr(rrset_clone, i));
	}

	/* sort the rrset in canonical order  */
	ldns_rr_list_sort(rrset_clone);

	/* put the signature rr (without the b64) to the verify_buf */
	if (ldns_rrsig2buffer_wire(verify_buf, rrsig) != LDNS_STATUS_OK) {
		ldns_rr_list_deep_free(rrset_clone);
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		return LDNS_STATUS_ERR;
	}

	/* add the rrset in verify_buf */
	if (ldns_rr_list2buffer_wire(verify_buf, rrset_clone) != LDNS_STATUS_OK) {
		ldns_rr_list_deep_free(rrset_clone);
		ldns_buffer_free(rawsig_buf);
		ldns_buffer_free(verify_buf);
		return LDNS_STATUS_ERR;
	}

	if (ldns_calc_keytag(key)
	    ==
	    ldns_rdf2native_int16(ldns_rr_rrsig_keytag(rrsig))
	    ) {
		key_buf = ldns_buffer_new(LDNS_MAX_PACKETLEN);
		
		/* before anything, check if the keytags match */

		/* put the key-data in a buffer, that's the third rdf, with
		 * the base64 encoded key data */
		if (ldns_rdf2buffer_wire(key_buf,
							ldns_rr_rdf(key, 3)) != LDNS_STATUS_OK) {
			ldns_rr_list_deep_free(rrset_clone);
			ldns_buffer_free(rawsig_buf);
			ldns_buffer_free(verify_buf);
			/* returning is bad might screw up
			   good keys later in the list
			   what to do? */
			return LDNS_STATUS_ERR;
		}
		
		if (sig_algo == ldns_rdf2native_int8(ldns_rr_rdf(key, 2))) {
			result = ldns_verify_rrsig_buffers(rawsig_buf,
										verify_buf,
										key_buf,
										sig_algo);
		}
		
		ldns_buffer_free(key_buf); 
	}
	else {
		/* No keys with the corresponding keytag are found */
		if (result == LDNS_STATUS_ERR) {
			result = LDNS_STATUS_CRYPTO_NO_MATCHING_KEYTAG_DNSKEY;
		}
	}
	/* no longer needed */
	ldns_rr_list_deep_free(rrset_clone);
	ldns_buffer_free(rawsig_buf);
	ldns_buffer_free(verify_buf);
	return result;
}

ldns_status
ldns_verify_rrsig_evp(ldns_buffer *sig,
				  ldns_buffer *rrset,
				  EVP_PKEY *key,
				  const EVP_MD *digest_type)
{
	return ldns_verify_rrsig_evp_raw(
			 (unsigned char*)ldns_buffer_begin(sig),
			 ldns_buffer_position(sig),
			 rrset,
			 key,
			 digest_type);
}

ldns_status
ldns_verify_rrsig_evp_raw(unsigned char *sig, size_t siglen, 
					 ldns_buffer *rrset, EVP_PKEY *key, const EVP_MD *digest_type)
{
	EVP_MD_CTX ctx;
	int res;

	EVP_MD_CTX_init(&ctx);
	
	EVP_VerifyInit(&ctx, digest_type);
	EVP_VerifyUpdate(&ctx,
				  ldns_buffer_begin(rrset),
				  ldns_buffer_position(rrset));
	res = EVP_VerifyFinal(&ctx, sig, siglen, key);
	
	EVP_MD_CTX_cleanup(&ctx);
	
	if (res == 1) {
		return LDNS_STATUS_OK;
	} else if (res == 0) {
		return LDNS_STATUS_CRYPTO_BOGUS;
	}
	/* TODO how to communicate internal SSL error?
	   let caller use ssl's get_error() */
	return LDNS_STATUS_SSL_ERR;
}

ldns_status
ldns_verify_rrsig_dsa(ldns_buffer *sig, ldns_buffer *rrset, ldns_buffer *key)
{
	return ldns_verify_rrsig_dsa_raw(
			 (unsigned char*) ldns_buffer_begin(sig),
			 ldns_buffer_position(sig),
			 rrset,
			 (unsigned char*) ldns_buffer_begin(key),
			 ldns_buffer_position(key));
}

ldns_status
ldns_verify_rrsig_rsasha1(ldns_buffer *sig, ldns_buffer *rrset, ldns_buffer *key)
{
	return ldns_verify_rrsig_rsasha1_raw(
			 (unsigned char*)ldns_buffer_begin(sig),
			 ldns_buffer_position(sig),
			 rrset,
			 (unsigned char*) ldns_buffer_begin(key),
			 ldns_buffer_position(key));
}

ldns_status
ldns_verify_rrsig_rsamd5(ldns_buffer *sig, ldns_buffer *rrset, ldns_buffer *key)
{
	return ldns_verify_rrsig_rsamd5_raw(
			 (unsigned char*)ldns_buffer_begin(sig),
			 ldns_buffer_position(sig),
			 rrset,
			 (unsigned char*) ldns_buffer_begin(key),
			 ldns_buffer_position(key));
}

ldns_status
ldns_verify_rrsig_dsa_raw(unsigned char* sig, size_t siglen,
					 ldns_buffer* rrset, unsigned char* key, size_t keylen)
{
	EVP_PKEY *evp_key;
	ldns_status result;

	evp_key = EVP_PKEY_new();
	EVP_PKEY_assign_DSA(evp_key, ldns_key_buf2dsa_raw(key, keylen));
	result = ldns_verify_rrsig_evp_raw(sig,
								siglen,
								rrset,
								evp_key,
								EVP_dss1());
	EVP_PKEY_free(evp_key);
	return result;

}

ldns_status
ldns_verify_rrsig_rsasha1_raw(unsigned char* sig, size_t siglen,
						ldns_buffer* rrset, unsigned char* key, size_t keylen)
{
	EVP_PKEY *evp_key;
	ldns_status result;

	evp_key = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(evp_key, ldns_key_buf2rsa_raw(key, keylen));
	result = ldns_verify_rrsig_evp_raw(sig,
								siglen,
								rrset,
								evp_key,
								EVP_sha1());
	EVP_PKEY_free(evp_key);

	return result;
}

ldns_status
ldns_verify_rrsig_rsasha256_raw(unsigned char* sig,
						  size_t siglen,
						  ldns_buffer* rrset,
						  unsigned char* key,
						  size_t keylen)
{
#ifdef USE_SHA2
	EVP_PKEY *evp_key;
	ldns_status result;

	evp_key = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(evp_key, ldns_key_buf2rsa_raw(key, keylen));
	result = ldns_verify_rrsig_evp_raw(sig,
								siglen,
								rrset,
								evp_key,
								EVP_sha256());
	EVP_PKEY_free(evp_key);

	return result;
#else
	/* touch these to prevent compiler warnings */
	(void) sig;
	(void) siglen;
	(void) rrset;
	(void) key;
	(void) keylen;
	return LDNS_STATUS_CRYPTO_UNKNOWN_ALGO;
#endif
}

ldns_status
ldns_verify_rrsig_rsasha512_raw(unsigned char* sig,
						  size_t siglen,
						  ldns_buffer* rrset,
						  unsigned char* key,
						  size_t keylen)
{
#ifdef USE_SHA2
	EVP_PKEY *evp_key;
	ldns_status result;

	evp_key = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(evp_key, ldns_key_buf2rsa_raw(key, keylen));
	result = ldns_verify_rrsig_evp_raw(sig,
								siglen,
								rrset,
								evp_key,
								EVP_sha512());
	EVP_PKEY_free(evp_key);

	return result;
#else
	/* touch these to prevent compiler warnings */
	(void) sig;
	(void) siglen;
	(void) rrset;
	(void) key;
	(void) keylen;
	return LDNS_STATUS_CRYPTO_UNKNOWN_ALGO;
#endif
}



ldns_status
ldns_verify_rrsig_rsamd5_raw(unsigned char* sig,
					    size_t siglen,
					    ldns_buffer* rrset,
					    unsigned char* key,
					    size_t keylen)
{
	EVP_PKEY *evp_key;
	ldns_status result;

	evp_key = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(evp_key, ldns_key_buf2rsa_raw(key, keylen));
	result = ldns_verify_rrsig_evp_raw(sig,
								siglen,
								rrset,
								evp_key,
								EVP_md5());
	EVP_PKEY_free(evp_key);

	return result;
}

#endif
