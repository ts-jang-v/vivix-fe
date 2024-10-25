#include <stdio.h>
#include <assert.h>			// assert()
#include <stdlib.h>			// malloc(), free()
#include <string.h>			// memset()

#include "gvcp.h"


gvcp_packet_type_e gvcp_packet_get_packet_type (gvcp_packet_t *packet)
{
	if (packet == NULL)
		return GVCP_PACKET_TYPE_ERROR;

	return (gvcp_packet_type_e) ntohs (packet->header.packet_type);
}

gvcp_command_e gvcp_packet_get_command (gvcp_packet_t *packet)
{
	if (packet == NULL)
		return (gvcp_command_e) 0;

	return (gvcp_command_e) ntohs (packet->header.command);
}

void gvcp_packet_set_packet_count (gvcp_packet_t *packet, uint16_t count)
{
	if (packet != NULL)
		packet->header.count = htons (count);
}

uint16_t gvcp_packet_get_packet_count (gvcp_packet_t *packet)
{
	if (packet == NULL)
		return 0;

	return ntohs (packet->header.count);
}

void gvcp_packet_get_read_memory_cmd_infos (const gvcp_packet_t *packet, uint32_t *address, uint32_t *size)
{
	if (packet == NULL)
		return;
	if (address != NULL)
		*address = ntohl (*((uint32_t *) ((char *) packet + sizeof (gvcp_packet_t))));
	if (size != NULL)
		*size = (ntohl (*((uint32_t *) ((char *) packet + sizeof (gvcp_packet_t) + sizeof (uint32_t))))) &
			0xffff;
}

int gvcp_packet_get_read_memory_ack_size (uint32_t data_size)
{
	return sizeof (gvcp_header_t) + sizeof (uint32_t) + data_size;
}

void * gvcp_packet_get_read_memory_ack_data (const gvcp_packet_t *packet)
{
	return (char *) packet + sizeof (gvcp_header_t) + sizeof (uint32_t);
}

void gvcp_packet_get_write_memory_cmd_infos (const gvcp_packet_t *packet, uint32_t *address, uint32_t *size)
{
	if (packet == NULL)
		return;

	if (address != NULL)
		*address = ntohl (*((uint32_t *) ((char *) packet + sizeof (gvcp_packet_t))));
	if (size != NULL)
		*size = ntohs (packet->header.size) - sizeof (uint32_t);
}

void * gvcp_packet_get_write_memory_cmd_data (const gvcp_packet_t *packet)
{
	return (char *) packet + sizeof (gvcp_packet_t) + sizeof (uint32_t);
}

size_t gvcp_packet_get_write_memory_ack_size (void)
{
	return sizeof (gvcp_packet_t) + sizeof (uint32_t);
}

void gvcp_packet_get_read_register_cmd_infos (const gvcp_packet_t *packet, uint32_t *address)
{
	if (packet == NULL)
		return;
	if (address != NULL)
		*address = ntohl (*((uint32_t *) ((char *) packet + sizeof (gvcp_packet_t))));
}

uint32_t gvcp_packet_get_read_register_ack_value (const gvcp_packet_t *packet)
{
	if (packet == NULL)
		return 0;
	return ntohl (*((uint32_t *) ((char *) packet + sizeof (gvcp_packet_t))));
}

void gvcp_packet_get_write_register_cmd_infos (const gvcp_packet_t *packet, uint32_t *address, uint32_t *value)
{
	if (packet == NULL)
		return;
	if (address != NULL)
		*address = ntohl (*((uint32_t *) ((char *) packet + sizeof (gvcp_packet_t))));
	if (value != NULL)
		*value = ntohl (*((uint32_t *) ((char *) packet + sizeof (gvcp_packet_t) + sizeof (uint32_t))));
}



void gvcp_packet_free (gvcp_packet_t *packet)
{
	free (packet);
}

/**
 * gvcp_packet_new_read_memory_cmd: (skip)
 * @address: read address
 * @size: read size, in bytes
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a memory read command.
 */

