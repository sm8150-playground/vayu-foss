/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <linux/qrtr.h>

#include "libqrtr.h"

#ifndef AF_QIPCRTR
#define AF_QIPCRTR 42
#endif

#define LOG_E(fmt, ...) fprintf(stderr, "E|qrtr: " fmt, ##__VA_ARGS__)


static uint32_t le32_to_cpu(uint32_t v)
{
	return v;
}


static void qrtr_addr_set(struct sockaddr_qrtr *sq, uint32_t node, uint32_t port)
{
	memset(sq, 0, sizeof(*sq));
	sq->sq_family = AF_QIPCRTR;
	sq->sq_node = node;
	sq->sq_port = port;
}

static int qrtr_get_addr(int sock, struct sockaddr_qrtr *sq)
{
	socklen_t addrlen = sizeof(*sq);
	int rc;

	rc = getsockname(sock, (struct sockaddr *)sq, &addrlen);
	if (rc) {
		LOG_E("getsockname(): %s\n", strerror(errno));
		return -1;
	}

	if (sq->sq_family != AF_QIPCRTR || addrlen != sizeof(*sq))
		return -1;

	return 0;
}

int qrtr_open(int rport)
{
	struct sockaddr_qrtr sq = { 0 };
	struct timeval tv;
	int sock;
	int rc;

	sock = socket(AF_QIPCRTR, SOCK_DGRAM, 0);
	if (sock < 0) {
		LOG_E("socket(AF_QIPCRTR): %s\n", strerror(errno));
		return -1;
	}

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	rc = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (rc) {
		LOG_E("setsockopt(SO_RCVTIMEO): %s\n", strerror(errno));
		goto err_close;
	}

	if (rport != 0) {
		qrtr_addr_set(&sq, 1, rport);

		rc = bind(sock, (struct sockaddr *)&sq, sizeof(sq));
		if (rc < 0) {
			LOG_E("bind(%d): %s\n", rport, strerror(errno));
			goto err_close;
		}
	}

	return sock;

err_close:
	close(sock);
	return -1;
}

int qrtr_close(int sock)
{
	return close(sock);
}


__attribute__((noinline))
int qrtr_sendto(int sock, uint32_t node, uint32_t port, const void *buf,
		unsigned int len)
{
	struct sockaddr_qrtr sq = { 0 };
	int rc;

	qrtr_addr_set(&sq, node, port);

	rc = sendto(sock, buf, len, 0, (struct sockaddr *)&sq, sizeof(sq));
	if (rc < 0) {
		LOG_E("sendto(): %s\n", strerror(errno));
		return -1;
	}

	return rc;
}

int qrtr_new_server(int sock, uint32_t service, uint16_t version,
		    uint16_t instance)
{
	struct qrtr_ctrl_pkt pkt;
	struct sockaddr_qrtr sq = { 0 };
	int rc;

	rc = qrtr_get_addr(sock, &sq);
	if (rc)
		return -1;

	memset(&pkt, 0, sizeof(pkt));
	pkt.cmd = le32_to_cpu(QRTR_TYPE_NEW_SERVER);
	pkt.server.service = le32_to_cpu(service);
	pkt.server.instance = le32_to_cpu(instance << 8 | version);
	pkt.server.node = le32_to_cpu(sq.sq_node);
	pkt.server.port = le32_to_cpu(sq.sq_port);

	rc = qrtr_sendto(sock, sq.sq_node, QRTR_PORT_CTRL, &pkt, sizeof(pkt));
	return rc < 0 ? rc : 0;
}

int qrtr_remove_server(int sock, uint32_t service, uint16_t version,
		       uint16_t instance)
{
	struct qrtr_ctrl_pkt pkt;
	struct sockaddr_qrtr sq = { 0 };
	int rc;

	rc = qrtr_get_addr(sock, &sq);
	if (rc)
		return -1;

	memset(&pkt, 0, sizeof(pkt));
	pkt.cmd = le32_to_cpu(QRTR_TYPE_DEL_SERVER);
	pkt.server.service = le32_to_cpu(service);
	pkt.server.instance = le32_to_cpu(instance << 8 | version);
	pkt.server.node = le32_to_cpu(sq.sq_node);
	pkt.server.port = le32_to_cpu(sq.sq_port);

	rc = qrtr_sendto(sock, sq.sq_node, QRTR_PORT_CTRL, &pkt, sizeof(pkt));
	return rc < 0 ? rc : 0;
}

