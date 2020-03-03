#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

#include "protocols/uip/uip.h"

#include "config.h"
#include "core/debug.h"
#include "core/bit-macros.h"

#include "mqtt_homie.h"

#ifdef ONEWIRE_HOMIE_SUPPORT
#include "hardware/onewire/onewire_homie.h"
#endif

#ifdef TANKLEVEL_HOMIE_SUPPORT
#include "services/tanklevel/tanklevel_homie.h"
#endif

typedef enum
{
  HOMIE_DISCONNECTED,
  HOMIE_INIT,
  HOMIE_DEVICE_META,
  HOMIE_NODE_META,
  HOMIE_READY,
  HOMIE_ACTIVE
} mqtt_homie_state_t;

void mqtt_homie_init(void);

static bool call_callback_ptr(mqtt_homie_output_callback_t const *value);

static bool send_device_meta_str_P(PGM_P attr, PGM_P value);
static bool send_device_meta_localip(void);
static bool send_device_meta_mac(void);
static bool send_device_meta_nodes(void);
static bool send_device_meta(void);
static bool send_device_state(const char *state);

static bool send_node_meta_str_P(PGM_P node_id, PGM_P attr, PGM_P value);
static bool send_node_meta_str_ptr(PGM_P node_id, PGM_P attr, PGM_P const *value);
static bool send_node_meta_props(PGM_P node_id, const mqtt_homie_property_t * const *props);
static bool send_node_meta(const mqtt_homie_node_t *node);
static bool send_nodes_meta(void);

static bool send_prop_meta_str_P(PGM_P node_id, PGM_P prop_id, PGM_P attr, PGM_P value);
static bool send_prop_meta_str_ptr(PGM_P node_id, PGM_P prop_id, PGM_P attr, PGM_P const *value);
static bool send_prop_meta_bool(PGM_P node_id, PGM_P prop_id, PGM_P attr, uint8_t const *value);
static bool send_prop_meta_datatype(PGM_P node_id, PGM_P prop_id, PGM_P attr, uint8_t const *value);
static bool send_prop_meta(const mqtt_homie_node_t *node, const mqtt_homie_property_t *prop);
static bool send_props_meta(const mqtt_homie_node_t *node);

static bool call_node_output_callback(const mqtt_homie_node_t *node);
static bool call_nodes_output_callback(void);

static void set_state(mqtt_homie_state_t new_state);

static const char *match_topic(const char *data, const char *end, PGM_P prefix, bool last);

static void connack_cb(void);
static void poll_cb(void);
static void close_cb(void);
static void publish_cb(char const *topic, uint16_t topic_length, const void *payload, uint16_t payload_length, bool retained);

static const mqtt_callback_config_t mqtt_homie_callback_config PROGMEM = {
  .connack_callback = connack_cb,
  .poll_callback = poll_cb,
  .close_callback = close_cb,
  .publish_callback = publish_cb
};

static const char str_empty[] PROGMEM = "";
static const char str_comma[] PROGMEM = ",";
static const char str_array[] PROGMEM = "[]";
static const char str_homie[] PROGMEM = "homie";

static const char dev_id[] PROGMEM = MQTT_HOMIE_CONF_DEV_ID;

static const char fmt_S_S_S[] PROGMEM = "%S/%S/%S";
static const char fmt_S_S_S_S[] PROGMEM = "%S/%S/%S/%S";
static const char fmt_S_S_S_S_S[] PROGMEM = "%S/%S/%S/%S/%S";
static const char fmt_S_S_S_d_S[] PROGMEM = "%S/%S/%S_%d/%S";

static const mqtt_homie_node_t * const nodes[] PROGMEM =
{
#ifdef ONEWIRE_HOMIE_SUPPORT
  &ow_homie_node,
#endif
#ifdef TANKLEVEL_HOMIE_SUPPORT
  &tanklevel_homie_node,
#endif
  NULL
};

static mqtt_homie_state_t state;
static uint8_t dev_field;
static uint8_t node_pos;
static uint8_t node_field;
static uint8_t prop_pos;
static uint8_t prop_field;

static bool
call_callback_ptr(mqtt_homie_output_callback_t const *value)
{
  mqtt_homie_output_callback_t cb = (mqtt_homie_output_callback_t) pgm_read_word(value);
  if (cb == NULL)
    return true;
  return cb();
}

static bool
send_device_meta_str_P(PGM_P attr, PGM_P value)
{
  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, attr);
  mqtt_construct_publish_packet_payload(PSTR("%S"), value);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta_localip(void)
{
  uip_ipaddr_t hostaddr;
  uip_gethostaddr(&hostaddr);

  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, PSTR("$localip"));