gvcp_packet_t * gvcp_packet_new_read_memory_cmd (uint32_t address, uint32_t size, uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_address = htonl (address);
	uint32_t n_size = htonl (size);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + 2 * sizeof (uint32_t);

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_CMD);
	packet->header.command = htons (GVCP_COMMAND_READ_MEMORY_CMD);
	packet->header.size = htons (2 * sizeof (uint32_t));
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (uint32_t));
	memcpy (&packet->data[sizeof(uint32_t)], &n_size, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_read_memory_ack: (skip)
 * @address: read address
 * @size: read size, in bytes
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a memory read acknowledge.
 */

gvcp_packet_t * gvcp_packet_new_read_memory_ack (uint32_t address, uint32_t size, uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_address = htonl (address);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + sizeof (uint32_t) + size;

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_ACK);
	packet->header.command = htons (GVCP_COMMAND_READ_MEMORY_ACK);
	packet->header.size = htons (sizeof (uint32_t) + size);
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_write_memory_cmd: (skip)
 * @address: write address
 * @size: write size, in bytes
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a memory write command.
 */

gvcp_packet_t *gvcp_packet_new_write_memory_cmd (uint32_t address, uint32_t size, uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_address = htonl (address);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + sizeof (uint32_t) + size;

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_CMD);
	packet->header.command = htons (GVCP_COMMAND_WRITE_MEMORY_CMD);
	packet->header.size = htons (sizeof (uint32_t) + size);
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_write_memory_ack: (skip)
 * @address: write address
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a memory write acknowledge.
 */

gvcp_packet_t * gvcp_packet_new_write_memory_ack (uint32_t address,uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_address = htonl (address);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + sizeof (uint32_t);

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_ACK);
	packet->header.command = htons (GVCP_COMMAND_WRITE_MEMORY_ACK);
	packet->header.size = htons (sizeof (uint32_t));
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_read_register_cmd: (skip)
 * @address: write address
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a register read command.
 */

gvcp_packet_t * gvcp_packet_new_read_register_cmd (uint32_t address, uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_address = htonl (address);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + sizeof (uint32_t);

	packet =(gvcp_packet_t*) malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_CMD);
	packet->header.command = htons (GVCP_COMMAND_READ_REGISTER_CMD);
	packet->header.size = htons (sizeof (uint32_t));
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_read_register_ack: (skip)
 * @value: read value
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a register read acknowledge.
 */

gvcp_packet_t * gvcp_packet_new_read_register_ack (uint32_t value,uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_value = htonl (value);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + sizeof (uint32_t);

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_ACK);
	packet->header.command = htons (GVCP_COMMAND_READ_REGISTER_ACK);
	packet->header.size = htons (sizeof (uint32_t));
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_value, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_write_register_cmd: (skip)
 * @address: write address
 * @value: value to write
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a register write command.
 */

gvcp_packet_t * gvcp_packet_new_write_register_cmd (uint32_t address, uint32_t value,uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_address = htonl (address);
	uint32_t n_value = htonl (value);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + 2 * sizeof (uint32_t);

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_CMD);
	packet->header.command = htons (GVCP_COMMAND_WRITE_REGISTER_CMD);
	packet->header.size = htons (2 * sizeof (uint32_t));
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (uint32_t));
	memcpy (&packet->data[sizeof (uint32_t)], &n_value, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_write_register_ack: (skip)
 * @address: write address
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a register write acknowledge.
 */

gvcp_packet_t * gvcp_packet_new_write_register_ack 	(uint32_t address,uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t n_address = htonl (address);

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + sizeof (uint32_t);

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_ACK);
	packet->header.command = htons (GVCP_COMMAND_WRITE_REGISTER_ACK);
	packet->header.size = htons (sizeof (uint32_t));
	packet->header.count = htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (uint32_t));

	return packet;
}

/**
 * gvcp_packet_new_discovery_cmd: (skip)
 * @size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a discovery command.
 */

gvcp_packet_t * gvcp_packet_new_discovery_cmd (size_t *packet_size)
{
	gvcp_packet_t *packet;

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t);

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_CMD);
	packet->header.command = htons (GVCP_COMMAND_DISCOVERY_CMD);
	packet->header.size = htons (0x0000);
	packet->header.count = htons (0xffff);

	return packet;
}

/**
 * gvcp_packet_new_discovery_ack: (skip)
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a discovery acknowledge.
 */

gvcp_packet_t * gvcp_packet_new_discovery_ack (size_t *packet_size)
{
	gvcp_packet_t *packet;

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + GVBS_DISCOVERY_DATA_SIZE ;

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_ACK);
	packet->header.command = htons (GVCP_COMMAND_DISCOVERY_ACK);
	packet->header.size = htons (GVBS_DISCOVERY_DATA_SIZE);
	packet->header.count = htons (0xffff);

	return packet;
}

/**
 * gvcp_packet_new_packet_resend_cmd: (skip)
 * @frame_id: frame id
 * @first_block: first missing packet
 * @last_block: last missing packet
 * @packet_count: current packet count
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #gvcp_packet_t
 *
 * Create a gvcp packet for a packet resend command.
 */

gvcp_packet_t * gvcp_packet_new_packet_resend_cmd (uint32_t frame_id,
				       uint32_t first_block, uint32_t last_block,
				       uint32_t packet_count, size_t *packet_size)
{
	gvcp_packet_t *packet;
	uint32_t *data;

	assert (packet_size);

	*packet_size = sizeof (gvcp_header_t) + 3 * sizeof (uint32_t);

	packet = (gvcp_packet_t*)malloc (*packet_size);

	packet->header.packet_type = htons (GVCP_PACKET_TYPE_RESEND);
	packet->header.command = htons (GVCP_COMMAND_PACKET_RESEND_CMD);
	packet->header.size = htons (3 * sizeof (uint32_t));
	packet->header.count = htons (packet_count);

	data = (uint32_t *) &packet->data;

	data[0] = htonl (frame_id);
	data[1] = htonl (first_block);
	data[2] = htonl (last_block);

	return packet;
}
