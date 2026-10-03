#ifndef ROUTER_H
#define ROUTER_H

/**
 * @brief Starts the router listening to the MQTT client.
 *
 * From then on it subscribes to the downlink topics (DOWNLINK_TOPICS in config.h) on every
 * broker connection, and writes each message on them to the MCU as one line, unchanged.
 * An empty message is ignored; one longer than LINE_MAX_LEN, or containing '\n', is dropped and logged.
 * Needs mqtt_link_init() run first.
 */
void router_init(void);

/**
 * @brief Finds the topic for a letter in the uplink table (UPLINK_ROUTES in config.h).
 *
 * @param letter The first letter of a line
 * @return The topic, or NULL if no row has that letter
 */
const char *router_find_topic(char letter);

/**
 * @brief Publishes a line from the MCU, unchanged and at QoS 1, to the topic its first letter picks.
 *
 * An empty line is ignored. A line whose letter isn't in the table is ignored and logged.
 *
 * @param line The line, ending in '\0'
 */
void router_uplink(const char *line);

#endif