#if UIP_CONF_IPV6
  mqtt_construct_publish_packet_payload(
    PSTR("%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x"),
    HTONS(hostaddr[0]), HTONS(hostaddr[1]),
    HTONS(hostaddr[2]), HTONS(hostaddr[3]),
    HTONS(hostaddr[4]), HTONS(hostaddr[5]),
    HTONS(hostaddr[6]), HTONS(hostaddr[7]));
#else
  mqtt_construct_publish_packet_payload(
    PSTR("%u.%u.%u.%u"),
    HI8(HTONS(hostaddr[0])), LO8(HTONS(hostaddr[0])),
    HI8(HTONS(hostaddr[1])), LO8(HTONS(hostaddr[1])));
#endif
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta_mac(void)
{
  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, PSTR("$mac"));
  mqtt_construct_publish_packet_payload(PSTR("%02X:%02X:%02X:%02X:%02X:%02X"),
    uip_ethaddr.addr[0], uip_ethaddr.addr[1], uip_ethaddr.addr[2],
    uip_ethaddr.addr[3], uip_ethaddr.addr[4], uip_ethaddr.addr[5]);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta_nodes(void)
{
  bool first;
  const mqtt_homie_node_t * const *np;
  const mqtt_homie_node_t *n;
  const char *node_id;
  mqtt_homie_output_callback_t array_callback;

  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, PSTR("$nodes"));
  for (first = true, np = nodes; (n = (const mqtt_homie_node_t *) pgm_read_word(np)) != NULL; first = false, np++) {
    node_id = (const char *) pgm_read_word(&n->id);
    array_callback = (mqtt_homie_output_callback_t) pgm_read_word(&n->array_callback);

    mqtt_construct_publish_packet_payload(PSTR("%S%S%S"),
      first ? str_empty : str_comma,
      node_id,
      array_callback != NULL ? str_array : str_empty);
  }
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta(void)
{
  switch (dev_field)
  {
    case 0:
      if (!send_device_meta_str_P(PSTR("$homie"), PSTR("3.0.1")))
        return false;
      dev_field++;

    case 1:
      if (!send_device_meta_str_P(PSTR("$name"), PSTR(MQTT_HOMIE_CONF_DEV_NAME)))
        return false;
      dev_field++;

    case 2:
      if (!send_device_meta_localip())
        return false;
      dev_field++;

    case 3:
      if (!send_device_meta_mac())
        return false;
      dev_field++;

    case 4:
      if (!send_device_meta_nodes())
        return false;
      dev_field++;

    case 5:
      if (!send_device_meta_str_P(PSTR("$implementation"), PSTR("Ethersex")))
        return false;
      dev_field++;
  }

  return true;
}

static bool
send_device_state(const char *state)
{
  return send_device_meta_str_P(PSTR("$state"), state);
}

static bool
send_node_meta_str_P(PGM_P node_id, PGM_P attr, PGM_P value)
{
  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S, str_homie, dev_id, node_id, attr);
  mqtt_construct_publish_packet_payload(PSTR("%S"), value);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_node_meta_str_ptr(PGM_P node_id, PGM_P attr, PGM_P const *value)
{
  PGM_P s = (PGM_P) pgm_read_word(value);
  if (s == NULL)
    return true;
  return send_node_meta_str_P(node_id, attr, s);
}

static bool
send_node_meta_props(PGM_P node_id, const mqtt_homie_property_t * const *props)
{
  const mqtt_homie_property_t *prop = (const mqtt_homie_property_t *) pgm_read_word(props);
  bool first;
  const char *prop_id;

  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S, str_homie, dev_id, node_id, PSTR("$properties"));
  for (first = true; (prop_id = (const char *) pgm_read_word(&prop->id)) != NULL; first = false, prop++) {
    mqtt_construct_publish_packet_payload(PSTR("%S%S"),
      first ? str_empty : str_comma,
      prop_id);
  }
  return mqtt_construct_publish_packet_fin();
}

static bool
send_node_meta(const mqtt_homie_node_t *node)
{
  PGM_P node_id = (PGM_P) pgm_read_word(&node->id);
  switch (node_field)
  {
    case 0:
      if (!call_callback_ptr(&node->init_callback))
        return false;
      node_field++;

    case 1:
      if (!send_node_meta_str_ptr(node_id, PSTR("$name"), &node->name))
        return false;
      node_field++;

    case 2:
      if (!send_node_meta_str_ptr(node_id, PSTR("$type"), &node->type))
        return false;
      node_field++;

    case 3:
      if (!send_node_meta_props(node_id, &node->properties))
        return false;
      node_field++;

    case 4:
      if (!call_callback_ptr(&node->array_callback))
        return false;
      node_field++;
  }

  return true;
}

