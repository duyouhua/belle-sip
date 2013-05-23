/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "belle_sip_internal.h"
#include "listeningpoint_internal.h"


belle_sip_hop_t* belle_sip_hop_new(const char* transport, const char *cname, const char* host,int port) {
	belle_sip_hop_t* hop = belle_sip_object_new(belle_sip_hop_t);
	if (transport) hop->transport=belle_sip_strdup(transport);
	if (host) hop->host=belle_sip_strdup(host);
	if (cname) hop->cname=belle_sip_strdup(cname);
	hop->port=port;
	return hop;
}

belle_sip_hop_t* belle_sip_hop_new_from_uri(const belle_sip_uri_t *uri){
	const char *host;
	const char *cname=NULL;
	host=belle_sip_uri_get_maddr_param(uri);
	if (!host) host=belle_sip_uri_get_host(uri);
	else cname=belle_sip_uri_get_host(uri);
	return belle_sip_hop_new(belle_sip_uri_get_transport_param(uri),
				cname,
				host,
				belle_sip_uri_get_listening_port(uri));
}

static void belle_sip_hop_destroy(belle_sip_hop_t *hop){
	if (hop->host) {
		belle_sip_free(hop->host);
		hop->host=NULL;
	}
	if (hop->cname){
		belle_sip_free(hop->cname);
		hop->cname=NULL;
	}
	if (hop->transport){
		belle_sip_free(hop->transport);
		hop->transport=NULL;
	}
}

static void belle_sip_hop_clone(belle_sip_hop_t *hop, const belle_sip_hop_t *orig){
	if (orig->host)
		hop->host=belle_sip_strdup(orig->host);
	if (orig->cname)
		hop->cname=belle_sip_strdup(orig->cname);
	if (orig->transport)
		hop->transport=belle_sip_strdup(orig->transport);
	
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(belle_sip_hop_t);
BELLE_SIP_INSTANCIATE_VPTR(belle_sip_hop_t,belle_sip_object_t,belle_sip_hop_destroy,belle_sip_hop_clone,NULL,TRUE);

static void belle_sip_stack_destroy(belle_sip_stack_t *stack){
	belle_sip_object_unref(stack->ml);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(belle_sip_stack_t);
BELLE_SIP_INSTANCIATE_VPTR(belle_sip_stack_t,belle_sip_object_t,belle_sip_stack_destroy,NULL,NULL,FALSE);

belle_sip_stack_t * belle_sip_stack_new(const char *properties){
	belle_sip_stack_t *stack=belle_sip_object_new(belle_sip_stack_t);
	stack->ml=belle_sip_main_loop_new ();
	stack->timer_config.T1=500;
	stack->timer_config.T2=4000;
	stack->timer_config.T4=5000;
	stack->transport_timeout=30000;
	stack->dns_timeout=15000;
	stack->inactive_transport_timeout=3600; /*one hour*/
	return stack;
}

const belle_sip_timer_config_t *belle_sip_stack_get_timer_config(const belle_sip_stack_t *stack){
	return &stack->timer_config;
}

int belle_sip_stack_get_transport_timeout(const belle_sip_stack_t *stack){
	return stack->transport_timeout;
}

int belle_sip_stack_get_dns_timeout(const belle_sip_stack_t *stack) {
	return stack->dns_timeout;
}

void belle_sip_stack_set_dns_timeout(belle_sip_stack_t *stack, int timeout) {
	stack->dns_timeout = timeout;
}

belle_sip_listening_point_t *belle_sip_stack_create_listening_point(belle_sip_stack_t *s, const char *ipaddress, int port, const char *transport){
	belle_sip_listening_point_t *lp=NULL;
	if (strcasecmp(transport,"UDP")==0) {
		lp=belle_sip_udp_listening_point_new(s,ipaddress,port);
	} else if (strcasecmp(transport,"TCP") == 0) {
		lp=belle_sip_stream_listening_point_new(s,ipaddress,port);
	}else if (strcasecmp(transport,"TLS") == 0) {
		lp=belle_sip_tls_listening_point_new(s,ipaddress,port);
	} else {
		belle_sip_fatal("Unsupported transport %s",transport);
	}
	return lp;
}

void belle_sip_stack_delete_listening_point(belle_sip_stack_t *s, belle_sip_listening_point_t *lp){
	belle_sip_object_unref(lp);
}

belle_sip_provider_t *belle_sip_stack_create_provider(belle_sip_stack_t *s, belle_sip_listening_point_t *lp){
	belle_sip_provider_t *p=belle_sip_provider_new(s,lp);
	return p;
}

void belle_sip_stack_delete_provider(belle_sip_stack_t *s, belle_sip_provider_t *p){
	belle_sip_object_unref(p);
}

belle_sip_main_loop_t * belle_sip_stack_get_main_loop(belle_sip_stack_t *stack){
	return stack->ml;
}

void belle_sip_stack_main(belle_sip_stack_t *stack){
	belle_sip_main_loop_run(stack->ml);
}

void belle_sip_stack_sleep(belle_sip_stack_t *stack, unsigned int milliseconds){
	belle_sip_main_loop_sleep (stack->ml,milliseconds);
}

belle_sip_hop_t * belle_sip_stack_get_next_hop(belle_sip_stack_t *stack, belle_sip_request_t *req) {
	belle_sip_header_route_t *route=BELLE_SIP_HEADER_ROUTE(belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"route"));
	belle_sip_uri_t *uri;

	if (route!=NULL){
		uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(route));
	}else{
		uri=belle_sip_request_get_uri(req);
	}
	return belle_sip_hop_new_from_uri(uri);
}



void belle_sip_stack_set_tx_delay(belle_sip_stack_t *stack, int delay_ms){
	stack->tx_delay=delay_ms;
}
void belle_sip_stack_set_send_error(belle_sip_stack_t *stack, int send_error){
	stack->send_error=send_error;
}

void belle_sip_stack_set_resolver_tx_delay(belle_sip_stack_t *stack, int delay_ms) {
	stack->resolver_tx_delay = delay_ms;
}

void belle_sip_stack_set_resolver_send_error(belle_sip_stack_t *stack, int send_error) {
	stack->resolver_send_error = send_error;
}

const char * belle_sip_stack_get_dns_user_hosts_file(const belle_sip_stack_t *stack) {
	return stack->dns_user_hosts_file;
}

void belle_sip_stack_set_dns_user_hosts_file(belle_sip_stack_t *stack, const char *hosts_file) {
	stack->dns_user_hosts_file = hosts_file;
}

const char* belle_sip_version_to_string() {
	return PACKAGE_VERSION;
}

int belle_sip_stack_get_inactive_transport_timeout(const belle_sip_stack_t *stack){
	return stack->inactive_transport_timeout;
}

void belle_sip_stack_set_inactive_transport_timeout(belle_sip_stack_t *stack, int seconds){
	stack->inactive_transport_timeout=seconds;
}

void belle_sip_stack_set_default_dscp(belle_sip_stack_t *stack, int dscp){
	stack->dscp=dscp;
}

int belle_sip_stack_get_default_dscp(belle_sip_stack_t *stack){
	return stack->dscp;
}

