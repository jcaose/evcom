#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <ev.h>
#include "oi.h"
#ifdef HAVE_GNUTLS
# include <gnutls/gnutls.h>
#endif

#define HOST "127.0.0.1"
#define SOCKFILE "/tmp/oi.sock"
#define PORT 5000

int nconnections; 
static int is_secure = 0;
static int is_tcp = 0;

static void 
on_peer_close(oi_socket *socket)
{
  //printf("server connection closed\n");
#ifdef HAVE_GNUTLS
  if(is_secure) { gnutls_deinit(socket->session); }
#endif
  free(socket);
}

static void 
on_client_timeout(oi_socket *socket)
{
  printf("client connection timeout\n");
  exit(1);
}

static void 
on_peer_timeout(oi_socket *socket)
{
  fprintf(stderr, "peer connection timeout\n");
  exit(1);
}

static void 
on_peer_error(oi_socket *socket, int domain, int code)
{
  fprintf(stderr, "error on the peer socket: %s\n", oi_strerror(domain, code));
  exit(1);
}

static void 
on_client_error(oi_socket *socket, int domain, int code)
{
  fprintf(stderr, "error on the client socket: %s\n", oi_strerror(domain, code));
  exit(1);
}


#ifdef HAVE_GNUTLS
#define DH_BITS 768
gnutls_anon_server_credentials_t server_credentials;
const int kx_prio[] = { GNUTLS_KX_ANON_DH, 0 };
static gnutls_dh_params_t dh_params;

void anon_tls_init()
{
  gnutls_global_init();

  gnutls_dh_params_init (&dh_params);

  fprintf(stderr, "..");
  fsync((int)stderr);
  gnutls_dh_params_generate2 (dh_params, DH_BITS);
  fprintf(stderr, ".");

  gnutls_anon_allocate_server_credentials (&server_credentials);
  gnutls_anon_set_server_dh_params (server_credentials, dh_params);
}

void anon_tls_server(oi_socket *socket)
{
  gnutls_session_t session;
  socket->data = session;

  int r = gnutls_init(&session, GNUTLS_SERVER);
  assert(r == 0);
  gnutls_set_default_priority(session);
  gnutls_kx_set_priority (session, kx_prio);
  gnutls_credentials_set(session, GNUTLS_CRD_ANON, server_credentials);
  gnutls_dh_set_prime_bits(session, DH_BITS);

  oi_socket_set_secure_session(socket, session);
}

void anon_tls_client(oi_socket *socket)
{
  gnutls_session_t client_session;
  gnutls_anon_client_credentials_t client_credentials;

  gnutls_anon_allocate_client_credentials (&client_credentials);
  gnutls_init (&client_session, GNUTLS_CLIENT);
  gnutls_set_default_priority(client_session);
  gnutls_kx_set_priority(client_session, kx_prio);
  /* Need to enable anonymous KX specifically. */
  gnutls_credentials_set (client_session, GNUTLS_CRD_ANON, client_credentials);

  oi_socket_set_secure_session(socket, client_session);
  assert(socket->secure);
}

#endif