static bool
send_nodes_meta(void)
{
  while(1)
  {
    const mqtt_homie_node_t *node = (const mqtt_homie_node_t *) pgm_read_word(&nodes[node_pos]);
    if (node == NULL)
      return true;

    if (!send_node_meta(node))
      return false;

    if (!send_props_meta(node))
      return false;

    node_pos++;
    node_field = 0;
    prop_pos = 0;
    prop_field = 0;
  }
}

static bool
send_prop_meta_str_P(PGM_P node_id, PGM_P prop_id, PGM_P attr, PGM_P value)
{
  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S_S, str_homie, dev_id, node_id, prop_id, attr);
  mqtt_construct_publish_packet_payload(PSTR("%S"), value);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_prop_meta_str_ptr(PGM_P node_id, PGM_P prop_id, PGM_P attr, PGM_P const *value)
{
  PGM_P s = (PGM_P) pgm_read_word(value);
  if (s == NULL)
    return true;
  return send_prop_meta_str_P(node_id, prop_id, attr, s);
}

static bool
send_prop_meta_bool(PGM_P node_id, PGM_P prop_id, PGM_P attr, uint8_t const *value)
{
  bool b = (bool) pgm_read_byte(value);
  return send_prop_meta_str_P(node_id, prop_id, attr, mqtt_homie_bool(b));
}

static bool
send_prop_meta_datatype(PGM_P node_id, PGM_P prop_id, PGM_P attr, uint8_t const *value)
{
  mqtt_homie_datatype_t dt = (mqtt_homie_datatype_t) pgm_read_byte(value);
  PGM_P s;
  switch(dt)
  {
    case HOMIE_DATATYPE_STRING:
      s = PSTR("string");
      break;
    case HOMIE_DATATYPE_INTEGER:
      s = PSTR("integer");
      break;
    case HOMIE_DATATYPE_FLOAT:
      s = PSTR("float");
      break;
    case HOMIE_DATATYPE_BOOL:
      s = PSTR("boolean");
      break;
    case HOMIE_DATATYPE_ENUM:
      s = PSTR("enum");
      break;
    case HOMIE_DATATYPE_COLOR:
      s = PSTR("color");
      break;
  }
  return send_prop_meta_str_P(node_id, prop_id, attr, s);
}

static bool
send_prop_meta(const mqtt_homie_node_t *node, const mqtt_homie_property_t *prop)
{
  PGM_P node_id = (PGM_P) pgm_read_word(&node->id);
  PGM_P prop_id = (PGM_P) pgm_read_word(&prop->id);
  switch (prop_field)
  {
    case 0:
      if (!send_prop_meta_str_ptr(node_id, prop_id, PSTR("$name"), &prop->name))
        return false;
      prop_field++;

    case 1:
      if (!send_prop_meta_bool(node_id, prop_id, PSTR("$settable"), &prop->settable))
        return false;
      prop_field++;

    case 2:
      if (!send_prop_meta_str_ptr(node_id, prop_id, PSTR("$unit"), &prop->unit))
        return false;
      prop_field++;

    case 3:
      if (!send_prop_meta_datatype(node_id, prop_id, PSTR("$datatype"), &prop->datatype))
        return false;
      prop_field++;

    case 4:
      if (!send_prop_meta_str_ptr(node_id, prop_id, PSTR("$format"), &prop->format))
        return false;
      prop_field++;

    case 5:
      if (!call_callback_ptr(&prop->format_callback))
        return false;
      prop_field++;
  }

  prop_field = 0;
  return true;
}

static bool
send_props_meta(const mqtt_homie_node_t *node)
{
  while(1)
  {
    const mqtt_homie_property_t *props = (const mqtt_homie_property_t *) pgm_read_word(&node->properties);
    const mqtt_homie_property_t *prop = &props[prop_pos];
    PGM_P prop_id = (PGM_P) pgm_read_word(&prop->id);
    if (prop_id == NULL)
      return true;

    if (!send_prop_meta(node, prop))
      return false;

    prop_pos++;
    prop_field = 0;
  }
}

static bool
call_node_output_callback(const mqtt_homie_node_t *node)
{
  while(1)
  {
    const mqtt_homie_property_t *props = (const mqtt_homie_property_t *) pgm_read_word(&node->properties);
    const mqtt_homie_property_t *prop = &props[prop_pos];
    PGM_P prop_id = (PGM_P) pgm_read_word(&prop->id);
    if (prop_id == NULL)
      return true;

    if (!call_callback_ptr(&prop->output_callback))
      return false;

    prop_pos++;
  }
}