int qrtr_new_lookup(int sock, uint32_t service, uint16_t version,
		    uint16_t instance)
{
	struct qrtr_ctrl_pkt pkt;
	struct sockaddr_qrtr sq = { 0 };
	int rc;

	rc = qrtr_get_addr(sock, &sq);
	if (rc)
		return -1;


	memset(&pkt, 0, sizeof(pkt));
	pkt.cmd = le32_to_cpu(QRTR_TYPE_NEW_LOOKUP);
	pkt.server.service = le32_to_cpu(service);
	pkt.server.instance = le32_to_cpu(instance << 8 | version);

	rc = qrtr_sendto(sock, sq.sq_node, QRTR_PORT_CTRL, &pkt, sizeof(pkt));
	return rc < 0 ? rc : 0;
}

int qrtr_remove_lookup(int sock, uint32_t service, uint16_t version,
		       uint16_t instance)
{
	struct qrtr_ctrl_pkt pkt;
	struct sockaddr_qrtr sq = { 0 };
	int rc;

	rc = qrtr_get_addr(sock, &sq);
	if (rc)
		return -1;


	memset(&pkt, 0, sizeof(pkt));
	pkt.cmd = le32_to_cpu(QRTR_TYPE_DEL_LOOKUP);
	pkt.server.service = le32_to_cpu(service);
	pkt.server.instance = le32_to_cpu(instance << 8 | version);
	pkt.server.node = le32_to_cpu(sq.sq_node);
	pkt.server.port = le32_to_cpu(sq.sq_port);

	rc = qrtr_sendto(sock, sq.sq_node, QRTR_PORT_CTRL, &pkt, sizeof(pkt));
	return rc < 0 ? rc : 0;
}

int qrtr_publish(int sock, uint32_t service, uint16_t version,
		 uint16_t instance)
{
	return qrtr_new_server(sock, service, version, instance);
}

int qrtr_bye(int sock, uint32_t service, uint16_t version, uint16_t instance)
{
	return qrtr_remove_server(sock, service, version, instance);
}

int qrtr_poll(int sock, unsigned int ms)
{
	struct pollfd fds;


	fds.fd = sock;
	fds.events = POLLIN | POLLERR;
	fds.revents = 0;

	return poll(&fds, 1, ms);
}

int qrtr_recv(int sock, void *buf, unsigned int len)
{
	int rc;

	rc = recvfrom(sock, buf, len, 0, NULL, NULL);
	if (rc < 0)
		LOG_E("recv(): %s\n", strerror(errno));

	return rc;
}

int qrtr_recvfrom(int sock, void *buf, unsigned int len, uint32_t *node,
		  uint32_t *port)
{
	struct sockaddr_qrtr sq = { 0 };
	socklen_t addrlen = sizeof(sq);
	int rc;

	memset(&sq, 0, sizeof(sq));

	rc = recvfrom(sock, buf, len, 0, (struct sockaddr *)&sq, &addrlen);
	if (rc < 0)
		LOG_E("recvfrom(): %s\n", strerror(errno));
	else {
		if (node)
			*node = sq.sq_node;
		if (port)
			*port = sq.sq_port;
	}

	return rc;
}

int qrtr_decode(struct qrtr_packet *pkt, const void *buf, size_t len,
		const struct sockaddr_qrtr *sq)
{
	const struct qrtr_ctrl_pkt *ctrl = buf;
	uint32_t cmd;

	if (sq->sq_port != QRTR_PORT_CTRL) {
		pkt->type = QRTR_TYPE_DATA;
		pkt->node = sq->sq_node;
		pkt->port = sq->sq_port;
		pkt->data = (void *)buf;
		pkt->data_len = len;
		return 0;
	}

	if (len < sizeof(*ctrl))
		return -EMSGSIZE;

	cmd = le32_to_cpu(ctrl->cmd);
	pkt->type = cmd;

	switch (cmd) {
	case QRTR_TYPE_BYE:
		pkt->node = le32_to_cpu(ctrl->client.node);
		break;
	case QRTR_TYPE_NEW_SERVER:
	case QRTR_TYPE_DEL_SERVER:
		pkt->service = le32_to_cpu(ctrl->server.service);
		pkt->instance = le32_to_cpu(ctrl->server.instance) >> 8;
		pkt->version = le32_to_cpu(ctrl->server.instance) & 0xff;
		pkt->node = le32_to_cpu(ctrl->server.node);
		pkt->port = le32_to_cpu(ctrl->server.port);
		break;
	case QRTR_TYPE_DEL_CLIENT:
		pkt->node = le32_to_cpu(ctrl->client.node);
		pkt->port = le32_to_cpu(ctrl->client.port);
		break;
	default:
		pkt->type = 0;
		break;
	}

	return 0;
}