static bool
call_nodes_output_callback(void)
{
  while(1)
  {
    const mqtt_homie_node_t *node = (const mqtt_homie_node_t *) pgm_read_word(&nodes[node_pos]);
    if (node == NULL)
      return true;

    if (!call_node_output_callback(node))
      return false;

    node_pos++;
    prop_pos = 0;
  }
}

static void
set_state(mqtt_homie_state_t new_state)
{
  state = new_state;
  dev_field = 0;
  node_pos = 0;
  node_field = 0;
  prop_pos = 0;
  prop_field = 0;
}

void
mqtt_homie_init(void)
{
  set_state(HOMIE_DISCONNECTED);
  mqtt_register_callback(&mqtt_homie_callback_config);
}

static void
connack_cb(void)
{
  set_state(HOMIE_INIT);
}

static void
poll_cb(void)
{
  switch (state)
  {
    case HOMIE_DISCONNECTED:
      break;
    case HOMIE_INIT:
      if (send_device_state(PSTR("init")))
        set_state(HOMIE_DEVICE_META);
      break;
    case HOMIE_DEVICE_META:
      if (send_device_meta())
        set_state(HOMIE_NODE_META);
      break;
    case HOMIE_NODE_META:
      if (send_nodes_meta())
        set_state(HOMIE_READY);
      break;
    case HOMIE_READY:
      if (send_device_state(PSTR("ready")))
        set_state(HOMIE_ACTIVE);
      break;
    case HOMIE_ACTIVE:
      if (call_nodes_output_callback())
        set_state(HOMIE_ACTIVE);
      break;
  }
}

static void
close_cb(void)
{
  set_state(HOMIE_DISCONNECTED);
}

static const char *
match_topic(const char *data, const char *end, PGM_P prefix, bool last)
{
  char c;
  for(; (c = (char) pgm_read_byte(prefix)) != 0; prefix++)
  {
    if (data >= end || *data != c)
      return NULL;
    data++;
  }

  if (last)
  {
    if (data < end)
      return NULL;
  } else {
    if (data >= end || *data != '/')
      return NULL;
    data++;
  }

  return data;
}

static void
publish_cb(char const *topic, uint16_t topic_length, const void *payload, uint16_t payload_length, bool retained)
{
  char const *topic_end = topic + topic_length;
  const char *node_in = match_topic(topic, topic_end, str_homie, false);
  if (node_in == NULL)
    return;
  node_in = match_topic(node_in, topic_end, dev_id, false);
  if (node_in == NULL)
    return;

  const mqtt_homie_node_t * const *np;
  const mqtt_homie_node_t *node;
  for (np = nodes; (node = (const mqtt_homie_node_t *) pgm_read_word(np)) != NULL; np++)
  {
    PGM_P node_id = (PGM_P) pgm_read_word(&node->id);

    const char *prop_in = match_topic(node_in, topic_end, node_id, false);
    if (prop_in == NULL)
      continue;

    const mqtt_homie_property_t *prop;
    PGM_P prop_id;
    for (prop = (const mqtt_homie_property_t *) pgm_read_word(&node->properties); (prop_id = (PGM_P) pgm_read_word(&prop->id)) != NULL; prop++)
    {
      if (match_topic(prop_in, topic_end, prop_id, true) == NULL)
        continue;

      publish_callback cb = (publish_callback) pgm_read_word(&prop->input_callback);
      if (cb != NULL)
        cb(topic, topic_length, payload, payload_length, retained);
    }
  }
}

bool
mqtt_homie_header_array_node_name(PGM_P node_id, uint8_t node_index)
{
  return mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_d_S, str_homie, dev_id, node_id, node_index, PSTR("$name"));
}

bool
mqtt_homie_header_array_range(PGM_P node_id)
{
  return mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S, str_homie, dev_id, node_id, PSTR("$array"));
}

bool
mqtt_homie_header_array_prop_value(PGM_P node_id, uint8_t node_index, PGM_P prop_id, bool retain)
{
  return mqtt_construct_publish_packet_header(1, retain, fmt_S_S_S_d_S, str_homie, dev_id, node_id, node_index, prop_id);
}

bool
mqtt_homie_header_prop_format(PGM_P node_id, PGM_P prop_id)
{
  return mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S_S, str_homie, dev_id, node_id, prop_id, PSTR("$format"));
}

bool
mqtt_homie_header_prop_value(PGM_P node_id, PGM_P prop_id, bool retain)
{
  return mqtt_construct_publish_packet_header(1, retain, fmt_S_S_S_S, str_homie, dev_id, node_id, prop_id);
}

PGM_P
mqtt_homie_bool(bool value)
{
  return value ? PSTR("true") : PSTR("false");
}

/*
  -- Ethersex META --
  header(protocols/mqtt/mqtt_homie.h)
  init(mqtt_homie_init)
*/
